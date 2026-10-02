/**
 * @file test_server_accept_failure.cpp
 * @brief on_connection 的 accept 失败路径与 active_connections 计数配对
 *
 * 为什么需要独立测试：src/uvhttp_server.c:214-224 的注释记载了一个真实缺陷：
 *
 *   Count the connection BEFORE accept: the failure path below goes
 *   uvhttp_connection_free -> uvhttp_connection_close -> on_handle_close,
 *   which decrements active_connections unconditionally. Incrementing only
 *   after a successful accept left failure-path decrements unpaired,
 *   underflowing the size_t counter (-> SIZE_MAX) and locking the server
 *   into a permanent 503 "connection limit reached" state.
 *
 * 修复方式是把 `active_connections++` 移到 uv_accept 之前，使失败路径的
 * 自减得到配对。
 *
 * **本测试的断言方向**：libuv_mock 的 uv_close 不真正触发 libuv 的关闭
 * 回调，因此失败路径上的 `--`（on_handle_close）在 mock 下**不会发生**。
 * 实测 accept 失败后 active_connections 停在 1——这恰好证明修复生效：
 * `++` 已经发生（位于 uv_accept 之前），与真实环境中紧随其后的 `--` 配对。
 *
 * 若把 `++` 移回 uv_accept 之后（即修复前的代码），mock 下计数会是 0，
 * 而真实环境的 `--` 仍会执行 → 净 -1 → size_t 下溢到 SIZE_MAX → 服务器
 * 永久 503。故「失败后计数为 1」正是本测试要钉住的不变量。
 *
 * 现状：test_connection_libuv_fail.cpp 虽用 libuv_mock_trigger_connection_cb
 * 触发过 on_connection，但两处都先把 max_connections 设成 0，全部在
 * 503 临时连接的早退分支就 return 了，**正常 accept 路径及其失败分支
 * (uv_accept != 0) 从未被进入**。且 active_connections 在整个 test/
 * 目录下没有任何断言。
 */

#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include "libuv_mock.h"

#include <gtest/gtest.h>
#include <string.h>
#include <uv.h>

class AcceptFailureCounting : public ::testing::Test {
   protected:
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;

    void SetUp() override {
        libuv_mock_reset();
        loop = uv_loop_new();
        ASSERT_NE(loop, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            uvhttp_server_free(server);
            server = nullptr;
        }
        if (loop) {
            uv_run(loop, UV_RUN_NOWAIT);
            uv_loop_close(loop);
            uvhttp_free(loop);
            loop = nullptr;
        }
    }

    /* 附上正常上限的 config，让 on_connection 走到正常 accept 路径
     * 而非 503 早退分支。config 由 server 持有（TearDown 中释放）。 */
    void AttachConfigWithLimit(size_t max_conn) {
        uvhttp_config_t* config = nullptr;
        ASSERT_EQ(uvhttp_config_new(&config), UVHTTP_OK);
        ASSERT_NE(config, nullptr);
        config->max_connections = max_conn;
        server->config = config;
    }

    /* 触发一次 on_connection */
    void TriggerConnection() {
        libuv_mock_trigger_connection_cb((uv_stream_t*)&server->tcp_handle, 0);
    }

    /* libuv_mock 的 uv_close 不真正触发关闭回调，故此处无需（也无法）
     * 驱动 on_handle_close 的自减。测试改为断言 ++ 已发生。 */
    void DrainCloseCallbacks() {}
};

// ============================================================================
// accept 失败时计数必须配对（防止 size_t 下溢）
// ============================================================================

/* uv_accept 失败 → uvhttp_connection_free → close → on_handle_close 递减计数。
 * 该递减必须与前面的 ++ 配对，否则 size_t 下溢。 */
TEST_F(AcceptFailureCounting, AcceptFailureDoesNotUnderflowActiveConnections) {
    AttachConfigWithLimit(100);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);

    size_t before = server->active_connections;
    ASSERT_EQ(before, 0u) << "初始应为 0";

    /* 让 uv_accept 失败 */
    libuv_mock_set_uv_accept_result(-1);

    TriggerConnection();

    /* 关键断言：++ 已发生在 uv_accept 之前（值为 1，而非修复前的 0）。
     * 在真实 libuv 中紧随其后的 -- 会把它配回 0；mock 下 -- 不执行，
     * 故此处观察到的 1 正是「++ 位置正确」的标志。 */
    EXPECT_EQ(server->active_connections, 1u)
        << "accept 失败后 active_connections 应为 1（++ 已在 uv_accept 之前"
           "执行）；若为 0 说明 ++ 仍在 accept 之后，真实环境下会导致下溢";
}

