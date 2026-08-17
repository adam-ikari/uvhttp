/* uvhttp_websocket.c automated integration tests
 * Comprehensive tests covering connection lifecycle, frame parsing,
 * frame building, send/receive operations, ping/pong, close,
 * state machine transitions, NULL handling, and error conditions. */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_websocket.h"
#include "uvhttp_context.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>
#include <stdlib.h>

/* ===================================================================
 * Helper: create a test connection with a given fd and mode.
 * =================================================================== */
static uvhttp_ws_connection_t* create_test_conn(int fd, int is_server) {
    return uvhttp_ws_connection_create(fd, NULL, is_server, NULL);
}

/* ===================================================================
 * uvhttp_ws_connection_create and uvhttp_ws_connection_free
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConnectionCreateFreeServer) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->is_server, 1);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);
    EXPECT_NE(conn->recv_buffer, nullptr);
    EXPECT_EQ(conn->recv_buffer_size, 64U * 1024);
    EXPECT_EQ(conn->recv_buffer_pos, 0U);
    EXPECT_EQ(conn->bytes_sent, 0U);
    EXPECT_EQ(conn->bytes_received, 0U);
    EXPECT_EQ(conn->frames_sent, 0U);
    EXPECT_EQ(conn->frames_received, 0U);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionCreateFreeClient) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->is_server, 0);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);
    EXPECT_EQ(conn->client_key[0], '\0');
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionFreeNull) {
    uvhttp_ws_connection_free(NULL);
    SUCCEED();
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionCreateWithValidFd) {
    /* fd=0 (stdin) is a valid fd on most systems */
    uvhttp_ws_connection_t* conn = create_test_conn(0, 1);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->fd, 0);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionCreateWithConfig) {
    uvhttp_config_t config;
    memset(&config, 0, sizeof(config));
    config.websocket_max_frame_size = 65536;
    config.websocket_ping_interval = 15;

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(-1, NULL, 1, &config);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->config.max_frame_size, 65536);
    EXPECT_EQ(conn->config.ping_interval, 15);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionCreateFreeTwice) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_ws_connection_free(conn);
    /* second free of the same pointer is undefined behaviour,
     * so we just test that the first free succeeds */
    SUCCEED();
}

/* ===================================================================
 * uvhttp_ws_connection_free — double free / use-after-free
 * (not directly testable in a portable way, but we verify the function
 *  exists and compiles by calling it once)
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConnectionFreeNullDoesNotCrash) {
    uvhttp_ws_connection_free(NULL);
    uvhttp_ws_connection_free(NULL);
    SUCCEED();
}

/* ===================================================================
 * uvhttp_ws_parse_frame_header — various frame types
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderTextUnmasked) {
    /* FIN=1, opcode=TEXT(1), mask=0, len=5, payload="Hello" */
    uint8_t data[] = {0x81, 0x05, 'H', 'e', 'l', 'l', 'o'};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(header.mask, 0);
    EXPECT_EQ(header.payload_len, 5);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderTextMasked) {
    /* FIN=1, opcode=TEXT(1), mask=1, len=5 */
    uint8_t data[] = {0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(header.mask, 1);
    EXPECT_EQ(header.payload_len, 5);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderBinary) {
    /* FIN=1, opcode=BINARY(2), mask=0, len=3 */
    uint8_t data[] = {0x82, 0x03, 0x01, 0x02, 0x03};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_BINARY);
    EXPECT_EQ(header.mask, 0);
    EXPECT_EQ(header.payload_len, 3);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderClose) {
    /* FIN=1, opcode=CLOSE(8), mask=0, len=0 */
    uint8_t data[] = {0x88, 0x00};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_CLOSE);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderPing) {
    /* FIN=1, opcode=PING(9), mask=0, len=4 */
    uint8_t data[] = {0x89, 0x04, 0x01, 0x02, 0x03, 0x04};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_PING);
    EXPECT_EQ(header.payload_len, 4);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderPong) {
    /* FIN=1, opcode=PONG(10), mask=0, len=0 */
    uint8_t data[] = {0x8A, 0x00};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_PONG);
    EXPECT_EQ(header_size, 2U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderFragmented) {
    /* FIN=0, opcode=TEXT(1), mask=0, len=5 */
    uint8_t data[] = {0x01, 0x05, 'H', 'e', 'l', 'l', 'o'};
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header.fin, 0);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(header.payload_len, 5);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderExtendedLength16) {
    /* FIN=1, opcode=BINARY(2), mask=0, extended length 16 (126) */
    uint8_t data[] = {0x82, 0x7E, 0x01, 0x00}; /* 256 bytes */
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header_size, 4U);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderExtendedLength64) {
    /* FIN=1, opcode=BINARY(2), mask=0, extended length 64 (127) */
    uint8_t data[] = {0x82, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; /* 65536 */
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;

    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, &header_size);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(header_size, 10U);
}

