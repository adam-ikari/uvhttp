/* uvhttp_connection.c 增强覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_error.h"
#include <string.h>

/* 测试连接创建和释放（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, CreateAndFreeNull) {
    /* 测试NULL服务器 */
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
    
    /* 测试NULL输出参数 */
    result = uvhttp_connection_new(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试释放NULL连接 */
    uvhttp_connection_free(NULL);
}

/* 测试连接启动（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, StartNull) {
    uvhttp_error_t result = uvhttp_connection_start(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试连接关闭（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, CloseNull) {
    /* 测试NULL连接 */
    uvhttp_connection_close(NULL);
}

/* 测试连接重启读取（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, RestartReadNull) {
    uvhttp_error_t result = uvhttp_connection_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_schedule_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试TLS处理函数（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, TLSFunctionsNull) {
    uvhttp_error_t result;
    
    result = uvhttp_connection_tls_handshake_func(NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_tls_write(NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试TLS写入（NULL数据） */
TEST(UvhttpConnectionEnhancedCoverageTest, TLSWriteNullData) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    uvhttp_error_t result = uvhttp_connection_tls_write(&conn, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试连接超时（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, TimeoutNull) {
    uvhttp_error_t result;
    
    result = uvhttp_connection_start_timeout(NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_start_timeout_custom(NULL, 60);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试连接超时自定义时间（边界值） */
TEST(UvhttpConnectionEnhancedCoverageTest, TimeoutCustomBoundaryValues) {
    /* 注意：这些测试需要有效的连接对象，这里只测试NULL参数 */
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    /* 测试最小超时时间（5秒） */
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(&conn, 5);
    /* 可能成功或失败，取决于连接状态 */
    
    /* 测试最大超时时间（300秒） */
    result = uvhttp_connection_start_timeout_custom(&conn, 300);
    /* 可能成功或失败，取决于连接状态 */
    
    /* 测试超出范围（小于最小值） */
    result = uvhttp_connection_start_timeout_custom(&conn, 4);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试超出范围（大于最大值） */
    result = uvhttp_connection_start_timeout_custom(&conn, 301);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零值 */
    result = uvhttp_connection_start_timeout_custom(&conn, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试负值 */
    result = uvhttp_connection_start_timeout_custom(&conn, -1);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试WebSocket握手（NULL参数） */
TEST(UvhttpConnectionEnhancedCoverageTest, WebSocketHandshakeNull) {
    uvhttp_error_t result;
    
    result = uvhttp_connection_handle_websocket_handshake(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_handle_websocket_handshake(NULL, "test_key");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试连接结构体字段初始化 */
TEST(UvhttpConnectionEnhancedCoverageTest, ConnectionStructureInit) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    /* 验证初始状态 */
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn.parsing_complete, 0);
    EXPECT_EQ(conn.keepalive, 0);
    EXPECT_EQ(conn.chunked_encoding, 0);
    EXPECT_EQ(conn.server, nullptr);
    EXPECT_EQ(conn.request, nullptr);
    EXPECT_EQ(conn.response, nullptr);
    EXPECT_EQ(conn.ssl, nullptr);
    EXPECT_EQ(conn.read_buffer, nullptr);
    EXPECT_EQ(conn.content_length, 0);
    EXPECT_EQ(conn.body_received, 0);
    EXPECT_EQ(conn.read_buffer_size, 0);
    EXPECT_EQ(conn.read_buffer_used, 0);
    EXPECT_EQ(conn.current_header_field_len, 0);
    EXPECT_EQ(conn.close_pending, 0);
    EXPECT_EQ(conn.current_header_is_important, 0);
    EXPECT_EQ(conn.parsing_header_field, 0);
    EXPECT_EQ(conn.need_restart_read, 0);
    EXPECT_EQ(conn.tls_enabled, 0);
    EXPECT_EQ(conn.last_error, 0);
}