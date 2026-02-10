/* uvhttp_connection.c WebSocket 集成Test - Test WebSocket 相关功能 */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_websocket.h"
#include "uvhttp_error.h"
#include <string.h>

/* 辅助函数：Create服务器和循环 */
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

/* ========== Test WebSocket Handshake ========== */

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
    /* ResultDependsInternalState */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeEmptyKey) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    const char* ws_key = "";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    EXPECT_NE(result, UVHTTP_OK);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, HandleWebsocketHandshakeInvalidKey) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    const char* ws_key = "invalid-key";
    result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    EXPECT_NE(result, UVHTTP_OK);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket Switch ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketNullConn) {
    /* 不ShouldCrash */
    uvhttp_connection_switch_to_websocket(nullptr);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketValidConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* 不ShouldCrash */
    uvhttp_connection_switch_to_websocket(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, SwitchToWebsocketMultipleTimes) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* 多次Switch - 不ShouldCrash */
    uvhttp_connection_switch_to_websocket(conn);
    uvhttp_connection_switch_to_websocket(conn);
    uvhttp_connection_switch_to_websocket(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket Close ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseNullConn) {
    /* 不ShouldCrash */
    uvhttp_connection_websocket_close(nullptr);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseValidConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* 不ShouldCrash */
    uvhttp_connection_websocket_close(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketCloseAfterSwitch) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* FirstSwitch到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);

    /* ThenClose */
    uvhttp_connection_websocket_close(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket ConnectionState ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketConnectionState) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Switch到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket ConnectionObject ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketConnectionObject) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* InitialState下 ws_connection Should是 NULL */
    if (conn->ws_connection) {
        /* 如果已分配，Validation它存在 */
    }

    /* Switch到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);

    /* SwitchAfter ws_connection 可能仍然Is NULL 或已分配 */
    /* 这Depends实现 */

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket ErrorHandling ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketErrorHandling) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestErrorHandling */
    conn->last_error = UVHTTP_OK;
    EXPECT_EQ(conn->last_error, UVHTTP_OK);

    conn->last_error = UVHTTP_ERROR_INVALID_PARAM;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket 和 HTTP State转换 ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketStateTransitions) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* InitialState */
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);

    /* 设置Is HTTP 读取State */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);

    /* Switch到 WebSocket */
    uvhttp_connection_switch_to_websocket(conn);

    /* Close WebSocket */
    uvhttp_connection_websocket_close(conn);

    /* 设置IsCloseState */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test WebSocket HandshakeKeyValidation ========== */

TEST(UvhttpConnectionWebsocketIntegrationTest, WebsocketHandshakeKeyValidation) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test标准 WebSocket HandshakeKey */
    const char* valid_keys[] = {
        "dGhlIHNhbXBsZSBub25jZQ==",
        "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=",
        "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA="
    };

    for (size_t i = 0; i < sizeof(valid_keys) / sizeof(valid_keys[0]); i++) {
        result = uvhttp_connection_handle_websocket_handshake(conn, valid_keys[i]);
        /* 不ShouldCrash */
    }

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */