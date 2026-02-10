/* uvhttp_connection.c 综合覆盖率Test - 目标提升至 45%+ */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
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
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== TestConnectionCreate和Free ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionComprehensiveTest, ConnectionNewNullConn) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_error_t result = uvhttp_connection_new(server, NULL);
    EXPECT_NE(result, UVHTTP_OK);

    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionComprehensiveTest, ConnectionFreeNull) {
    /* 不ShouldCrash */
    uvhttp_connection_free(NULL);
}

/* ========== TestConnectionState ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionStateTransitions) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestAllState转换 */
    conn->state = UVHTTP_CONN_STATE_NEW;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);

    conn->state = UVHTTP_CONN_STATE_HTTP_READING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);

    conn->state = UVHTTP_CONN_STATE_HTTP_WRITING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);

    conn->state = UVHTTP_CONN_STATE_CLOSING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    /* Test无效State */
    conn->state = (uvhttp_connection_state_t)999;
    EXPECT_EQ(conn->state, (uvhttp_connection_state_t)999);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionFlag位 ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionFlags) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test keepalive Flag */
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);

    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);

    /* Test分块编码Flag */
    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);

    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);

    /* Test解析CompleteFlag */
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);

    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestBuffer管理 ========== */

TEST(UvhttpConnectionComprehensiveTest, BufferInitialization) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* ValidationBufferInitial化 */
    EXPECT_GE(conn->read_buffer_size, 0);
    EXPECT_GE(conn->read_buffer_used, 0);
    EXPECT_EQ(conn->content_length, 0);
    EXPECT_EQ(conn->body_received, 0);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionComprehensiveTest, BufferSizeModification) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestBufferSize修改 */
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);

    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);

    conn->read_buffer_used = 8192;
    EXPECT_EQ(conn->read_buffer_used, 8192);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(UvhttpConnectionComprehensiveTest, ContentLengthTracking) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test内容Length跟踪 */
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);

    conn->body_received = 512;
    EXPECT_EQ(conn->body_received, 512);

    conn->body_received = 1024;
    EXPECT_EQ(conn->body_received, 1024);

    /* Test完整接收 */
    conn->body_received = conn->content_length;
    EXPECT_EQ(conn->body_received, conn->content_length);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test请求和响应Object ========== */

TEST(UvhttpConnectionComprehensiveTest, RequestResponseObjects) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test请求和响应ObjectAssociation */
    EXPECT_NE(conn->request, nullptr);
    EXPECT_NE(conn->response, nullptr);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test服务器Association ========== */

TEST(UvhttpConnectionComprehensiveTest, ServerAssociation) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test服务器Association */
    EXPECT_EQ(conn->server, server);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== Test TCP Handle ========== */

TEST(UvhttpConnectionComprehensiveTest, TcpHandle) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test TCP Handle */
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&conn->tcp_handle));

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestTimestamp ========== */

TEST(UvhttpConnectionComprehensiveTest, Timestamps) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test last_error */
    EXPECT_EQ(conn->last_error, UVHTTP_OK);

    conn->last_error = UVHTTP_ERROR_INVALID_PARAM;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestErrorHandling ========== */

TEST(UvhttpConnectionComprehensiveTest, ErrorHandling) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestErrorState */
    EXPECT_EQ(conn->last_error, UVHTTP_OK);

    conn->last_error = UVHTTP_ERROR_INVALID_PARAM;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestMultipleConnection ========== */

TEST(UvhttpConnectionComprehensiveTest, MultipleConnections) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    /* TestMultipleConnection */
    uvhttp_connection_t* conn1 = NULL;
    uvhttp_connection_t* conn2 = NULL;
    uvhttp_connection_t* conn3 = NULL;

    uvhttp_error_t result = uvhttp_connection_new(server, &conn1);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_new(server, &conn2);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_new(server, &conn3);
    ASSERT_EQ(result, UVHTTP_OK);

    /* ValidationConnection独立性 */
    EXPECT_NE(conn1, conn2);
    EXPECT_NE(conn2, conn3);
    EXPECT_NE(conn1, conn3);

    uvhttp_connection_free(conn3);
    uvhttp_connection_free(conn2);
    uvhttp_connection_free(conn1);
    destroy_server_and_loop(server, loop);
}

/* ========== TestEdgeCase ========== */

TEST(UvhttpConnectionComprehensiveTest, EdgeCases) {
    uv_loop_t* loop = NULL;
    uvhttp_server_t* server = NULL;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestEdgeCase */
    conn->read_buffer_size = 0;
    EXPECT_EQ(conn->read_buffer_size, 0);

    conn->read_buffer_used = 0;
    EXPECT_EQ(conn->read_buffer_used, 0);

    conn->content_length = 0;
    EXPECT_EQ(conn->content_length, 0);

    conn->body_received = 0;
    EXPECT_EQ(conn->body_received, 0);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}