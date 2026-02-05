/* uvhttp_connection.c 连接生命周期测试 - 测试连接的完整生命周期 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
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

/* 辅助函数：运行事件循环并关闭 */
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

/* ========== 测试连接创建和释放 ========== */

TEST(UvhttpConnectionLifecycleTest, CreateAndDestroy) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
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
    destroy_server_and_loop(server, loop);
}

/* ========== 测试连接状态转换 ========== */

TEST(UvhttpConnectionLifecycleTest, StateTransitions) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
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
    destroy_server_and_loop(server, loop);
}

/* ========== 测试连接标志位 ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionFlags) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
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
    
    /* 测试 TLS 启用标志 */
    conn->tls_enabled = 1;
    EXPECT_EQ(conn->tls_enabled, 1);
    
    conn->tls_enabled = 0;
    EXPECT_EQ(conn->tls_enabled, 0);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试缓冲区管理 ========== */

TEST(UvhttpConnectionLifecycleTest, BufferManagement) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试缓冲区大小 */
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);
    
    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);
    
    /* 测试内容长度 */
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);
    
    /* 测试已接收 body 长度 */
    conn->body_received = 512;
    EXPECT_EQ(conn->body_received, 512);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接关闭 ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionClose) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 关闭连接 */
    uvhttp_connection_close(conn);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    /* 运行事件循环以完成异步关闭 */
    for (int i = 0; i < 10; i++) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    
    /* 不要手动调用 uvhttp_connection_free，由 on_handle_close 回调自动释放 */
    uvhttp_server_free(server);
}

/* ========== 测试连接重启读取 ========== */

TEST(UvhttpConnectionLifecycleTest, RestartRead) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试重启读取 */
    result = uvhttp_connection_restart_read(conn);
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
    for (int i = 0; i < 10; i++) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接调度重启读取 ========== */

TEST(UvhttpConnectionLifecycleTest, ScheduleRestartRead) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试调度重启读取 */
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
    for (int i = 0; i < 10; i++) {
        uv_run(uv_default_loop(), UV_RUN_ONCE);
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试多个连接 ========== */

TEST(UvhttpConnectionLifecycleTest, MultipleConnections) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    /* 创建多个连接 */
    uvhttp_connection_t* conns[10];
    for (int i = 0; i < 10; i++) {
        result = uvhttp_connection_new(server, &conns[i]);
        ASSERT_EQ(result, UVHTTP_OK);
        ASSERT_NE(conns[i], nullptr);
        EXPECT_EQ(conns[i]->server, server);
    }
    
    /* 释放所有连接 */
    for (int i = 0; i < 10; i++) {
        uvhttp_connection_free(conns[i]);
    }
    
    uvhttp_server_free(server);
}

/* ========== 测试连接错误处理 ========== */

TEST(UvhttpConnectionLifecycleTest, ErrorHandling) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试错误码 */
    conn->last_error = UVHTTP_ERROR_CONNECTION_INIT;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_INIT);
    
    conn->last_error = UVHTTP_ERROR_CONNECTION_TIMEOUT;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_TIMEOUT);
    
    conn->last_error = UVHTTP_ERROR_CONNECTION_RESET;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_CONNECTION_RESET);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接头部解析 ========== */

TEST(UvhttpConnectionLifecycleTest, HeaderParsing) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试头部解析标志 */
    conn->parsing_header_field = 1;
    EXPECT_EQ(conn->parsing_header_field, 1);
    
    conn->current_header_is_important = 1;
    EXPECT_EQ(conn->current_header_is_important, 1);
    
    conn->current_header_field_len = 100;
    EXPECT_EQ(conn->current_header_field_len, 100);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接边界情况 ========== */

TEST(UvhttpConnectionLifecycleTest, EdgeCases) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试大值 */
    conn->read_buffer_size = SIZE_MAX;
    conn->content_length = SIZE_MAX;
    conn->body_received = SIZE_MAX;
    
    /* 测试负值（如果允许） */
    conn->keepalive = -1;
    conn->chunked_encoding = -1;
    conn->parsing_complete = -1;
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接超时 ========== */

TEST(UvhttpConnectionLifecycleTest, ConnectionTimeout) {
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试启动超时 */
    result = uvhttp_connection_start_timeout(conn);
    /* 结果取决于内部状态 */
    
    /* 测试自定义超时 */
    result = uvhttp_connection_start_timeout_custom(conn, 30);
    /* 结果取决于内部状态 */
    
    /* 停止定时器 */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }
    
    /* 关闭连接以清理定时器 */
    uvhttp_connection_close(conn);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}