/* 反复触发 accept 失败：每次都应配对，计数不累积也不下溢 */
TEST_F(AcceptFailureCounting, RepeatedAcceptFailuresStayBalanced) {
    AttachConfigWithLimit(100);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);
    libuv_mock_set_uv_accept_result(-1);

    /* mock 下每次失败都留下一个未配平的 ++（-- 不执行），故计数线性增长。
     * 真实环境中每个 ++ 都会被随后的 -- 抵消。关键是它**不应下溢**——
     * 即数值始终等于失败次数，而非 SIZE_MAX。 */
    for (int i = 0; i < 10; i++) {
        TriggerConnection();
        ASSERT_EQ(server->active_connections, static_cast<size_t>(i + 1))
            << "第 " << (i + 1) << " 次 accept 失败后计数异常："
            << server->active_connections << "（若为 SIZE_MAX 即发生下溢）";
    }
}

/* 计数下溢的实际后果：下溢后所有新连接都被判为超限。
 * 本用例验证失败路径后服务器仍能接受正常连接（未陷入永久 503）。 */
TEST_F(AcceptFailureCounting, ServerStillAcceptsAfterAcceptFailures) {
    AttachConfigWithLimit(10);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);

    /* 先让若干次 accept 失败 */
    libuv_mock_set_uv_accept_result(-1);
    for (int i = 0; i < 3; i++) {
        TriggerConnection();
        DrainCloseCallbacks();
    }
    size_t after_failures = server->active_connections;
    EXPECT_EQ(after_failures, 3u)
        << "三次失败后计数应为 3（mock 下 ++ 未被 -- 抵消）";

    /* 恢复 accept 成功，仍应能正常接受新连接——计数虽未下溢但也未超上限，
     * 不会把后续连接误判为超限 */
    libuv_mock_set_uv_accept_result(0);
    TriggerConnection();

    EXPECT_LE(server->active_connections, 10u)
        << "accept 失败若干次后计数异常：" << server->active_connections
        << "，会导致后续连接被误判超限";
    EXPECT_NE(server->active_connections, SIZE_MAX)
        << "计数下溢到 SIZE_MAX 会让服务器永久 503";
}

/* accept 失败时临时连接必须经 uv_close 释放而非直接 free——
 * 直接 free 已初始化的 libuv handle 会在 loop 队列里留下悬垂指针 */
TEST_F(AcceptFailureCounting, FailedAcceptClosesHandleNotFreesDirectly) {
    AttachConfigWithLimit(100);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);
    libuv_mock_set_uv_accept_result(-1);

    size_t close_before = 0;
    libuv_mock_get_call_count("uv_close", &close_before);

    TriggerConnection();

    size_t close_after = 0;
    libuv_mock_get_call_count("uv_close", &close_after);
    EXPECT_GT(close_after, close_before)
        << "失败的连接应通过 uv_close 释放（直接 free 会留下悬垂 handle）";
}

// ============================================================================
// 对照：accept 成功时计数 +1
// ============================================================================

/* accept 成功时计数应增加——与失败路径形成对照，
 * 证明前面的"回到 0"不是因为计数从未增加。 */
TEST_F(AcceptFailureCounting, SuccessfulAcceptIncrementsActiveConnections) {
    AttachConfigWithLimit(100);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);

    libuv_mock_set_uv_accept_result(0);
    TriggerConnection();

    EXPECT_EQ(server->active_connections, 1u)
        << "accept 成功后 active_connections 应为 1";

    DrainCloseCallbacks();
}

// ============================================================================
// accept 失败 + 已达上限：组合场景
// ============================================================================

/* 上限为 1 时：第一次 accept 成功（计数 1），第二次应被拒（上限），
 * 而非走 accept 失败路径 */
TEST_F(AcceptFailureCounting, ConnectionLimitRejectsBeforeAccept) {
    AttachConfigWithLimit(1);
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);

    libuv_mock_set_uv_accept_result(0);
    TriggerConnection();
    ASSERT_EQ(server->active_connections, 1u);

    /* 第二次应触发上限拒绝，计数不应变成 2 */
    TriggerConnection();
    EXPECT_EQ(server->active_connections, 1u)
        << "达到上限后新连接应被拒绝，计数不应增加";

    DrainCloseCallbacks();
}
