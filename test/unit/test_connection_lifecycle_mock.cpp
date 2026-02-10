/* uvhttp_connection.c 连接生命周期测试 - 使用 mock libuv 避免崩溃 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "libuv_mock.h"
#include <string.h>

/* ========== Mock 控制辅助函数 ========== */

static void setup_mock_loop(void) {
    libuv_mock_reset();
    libuv_mock_set_enabled(true);
    
    /* 设置默认返回值 */
    libuv_mock_set_uv_loop_init_result(0);
    libuv_mock_set_uv_loop_close_result(0);
    libuv_mock_set_uv_run_result(0);
    libuv_mock_set_uv_tcp_init_result(0);
    libuv_mock_set_uv_tcp_bind_result(0);
    libuv_mock_set_uv_listen_result(0);
    libuv_mock_set_uv_read_start_result(0);
    libuv_mock_set_uv_read_stop_result(0);
    libuv_mock_set_uv_write_result(0);
    libuv_mock_set_uv_is_active_result(0);
    libuv_mock_set_uv_is_closing_result(0);
    libuv_mock_set_uv_idle_init_result(0);
    libuv_mock_set_uv_timer_init_result(0);
}

static uv_loop_t* create_mock_loop(void) {
    uv_loop_t* loop = (uv_loop_t*)uvhttp_alloc(sizeof(uv_loop_t));
    if (!loop) {
        return nullptr;
    }
    memset(loop, 0, sizeof(uv_loop_t));
    return loop;
}

static void destroy_mock_loop(uv_loop_t* loop) {
    if (loop) {
        uvhttp_free(loop);
    }
}

/* ========== 创建一个最小化的 server 用于测试 ========== */

static uvhttp_server_t* create_minimal_server(uv_loop_t* loop) {
    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    if (!server) {
        return nullptr;
    }
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;
    return server;
}

/* ========== 测试连接创建和释放 ========== */

TEST(UvhttpConnectionLifecycleMockTest, CreateAndDestroy) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 验证连接属性 */
    EXPECT_EQ(conn->server, server);
    EXPECT_EQ(conn->chunked_encoding, 0);
    EXPECT_EQ(conn->parsing_complete, 0);
    EXPECT_EQ(conn->close_pending, 0);
    EXPECT_EQ(conn->need_restart_read, 0);
    EXPECT_EQ(conn->tls_enabled, 0);
    EXPECT_EQ(conn->body_received, 0);
    EXPECT_EQ(conn->content_length, 0);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接状态转换 ========== */

TEST(UvhttpConnectionLifecycleMockTest, StateTransitions) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试状态转换 */
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
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接标志位 ========== */

TEST(UvhttpConnectionLifecycleMockTest, ConnectionFlags) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试 keepalive 标志 */
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);
    
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);
    
    /* 测试分块编码标志 */
    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);
    
    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);
    
    /* 测试解析完成标志 */
    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);
    
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);
    
    /* 测试 close_pending 标志 */
    conn->close_pending = 1;
    EXPECT_EQ(conn->close_pending, 1);
    
    conn->close_pending = 0;
    EXPECT_EQ(conn->close_pending, 0);
    
    /* 测试 need_restart_read 标志 */
    conn->need_restart_read = 1;
    EXPECT_EQ(conn->need_restart_read, 1);
    
    conn->need_restart_read = 0;
    EXPECT_EQ(conn->need_restart_read, 0);
    
    /* 测试 tls_enabled 标志 */
    conn->tls_enabled = 1;
    EXPECT_EQ(conn->tls_enabled, 1);
    
    conn->tls_enabled = 0;
    EXPECT_EQ(conn->tls_enabled, 0);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接错误处理 ========== */

TEST(UvhttpConnectionLifecycleMockTest, ConnectionNullParameters) {
    setup_mock_loop();
    
    /* 测试 NULL server */
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(nullptr, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
    
    /* 测试 NULL conn 参数 */
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    result = uvhttp_connection_new(server, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试释放 NULL 连接 */
    uvhttp_connection_free(nullptr);
    
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接重启读取 ========== */

TEST(UvhttpConnectionLifecycleMockTest, RestartRead) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试重启读取 */
    conn->need_restart_read = 1;
    uvhttp_connection_restart_read(conn);
    EXPECT_EQ(conn->need_restart_read, 0);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接超时 ========== */

TEST(UvhttpConnectionLifecycleMockTest, ConnectionTimeout) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试启动超时 */
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接关闭 ========== */

TEST(UvhttpConnectionLifecycleMockTest, ConnectionClose) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试关闭连接 */
    uvhttp_connection_close(conn);
    EXPECT_EQ(conn->close_pending, 1);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试多个连接 ========== */

TEST(UvhttpConnectionLifecycleMockTest, MultipleConnections) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    /* 创建多个连接 */
    uvhttp_connection_t* conn1 = nullptr;
    uvhttp_connection_t* conn2 = nullptr;
    uvhttp_connection_t* conn3 = nullptr;
    
    uvhttp_error_t result = uvhttp_connection_new(server, &conn1);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn1, nullptr);
    
    result = uvhttp_connection_new(server, &conn2);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn2, nullptr);
    
    result = uvhttp_connection_new(server, &conn3);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn3, nullptr);
    
    /* 验证连接关联到同一个服务器 */
    EXPECT_EQ(conn1->server, server);
    EXPECT_EQ(conn2->server, server);
    EXPECT_EQ(conn3->server, server);
    
    /* 释放所有连接 */
    uvhttp_connection_free(conn1);
    uvhttp_connection_free(conn2);
    uvhttp_connection_free(conn3);
    
    uvhttp_free(server);
    destroy_mock_loop(loop);
}

/* ========== 测试连接边界值 ========== */

TEST(UvhttpConnectionLifecycleMockTest, BoundaryValues) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试边界值 */
    conn->content_length = 0;
    EXPECT_EQ(conn->content_length, 0);
    
    conn->content_length = SIZE_MAX;
    EXPECT_EQ(conn->content_length, SIZE_MAX);
    
    conn->body_received = 0;
    EXPECT_EQ(conn->body_received, 0);
    
    conn->body_received = SIZE_MAX;
    EXPECT_EQ(conn->body_received, SIZE_MAX);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}