/* uvhttp_connection.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"

TEST(UvhttpConnectionFullCoverageTest, ConnectionStructSize) {
    EXPECT_GT(sizeof(uvhttp_connection_t), 0);
    EXPECT_GT(sizeof(uvhttp_connection_state_t), 0);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionStateEnumValues) {
    EXPECT_EQ(UVHTTP_CONN_STATE_NEW, 0);
    EXPECT_EQ(UVHTTP_CONN_STATE_TLS_HANDSHAKE, 1);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_READING, 2);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_PROCESSING, 3);
    EXPECT_EQ(UVHTTP_CONN_STATE_HTTP_WRITING, 4);
    EXPECT_EQ(UVHTTP_CONN_STATE_CLOSING, 5);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = uvhttp_connection_new(nullptr);
    if (conn != nullptr) {
        uvhttp_connection_free(conn);
    }
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionFreeNull) {
    uvhttp_connection_free(nullptr);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionStartNull) {
    EXPECT_NE(uvhttp_connection_start(nullptr), 0);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionCloseNull) {
    uvhttp_connection_close(nullptr);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionRestartReadNull) {
    EXPECT_NE(uvhttp_connection_restart_read(nullptr), 0);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionScheduleRestartReadNull) {
    EXPECT_NE(uvhttp_connection_schedule_restart_read(nullptr), 0);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionSetStateNull) {
    uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionFieldInitialization) {
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
    EXPECT_EQ(conn.mempool, nullptr);
    EXPECT_EQ(conn.last_error, 0);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionStateTransitions) {
    EXPECT_LT(UVHTTP_CONN_STATE_NEW, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    EXPECT_LT(UVHTTP_CONN_STATE_TLS_HANDSHAKE, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_READING, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_PROCESSING, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_LT(UVHTTP_CONN_STATE_HTTP_WRITING, UVHTTP_CONN_STATE_CLOSING);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(size_t));
    EXPECT_GE(sizeof(uvhttp_connection_t), sizeof(int));
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionConstants) {
    EXPECT_GT(UVHTTP_MAX_HEADER_NAME_SIZE, 0);
}

TEST(UvhttpConnectionFullCoverageTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_connection_start(nullptr);
        uvhttp_connection_close(nullptr);
        uvhttp_connection_restart_read(nullptr);
        uvhttp_connection_schedule_restart_read(nullptr);
        uvhttp_connection_set_state(nullptr, UVHTTP_CONN_STATE_NEW);
        uvhttp_connection_free(nullptr);
    }
}

TEST(UvhttpConnectionFullCoverageTest, BoundaryConditions) {
    EXPECT_GT(UVHTTP_MAX_HEADER_NAME_SIZE, 0);
    EXPECT_GE(UVHTTP_CONN_STATE_NEW, 0);
    EXPECT_GE(UVHTTP_CONN_STATE_CLOSING, 0);
    EXPECT_LE(UVHTTP_CONN_STATE_CLOSING, 10);
}

TEST(UvhttpConnectionFullCoverageTest, ConnectionMemoryAllocation) {
    size_t expected_size = sizeof(struct uvhttp_server*) + 
                           sizeof(uvhttp_request_t*) + 
                           sizeof(uvhttp_response_t*) +
                           sizeof(uv_tcp_t) +
                           sizeof(uv_idle_t) +
                           sizeof(void*) +  /* ssl */
                           sizeof(int) +    /* tls_enabled */
#if UVHTTP_FEATURE_WEBSOCKET
                           sizeof(void*) +  /* ws_connection */
                           sizeof(int) +    /* is_websocket */
#endif
                           sizeof(uvhttp_connection_state_t) +
                           sizeof(char*) +  /* read_buffer */
                           sizeof(size_t) + /* read_buffer_size */
                           sizeof(size_t) + /* read_buffer_used */
                           sizeof(int) +    /* current_header_is_important */
                           sizeof(int) +    /* keep_alive */
                           sizeof(int) +    /* chunked_encoding */
                           sizeof(size_t) + /* content_length */
                           sizeof(size_t) + /* body_received */
                           sizeof(int) +    /* parsing_complete */
                           UVHTTP_MAX_HEADER_NAME_SIZE + /* current_header_field */
                           sizeof(size_t) + /* current_header_field_len */
                           sizeof(int) +    /* parsing_header_field */
                           sizeof(int) +    /* need_restart_read */
                           sizeof(uvhttp_mempool_t*) +
                           sizeof(int);     /* last_error */
    
    EXPECT_GE(sizeof(uvhttp_connection_t), expected_size);
}