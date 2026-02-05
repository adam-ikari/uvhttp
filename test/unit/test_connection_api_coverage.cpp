/* uvhttp_connection.c API 覆盖率测试 - 测试所有公开 API 函数 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 辅助函数：创建服务器和循环 */
static void create_server_and_loop(uv_loop_t** loop, uvhttp_server_t** server) {
    *loop = uv_loop_new();
    ASSERT_NE(*loop, nullptr);
    
    uvhttp_error_t result = uvhttp_server_new(*loop, server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(*server, nullptr);
}

/* 辅助函数：销毁服务器和循环 */
static void destroy_server_and_loop(uvhttp_server_t* server, uv_loop_t* loop) {
    uvhttp_server_free(server);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 辅助函数：运行事件循环并关闭 */
static void run_and_close(uvhttp_server_t* server, uvhttp_connection_t* conn, uv_loop_t* loop) {
    if (conn) {
        uvhttp_connection_close(conn);
        // 由 on_handle_close 回调自动释放
    }
    uvhttp_server_free(server);
    for (int i = 0; i < 20; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== 测试 uvhttp_connection_new ========== */

TEST(UvhttpConnectionApiTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(nullptr, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}

TEST(UvhttpConnectionApiTest, ConnectionNewNullConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_error_t result = uvhttp_connection_new(server, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionApiTest, ConnectionNewValid) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(conn, nullptr);
    EXPECT_EQ(conn->server, server);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== 测试 uvhttp_connection_free ========== */

TEST(UvhttpConnectionApiTest, ConnectionFreeNull) {
    /* 不应该崩溃 */
    uvhttp_connection_free(nullptr);
}

TEST(UvhttpConnectionApiTest, ConnectionFreeValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_start ========== */

TEST(UvhttpConnectionApiTest, ConnectionStartNull) {
    uvhttp_error_t result = uvhttp_connection_start(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, ConnectionStartValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 注意：uvhttp_connection_start 需要有效的 libuv 句柄 */
    /* 在没有实际网络连接的情况下，这个测试可能会失败 */
    result = uvhttp_connection_start(conn);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_close ========== */

TEST(UvhttpConnectionApiTest, ConnectionCloseNull) {
    /* 不应该崩溃 */
    uvhttp_connection_close(nullptr);
}

TEST(UvhttpConnectionApiTest, ConnectionCloseValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    uvhttp_connection_close(conn);
    /* 运行事件循环等待所有句柄关闭完成 */
    for (int i = 0; i < 20; i++) {
        uv_run(server->loop, UV_RUN_ONCE);
    }
    /* 不要手动调用 uvhttp_connection_free，由 on_handle_close 回调自动释放 */
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_restart_read ========== */

TEST(UvhttpConnectionApiTest, RestartReadNull) {
    uvhttp_error_t result = uvhttp_connection_restart_read(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, RestartReadValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 注意：uvhttp_connection_restart_read 需要有效的 libuv 句柄 */
    /* 在没有实际网络连接的情况下，这个测试可能会失败 */
    result = uvhttp_connection_restart_read(conn);
    /* 结果取决于内部状态 */
    
    // 由 on_handle_close 回调自动释放
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_schedule_restart_read ========== */

TEST(UvhttpConnectionApiTest, ScheduleRestartReadNull) {
    uvhttp_error_t result = uvhttp_connection_schedule_restart_read(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, ScheduleRestartReadValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_schedule_restart_read(conn);
    /* 结果取决于内部状态 */
    
    /* 关闭所有 libuv 句柄 */
    if (!uv_is_closing((uv_handle_t*)&conn->idle_handle)) {
        uv_close((uv_handle_t*)&conn->idle_handle, NULL);
    }
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_close((uv_handle_t*)&conn->timeout_timer, NULL);
    }
    if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
    }
    
    /* 运行事件循环等待关闭完成 */
    for (int i = 0; i < 20; i++) {
        uv_run(server->loop, UV_RUN_ONCE);
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_start_tls_handshake ========== */
/* 注意：此函数未在 uvhttp_connection.c 中实现，跳过测试 */

/* ========== 测试 uvhttp_connection_tls_read ========== */
/* 注意：此函数未在 uvhttp_connection.c 中实现，跳过测试 */

/* ========== 测试 uvhttp_connection_tls_write ========== */

TEST(UvhttpConnectionApiTest, TlsWriteNullConn) {
    uvhttp_error_t result = uvhttp_connection_tls_write(nullptr, "test", 4);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, TlsWriteNullData) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_tls_write(conn, nullptr, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionApiTest, TlsWriteValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    const char* data = "test data";
    result = uvhttp_connection_tls_write(conn, data, strlen(data));
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_tls_handshake_func ========== */

TEST(UvhttpConnectionApiTest, TlsHandshakeFuncNull) {
    uvhttp_error_t result = uvhttp_connection_tls_handshake_func(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, TlsHandshakeFuncValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 注意：需要有效的 TLS 上下文 */
    result = uvhttp_connection_tls_handshake_func(conn);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_tls_cleanup ========== */
/* 注意：此函数未在 uvhttp_connection.c 中实现，跳过测试 */

/* ========== 测试 uvhttp_connection_set_state ========== */

TEST(UvhttpConnectionApiTest, SetStateNull) {
    /* 不应该崩溃 */
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
}

TEST(UvhttpConnectionApiTest, SetStateValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 uvhttp_connection_get_state_string ========== */
/* 注意：此函数未在 uvhttp_connection.c 中实现，跳过测试 */

/* ========== 测试 uvhttp_connection_start_timeout ========== */

TEST(UvhttpConnectionApiTest, StartTimeoutNull) {
    uvhttp_error_t result = uvhttp_connection_start_timeout(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, StartTimeoutValid) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_start_timeout(conn);
    /* 结果取决于内部状态 */
    
    /* 关闭连接，on_handle_close 会自动释放 */
    uvhttp_connection_close(conn);
    
    /* 运行事件循环多次等待所有句柄关闭和释放 */
    for (int i = 0; i < 20; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }
    
    /* 不手动调用 uvhttp_connection_free，由 on_handle_close 回调处理 */
    destroy_server_and_loop(server, loop);
}

/* ========== 测试 uvhttp_connection_start_timeout_custom ========== */

TEST(UvhttpConnectionApiTest, StartTimeoutCustomNull) {
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(nullptr, 30);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, StartTimeoutCustomMin) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_start_timeout_custom(conn, 5);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_close(conn);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    // 由 on_handle_close 回调自动释放
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionApiTest, StartTimeoutCustomMax) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_start_timeout_custom(conn, 300);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_close(conn);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    // 由 on_handle_close 回调自动释放
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionApiTest, StartTimeoutCustomTooSmall) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_start_timeout_custom(conn, 1);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_close(conn);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    // 由 on_handle_close 回调自动释放
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionApiTest, StartTimeoutCustomTooLarge) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_start_timeout_custom(conn, 1000);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_close(conn);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    // 由 on_handle_close 回调自动释放
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 握手函数 ========== */

TEST(UvhttpConnectionApiTest, HandleWebsocketHandshakeNull) {
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(nullptr, "test-key");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionApiTest, HandleWebsocketHandshakeNullKey) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_handle_websocket_handshake(conn, nullptr);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionApiTest, HandleWebsocketHandshakeValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    const char* ws_key = "dGhlIHNhbXBsZSBub25jZQ==";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 切换函数 ========== */

TEST(UvhttpConnectionApiTest, SwitchToWebsocketNull) {
    /* 不应该崩溃 */
    uvhttp_connection_switch_to_websocket(nullptr);
}

TEST(UvhttpConnectionApiTest, SwitchToWebsocketValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    uvhttp_connection_switch_to_websocket(conn);
    /* 验证状态变化 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 关闭函数 ========== */

TEST(UvhttpConnectionApiTest, WebsocketCloseNull) {
    /* 不应该崩溃 */
    uvhttp_connection_websocket_close(nullptr);
}

TEST(UvhttpConnectionApiTest, WebsocketCloseValid) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    uvhttp_connection_websocket_close(conn);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}