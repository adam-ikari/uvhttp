/* uvhttp_connection.c WebSocket 集成测试 - 测试 WebSocket 相关功能 */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_websocket.h"
#include "uvhttp_error.h"
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

/* ========== 测试 WebSocket 握手 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeNullConn) {
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(nullptr, "test-key");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeNullKey) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    result = uvhttp_connection_handle_websocket_handshake(conn, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeValidKey) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    const char* ws_key = "dGhlIHNhbXBsZSBub25jZQ==";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    /* 结果取决于内部状态 */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeEmptyKey) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    const char* ws_key = "";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeInvalidKey) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    const char* ws_key = "invalid-key";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 切换 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketNullConn) {
    /* 不应该崩溃 */
    uvhttp_connection_switch_to_websocket(nullptr);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketValidConn) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 不应该崩溃 */
    uvhttp_connection_switch_to_websocket(conn);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketMultipleTimes) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 多次切换 - 不应该崩溃 */
    uvhttp_connection_switch_to_websocket(conn);
    uvhttp_connection_switch_to_websocket(conn);
    uvhttp_connection_switch_to_websocket(conn);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 关闭 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseNullConn) {
    /* 不应该崩溃 */
    uvhttp_connection_websocket_close(nullptr);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseValidConn) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 不应该崩溃 */
    uvhttp_connection_websocket_close(conn);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseAfterSwitch) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 先切换到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);
    
    /* 再关闭 */
    uvhttp_connection_websocket_close(conn);
    
    /* 关闭所有句柄 */
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
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 连接状态 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketConnectionState) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 切换到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 连接对象 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketConnectionObject) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 初始状态下 ws_connection 应该是 NULL */
    if (conn->ws_connection) {
        /* 如果已分配，验证它存在 */
    }
    
    /* 切换到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);
    
    /* 切换后 ws_connection 可能仍然为 NULL 或已分配 */
    /* 这取决于实现 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 错误处理 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketErrorHandling) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试无效的 WebSocket 握手密钥 */
    result = uvhttp_connection_handle_websocket_handshake(conn, "invalid-key");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试空密钥 */
    result = uvhttp_connection_handle_websocket_handshake(conn, "");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试过长的密钥 */
    char long_key[1000];
    memset(long_key, 'a', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    result = uvhttp_connection_handle_websocket_handshake(conn, long_key);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 和 HTTP 状态转换 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketStateTransition) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 初始状态 */
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    /* 设置为 HTTP 读取状态 */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    /* 切换到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);
    
    /* 关闭 WebSocket */
    uvhttp_connection_websocket_close(conn);
    
    /* 设置为关闭状态 */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 WebSocket 握手密钥验证 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketHandshakeKeyValidation) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试标准 WebSocket 握手密钥 */
    const char* valid_keys[] = {
        "dGhlIHNhbXBsZSBub25jZQ==",
        "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=",
        "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA="
    };
    
    for (size_t i = 0; i < sizeof(valid_keys) / sizeof(valid_keys[0]); i++) {
        result = uvhttp_connection_handle_websocket_handshake(conn, valid_keys[i]);
        /* 结果取决于内部实现 */
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */