/*
 * UVHTTP 连接空闲超时回调测试
 *
 * 为什么需要独立测试：connection_timeout_cb（src/uvhttp_connection.c:1544）
 * 是超时统计回调的唯一触发点，函数体有四条分支：
 *
 *     1. !conn || !conn->server → 直接 return
 *     2. server->config 有 → timeout_ms = config->connection_timeout * 1000
 *        无 config → 走 UVHTTP_CONNECTION_TIMEOUT_DEFAULT
 *     3. server->timeout_callback 非空 → 回调 (server, conn, timeout_ms,
 * user_data) 为空 → 跳过回调，直接关闭
 *     4. 最后 uvhttp_connection_close(conn)
 *
 * 现有 test_connection_boost_coverage.cpp:221 定义了 g_timeout_callback_called
 * 与 test_timeout_callback，在 TimeoutCallback_WithCallbackSet 中把计数器
 * 置 0 后调用 start_timeout —— 但**从不跑 loop 等 timer 真正触发**，
 * 全文**没有任何 EXPECT 断言该计数器**。回调体四条分支一次都没执行过。
 *
 * 超时回调是连接回收的关键路径：回调不触发意味着应用层永远收不到超时
 * 统计（无法区分「客户端慢」与「客户端消失」）；回调重复触发则会让
 * 上层统计虚高。
 *
 * 注：UVHTTP_CONNECTION_TIMEOUT_MIN = 5 秒（uvhttp_defaults.h:81），
 * 每个等待 timer 的测试至少耗时 5 秒。为控制总时长，需要等 timer 的
 * 断言合并在少数几个测试内完成。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_connection.h"
#include "uvhttp_defaults.h"
#include "uvhttp_server.h"
}

#include <string.h>
#include <uv.h>

namespace {

/* 记录回调收到的参数，用于断言而非只数次数 */
int g_call_count = 0;
uint64_t g_last_timeout_ms = 0;
void* g_last_user_data = nullptr;
uvhttp_connection_t* g_last_conn = nullptr;

void RecordingTimeoutCallback(uvhttp_server_t* server,
                              uvhttp_connection_t* conn, uint64_t timeout_ms,
                              void* user_data) {
    (void)server;
    g_call_count++;
    g_last_timeout_ms = timeout_ms;
    g_last_user_data = user_data;
    g_last_conn = conn;
}

void ResetRecording() {
    g_call_count = 0;
    g_last_timeout_ms = 0;
    g_last_user_data = nullptr;
    g_last_conn = nullptr;
}

/* 合法范围内的最小超时——UVHTTP_CONNECTION_TIMEOUT_MIN 为 5 秒，
 * 小于该值 start_timeout_custom 直接返回 INVALID_PARAM */
constexpr int kMinTimeout = UVHTTP_CONNECTION_TIMEOUT_MIN;

}  // namespace

class ConnectionTimeoutCallback : public ::testing::Test {
   protected:
    uv_loop_t loop_{};
    uvhttp_server_t* server_ = nullptr;
    uvhttp_connection_t* conn_ = nullptr;

    void SetUp() override {
        ResetRecording();
        ASSERT_EQ(uv_loop_init(&loop_), 0);
        ASSERT_EQ(uvhttp_server_new(&loop_, &server_), UVHTTP_OK);
        ASSERT_NE(server_, nullptr);
        ASSERT_EQ(uvhttp_connection_new(server_, &conn_), UVHTTP_OK);
        ASSERT_NE(conn_, nullptr);
    }

    void TearDown() override {
        /* Free the connection before the server. uvhttp_connection_free()
         * closes its handles; the close callbacks that actually release the
         * connection only run once the loop is pumped below. Skipping the
         * free leaks the ~76 KB uvhttp_connection_new() allocated —
         * LeakSanitizer caught exactly this. */
        if (conn_) {
            if (!uv_is_closing((uv_handle_t*)&conn_->timeout_timer)) {
                uv_timer_stop(&conn_->timeout_timer);
            }
            uvhttp_connection_free(conn_);
            conn_ = nullptr;
        }
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        /* pump so the close callbacks land, then close the loop */
        uv_run(&loop_, UV_RUN_DEFAULT);
        uv_loop_close(&loop_);
    }

    /* Run the loop until the timeout fires. connection_timeout_cb calls
     * uvhttp_connection_close(), and on_handle_close then RELEASES the
     * connection — after this returns the pointer is dangling, so ownership
     * is dropped here to keep TearDown from freeing it a second time
     * (uvhttp_connection_free's double-free guard still dereferences the
     * struct to read conn->freed, so calling it on freed memory is a
     * use-after-free, not a no-op). */
    void RunUntilTimeoutFires() {
        uv_run(&loop_, UV_RUN_DEFAULT);
        conn_ = nullptr;
    }
};

// ============================================================================
// 回调触发 + 关闭 + 参数（合并为一个测试以控制耗时）
// ============================================================================

TEST_F(ConnectionTimeoutCallback, CallbackFiresOnceThenClosesConnection) {
    int marker = 42;
    server_->timeout_callback = RecordingTimeoutCallback;
    server_->timeout_callback_user_data = &marker;
    /* uvhttp_server_new 不分配 config，故此处 config 为 NULL——
     * 走 connection_timeout_cb 的分支 2 else 侧：用
     * UVHTTP_CONNECTION_TIMEOUT_DEFAULT(60) 而非 config 值。 */
    ASSERT_EQ(server_->config, nullptr)
        << "本用例依赖 config 为 NULL 的默认分支";

    ASSERT_EQ(uvhttp_connection_start_timeout_custom(conn_, kMinTimeout),
              UVHTTP_OK);
    /* keep the pointer for the assertion below — RunUntilTimeoutFires()
     * drops our ownership of it */
    uvhttp_connection_t* expected_conn = conn_;
    RunUntilTimeoutFires();

    /* 分支 3：回调恰好触发一次，且参数正确 */
    EXPECT_EQ(g_call_count, 1) << "超时后应用层回调应恰好触发一次";
    EXPECT_EQ(g_last_conn, expected_conn) << "回调应收到超时的那个连接";
    EXPECT_EQ(g_last_user_data, &marker) << "user_data 应原样透传";
    EXPECT_EQ(g_last_timeout_ms,
              static_cast<uint64_t>(UVHTTP_CONNECTION_TIMEOUT_DEFAULT) * 1000)
        << "config 为 NULL 时 timeout_ms 应取默认值 * 1000";

    /* 分支 4：回调后 uvhttp_connection_close(conn) 被调用，随后
     * on_handle_close 释放整个 conn——此后访问 conn_ 任何字段都是
     * use-after-free（ASan 会直接报错），故不在此断言连接状态。
     * 回调本身被调用即是进入关闭流程的证据：close 是回调之后的下一条语句。 */
}

// ============================================================================
// 反复 start 不累积多个 timer
// ============================================================================

/* 反复 start 应只保留一个 timer——否则单次超时会触发多次回调，
 * 令应用层超时统计虚高 */
TEST_F(ConnectionTimeoutCallback, RepeatedStartDoesNotMultiplyCallbacks) {
    server_->timeout_callback = RecordingTimeoutCallback;

    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(uvhttp_connection_start_timeout_custom(conn_, kMinTimeout),
                  UVHTTP_OK);
    }
    RunUntilTimeoutFires();

    EXPECT_EQ(g_call_count, 1)
        << "反复 start_timeout 不应导致回调被多次调用（实际 " << g_call_count
        << " 次）";
}

// ============================================================================
// 无回调时安全关闭
// ============================================================================

/* timeout_callback 为 NULL 时应直接关闭连接，不崩溃（分支 3 的 else 侧） */
TEST_F(ConnectionTimeoutCallback, NoCallbackConfiguredDoesNotCrash) {
    server_->timeout_callback = nullptr;
    ASSERT_EQ(uvhttp_connection_start_timeout_custom(conn_, kMinTimeout),
              UVHTTP_OK);

    /* 分支 3 的 else 侧：无回调时直接关闭连接，不应崩溃。
     * 同样不能断言 conn_ 字段——超时后连接已被释放。 */
    RunUntilTimeoutFires();
    EXPECT_EQ(g_call_count, 0) << "未配置回调时不应有回调被调用";
}

// ============================================================================
// 参数校验（无需等待 timer，快速用例）
// ============================================================================

TEST_F(ConnectionTimeoutCallback, NullConnReturnsError) {
    EXPECT_EQ(uvhttp_connection_start_timeout(nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_connection_start_timeout_custom(nullptr, 10),
              UVHTTP_ERROR_INVALID_PARAM);
}

/* 小于 MIN 的超时值被拒——start_timeout_custom 的范围校验分支 */
TEST_F(ConnectionTimeoutCallback, TimeoutBelowMinimumIsRejected) {
    ASSERT_GT(UVHTTP_CONNECTION_TIMEOUT_MIN, 0);
    EXPECT_EQ(uvhttp_connection_start_timeout_custom(
                  conn_, UVHTTP_CONNECTION_TIMEOUT_MIN - 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

/* 大于 MAX 的超时值被拒 */
TEST_F(ConnectionTimeoutCallback, TimeoutAboveMaximumIsRejected) {
    EXPECT_EQ(uvhttp_connection_start_timeout_custom(
                  conn_, UVHTTP_CONNECTION_TIMEOUT_MAX + 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

/* 恰好等于 MIN 的值应合法 */
TEST_F(ConnectionTimeoutCallback, TimeoutExactlyAtMinimumIsAccepted) {
    /* 不启动就关闭，只验证参数校验通过 */
    EXPECT_EQ(uvhttp_connection_start_timeout_custom(
                  conn_, UVHTTP_CONNECTION_TIMEOUT_MIN),
              UVHTTP_OK);
    uv_timer_stop(&conn_->timeout_timer);
}

/* 恰好等于 MAX 的值应合法 */
TEST_F(ConnectionTimeoutCallback, TimeoutExactlyAtMaximumIsAccepted) {
    EXPECT_EQ(uvhttp_connection_start_timeout_custom(
                  conn_, UVHTTP_CONNECTION_TIMEOUT_MAX),
              UVHTTP_OK);
    uv_timer_stop(&conn_->timeout_timer);
}

/* 关闭连接后 start_timeout 应仍安全（timer 已停，不会再触发回调） */
TEST_F(ConnectionTimeoutCallback, StartAfterCloseDoesNotFireCallback) {
    server_->timeout_callback = RecordingTimeoutCallback;
    uvhttp_connection_close(conn_);
    /* close 后再 start timer 不应导致回调触发 */
    uvhttp_connection_start_timeout_custom(conn_, kMinTimeout);
    RunUntilTimeoutFires();

    EXPECT_EQ(g_call_count, 0) << "连接关闭后不应再触发超时回调";
}
