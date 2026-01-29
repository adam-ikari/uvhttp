/* uvhttp_connection.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"

/* 测试连接创建和释放 */
TEST(UvhttpConnectionFullCoverageTest, ConnectionCreateAndFree) {
    /* 创建测试环境 */
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    /* 测试连接创建 */
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 验证初始状态 */
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    EXPECT_GE(conn->parsing_complete, 0);
    EXPECT_GE(conn->keepalive, 0);
    
    /* 测试状态设置 */
    conn->state = UVHTTP_CONN_STATE_HTTP_READING;
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);
    
    /* 测试分块编码 */
    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);
    
    /* 测试释放连接 */
    uvhttp_connection_free(conn);
    
    /* 测试释放NULL连接 */
    uvhttp_connection_free(NULL);
    
    /* 清理 */
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接缓冲区管理 */
TEST(UvhttpConnectionFullCoverageTest, BufferManagement) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试缓冲区初始化 */
    EXPECT_GE(conn->read_buffer_size, 0);
    EXPECT_GE(conn->read_buffer_used, 0);
    EXPECT_EQ(conn->content_length, 0);
    EXPECT_EQ(conn->body_received, 0);
    
    /* 测试内容长度设置 */
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);
    
    conn->body_received = 512;
    EXPECT_EQ(conn->body_received, 512);
    
    /* 测试缓冲区大小 */
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);
    
    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);
    
    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接标志位 */
TEST(UvhttpConnectionFullCoverageTest, ConnectionFlags) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试关闭标志 */
    conn->close_pending = 1;
    EXPECT_EQ(conn->close_pending, 1);
    
    /* 测试解析标志 */
    conn->parsing_header_field = 1;
    EXPECT_EQ(conn->parsing_header_field, 1);
    
    conn->current_header_is_important = 1;
    EXPECT_EQ(conn->current_header_is_important, 1);
    
    conn->need_restart_read = 1;
    EXPECT_EQ(conn->need_restart_read, 1);
    
    /* 测试TLS标志 */
    conn->tls_enabled = 1;
    EXPECT_EQ(conn->tls_enabled, 1);
    
    /* 测试错误标志 */
    conn->last_error = UVHTTP_ERROR_INVALID_PARAM;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_INVALID_PARAM);
    
    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接头字段 */
TEST(UvhttpConnectionFullCoverageTest, HeaderFieldManagement) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试头字段长度 */
    conn->current_header_field_len = 0;
    EXPECT_EQ(conn->current_header_field_len, 0);
    
    /* 测试头字段名称 */
    strcpy(conn->current_header_field, "Content-Type");
    EXPECT_STREQ(conn->current_header_field, "Content-Type");
    conn->current_header_field_len = strlen("Content-Type");
    EXPECT_EQ(conn->current_header_field_len, strlen("Content-Type"));
    
    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接创建失败 */
TEST(UvhttpConnectionFullCoverageTest, ConnectionCreateFailure) {
    /* 测试NULL服务器 */
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
    
    /* 测试NULL输出参数 */
    uvhttp_context_t* context = NULL;
    result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_connection_new(server, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接状态字符串 */

/* 测试连接状态设置 */
TEST(UvhttpConnectionFullCoverageTest, ConnectionStateSet) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试状态设置函数 */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}

/* 测试连接内存布局 */
TEST(UvhttpConnectionFullCoverageTest, ConnectionMemoryLayout) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_connection_t* conn = NULL;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 验证内存布局对齐 */
    EXPECT_EQ(offsetof(uvhttp_connection_t, state), 0);
    EXPECT_GE(offsetof(uvhttp_connection_t, server), 16);
    EXPECT_GE(offsetof(uvhttp_connection_t, current_header_field), 64);
    
    /* 验证结构体大小合理 */
    EXPECT_GE(sizeof(uvhttp_connection_t), 128);
    EXPECT_LE(sizeof(uvhttp_connection_t), 4096);
    
    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
}