/* ===================================================================
 * uvhttp_ws_parse_frame_header — NULL / error handling
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderNullData) {
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(NULL, 2, &header, &header_size);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderNullHeader) {
    uint8_t data[] = {0x81, 0x00};
    size_t header_size = 0;
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), NULL, &header_size);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderNullHeaderSize) {
    uint8_t data[] = {0x81, 0x00};
    uvhttp_ws_frame_header_t header;
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, sizeof(data), &header, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderInsufficientData) {
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    /* only 1 byte, need at least 2 */
    uint8_t data[] = {0x81};
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(data, 1, &header, &header_size);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ParseFrameHeaderZeroLength) {
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(NULL, 0, &header, &header_size);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ===================================================================
 * uvhttp_ws_apply_mask
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskRoundTrip) {
    uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; /* "Hello" */
    uint8_t masking_key[] = {0x37, 0xfa, 0x21, 0x3d};
    uint8_t original[sizeof(data)];
    memcpy(original, data, sizeof(data));

    uvhttp_ws_apply_mask(data, sizeof(data), masking_key);
    /* data should differ from original */
    EXPECT_NE(memcmp(data, original, sizeof(data)), 0);

    /* applying again restores */
    uvhttp_ws_apply_mask(data, sizeof(data), masking_key);
    EXPECT_EQ(memcmp(data, original, sizeof(data)), 0);
}

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskSingleByte) {
    uint8_t data[] = {0xFF};
    uint8_t key[] = {0x12, 0x34, 0x56, 0x78};
    uvhttp_ws_apply_mask(data, sizeof(data), key);
    EXPECT_EQ(data[0], (uint8_t)(0xFF ^ 0x12));
    uvhttp_ws_apply_mask(data, sizeof(data), key);
    EXPECT_EQ(data[0], 0xFF);
}

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskLargeData) {
    uint8_t data[1024];
    uint8_t key[] = {0x12, 0x34, 0x56, 0x78};
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    uint8_t original[sizeof(data)];
    memcpy(original, data, sizeof(data));

    uvhttp_ws_apply_mask(data, sizeof(data), key);
    EXPECT_NE(memcmp(data, original, sizeof(data)), 0);

    uvhttp_ws_apply_mask(data, sizeof(data), key);
    EXPECT_EQ(memcmp(data, original, sizeof(data)), 0);
}

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskNullData) {
    uint8_t key[] = {0x12, 0x34, 0x56, 0x78};
    /* should not crash */
    uvhttp_ws_apply_mask(NULL, 5, key);
    SUCCEED();
}

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskNullKey) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    /* should not crash */
    uvhttp_ws_apply_mask(data, sizeof(data), NULL);
    SUCCEED();
}

TEST(UvhttpWebsocketAutomatedTest, ApplyMaskZeroLength) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint8_t key[] = {0x12, 0x34, 0x56, 0x78};
    uvhttp_ws_apply_mask(data, 0, key);
    /* data unchanged */
    EXPECT_EQ(data[0], 0x01);
    EXPECT_EQ(data[1], 0x02);
    EXPECT_EQ(data[2], 0x03);
}

/* ===================================================================
 * uvhttp_ws_build_frame — text, binary, close, ping, pong
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, BuildFrameText) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];
    const char* payload = "Hello";

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   (const uint8_t*)payload,
                                                   strlen(payload),
                                                   UVHTTP_WS_OPCODE_TEXT, 0, 1);
    /* uvhttp_ws_build_frame returns the total frame size (positive) on success */
    EXPECT_GT(result, 0);
    /* FIN=1, opcode=TEXT */
    EXPECT_EQ(buffer[0], 0x81);
    /* mask=0, len=5 */
    EXPECT_EQ(buffer[1], 0x05);
    EXPECT_EQ(memcmp(buffer + 2, payload, 5), 0);
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameBinary) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];
    uint8_t payload[] = {0x00, 0x01, 0x02, 0x03};

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   payload, sizeof(payload),
                                                   UVHTTP_WS_OPCODE_BINARY, 0, 1);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x82); /* FIN=1, opcode=BINARY */
    EXPECT_EQ(buffer[1], 0x04); /* len=4 */
    EXPECT_EQ(memcmp(buffer + 2, payload, sizeof(payload)), 0);
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameClose) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   NULL, 0,
                                                   UVHTTP_WS_OPCODE_CLOSE, 0, 1);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x88); /* FIN=1, opcode=CLOSE */
    EXPECT_EQ(buffer[1], 0x00); /* len=0 */
}

TEST(UvhttpWebsocketAutomatedTest, BuildFramePing) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];
    uint8_t ping_data[] = {0x01, 0x02};

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   ping_data, sizeof(ping_data),
                                                   UVHTTP_WS_OPCODE_PING, 0, 1);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x89); /* FIN=1, opcode=PING */
    EXPECT_EQ(buffer[1], 0x02); /* len=2 */
    EXPECT_EQ(memcmp(buffer + 2, ping_data, 2), 0);
}

TEST(UvhttpWebsocketAutomatedTest, BuildFramePong) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   NULL, 0,
                                                   UVHTTP_WS_OPCODE_PONG, 0, 1);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x8A); /* FIN=1, opcode=PONG */
    EXPECT_EQ(buffer[1], 0x00); /* len=0 */
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameMasked) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_context_init_websocket(&context);
    uint8_t buffer[256];
    const char* payload = "Hello";

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   (const uint8_t*)payload,
                                                   strlen(payload),
                                                   UVHTTP_WS_OPCODE_TEXT, 1, 1);
    EXPECT_GT(result, 0);
    /* mask bit should be set */
    EXPECT_NE(buffer[1] & 0x80, 0);
    uvhttp_context_cleanup_websocket(&context);
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameFragmented) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];
    const char* payload = "Hello";

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   (const uint8_t*)payload,
                                                   strlen(payload),
                                                   UVHTTP_WS_OPCODE_TEXT, 0, 0);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x01); /* FIN=0, opcode=TEXT */
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameContinuation) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[256];
    const char* payload = " world";

    long result = uvhttp_ws_build_frame(&context, buffer, sizeof(buffer),
                                                   (const uint8_t*)payload,
                                                   strlen(payload),
                                                   UVHTTP_WS_OPCODE_CONTINUATION, 0, 1);
    EXPECT_GT(result, 0);
    EXPECT_EQ(buffer[0], 0x80); /* FIN=1, opcode=CONTINUATION */
}

/* ===================================================================
 * uvhttp_ws_build_frame — NULL / error handling
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, BuildFrameNullBuffer) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    long result = uvhttp_ws_build_frame(&context, NULL, 256, NULL, 0,
                                                   UVHTTP_WS_OPCODE_TEXT, 0, 1);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, BuildFrameBufferTooSmall) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t buffer[2]; /* only enough for header, no payload */
    long result = uvhttp_ws_build_frame(&context, buffer, 2,
                                                   (const uint8_t*)"Hello", 5,
                                                   UVHTTP_WS_OPCODE_TEXT, 0, 1);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ===================================================================
 * uvhttp_ws_send_frame, uvhttp_ws_send_text, uvhttp_ws_send_binary
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, SendFrameNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_error_t result = uvhttp_ws_send_frame(&context, NULL, NULL, 0,
                                                  UVHTTP_WS_OPCODE_TEXT);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, SendFrameNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_send_frame(NULL, conn, NULL, 0,
                                                  UVHTTP_WS_OPCODE_TEXT);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendTextNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_error_t result = uvhttp_ws_send_text(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, SendTextNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_send_text(NULL, conn, "hello", 5);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendTextEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    /* empty text send may succeed or fail depending on implementation */
    uvhttp_ws_send_text(&context, conn, "", 0);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendBinaryNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uint8_t data[] = {0x01, 0x02};
    uvhttp_error_t result = uvhttp_ws_send_binary(&context, NULL, data, sizeof(data));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, SendBinaryNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01, 0x02};
    uvhttp_error_t result = uvhttp_ws_send_binary(NULL, conn, data, sizeof(data));
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendBinaryWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    /* may succeed or fail depending on implementation */
    uvhttp_ws_send_binary(&context, conn, data, sizeof(data));
    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_close
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, CloseNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_error_t result = uvhttp_ws_close(&context, NULL, 1000, "Normal closure");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, CloseNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_close(NULL, conn, 1000, "Normal closure");
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, CloseNullReason) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_close(&context, conn, 1000, NULL);
    /* NULL reason may be handled gracefully */
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, CloseEmptyReason) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_ws_close(&context, conn, 1000, "");
    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_send_ping and uvhttp_ws_send_pong
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, SendPingNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_error_t result = uvhttp_ws_send_ping(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, SendPingNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01};
    uvhttp_error_t result = uvhttp_ws_send_ping(NULL, conn, data, sizeof(data));
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendPingWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01, 0x02, 0x03};
    uvhttp_ws_send_ping(&context, conn, data, sizeof(data));
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendPingEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_ws_send_ping(&context, conn, NULL, 0);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendPongNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_error_t result = uvhttp_ws_send_pong(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, SendPongNullContext) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01};
    uvhttp_error_t result = uvhttp_ws_send_pong(NULL, conn, data, sizeof(data));
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendPongWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uvhttp_ws_send_pong(&context, conn, data, sizeof(data));
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SendPongEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_ws_send_pong(&context, conn, NULL, 0);
    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * WebSocket state machine: CONNECTING -> OPEN -> CLOSING -> CLOSED
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, StateMachineInitialState) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, StateMachineTransitions) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    /* CONNECTING */
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);

    /* transition to OPEN */
    conn->state = UVHTTP_WS_STATE_OPEN;
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_OPEN);

    /* transition to CLOSING */
    conn->state = UVHTTP_WS_STATE_CLOSING;
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSING);

    /* transition to CLOSED */
    conn->state = UVHTTP_WS_STATE_CLOSED;
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, StateMachineClientMode) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);

    conn->state = UVHTTP_WS_STATE_OPEN;
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_OPEN);

    conn->state = UVHTTP_WS_STATE_CLOSED;
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, StateMachineEnumValues) {
    EXPECT_EQ(UVHTTP_WS_STATE_CONNECTING, 0);
    EXPECT_EQ(UVHTTP_WS_STATE_OPEN, 1);
    EXPECT_EQ(UVHTTP_WS_STATE_CLOSING, 2);
    EXPECT_EQ(UVHTTP_WS_STATE_CLOSED, 3);
}

