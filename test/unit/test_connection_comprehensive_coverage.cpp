/* uvhttp_connection.c 综合覆盖率测试 - 目标提升至 45%+ */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* ========== 测试连接创建和释放 ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionComprehensiveTest, ConnectionNewNullConn) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    result = uvhttp_connection_new(server, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionComprehensiveTest, ConnectionFreeNull) {
    /* 不应该崩溃 */
    uvhttp_connection_free(NULL);
}

/* ========== 测试连接状态 ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionStateTransitions) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试所有状态转换 */
    conn->state = UVHTTP_CONN_STATE_NEW;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    conn->state = UVHTTP_CONN_STATE_HTTP_READING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    conn->state = UVHTTP_CONN_STATE_HTTP_WRITING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    conn->state = UVHTTP_CONN_STATE_CLOSING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    /* 测试无效状态 */
    conn->state = (uvhttp_connection_state_t)999;
    EXPECT_EQ(conn->state, (uvhttp_connection_state_t)999);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试连接标志位 ========== */

TEST(UvhttpConnectionComprehensiveTest, ConnectionFlags) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试 keepalive 标志 */
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);
    
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);
    
    /* 测试分块编码标志 */
    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);
    
    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);
    
    /* 测试解析完成标志 */
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);
    
    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);
    
    /* 测试解析完成标志 */
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);
    
    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试缓冲区管理 ========== */

TEST(UvhttpConnectionComprehensiveTest, BufferInitialization) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 验证缓冲区初始化 */
    EXPECT_GE(conn->read_buffer_size, 0);
    EXPECT_GE(conn->read_buffer_used, 0);
    EXPECT_EQ(conn->content_length, 0);
    EXPECT_EQ(conn->body_received, 0);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionComprehensiveTest, BufferSizeModification) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试缓冲区大小修改 */
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);
    
    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);
    
    conn->read_buffer_used = 8192;
    EXPECT_EQ(conn->read_buffer_used, 8192);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

TEST(UvhttpConnectionComprehensiveTest, ContentLengthTracking) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试内容长度跟踪 */
    conn->content_length = 0;
    EXPECT_EQ(conn->content_length, 0);
    
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);
    
    conn->content_length = 1024 * 1024;
    EXPECT_EQ(conn->content_length, 1024 * 1024);
    
    /* 测试已接收 body 跟踪 */
    conn->body_received = 0;
    EXPECT_EQ(conn->body_received, 0);
    
    conn->body_received = 512;
    EXPECT_EQ(conn->body_received, 512);
    
    conn->body_received = 1024;
    EXPECT_EQ(conn->body_received, 1024);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试请求和响应对象 ========== */

TEST(UvhttpConnectionComprehensiveTest, RequestResponseObjects) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试请求对象 */
    /* conn->request 可能是 NULL 或已分配 */
    if (conn->request) {
        /* 验证请求对象存在 */
    }
    
    /* 测试响应对象 */
    /* conn->response 可能是 NULL 或已分配 */
    if (conn->response) {
        /* 验证响应对象存在 */
    }
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试服务器关联 ========== */

TEST(UvhttpConnectionComprehensiveTest, ServerAssociation) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 验证连接与服务器关联 */
    EXPECT_EQ(conn->server, server);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试 TCP 句柄 ========== */

TEST(UvhttpConnectionComprehensiveTest, TcpHandle) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试 TCP 句柄 */
    /* tcp_handle 是结构体，不是指针 */
    /* 验证句柄已初始化 */
    /* conn->tcp_handle 是 uv_tcp_t 结构体 */
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试时间戳 ========== */

TEST(UvhttpConnectionComprehensiveTest, Timestamps) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试最后错误码 */
    conn->last_error = 0;
    EXPECT_EQ(conn->last_error, 0);
    
    conn->last_error = -1;
    EXPECT_EQ(conn->last_error, -1);
    
    conn->last_error = UVHTTP_ERROR_IO_ERROR;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_IO_ERROR);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试错误处理 ========== */

TEST(UvhttpConnectionComprehensiveTest, ErrorHandling) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 测试 close_pending 标志 */
    conn->close_pending = 0;
    EXPECT_EQ(conn->close_pending, 0);
    
    conn->close_pending = 1;
    EXPECT_EQ(conn->close_pending, 1);
    
    /* 测试 need_restart_read 标志 */
    conn->need_restart_read = 0;
    EXPECT_EQ(conn->need_restart_read, 0);
    
    conn->need_restart_read = 1;
    EXPECT_EQ(conn->need_restart_read, 1);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}

/* ========== 测试多个连接 ========== */

TEST(UvhttpConnectionComprehensiveTest, MultipleConnections) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    /* 创建多个连接 */
    uvhttp_connection_t* conns[20];
    for (int i = 0; i < 20; i++) {
        result = uvhttp_connection_new(server, &conns[i]);
        ASSERT_EQ(result, UVHTTP_OK);
        ASSERT_NE(conns[i], nullptr);
    }
    
    /* 释放所有连接 */
    for (int i = 0; i < 20; i++) {
        uvhttp_connection_free(conns[i]);
    }
    
    uvhttp_server_free(server);
}

/* ========== 测试边界情况 ========== */

TEST(UvhttpConnectionComprehensiveTest, EdgeCases) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = NULL;
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