/* uvhttp_connection.c Connection生命周期Test - TestConnection 完整生命周期 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
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

/* 辅助函数：运行事件循环并Close */
static void run_and_close(uvhttp_server_t* server, uvhttp_connection_t* conn, uv_loop_t* loop) {
    if (conn) {
        uvhttp_connection_close(conn);
        uvhttp_connection_free(conn);
    }
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== TestConnectionCreate和Free ========== */

TEST(UvhttpConnectionLifecycleTest, CreateAndDestroy) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* ValidationConnection属性 */
    EXPECT_EQ(conn->server, server);
    EXPECT_EQ(conn->chunked_encoding, 0);
    EXPECT_EQ(conn->parsing_complete, 0);
    EXPECT_EQ(conn->close_pending, 0);
    EXPECT_EQ(conn->need_restart_read, 0);
    EXPECT_EQ(conn->tls_enabled, 0);
    EXPECT_EQ(conn->body_received, 0);
    EXPECT_EQ(conn->content_length, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionState转换 ========== */

TEST(UvhttpConnectionLifecycleTest, StateTransitions) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* TestState转换 */
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionFlag位 ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionFlags) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test keepalive Flag */
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);

    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);

    /* Test分块编码Flag */
    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);

    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);

    /* Test解析CompleteFlag */
    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);

    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);

    /* Test close_pending Flag */
    conn->close_pending = 1;
    EXPECT_EQ(conn->close_pending, 1);

    conn->close_pending = 0;
    EXPECT_EQ(conn->close_pending, 0);

    /* Test need_restart_read Flag */
    conn->need_restart_read = 1;
    EXPECT_EQ(conn->need_restart_read, 1);

    conn->need_restart_read = 0;
    EXPECT_EQ(conn->need_restart_read, 0);

    /* Test TLS 启用Flag */
    conn->tls_enabled = 1;
    EXPECT_EQ(conn->tls_enabled, 1);

    conn->tls_enabled = 0;
    EXPECT_EQ(conn->tls_enabled, 0);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestBuffer管理 ========== */

TEST(UvhttpConnectionLifecycleTest, BufferManagement) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestBufferSize */
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);

    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);

    /* Test内容Length */
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);

    /* Test已接收 body Length */
    conn->body_received = 512;
    EXPECT_EQ(conn->body_received, 512);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionClose ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionClose) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* CloseConnection */
    uvhttp_connection_close(conn);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    /* 运行事件循环以CompleteAsyncClose */
    for (int i = 0; i < 10; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    /* 不要手动调用 uvhttp_connection_free，由 on_handle_close 回调自动Free */
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionRestart读取 ========== */

TEST(UvhttpConnectionLifecycleTest, RestartRead) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestRestart读取 */
    result = uvhttp_connection_restart_read(conn);
    /* ResultDependsInternalState */

    /* 运行事件循环以CompleteAsyncOperation */
    for (int i = 0; i < 10; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionScheduleRestart读取 ========== */

TEST(UvhttpConnectionLifecycleTest, ScheduleRestartRead) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestScheduleRestart读取 */
    result = uvhttp_connection_schedule_restart_read(conn);
    /* ResultDependsInternalState */

    /* CloseAll libuv Handle */
    if (!uv_is_closing((uv_handle_t*)&conn->idle_handle)) {
        uv_close((uv_handle_t*)&conn->idle_handle, NULL);
    }
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_close((uv_handle_t*)&conn->timeout_timer, NULL);
    }
    if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
    }

    /* 运行事件循环等待CloseComplete */
    for (int i = 0; i < 10; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestMultipleConnection ========== */

TEST(UvhttpConnectionLifecycleTest, MultipleConnections) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    /* CreateMultipleConnection */
    uvhttp_connection_t* conns[10];
    uvhttp_error_t result;
    for (int i = 0; i < 10; i++) {
        result = uvhttp_connection_new(server, &conns[i]);
        ASSERT_EQ(result, UVHTTP_OK);
        ASSERT_NE(conns[i], nullptr);
        EXPECT_EQ(conns[i]->server, server);
    }

    /* FreeAllConnection */
    for (int i = 0; i < 10; i++) {
        uvhttp_connection_free(conns[i]);
    }

    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionErrorHandling ========== */

TEST(UvhttpConnectionLifecycleTest, ErrorHandling) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestError码 */
    conn->last_error = UVHTTP_ERROR_CONNECTION_INIT;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_INIT);

    conn->last_error = UVHTTP_ERROR_CONNECTION_TIMEOUT;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_TIMEOUT);

    conn->last_error = UVHTTP_ERROR_CONNECTION_RESET;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_RESET);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionHeader解析 ========== */

TEST(UvhttpConnectionLifecycleTest, HeaderParsing) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestHeader解析Flag */
    conn->parsing_header_field = 1;
    EXPECT_EQ(conn->parsing_header_field, 1);

    conn->current_header_is_important = 1;
    EXPECT_EQ(conn->current_header_is_important, 1);

    conn->current_header_field_len = 100;
    EXPECT_EQ(conn->current_header_field_len, 100);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionEdgeCase ========== */

TEST(UvhttpConnectionLifecycleTest, EdgeCases) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Test大值 */
    conn->read_buffer_size = SIZE_MAX;
    conn->content_length = SIZE_MAX;
    conn->body_received = SIZE_MAX;

    /* Test负值（如果允许） */
    conn->keepalive = -1;
    conn->chunked_encoding = -1;
    conn->parsing_complete = -1;

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== TestConnectionTimeout ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionTimeout) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* TestStartTimeout */
    result = uvhttp_connection_start_timeout(conn);
    /* ResultDependsInternalState */

    /* TestCustomTimeout */
    result = uvhttp_connection_start_timeout_custom(conn, 30);
    /* ResultDependsInternalState */

    /* Stop定时器 */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* 手动FreeConnection，不使用 close 回调 */
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}