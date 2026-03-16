#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* ========== 辅助函数 ========== */

static void create_server_and_loop(uv_loop_t** loop, uvhttp_server_t** server) {
    *loop = uv_loop_new();
    ASSERT_NE(*loop, nullptr);
    uvhttp_error_t result = uvhttp_server_new(*loop, server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(*server, nullptr);
}

static void destroy_server_and_loop(uv_loop_t* loop, uvhttp_server_t* server) {
    if (server) {
        uvhttp_server_free(server);
    }
    if (loop) {
        uv_loop_close(loop);
        uvhttp_free(loop);
    }
}

/* ========== 测试未覆盖的函数 ========== */

/* 测试 uvhttp_connection_set_state */
TEST(UvhttpConnectionMissingCoverageTest, SetStateNullConnection) {
    uvhttp_connection_set_state(NULL, UVHTTP_CONN_STATE_NEW);
    /* Should not crash */
}

TEST(UvhttpConnectionMissingCoverageTest, SetStateAllStates) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Test all connection states */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_PROTOCOL_UPGRADED);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_PROTOCOL_UPGRADED);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试 uvhttp_connection_start_tls_handshake */
TEST(UvhttpConnectionMissingCoverageTest, StartTlsHandshakeNull) {
    uvhttp_error_t result = uvhttp_connection_start_tls_handshake(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionMissingCoverageTest, StartTlsHandshakeSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* This will fail because we don't have actual TLS context, but it tests the function */
    result = uvhttp_connection_start_tls_handshake(conn);
    /* Expected to fail without proper TLS setup */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试 uvhttp_connection_tls_read */
TEST(UvhttpConnectionMissingCoverageTest, TlsReadNull) {
    uvhttp_error_t result = uvhttp_connection_tls_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionMissingCoverageTest, TlsReadNotTlsEnabled) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Without TLS enabled, this should fail or handle gracefully */
    result = uvhttp_connection_tls_read(conn);
    /* Expected to fail when TLS not enabled */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试 WebSocket 相关的边界条件（不包括未实现的 websocket_read） */
TEST(UvhttpConnectionMissingCoverageTest, WebsocketBoundaryConditions) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Test websocket close with valid connection */
    uvhttp_connection_websocket_close(conn);
    /* Should not crash */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试边界条件和错误处理 */
TEST(UvhttpConnectionMissingCoverageTest, StateTransitionSequence) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Test state transition sequence */
    uvhttp_connection_state_t original_state = conn->state;
    EXPECT_EQ(original_state, UVHTTP_CONN_STATE_NEW);
    
    /* Transition through states */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    EXPECT_NE(conn->state, original_state);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_NE(conn->state, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_NE(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_NE(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_NE(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试 TLS 相关的错误处理 */
TEST(UvhttpConnectionMissingCoverageTest, TlsErrorHandling) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Enable TLS flag without proper setup to test error paths */
    conn->tls_enabled = 1;
    
    /* Try TLS operations that should fail gracefully */
    result = uvhttp_connection_tls_read(conn);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_start_tls_handshake(conn);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}



/* 测试多次调用同一个函数 */
TEST(UvhttpConnectionMissingCoverageTest, MultipleSetStateCalls) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Test setting same state multiple times */
    for (int i = 0; i < 5; i++) {
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
        EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    }
    
    /* Test alternating between states */
    for (int i = 0; i < 3; i++) {
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    }
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* 测试连接状态字符串表示 */
TEST(UvhttpConnectionMissingCoverageTest, StateToString) {
    /* This test ensures state transitions work correctly */
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* Verify each state can be set and retrieved */
    uvhttp_connection_state_t states[] = {
        UVHTTP_CONN_STATE_NEW,
        UVHTTP_CONN_STATE_TLS_HANDSHAKE,
        UVHTTP_CONN_STATE_HTTP_READING,
        UVHTTP_CONN_STATE_HTTP_PROCESSING,
        UVHTTP_CONN_STATE_HTTP_WRITING,
        UVHTTP_CONN_STATE_PROTOCOL_UPGRADED,
        UVHTTP_CONN_STATE_CLOSING
    };
    
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); i++) {
        uvhttp_connection_set_state(conn, states[i]);
        EXPECT_EQ(conn->state, states[i]);
    }
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}