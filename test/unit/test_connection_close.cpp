/*
 * UVHTTP 连接关闭路径测试
 *
 * 为什么需要独立测试：uvhttp_connection_close（src/uvhttp_connection.c:940）
 * 的三个分支在既有测试中从未执行：
 *
 *   1. **幂等重入守卫**（:947-949）——state==CLOSING 且 close_pending>0 时直接
 *      return。代码注释明确指出这是历史泄漏修复点：
 *        "A second close previously reset close_pending to 0 here, discarding
 *         the in-flight count so on_handle_close would underflow past 0 and
 *         never free the connection (leak)."
 *      既有测试要么只 close 一次，要么（如
 * test_connection_timeout_callback.cpp:107）
 *      在回调已关闭连接后刻意丢弃所有权来避免二次 close——恰好绕开此守卫。
 *
 *   2. **三个句柄各自的 else 分支**（:965/971/980 的 `else if
 * (uv_is_closing(...))` → already_closing++）——既有 close
 * 测试调用时三个句柄都未 mid-close， 恒走 if(!uv_is_closing) 正分支。
 *
 *   3. **同步释放兜底**（:993）——close_pending==0 且 already_closing==0 时直接
 *      free_resources（全部句柄已 mid-close 的情形）。
 *
 * on_handle_close（:924）执行 `conn->close_pending--`，故 1 与 2 的正确性
 * 直接决定连接是否泄漏或下溢——lcov 显示本文件 8 个函数完全零覆盖。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
}

#include <string.h>
#include <uv.h>

class ConnectionClose : public ::testing::Test {
   protected:
    uv_loop_t loop_{};
    uvhttp_server_t* server_ = nullptr;
    uvhttp_connection_t* conn_ = nullptr;

    void SetUp() override {
        ASSERT_EQ(uv_loop_init(&loop_), 0);
        ASSERT_EQ(uvhttp_server_new(&loop_, &server_), UVHTTP_OK);
        ASSERT_NE(server_, nullptr);
        ASSERT_EQ(uvhttp_connection_new(server_, &conn_), UVHTTP_OK);
        ASSERT_NE(conn_, nullptr);
    }

    /* 排水让 close 回调落地。调用后 conn_ 已被释放，测试不得再访问它。 */
    void Drain() {
        uv_run(&loop_, UV_RUN_DEFAULT);
        conn_ = nullptr;
    }

    void TearDown() override {
        if (conn_) {
            /* 未被 close 的连接：先 close 再排水 */
            uvhttp_connection_close(conn_);
            uv_run(&loop_, UV_RUN_DEFAULT);
            conn_ = nullptr;
        }
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        uv_run(&loop_, UV_RUN_DEFAULT);
        uv_loop_close(&loop_);
    }
};

/* ========== 分支 1：幂等重入守卫 ========== */

/* 关闭进行中时二次 close：必须直接返回，不得重置 close_pending。
 * 若守卫失效，close_pending 被重置为 0 并重新 close 三个句柄，随后三个
 * on_handle_close 各自 -- 会下溢到负数——连接永不释放（泄漏）。 */
TEST_F(ConnectionClose, SecondCloseWhileInProgressDoesNotResetPending) {
    uvhttp_connection_close(conn_);
    ASSERT_EQ(conn_->state, UVHTTP_CONN_STATE_CLOSING);
    const int pending_after_first = conn_->close_pending;
    ASSERT_GT(pending_after_first, 0) << "首次 close 应登记待关闭句柄数";

    /* 第二次 close：应被幂等守卫拦下 */
    uvhttp_connection_close(conn_);

    EXPECT_EQ(conn_->close_pending, pending_after_first)
        << "重入 close 不得重置 close_pending（重置会让 on_handle_close 的 -- "
           "下溢，连接永不释放——这正是该守卫修复的历史泄漏）";

    Drain();
}

/* 连续多次 close 同样不得扰动计数 */
TEST_F(ConnectionClose, RepeatedCloseDoesNotChangePending) {
    uvhttp_connection_close(conn_);
    const int pending_after_first = conn_->close_pending;
    ASSERT_GT(pending_after_first, 0);

    for (int i = 0; i < 5; i++) {
        uvhttp_connection_close(conn_);
        EXPECT_EQ(conn_->close_pending, pending_after_first)
            << "第 " << (i + 1) << " 次重入 close 扰动了 close_pending";
    }

    Drain();
}

/* ========== 分支 2：句柄已 mid-close（already_closing） ========== */

/* 先把 timeout_timer 置为 mid-close，再调 close：该句柄走 already_closing++，
 * 不计入 close_pending，其余两个句柄正常计数。 */
TEST_F(ConnectionClose, AlreadyClosingHandleIsCountedSeparately) {
    ASSERT_FALSE(uv_is_closing((uv_handle_t*)&conn_->timeout_timer));
    uv_close((uv_handle_t*)&conn_->timeout_timer, nullptr);
    ASSERT_TRUE(uv_is_closing((uv_handle_t*)&conn_->timeout_timer))
        << "预置条件：timeout_timer 应处于 mid-close";

    uvhttp_connection_close(conn_);

    /* idle + tcp 两个句柄计入 close_pending（timer 不计） */
    EXPECT_EQ(conn_->close_pending, 2)
        << "已 mid-close 的 timer 不应计入 close_pending";

    Drain();
}

/* 注：末尾同步释放兜底（src/uvhttp_connection.c:993
 * `if (close_pending == 0 && already_closing == 0) free_resources()`）
 * 在真实 libuv 下不可达——只要有任何句柄处于 mid-close，already_closing
 * 就非零；而全部句柄都未 mid-close 时 uvhttp_connection_close 必然为其
 * 中三个发起 uv_close 并令 close_pending 增至 3。两条件不可能同时成立。
 *
 * 初版曾试图用「三个句柄全手工 uv_close」构造该状态，但那样
 * already_closing==3 而非 0，分支不执行；同时手工 close 传的是
 * nullptr 回调，on_handle_close 永不触发，连接无释放路径——ASan 报出
 * 175 KB 泄漏。故移除该用例：它测的是一个在真实运行时不可达的防御分支。 */

/* ========== 分支 3：close 前置校验 ========== */

TEST_F(ConnectionClose, NullConnectionIsSafeNoOp) {
    /* close 返回 void 且首行即判空返回——传 NULL 不应崩溃，且不应影响
     * server 的连接计数。用计数比对给出真实断言（SUCCEED() 不算断言）。 */
    const size_t before = server_->active_connections;
    uvhttp_connection_close(nullptr);
    EXPECT_EQ(server_->active_connections, before)
        << "close(NULL) 不应触碰 server 计数";
    EXPECT_EQ(server_->active_connections, 0u)
        << "本 fixture 未接受任何连接，计数应仍为 0";
}

/* 未 close 过的连接：close_pending 应登记全部三个句柄 */
TEST_F(ConnectionClose, FirstCloseRegistersAllThreeHandles) {
    ASSERT_EQ(conn_->close_pending, 0);
    uvhttp_connection_close(conn_);
    EXPECT_EQ(conn_->close_pending, 3)
        << "idle/timer/tcp 三个句柄都应被计入待关闭数";
    EXPECT_EQ(conn_->state, UVHTTP_CONN_STATE_CLOSING);
    Drain();
}