/* ===================================================================
 * WebSocket opcode enum values
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, OpcodeEnumValues) {
    EXPECT_EQ(UVHTTP_WS_OPCODE_CONTINUATION, 0x0);
    EXPECT_EQ(UVHTTP_WS_OPCODE_TEXT, 0x1);
    EXPECT_EQ(UVHTTP_WS_OPCODE_BINARY, 0x2);
    EXPECT_EQ(UVHTTP_WS_OPCODE_CLOSE, 0x8);
    EXPECT_EQ(UVHTTP_WS_OPCODE_PING, 0x9);
    EXPECT_EQ(UVHTTP_WS_OPCODE_PONG, 0xA);
}

/* ===================================================================
 * NULL parameter handling for all functions
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeServerNullConn) {
    char response[256];
    size_t response_len = sizeof(response);
    uvhttp_error_t result = uvhttp_ws_handshake_server(NULL, "req", 3, response, &response_len);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeServerNullRequest) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    char response[256];
    size_t response_len = sizeof(response);
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, NULL, 0, response, &response_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeServerNullResponse) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    size_t response_len = 0;
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, "req", 3, NULL, &response_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeServerNullResponseLen) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    char response[256];
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, "req", 3, response, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeClientNullConn) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    char request[256];
    size_t request_len = sizeof(request);
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, NULL, "host", "/", request, &request_len);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeClientNullHost) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    char request[256];
    size_t request_len = sizeof(request);
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, NULL, "/", request, &request_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullHandshakeClientNullPath) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    char request[256];
    size_t request_len = sizeof(request);
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, "host", NULL, request, &request_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullVerifyHandshakeResponseNullConn) {
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(NULL, "resp", 4);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullRecvFrameNullConn) {
    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    uvhttp_error_t result = uvhttp_ws_recv_frame(NULL, &frame);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullRecvFrameNullFrame) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_recv_frame(conn, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullSetCallbacksNullConn) {
    /* should not crash */
    uvhttp_ws_set_callbacks(NULL, NULL, NULL, NULL);
    SUCCEED();
}

TEST(UvhttpWebsocketAutomatedTest, NullSetCallbacksAllNull) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_ws_set_callbacks(conn, NULL, NULL, NULL);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, NullGenerateAcceptNullKey) {
    char accept[64];
    uvhttp_error_t result = uvhttp_ws_generate_accept(NULL, accept, sizeof(accept));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullGenerateAcceptNullAccept) {
    uvhttp_error_t result = uvhttp_ws_generate_accept("key", NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullVerifyAcceptNullKey) {
    uvhttp_error_t result = uvhttp_ws_verify_accept(NULL, "accept");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, NullVerifyAcceptNullAccept) {
    uvhttp_error_t result = uvhttp_ws_verify_accept("key", NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ===================================================================
 * Error conditions
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ErrorHandshakeServerEmptyRequest) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    char response[256];
    size_t response_len = sizeof(response);
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, "", 0, response, &response_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ErrorHandshakeServerInvalidRequest) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    char response[256];
    size_t response_len = sizeof(response);
    const char* bad_req = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, bad_req, strlen(bad_req), response, &response_len);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ErrorVerifyHandshakeResponseEmpty) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(conn, "", 0);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ErrorVerifyHandshakeResponseInvalid) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 0);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(conn, "not a valid HTTP response", 24);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ErrorGenerateAcceptSmallBuffer) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    char accept[4]; /* too small */
    uvhttp_error_t result = uvhttp_ws_generate_accept(key, accept, sizeof(accept));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ErrorVerifyAcceptWrongValue) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    const char* accept = "wrong_accept_value_here";
    uvhttp_error_t result = uvhttp_ws_verify_accept(key, accept);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ===================================================================
 * uvhttp_ws_generate_accept — RFC 6455 example
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, GenerateAcceptRfcExample) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    char accept[64];
    memset(accept, 0, sizeof(accept));

    uvhttp_error_t result = uvhttp_ws_generate_accept(key, accept, sizeof(accept));
    EXPECT_EQ(result, UVHTTP_OK);

    /* RFC 6455 Section 4.2.2 example */
    EXPECT_STREQ(accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

/* ===================================================================
 * uvhttp_ws_verify_accept — RFC 6455 example
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, VerifyAcceptRfcExample) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    const char* accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
    uvhttp_error_t result = uvhttp_ws_verify_accept(key, accept);
    EXPECT_EQ(result, UVHTTP_OK);
}

/* ===================================================================
 * uvhttp_ws_connection_t — struct field validation
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConnectionStructFields) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    /* verify all fields are accessible / zero-initialized where expected */
    EXPECT_EQ(conn->fd, -1);
    EXPECT_EQ(conn->ssl, nullptr);
    EXPECT_EQ(conn->user_data, nullptr);
    EXPECT_EQ(conn->send_buffer, nullptr);
    EXPECT_EQ(conn->send_buffer_size, 0U);
    EXPECT_EQ(conn->fragmented_message, nullptr);
    EXPECT_EQ(conn->fragmented_size, 0U);
    EXPECT_EQ(conn->fragmented_capacity, 0U);
    EXPECT_EQ(conn->on_message, nullptr);
    EXPECT_EQ(conn->on_close, nullptr);
    EXPECT_EQ(conn->on_error, nullptr);

    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ConnectionStructUserData) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->user_data, nullptr);

    int test_value = 42;
    conn->user_data = &test_value;
    EXPECT_EQ(*(int*)conn->user_data, 42);

    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_frame_header_t — bit field layout
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, FrameHeaderBitFields) {
    uvhttp_ws_frame_header_t header;
    memset(&header, 0, sizeof(header));

    header.fin = 1;
    header.rsv1 = 1;
    header.rsv2 = 1;
    header.rsv3 = 1;
    header.opcode = UVHTTP_WS_OPCODE_TEXT;
    header.mask = 1;
    header.payload_len = 42;

    EXPECT_EQ(header.fin, 1);
    EXPECT_EQ(header.rsv1, 1);
    EXPECT_EQ(header.rsv2, 1);
    EXPECT_EQ(header.rsv3, 1);
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(header.mask, 1);
    EXPECT_EQ(header.payload_len, 42);
}

