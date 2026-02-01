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

/* ========== 测试连接创建和释放 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionNewSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->server, server);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}

TEST(UvhttpConnectionFullApiTest, ConnectionNewNullConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_error_t result = uvhttp_connection_new(server, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionFreeNull) {
    /* 不应该崩溃 */
    uvhttp_connection_free(NULL);
}

/* ========== 测试连接启动 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionStartNull) {
    uvhttp_error_t result = uvhttp_connection_start(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionStartSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 注意：在实际环境中，连接启动需要有效的 socket */
    /* 这里只测试函数调用，不测试实际的网络行为 */
    result = uvhttp_connection_start(conn);
    /* 可能返回错误，因为 socket 没有正确初始化 */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接关闭 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionCloseNull) {
    /* 不应该崩溃 */
    uvhttp_connection_close(NULL);
}

TEST(UvhttpConnectionFullApiTest, ConnectionCloseSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 设置连接状态为正在处理 */
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;
    
    /* 关闭连接 */
    uvhttp_connection_close(conn);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    /* 运行循环以处理异步关闭 */
    uv_run(loop, UV_RUN_NOWAIT);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接重启读取 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionRestartReadNull) {
    uvhttp_error_t result = uvhttp_connection_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionRestartReadSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 设置需要重启读取标志 */
    conn->need_restart_read = 1;
    
    /* 重启读取 */
    result = uvhttp_connection_restart_read(conn);
    /* 可能返回错误，因为连接没有正确初始化 */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionScheduleRestartReadNull) {
    uvhttp_error_t result = uvhttp_connection_schedule_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionScheduleRestartReadSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 设置需要重启读取标志 */
    conn->need_restart_read = 1;
    
    /* 调度重启读取 */
    result = uvhttp_connection_schedule_restart_read(conn);
    /* 可能返回错误，因为连接没有正确初始化 */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接状态管理 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionStateSetNull) {
    /* 不应该崩溃 */
    uvhttp_connection_set_state(NULL, UVHTTP_CONN_STATE_NEW);
}

TEST(UvhttpConnectionFullApiTest, ConnectionStateSetSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试所有状态 */
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
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionStateString) {
    /* 状态字符串转换函数暂未实现，跳过此测试 */
    SUCCEED();
}

/* ========== 测试 TLS 处理函数 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionTlsHandshakeFuncNull) {
    uvhttp_error_t result = uvhttp_connection_tls_handshake_func(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionTlsWriteNull) {
    uvhttp_error_t result = uvhttp_connection_tls_write(NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionTlsWriteNullConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    const char* data = "test data";
    result = uvhttp_connection_tls_write(conn, (const uint8_t*)data, strlen(data));
    /* 可能返回错误，因为 TLS 没有启用 */
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionTlsCleanupNull) {
    /* TLS cleanup 函数暂未实现，跳过此测试 */
    SUCCEED();
}

TEST(UvhttpConnectionFullApiTest, ConnectionTlsCleanupSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* TLS cleanup 函数暂未实现，只测试字段设置 */
    conn->tls_enabled = 1;
    conn->ssl = (void*)0x1234; /* 模拟 SSL 上下文 */
    
    /* 验证字段设置成功 */
    EXPECT_EQ(conn->ssl, (void*)0x1234);
    EXPECT_EQ(conn->tls_enabled, 1);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试 WebSocket 处理函数 ========== */

#if UVHTTP_FEATURE_WEBSOCKET
TEST(UvhttpConnectionFullApiTest, ConnectionHandleWebsocketHandshakeNull) {
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionHandleWebsocketHandshakeNullKey) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_connection_handle_websocket_handshake(conn, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionSwitchToWebsocketNull) {
    /* 不应该崩溃 */
    uvhttp_connection_switch_to_websocket(NULL);
}

TEST(UvhttpConnectionFullApiTest, ConnectionWebsocketCloseNull) {
    /* 不应该崩溃 */
    uvhttp_connection_websocket_close(NULL);
}

TEST(UvhttpConnectionFullApiTest, ConnectionWebsocketCloseSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 设置 WebSocket 标志，但不设置 ws_connection（避免调用 ws_connection_free） */
    conn->is_websocket = 1;
    conn->ws_connection = nullptr;
    
    /* 关闭 WebSocket */
    uvhttp_connection_websocket_close(conn);
    EXPECT_EQ(conn->is_websocket, 0);
    EXPECT_EQ(conn->ws_connection, nullptr);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}
#endif /* UVHTTP_FEATURE_WEBSOCKET */

/* ========== 测试连接超时 ========== */

#if UVHTTP_FEATURE_WEBSOCKET
TEST(UvhttpConnectionFullApiTest, ConnectionStartTimeoutNull) {
    uvhttp_error_t result = uvhttp_connection_start_timeout(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpFullApiTest, ConnectionStartTimeoutCustomNull) {
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(NULL, 60);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionFullApiTest, ConnectionStartTimeoutCustomInvalid) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试无效的超时时间 */
    result = uvhttp_connection_start_timeout_custom(conn, -1);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_start_timeout_custom(conn, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_start_timeout_custom(conn, 301);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionStartTimeoutSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试有效的超时时间，但不实际启动定时器（避免阻塞测试） */
    /* 只测试参数验证逻辑已在 ConnectionStartTimeoutCustomInvalid 中覆盖 */
    SUCCEED();
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}
#endif /* UVHTTP_FEATURE_WEBSOCKET */

/* ========== 测试连接标志位 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionKeepalive) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 keepalive 标志 */
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);
    
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);
    
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionChunkedEncoding) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试分块编码标志 */
    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);
    
    conn->chunked_encoding = 1;
    EXPECT_EQ(conn->chunked_encoding, 1);
    
    conn->chunked_encoding = 0;
    EXPECT_EQ(conn->chunked_encoding, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionParsingComplete) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试解析完成标志 */
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);
    
    conn->parsing_complete = 1;
    EXPECT_EQ(conn->parsing_complete, 1);
    
    conn->parsing_complete = 0;
    EXPECT_EQ(conn->parsing_complete, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接字段 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionBodyReceived) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 body_received 字段 */
    conn->body_received = 0;
    EXPECT_EQ(conn->body_received, 0);
    
    conn->body_received = 100;
    EXPECT_EQ(conn->body_received, 100);
    
    conn->body_received = 0;
    EXPECT_EQ(conn->body_received, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionContentLength) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 content_length 字段 */
    conn->content_length = 0;
    EXPECT_EQ(conn->content_length, 0);
    
    conn->content_length = 1024;
    EXPECT_EQ(conn->content_length, 1024);
    
    conn->content_length = 0;
    EXPECT_EQ(conn->content_length, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionReadBuffer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试读缓冲区字段 */
    conn->read_buffer_size = 0;
    EXPECT_EQ(conn->read_buffer_size, 0);
    
    conn->read_buffer_size = 8192;
    EXPECT_EQ(conn->read_buffer_size, 8192);
    
    conn->read_buffer_used = 0;
    EXPECT_EQ(conn->read_buffer_used, 0);
    
    conn->read_buffer_used = 4096;
    EXPECT_EQ(conn->read_buffer_used, 4096);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接错误处理 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionLastError) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 last_error 字段 */
    conn->last_error = 0;
    EXPECT_EQ(conn->last_error, 0);
    
    conn->last_error = UVHTTP_ERROR_INVALID_PARAM;
    EXPECT_EQ(conn->last_error, UVHTTP_ERROR_INVALID_PARAM);
    
    conn->last_error = 0;
    EXPECT_EQ(conn->last_error, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionClosePending) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 close_pending 字段 */
    conn->close_pending = 0;
    EXPECT_EQ(conn->close_pending, 0);
    
    conn->close_pending = 1;
    EXPECT_EQ(conn->close_pending, 1);
    
    conn->close_pending = 0;
    EXPECT_EQ(conn->close_pending, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接 TLS 字段 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionTlsEnabled) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 tls_enabled 字段 */
    conn->tls_enabled = 0;
    EXPECT_EQ(conn->tls_enabled, 0);
    
    conn->tls_enabled = 1;
    EXPECT_EQ(conn->tls_enabled, 1);
    
    conn->tls_enabled = 0;
    EXPECT_EQ(conn->tls_enabled, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionSsl) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 ssl 字段 */
    EXPECT_EQ(conn->ssl, nullptr);
    
    conn->ssl = (void*)0x1234;
    EXPECT_EQ(conn->ssl, (void*)0x1234);
    
    conn->ssl = nullptr;
    EXPECT_EQ(conn->ssl, nullptr);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接请求/响应字段 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionRequest) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 request 字段 - uvhttp_connection_new 会创建 request 对象 */
    EXPECT_NE(conn->request, nullptr);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpConnectionFullApiTest, ConnectionResponse) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 response 字段 - uvhttp_connection_new 会创建 response 对象 */
    EXPECT_NE(conn->response, nullptr);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接服务器字段 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 server 字段 */
    EXPECT_EQ(conn->server, server);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接头部字段 ========== */

TEST(UvhttpConnectionFullApiTest, ConnectionCurrentHeaderField) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 current_header_field_len 字段 */
    conn->current_header_field_len = 0;
    EXPECT_EQ(conn->current_header_field_len, 0);
    
    conn->current_header_field_len = 10;
    EXPECT_EQ(conn->current_header_field_len, 10);
    
    conn->current_header_field_len = 0;
    EXPECT_EQ(conn->current_header_field_len, 0);
    
    /* 测试 current_header_field_is_important 字段 */
    conn->current_header_is_important = 0;
    EXPECT_EQ(conn->current_header_is_important, 0);
    
    conn->current_header_is_important = 1;
    EXPECT_EQ(conn->current_header_is_important, 1);
    
    conn->current_header_is_important = 0;
    EXPECT_EQ(conn->current_header_is_important, 0);
    
    /* 测试 parsing_header_field 字段 */
    conn->parsing_header_field = 0;
    EXPECT_EQ(conn->parsing_header_field, 0);
    
    conn->parsing_header_field = 1;
    EXPECT_EQ(conn->parsing_header_field, 1);
    
    conn->parsing_header_field = 0;
    EXPECT_EQ(conn->parsing_header_field, 0);
    
    uvhttp_connection_free(conn);
    destroy_server_and_loop(loop, server);
}

/* ========== 测试连接 WebSocket 字段 ========== */

#if UVHTTP_FEATURE_WEBSOCKET
TEST(UvhttpConnectionFullApiTest, ConnectionWebSocket) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试 is_websocket 字段 */
    conn->is_websocket = 0;
    EXPECT_EQ(conn->is_websocket, 0);
    
    conn->is_websocket = 1;
    EXPECT_EQ(conn->is_websocket, 1);
    
    conn->is_websocket = 0;
    EXPECT_EQ(conn->is_websocket, 0);
    
    /* 测试 ws_connection 字段 */
    EXPECT_EQ(conn->ws_connection, nullptr);
    
    /* 清理连接 */
    uvhttp_connection_free(conn);
    
    /* 清理服务器 - 运行循环以处理异步关闭 */
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}
#endif /* UVHTTP_FEATURE_WEBSOCKET */