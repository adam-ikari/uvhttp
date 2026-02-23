/* UVHTTP Connection Extended Coverage Test - Target: 60%+ */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_constants.h"

/* ========== Test Connection Restart Read ========== */

TEST(UvhttpConnectionExtendedTest, RestartReadNullConnection) {
    uvhttp_error_t result = uvhttp_connection_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== Test Connection Free ========== */

TEST(UvhttpConnectionExtendedTest, FreeNullConnection) {
    /* Should not crash */
    uvhttp_connection_free(NULL);
}

/* ========== Test Connection Close ========== */

TEST(UvhttpConnectionExtendedTest, CloseNullConnection) {
    /* Should not crash */
    uvhttp_connection_close(NULL);
}

/* ========== Test Connection Set State ========== */

TEST(UvhttpConnectionExtendedTest, SetStateNullConnection) {
    /* Should not crash */
    uvhttp_connection_set_state(NULL, UVHTTP_CONN_STATE_NEW);
}

/* ========== Test Connection Start ========== */

TEST(UvhttpConnectionExtendedTest, StartNullConnection) {
    uvhttp_error_t result = uvhttp_connection_start(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== Test Connection Start Timeout ========== */

TEST(UvhttpConnectionExtendedTest, StartTimeoutNullConnection) {
    uvhttp_error_t result = uvhttp_connection_start_timeout(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, StartTimeoutCustomNullConnection) {
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(NULL, 60);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, StartTimeoutCustomInvalidValue) {
    /* Test timeout value below minimum */
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(NULL, 1);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* Test timeout value above maximum */
    result = uvhttp_connection_start_timeout_custom(NULL, 1000);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== Test Connection TLS Functions ========== */

TEST(UvhttpConnectionExtendedTest, TlsHandshakeFuncNullConnection) {
    uvhttp_error_t result = uvhttp_connection_tls_handshake_func(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, TlsReadNullConnection) {
    uvhttp_error_t result = uvhttp_connection_tls_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, TlsCleanupNullConnection) {
    /* Should not crash */
    uvhttp_connection_tls_cleanup(NULL);
}

TEST(UvhttpConnectionExtendedTest, TlsWriteNullConnection) {
    uvhttp_error_t result = uvhttp_connection_tls_write(NULL, "test", 4);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, TlsWriteNullData) {
    /* Create a minimal connection structure for testing */
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    uvhttp_error_t result = uvhttp_connection_tls_write(&conn, NULL, 4);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, TlsWriteZeroLength) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    uvhttp_error_t result = uvhttp_connection_tls_write(&conn, "test", 0);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== Test Connection Schedule Restart Read ========== */

TEST(UvhttpConnectionExtendedTest, ScheduleRestartReadNullConnection) {
    uvhttp_error_t result = uvhttp_connection_schedule_restart_read(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

#if UVHTTP_FEATURE_WEBSOCKET

/* ========== Test WebSocket Functions ========== */

TEST(UvhttpConnectionExtendedTest, HandleWebsocketHandshakeNullConnection) {
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(NULL, "test-key");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, HandleWebsocketHandshakeNullKey) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(&conn, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpConnectionExtendedTest, SwitchToWebsocketNullConnection) {
    /* Should not crash */
    uvhttp_connection_switch_to_websocket(NULL);
}

TEST(UvhttpConnectionExtendedTest, WebsocketCloseNullConnection) {
    /* Should not crash */
    uvhttp_connection_websocket_close(NULL);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */

/* ========== Test Connection New ========== */

TEST(UvhttpConnectionExtendedTest, NewNullServer) {
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, (uvhttp_connection_t*)NULL);
}

TEST(UvhttpConnectionExtendedTest, NewNullConnectionPtr) {
    uvhttp_server_t server;
    memset(&server, 0, sizeof(server));
    
    uvhttp_error_t result = uvhttp_connection_new(&server, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== Test Connection State Transitions ========== */

TEST(UvhttpConnectionExtendedTest, StateTransitionSequence) {
    /* Test that state transitions can be set in sequence */
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_HTTP_WRITING);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_HTTP_WRITING);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_PROTOCOL_UPGRADED);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_PROTOCOL_UPGRADED);
    
    uvhttp_connection_set_state(&conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn.state, UVHTTP_CONN_STATE_CLOSING);
}

/* ========== Test Connection Field Initialization ========== */

TEST(UvhttpConnectionExtendedTest, ConnectionFieldsInitialization) {
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    
    /* Verify all important fields are initialized to zero */
    EXPECT_EQ(conn.state, (uvhttp_connection_state_t)0);
    EXPECT_EQ(conn.parsing_complete, 0);
    EXPECT_EQ(conn.keepalive, 0);
    EXPECT_EQ(conn.chunked_encoding, 0);
    EXPECT_EQ(conn.close_pending, 0);
    EXPECT_EQ(conn.need_restart_read, 0);
    EXPECT_EQ(conn.tls_enabled, 0);
    EXPECT_EQ(conn.freed, 0);
    EXPECT_EQ(conn.body_received, (size_t)0);
    EXPECT_EQ(conn.content_length, (size_t)0);
    EXPECT_EQ(conn.read_buffer_used, (size_t)0);
    EXPECT_EQ(conn.read_buffer_size, (size_t)0);
    EXPECT_EQ(conn.server, (uvhttp_server_t*)NULL);
    EXPECT_EQ(conn.request, (uvhttp_request_t*)NULL);
    EXPECT_EQ(conn.response, (uvhttp_response_t*)NULL);
    EXPECT_EQ(conn.ssl, (void*)NULL);
    EXPECT_EQ(conn.read_buffer, (char*)NULL);
}

/* ========== Test Connection Timeout Range Validation ========== */

TEST(UvhttpConnectionExtendedTest, TimeoutRangeValidation) {
    /* Test minimum timeout (5 seconds) */
    uvhttp_error_t result = uvhttp_connection_start_timeout_custom(NULL, 5);
    EXPECT_NE(result, UVHTTP_OK);  /* NULL connection should fail */
    
    /* Test maximum timeout (300 seconds) */
    result = uvhttp_connection_start_timeout_custom(NULL, 300);
    EXPECT_NE(result, UVHTTP_OK);  /* NULL connection should fail */
    
    /* Test boundary values */
    result = uvhttp_connection_start_timeout_custom(NULL, 4);   /* Below minimum */
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_connection_start_timeout_custom(NULL, 301); /* Above maximum */
    EXPECT_NE(result, UVHTTP_OK);
}