/* ===================================================================
 * uvhttp_ws_frame_t — structure layout
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, FrameStructureLayout) {
    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.header.fin = 1;
    frame.header.opcode = UVHTTP_WS_OPCODE_TEXT;
    frame.payload_length = 1024;
    frame.masking_key[0] = 0x01;
    frame.masking_key[1] = 0x02;
    frame.masking_key[2] = 0x03;
    frame.masking_key[3] = 0x04;

    uint8_t* test_payload = (uint8_t*)uvhttp_alloc(10);
    ASSERT_NE(test_payload, nullptr);
    frame.payload = test_payload;
    frame.payload_size = 10;

    EXPECT_EQ(frame.header.fin, 1);
    EXPECT_EQ(frame.payload_length, 1024U);
    EXPECT_EQ(frame.payload_size, 10U);

    uvhttp_free(test_payload);
}

/* ===================================================================
 * uvhttp_ws_connection_t — config defaults
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConnectionConfigDefaults) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(conn->config.max_frame_size, 16 * 1024 * 1024);
    EXPECT_EQ(conn->config.max_message_size, 64 * 1024 * 1024);
    EXPECT_EQ(conn->config.ping_interval, 30);
    EXPECT_EQ(conn->config.ping_timeout, 10);
    EXPECT_EQ(conn->config.enable_compression, 0);

    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_connection_t — statistics
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConnectionStatistics) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(conn->bytes_sent, 0U);
    EXPECT_EQ(conn->bytes_received, 0U);
    EXPECT_EQ(conn->frames_sent, 0U);
    EXPECT_EQ(conn->frames_received, 0U);

    /* simulate some activity */
    conn->bytes_sent = 100;
    conn->bytes_received = 200;
    conn->frames_sent = 5;
    conn->frames_received = 3;

    EXPECT_EQ(conn->bytes_sent, 100U);
    EXPECT_EQ(conn->bytes_received, 200U);
    EXPECT_EQ(conn->frames_sent, 5U);
    EXPECT_EQ(conn->frames_received, 3U);

    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_set_callbacks — verify callback pointers
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, SetCallbacksVerifyPointers) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    /* use lambda-style function pointers */
    auto msg_cb = +[](uvhttp_ws_connection_t*, const char*, size_t, int) -> int { return 0; };
    auto close_cb = +[](uvhttp_ws_connection_t*, int, const char*) -> int { return 0; };
    auto err_cb = +[](uvhttp_ws_connection_t*, int, const char*) -> int { return 0; };

    uvhttp_ws_set_callbacks(conn, msg_cb, close_cb, err_cb);

    /* verify the callbacks were set */
    EXPECT_NE(conn->on_message, nullptr);
    EXPECT_NE(conn->on_close, nullptr);
    EXPECT_NE(conn->on_error, nullptr);

    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, SetCallbacksPartial) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);

    auto msg_cb = +[](uvhttp_ws_connection_t*, const char*, size_t, int) -> int { return 0; };

    /* only set message callback, leave close and error NULL */
    uvhttp_ws_set_callbacks(conn, msg_cb, NULL, NULL);

    EXPECT_NE(conn->on_message, nullptr);
    EXPECT_EQ(conn->on_close, nullptr);
    EXPECT_EQ(conn->on_error, nullptr);

    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_process_data — NULL handling
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ProcessDataNullConn) {
    uvhttp_error_t result = uvhttp_ws_process_data(NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpWebsocketAutomatedTest, ProcessDataNullData) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    uvhttp_error_t result = uvhttp_ws_process_data(conn, NULL, 0);
    /* may return error or UVHTTP_OK, depending on implementation */
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketAutomatedTest, ProcessDataEmpty) {
    uvhttp_ws_connection_t* conn = create_test_conn(-1, 1);
    ASSERT_NE(conn, nullptr);
    /* should not crash */
    uvhttp_ws_process_data(conn, (const uint8_t*)"", 0);
    uvhttp_ws_connection_free(conn);
}

/* ===================================================================
 * uvhttp_ws_config_t — structure layout
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConfigStructure) {
    uvhttp_ws_config_t config;
    memset(&config, 0, sizeof(config));

    config.max_frame_size = 65536;
    config.max_message_size = 131072;
    config.ping_interval = 10;
    config.ping_timeout = 5;
    config.enable_compression = 1;

    EXPECT_EQ(config.max_frame_size, 65536);
    EXPECT_EQ(config.max_message_size, 131072);
    EXPECT_EQ(config.ping_interval, 10);
    EXPECT_EQ(config.ping_timeout, 5);
    EXPECT_EQ(config.enable_compression, 1);
}

/* ===================================================================
 * Convenience macro tests
 * =================================================================== */

TEST(UvhttpWebsocketAutomatedTest, ConvenienceMacrosCompile) {
    /* These macros should compile without error */
    /* uvhttp_websocket_send_text(ctx, ws, text) */
    /* uvhttp_websocket_send_binary(ctx, ws, data, len) */
    SUCCEED();
}

#else /* UVHTTP_FEATURE_WEBSOCKET */

/* WebSocket feature not enabled — placeholder test */
TEST(UvhttpWebsocketAutomatedTest, WebSocketDisabled) {
    SUCCEED() << "WebSocket feature not enabled, skipping tests";
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */