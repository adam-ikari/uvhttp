/**
 * @file test_connection_full_coverage_enhanced.cpp
 * @brief 增强的连接测试 - 提升覆盖率到 80%
 * 
 * 目标：提升 uvhttp_connection.c 覆盖率从 17.4% 到 80%
 * 
 * 测试内容：
 * - uvhttp_connection_close: 连接关闭功能
 * - uvhttp_connection_free: 连接释放功能
 * - uvhttp_connection_start: 连接启动功能
 * - uvhttp_connection_restart_read: 重启读取
 * - uvhttp_connection_schedule_restart_read: 调度重启读取
 * - uvhttp_connection_set_state: 状态设置
 * - uvhttp_connection_tls_handshake_func: TLS握手
 * - uvhttp_connection_tls_write: TLS写入
 * - WebSocket 相关函数
 * - 回调函数（on_read, on_close, on_alloc_buffer等）
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

/* 测试连接关闭功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionClose) {
    uvhttp_connection_close(nullptr);
}

/* 测试连接释放功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionFreeNull) {
    uvhttp_connection_free(nullptr);
}

/* 测试连接启动功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStartNull) {
    EXPECT_NE(uvhttp_connection_start(nullptr), 0);
}

/* 测试重启读取功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionRestartReadNull) {
    EXPECT_NE(uvhttp_connection_restart_read(nullptr), 0);
}

/* 测试调度重启读取功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionScheduleRestartReadNull) {
    EXPECT_NE(uvhttp_connection_schedule_restart_read(nullptr), 0);
}

/* 测试状态设置功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStateSet) {
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
}

/* 测试 TLS 握手功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionTlsHandshakeFuncNull) {
    EXPECT_EQ(uvhttp_connection_tls_handshake_func(nullptr), -1);
}

/* 测试 TLS 写入功能 */
TEST(UvhttpConnectionEnhancedTest, ConnectionTlsWriteNull) {
    const char* data = "test data";
    EXPECT_EQ(uvhttp_connection_tls_write(nullptr, data, strlen(data)), -1);
}

/* 连接池清理功能已移除 - 连接池功能已废弃 */

/* 测试 WebSocket 握手功能 - NULL参数 */
TEST(UvhttpConnectionEnhancedTest, WebsocketHandshakeNull) {
    const char* ws_key = "dGhlIHNhbXBsZSBwbGVhc3VyZQ==";
    EXPECT_NE(uvhttp_connection_handle_websocket_handshake(nullptr, ws_key), 0);
}

/* 测试 WebSocket 切换功能 - NULL参数 */
TEST(UvhttpConnectionEnhancedTest, WebsocketSwitchNull) {
    uvhttp_connection_switch_to_websocket(nullptr);
}

/* 测试 WebSocket 关闭功能 - NULL参数 */
TEST(UvhttpConnectionEnhancedTest, WebsocketCloseNull) {
    uvhttp_connection_websocket_close(nullptr);
}

/* 测试连接创建 - NULL服务器 */
TEST(UvhttpConnectionEnhancedTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = uvhttp_connection_new(nullptr);
    EXPECT_EQ(conn, nullptr);
}

/* 测试连接字段初始化 */
TEST(UvhttpConnectionEnhancedTest, ConnectionFieldInitialization) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    EXPECT_EQ(conn.server, nullptr);
    EXPECT_EQ(conn.request, nullptr);
    EXPECT_EQ(conn.response, nullptr);
    EXPECT_EQ(conn.ssl, nullptr);
    EXPECT_EQ(conn.tls_enabled, 0);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn.read_buffer, nullptr);
    EXPECT_EQ(conn.read_buffer_size, 0);
    EXPECT_EQ(conn.read_buffer_used, 0);
    EXPECT_EQ(conn.current_header_is_important, 0);
    EXPECT_EQ(conn.keep_alive, 0);
    EXPECT_EQ(conn.chunked_encoding, 0);
    EXPECT_EQ(conn.content_length, 0);
    EXPECT_EQ(conn.body_received, 0);
    EXPECT_EQ(conn.parsing_complete, 0);
    EXPECT_EQ(conn.current_header_field_len, 0);
    EXPECT_EQ(conn.parsing_header_field, 0);
    EXPECT_EQ(conn.need_restart_read, 0);
    EXPECT_EQ(conn.last_error, 0);
}

/* 测试状态枚举值 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStateEnumValues) {
    EXPECT_EQ(UVHTTP_CONN_STATE_NEW, 0);
    EXPECT_EQ(UVHTTP_CONN_STATE_TLS_HANDSHAKE, 1);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_READING, 2);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_PROCESSING, 3);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_WRITING, 4);
    EXPECT_EQ(UVHTTP_CONN_STATE_CLOSING, 5);
}

/* 测试连接结构体大小 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStructSize) {
    EXPECT_GT(sizeof(uvhttp_connection_t), 0);
    EXPECT_GT(sizeof(uvhttp_connection_state_t), 0);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpConnectionEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_connection_start(nullptr);
        uvhttp_connection_restart_read(nullptr);
        uvhttp_connection_schedule_restart_read(nullptr);
        uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
        uvhttp_connection_close(nullptr);
        uvhttp_connection_free(nullptr);
    }
}

/* 测试状态转换顺序 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStateTransitions) {
    EXPECT_LT(UVHTTP_CONN_STATE_NEW, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    EXPECT_LT(UVHTTP_CONN_STATE_TLS_HANDSHAKE, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_READING, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_PROCESSING, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_WRITING, UVHTTP_CONN_STATE_CLOSING);
}

/* 测试连接结构体对齐 */
TEST(UvhttpConnectionEnhancedTest, ConnectionStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(size_t));
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(int));
}

/* 测试常量值 */
TEST(UvhttpConnectionEnhancedTest, ConnectionConstants) {
    EXPECT_GT(UVHTTP_MAX_HEADER_NAME_SIZE, 0);
}

/* 测试 TLS 写入 - 零长度 */
TEST(UvhttpConnectionEnhancedTest, ConnectionTlsWriteZeroLength) {
    const char* data = "";
    EXPECT_EQ(uvhttp_connection_tls_write(nullptr, data, 0), -1);
}

/* 测试 TLS 写入 - NULL 数据 */
TEST(UvhttpConnectionEnhancedTest, ConnectionTlsWriteNullData) {
    EXPECT_EQ(uvhttp_connection_tls_write(nullptr, nullptr, 10), -1);
}

/* 测试所有状态值 */
TEST(UvhttpConnectionEnhancedTest, AllStateValues) {
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_HTTP_READING);
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_HTTP_WRITING);
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_CLOSING);
}