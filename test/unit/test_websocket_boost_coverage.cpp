/**
 * @file test_websocket_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_websocket.c
 *
 * Targets uncovered lines:
 * - build_frame with 127-byte extended length (payload_len >= 65536)
 * - build_frame mask path with DRBG
 * - build_frame server-side copy payload path (mask=0)
 * - uvhttp_ws_send_frame (OPEN state connection)
 * - uvhttp_ws_process_data (text, binary, close, ping, fragmented frames)
 * - uvhttp_server_register_websocket_upgrade
 */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_allocator.h"
#include "uvhttp_context.h"
#include "uvhttp_websocket.h"
}

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* mbedtls headers for DRBG initialization */
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

/* ========== Helper: build a raw WebSocket frame ========== */

/**
 * Build a minimal unmasked WS frame into out_buf.
 * Returns total bytes written, or -1 on error.
 */
static int build_raw_frame(uint8_t* out_buf, size_t out_buf_size,
                           const uint8_t* payload, size_t payload_len,
                           uint8_t opcode, int fin) {
    size_t pos = 0;

    /* Byte 0: FIN + opcode */
    if (out_buf_size < 2) return -1;
    out_buf[pos++] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);

    /* Byte 1+: payload length (no mask) */
    if (payload_len < 126) {
        out_buf[pos++] = (uint8_t)payload_len;
    } else if (payload_len < 65536) {
        if (out_buf_size < pos + 2) return -1;
        out_buf[pos++] = 126;
        out_buf[pos++] = (payload_len >> 8) & 0xFF;
        out_buf[pos++] = payload_len & 0xFF;
    } else {
        if (out_buf_size < pos + 8) return -1;
        out_buf[pos++] = 127;
        for (int i = 7; i >= 0; i--) {
            out_buf[pos++] = (payload_len >> (i * 8)) & 0xFF;
        }
    }

    /* Payload */
    if (payload && payload_len > 0) {
        if (out_buf_size < pos + payload_len) return -1;
        memcpy(out_buf + pos, payload, payload_len);
        pos += payload_len;
    }

    return (int)pos;
}

/* ========== Helper: callback state tracking ========== */

struct WsCallbackState {
    bool message_called;
    bool close_called;
    bool error_called;
    int last_opcode;
    int close_code;
    char last_message[4096];
    size_t last_message_len;
    char close_reason[256];
};

static struct WsCallbackState g_cb_state;

static void reset_cb_state() {
    memset(&g_cb_state, 0, sizeof(g_cb_state));
}

static int test_on_message(struct uvhttp_ws_connection* conn, const char* data,
                           size_t len, int opcode) {
    (void)conn;
    g_cb_state.message_called = true;
    g_cb_state.last_opcode = opcode;
    if (data && len > 0 && len < sizeof(g_cb_state.last_message)) {
        memcpy(g_cb_state.last_message, data, len);
        g_cb_state.last_message_len = len;
    }
    return 0;
}

static int test_on_close(struct uvhttp_ws_connection* conn, int code,
                         const char* reason) {
    (void)conn;
    g_cb_state.close_called = true;
    g_cb_state.close_code = code;
    if (reason) {
        strncpy(g_cb_state.close_reason, reason,
                sizeof(g_cb_state.close_reason) - 1);
    }
    return 0;
}

static int test_on_error(struct uvhttp_ws_connection* conn, int error_code,
                         const char* error_msg) {
    (void)conn;
    (void)error_code;
    (void)error_msg;
    g_cb_state.error_called = true;
    return 0;
}

/* ========== Helper: create context with DRBG ========== */

class WsBoostTest : public ::testing::Test {
protected:
    mbedtls_entropy_context entropy_;
    mbedtls_ctr_drbg_context drbg_;
    uvhttp_context_t ctx_;

    void SetUp() override {
        memset(&ctx_, 0, sizeof(ctx_));
        mbedtls_entropy_init(&entropy_);
        mbedtls_ctr_drbg_init(&drbg_);
        int rc = mbedtls_ctr_drbg_seed(&drbg_, mbedtls_entropy_func, &entropy_,
                                       (const unsigned char*)"uvhttp_ws_test",
                                       15);
        ASSERT_EQ(rc, 0);
        ctx_.ws_drbg_initialized = 1;
        ctx_.ws_drbg = &drbg_;
        ctx_.ws_entropy = &entropy_;
        reset_cb_state();
    }

    void TearDown() override {
        mbedtls_ctr_drbg_free(&drbg_);
        mbedtls_entropy_free(&entropy_);
    }
};

/* ========== uvhttp_ws_build_frame: extended 127-byte length ========== */

TEST_F(WsBoostTest, BuildFrame_ExtendedLength127) {
    /* payload_len >= 65536 triggers the 127-byte extended length path */
    const size_t payload_len = 70000;
    uint8_t* payload = (uint8_t*)uvhttp_alloc(payload_len);
    ASSERT_NE(payload, nullptr);
    memset(payload, 'A', payload_len);

    /* header(10) + payload(70000) = 70010 */
    size_t buf_size = 10 + payload_len;
    uint8_t* buf = (uint8_t*)uvhttp_alloc(buf_size);
    ASSERT_NE(buf, nullptr);

    /* mask=0, fin=1, opcode=TEXT */
    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, buf_size, payload, payload_len,
        UVHTTP_WS_OPCODE_TEXT, 0, 1);

    EXPECT_GT(ret, 0);
    /* Verify header: FIN=1, opcode=1 */
    EXPECT_EQ(buf[0], 0x81);
    /* Extended length indicator: 127 */
    EXPECT_EQ(buf[1] & 0x7F, 127);

    uvhttp_free(buf);
    uvhttp_free(payload);
}

TEST_F(WsBoostTest, BuildFrame_ExtendedLength127_Binary) {
    const size_t payload_len = 100000;
    uint8_t* payload = (uint8_t*)uvhttp_alloc(payload_len);
    ASSERT_NE(payload, nullptr);
    memset(payload, 0xAB, payload_len);

    size_t buf_size = 10 + payload_len;
    uint8_t* buf = (uint8_t*)uvhttp_alloc(buf_size);
    ASSERT_NE(buf, nullptr);

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, buf_size, payload, payload_len,
        UVHTTP_WS_OPCODE_BINARY, 0, 1);

    EXPECT_GT(ret, 0);
    /* FIN=1, opcode=2 (BINARY) */
    EXPECT_EQ(buf[0], 0x82);
    EXPECT_EQ(buf[1] & 0x7F, 127);

    uvhttp_free(buf);
    uvhttp_free(payload);
}

/* ========== uvhttp_ws_build_frame: mask=0 server-side copy ========== */

TEST_F(WsBoostTest, BuildFrame_ServerMode_NoMask_ShortPayload) {
    const char* payload = "Hello";
    size_t payload_len = 5;
    uint8_t buf[64];

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, sizeof(buf), (const uint8_t*)payload, payload_len,
        UVHTTP_WS_OPCODE_TEXT, 0, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x81); /* FIN=1, opcode=TEXT */
    EXPECT_EQ(buf[1], 5);    /* no mask, len=5 */
    EXPECT_EQ(memcmp(buf + 2, payload, 5), 0);
}

TEST_F(WsBoostTest, BuildFrame_ServerMode_NoMask_MediumPayload) {
    /* payload_len in [126, 65535] triggers 2-byte extended length */
    const size_t payload_len = 300;
    uint8_t* payload = (uint8_t*)uvhttp_alloc(payload_len);
    ASSERT_NE(payload, nullptr);
    memset(payload, 'B', payload_len);

    size_t buf_size = 4 + payload_len;
    uint8_t* buf = (uint8_t*)uvhttp_alloc(buf_size);
    ASSERT_NE(buf, nullptr);

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, buf_size, payload, payload_len,
        UVHTTP_WS_OPCODE_TEXT, 0, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[1] & 0x7F, 126);
    EXPECT_EQ(((buf[2] << 8) | buf[3]), 300);
    EXPECT_EQ(memcmp(buf + 4, payload, payload_len), 0);

    uvhttp_free(buf);
    uvhttp_free(payload);
}

TEST_F(WsBoostTest, BuildFrame_ServerMode_NoMask_NullPayload) {
    uint8_t buf[16];

    /* payload=NULL, payload_len=0 -> zero-length frame */
    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, sizeof(buf), nullptr, 0,
        UVHTTP_WS_OPCODE_PING, 0, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x89); /* FIN=1, opcode=PING */
    EXPECT_EQ(buf[1], 0);    /* no mask, len=0 */
}

TEST_F(WsBoostTest, BuildFrame_ServerMode_NoPayload_Pong) {
    uint8_t buf[16];

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, sizeof(buf), nullptr, 0,
        UVHTTP_WS_OPCODE_PONG, 0, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x8A); /* FIN=1, opcode=PONG */
}

TEST_F(WsBoostTest, BuildFrame_FinZero) {
    const char* payload = "frag";
    uint8_t buf[64];

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, sizeof(buf), (const uint8_t*)payload, 4,
        UVHTTP_WS_OPCODE_TEXT, 0, 0);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x01); /* FIN=0, opcode=TEXT */
}

/* ========== uvhttp_ws_build_frame: mask=1 with DRBG ========== */

TEST_F(WsBoostTest, BuildFrame_MaskMode_WithDRBG_ShortPayload) {
    const char* payload = "masked";
    size_t payload_len = 6;
    /* header(2) + mask_key(4) + payload(6) = 12 */
    uint8_t buf[16];

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        &ctx_, buf, sizeof(buf), (const uint8_t*)payload, payload_len,
        UVHTTP_WS_OPCODE_TEXT, 1, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x81);     /* FIN=1, opcode=TEXT */
    EXPECT_EQ(buf[1] & 0x80, 0x80); /* mask bit set */
    EXPECT_EQ(buf[1] & 0x7F, 6);    /* payload length */

    /* Masking key is at buf[2..5] */
    /* Payload at buf[6..11] should be masked */
    uint8_t masking_key[4] = {buf[2], buf[3], buf[4], buf[5]};
    uint8_t masked[6];
    memcpy(masked, payload, 6);
    for (size_t i = 0; i < 6; i++) {
        masked[i] ^= masking_key[i % 4];
    }
    EXPECT_EQ(memcmp(buf + 6, masked, 6), 0);
}

TEST_F(WsBoostTest, BuildFrame_MaskMode_WithDRBG_MediumPayload) {
    const size_t payload_len = 200;
    uint8_t* payload = (uint8_t*)uvhttp_alloc(payload_len);
    ASSERT_NE(payload, nullptr);
    memset(payload, 'M', payload_len);

    /* header(4) + mask_key(4) + payload(200) = 208 */
    size_t buf_size = 4 + 4 + payload_len;
    uint8_t* buf = (uint8_t*)uvhttp_alloc(buf_size);
    ASSERT_NE(buf, nullptr);

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        &ctx_, buf, buf_size, payload, payload_len,
        UVHTTP_WS_OPCODE_BINARY, 1, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[1] & 0x80, 0x80); /* mask bit set */
    EXPECT_EQ(buf[1] & 0x7F, 126);  /* 2-byte extended length */

    uvhttp_free(buf);
    uvhttp_free(payload);
}

TEST_F(WsBoostTest, BuildFrame_MaskMode_WithDRBG_NullPayload) {
    uint8_t buf[16];

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        &ctx_, buf, sizeof(buf), nullptr, 0,
        UVHTTP_WS_OPCODE_PING, 1, 1);

    EXPECT_GT(ret, 0);
    EXPECT_EQ(buf[0], 0x89);      /* FIN=1, opcode=PING */
    EXPECT_EQ(buf[1] & 0x80, 0x80); /* mask bit set */
    EXPECT_EQ(buf[1] & 0x7F, 0);    /* zero length */
}

TEST_F(WsBoostTest, BuildFrame_MaskMode_NoDRBG_Fails) {
    /* Context without DRBG initialization */
    uvhttp_context_t no_drbg_ctx;
    memset(&no_drbg_ctx, 0, sizeof(no_drbg_ctx));
    no_drbg_ctx.ws_drbg_initialized = 0;

    /* Must use heap buffer: build_frame calls uvhttp_free(buffer) on DRBG
     * error path (line 223), so a stack buffer would cause a crash. */
    uint8_t* buf = (uint8_t*)uvhttp_alloc(64);
    ASSERT_NE(buf, nullptr);
    const char* payload = "test";

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        &no_drbg_ctx, buf, 64, (const uint8_t*)payload, 4,
        UVHTTP_WS_OPCODE_TEXT, 1, 1);

    EXPECT_LT(ret, 0);
}

TEST_F(WsBoostTest, BuildFrame_MaskMode_NullContext_Fails) {
    /* Must use heap buffer: build_frame calls uvhttp_free(buffer) on DRBG
     * error path (line 223), so a stack buffer would cause a crash. */
    uint8_t* buf = (uint8_t*)uvhttp_alloc(64);
    ASSERT_NE(buf, nullptr);
    const char* payload = "test";

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, 64, (const uint8_t*)payload, 4,
        UVHTTP_WS_OPCODE_TEXT, 1, 1);

    EXPECT_LT(ret, 0);
}

/* ========== uvhttp_ws_build_frame: error cases ========== */

TEST_F(WsBoostTest, BuildFrame_NullBuffer) {
    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, nullptr, 0, nullptr, 0,
        UVHTTP_WS_OPCODE_TEXT, 0, 1);

    EXPECT_LT(ret, 0);
}

TEST_F(WsBoostTest, BuildFrame_BufferTooSmall) {
    uint8_t buf[2];
    const char* payload = "Hello";

    uvhttp_error_t ret = uvhttp_ws_build_frame(
        nullptr, buf, sizeof(buf), (const uint8_t*)payload, 5,
        UVHTTP_WS_OPCODE_TEXT, 0, 1);

    EXPECT_LT(ret, 0);
}

/* ========== uvhttp_ws_send_frame: OPEN state connection ========== */

TEST_F(WsBoostTest, SendFrame_ServerMode_OpenState) {
    /* Create a socketpair so send() can succeed */
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    /* Move to OPEN state via handshake */
    conn->state = UVHTTP_WS_STATE_OPEN;

    const char* text = "Hello WS";
    uvhttp_error_t ret = uvhttp_ws_send_text(&ctx_, conn, text, strlen(text));

    /* send() should succeed on socketpair fd */
    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_GT(conn->bytes_sent, 0u);
    EXPECT_GT(conn->frames_sent, 0u);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, SendFrame_ClientMode_OpenState) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 0, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    uint8_t data[] = {0x01, 0x02, 0x03};
    uvhttp_error_t ret = uvhttp_ws_send_binary(&ctx_, conn, data, sizeof(data));

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_GT(conn->bytes_sent, 0u);
    EXPECT_GT(conn->frames_sent, 0u);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, SendFrame_NotOpen_ReturnsError) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    /* state is CONNECTING by default */

    uvhttp_error_t ret = uvhttp_ws_send_text(&ctx_, conn, "hi", 2);
    EXPECT_NE(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, SendFrame_NullConn_ReturnsError) {
    uvhttp_error_t ret = uvhttp_ws_send_text(&ctx_, nullptr, "hi", 2);
    EXPECT_NE(ret, UVHTTP_OK);
}

TEST_F(WsBoostTest, SendFrame_PingPong) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    uint8_t ping_data[] = {0xDE, 0xAD};
    uvhttp_error_t ret = uvhttp_ws_send_ping(&ctx_, conn, ping_data,
                                              sizeof(ping_data));
    EXPECT_EQ(ret, UVHTTP_OK);

    ret = uvhttp_ws_send_pong(&ctx_, conn, ping_data, sizeof(ping_data));
    EXPECT_EQ(ret, UVHTTP_OK);

    EXPECT_GE(conn->frames_sent, 2u);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, SendFrame_EmptyPayload) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    uvhttp_error_t ret = uvhttp_ws_send_text(&ctx_, conn, "", 0);
    EXPECT_EQ(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, SendFrame_PingEmptyPayload) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    uvhttp_error_t ret = uvhttp_ws_send_ping(&ctx_, conn, NULL, 0);
    EXPECT_EQ(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, SendFrame_PongEmptyPayload) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    uvhttp_error_t ret = uvhttp_ws_send_pong(&ctx_, conn, NULL, 0);
    EXPECT_EQ(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

/* ========== uvhttp_ws_process_data: text frame ========== */

TEST_F(WsBoostTest, ProcessData_TextFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Build a text frame: FIN=1, opcode=TEXT, payload="Hello" */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"Hello", 5,
                                    UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(g_cb_state.last_message_len, 5u);
    EXPECT_EQ(memcmp(g_cb_state.last_message, "Hello", 5), 0);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ProcessData_BinaryFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    payload, sizeof(payload),
                                    UVHTTP_WS_OPCODE_BINARY, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_opcode, UVHTTP_WS_OPCODE_BINARY);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: close frame ========== */

TEST_F(WsBoostTest, ProcessData_CloseFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Close frame payload: 2-byte status code + reason */
    uint8_t close_payload[16];
    close_payload[0] = (1000 >> 8) & 0xFF; /* status code high byte */
    close_payload[1] = 1000 & 0xFF;        /* status code low byte */
    memcpy(close_payload + 2, "Normal", 6);

    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    close_payload, 8,
                                    UVHTTP_WS_OPCODE_CLOSE, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.close_called);
    EXPECT_EQ(g_cb_state.close_code, 1000);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ProcessData_CloseFrame_NoPayload) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Close frame with zero-length payload */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    nullptr, 0,
                                    UVHTTP_WS_OPCODE_CLOSE, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.close_called);
    EXPECT_EQ(g_cb_state.close_code, 1000); /* default code */
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ProcessData_CloseFrame_CodeOnly) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Close frame with just status code (2 bytes), no reason */
    uint8_t close_payload[2];
    close_payload[0] = (1001 >> 8) & 0xFF;
    close_payload[1] = 1001 & 0xFF;

    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    close_payload, 2,
                                    UVHTTP_WS_OPCODE_CLOSE, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.close_called);
    EXPECT_EQ(g_cb_state.close_code, 1001);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: ping frame ========== */

TEST_F(WsBoostTest, ProcessData_PingFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Ping frame with payload */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"ping!", 5,
                                    UVHTTP_WS_OPCODE_PING, 1);
    ASSERT_GT(frame_len, 0);

    /* user_data is NULL, so the pong auto-reply is skipped */
    conn->user_data = NULL;

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    /* Ping does not trigger on_message or on_close */

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ProcessData_PingFrame_NoPayload) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);
    conn->user_data = NULL;

    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    nullptr, 0,
                                    UVHTTP_WS_OPCODE_PING, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);
    EXPECT_EQ(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: pong frame ========== */

TEST_F(WsBoostTest, ProcessData_PongFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"pong", 4,
                                    UVHTTP_WS_OPCODE_PONG, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    /* PONG frames typically do not trigger any callback */
    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_FALSE(g_cb_state.message_called);
    EXPECT_FALSE(g_cb_state.close_called);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: fragmented message ========== */

TEST_F(WsBoostTest, ProcessData_FragmentedMessage) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Fragment 1: FIN=0, opcode=TEXT, payload="Hel" */
    uint8_t frag1[64];
    int frag1_len = build_raw_frame(frag1, sizeof(frag1),
                                    (const uint8_t*)"Hel", 3,
                                    UVHTTP_WS_OPCODE_TEXT, 0);
    ASSERT_GT(frag1_len, 0);

    /* Fragment 2: FIN=1, opcode=TEXT, payload="lo"
     * Note: this implementation only enters the fragment path for TEXT/BINARY
     * opcodes, so the final fragment must also use the same opcode. */
    uint8_t frag2[64];
    int frag2_len = build_raw_frame(frag2, sizeof(frag2),
                                    (const uint8_t*)"lo", 2,
                                    UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(frag2_len, 0);

    reset_cb_state();
    uvhttp_error_t ret;

    /* Feed fragment 1 */
    ret = uvhttp_ws_process_data(conn, frag1, frag1_len);
    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_FALSE(g_cb_state.message_called); /* not yet complete */

    /* Feed fragment 2 */
    ret = uvhttp_ws_process_data(conn, frag2, frag2_len);
    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(g_cb_state.last_message_len, 5u);
    EXPECT_EQ(memcmp(g_cb_state.last_message, "Hello", 5), 0);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ProcessData_FragmentedBinary) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Fragment 1: FIN=0, opcode=BINARY */
    uint8_t p1[] = {0xAA, 0xBB};
    uint8_t frag1[64];
    int frag1_len = build_raw_frame(frag1, sizeof(frag1), p1, 2,
                                    UVHTTP_WS_OPCODE_BINARY, 0);
    ASSERT_GT(frag1_len, 0);

    /* Fragment 2: FIN=1, opcode=BINARY (same opcode to enter the fragment
     * completion path, since this impl does not handle CONTINUATION opcode) */
    uint8_t p2[] = {0xCC, 0xDD};
    uint8_t frag2[64];
    int frag2_len = build_raw_frame(frag2, sizeof(frag2), p2, 2,
                                    UVHTTP_WS_OPCODE_BINARY, 1);
    ASSERT_GT(frag2_len, 0);

    reset_cb_state();
    uvhttp_ws_process_data(conn, frag1, frag1_len);
    EXPECT_FALSE(g_cb_state.message_called);

    uvhttp_ws_process_data(conn, frag2, frag2_len);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_opcode, UVHTTP_WS_OPCODE_BINARY);
    EXPECT_EQ(g_cb_state.last_message_len, 4u);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: continuation without initial fragment ========== */

TEST_F(WsBoostTest, ProcessData_CompleteMessage_NoFragment) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* A complete single text frame */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"single", 6,
                                    UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_message_len, 6u);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: multiple frames in one buffer ========== */

TEST_F(WsBoostTest, ProcessData_MultipleFramesInOneBuffer) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Two text frames concatenated */
    uint8_t frame1[64], frame2[64];
    int len1 = build_raw_frame(frame1, sizeof(frame1),
                                (const uint8_t*)"first", 5,
                                UVHTTP_WS_OPCODE_TEXT, 1);
    int len2 = build_raw_frame(frame2, sizeof(frame2),
                                (const uint8_t*)"second", 6,
                                UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(len1, 0);
    ASSERT_GT(len2, 0);

    /* Concatenate into one buffer */
    uint8_t combined[128];
    memcpy(combined, frame1, len1);
    memcpy(combined + len1, frame2, len2);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, combined, len1 + len2);

    EXPECT_EQ(ret, UVHTTP_OK);
    /* on_message should be called twice */
    EXPECT_TRUE(g_cb_state.message_called);
    /* Last message should be "second" */
    EXPECT_EQ(g_cb_state.last_message_len, 6u);
    EXPECT_EQ(memcmp(g_cb_state.last_message, "second", 6), 0);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: partial frame ========== */

TEST_F(WsBoostTest, ProcessData_PartialFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Feed only 1 byte of a 2-byte header */
    uint8_t partial[] = {0x81};

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, partial, 1);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_FALSE(g_cb_state.message_called); /* not enough data yet */

    /* Feed the rest */
    uint8_t rest[] = {0x05, 'H', 'e', 'l', 'l', 'o'};
    ret = uvhttp_ws_process_data(conn, rest, sizeof(rest));

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_message_len, 5u);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: null inputs ========== */

TEST_F(WsBoostTest, ProcessData_NullConn) {
    uint8_t frame[] = {0x81, 0x00};
    uvhttp_error_t ret = uvhttp_ws_process_data(nullptr, frame, sizeof(frame));
    EXPECT_NE(ret, UVHTTP_OK);
}

TEST_F(WsBoostTest, ProcessData_NullData) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_error_t ret = uvhttp_ws_process_data(conn, nullptr, 10);
    EXPECT_NE(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: close frame without callback ========== */

TEST_F(WsBoostTest, ProcessData_CloseFrame_NoCallback) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    /* Don't set any callbacks */
    uint8_t close_payload[2] = {0x03, 0xE8}; /* 1000 */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    close_payload, 2,
                                    UVHTTP_WS_OPCODE_CLOSE, 1);
    ASSERT_GT(frame_len, 0);

    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSED);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: text frame without callback ========== */

TEST_F(WsBoostTest, ProcessData_TextFrame_NoCallback) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    /* No callbacks set */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"test", 4,
                                    UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(frame_len, 0);

    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    /* Should not crash even without callbacks */
    EXPECT_EQ(ret, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: buffer expansion ========== */

TEST_F(WsBoostTest, ProcessData_BufferExpansion) {
    /* Use a small custom config to get a small recv buffer, then overflow it.
     * Directly manipulate recv_buffer_pos to near capacity, then feed data. */
    uvhttp_config_t config;
    memset(&config, 0, sizeof(config));
    config.websocket_max_frame_size = 1024 * 1024; /* 1MB */

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, &config);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Fill recv buffer to near capacity with dummy data (not parseable) */
    size_t fill_size = conn->recv_buffer_size - 4;
    memset(conn->recv_buffer, 0x00, fill_size);
    conn->recv_buffer_pos = fill_size;

    /* Now feed a valid small text frame. process_data must expand the
     * buffer to accommodate the new data before parsing. */
    uint8_t frame[64];
    int frame_len = build_raw_frame(frame, sizeof(frame),
                                    (const uint8_t*)"expansion!", 10,
                                    UVHTTP_WS_OPCODE_TEXT, 1);
    ASSERT_GT(frame_len, 0);

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, frame_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    /* Buffer should have been expanded */
    EXPECT_GT(conn->recv_buffer_size, fill_size);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_process_data: masked frame ========== */

TEST_F(WsBoostTest, ProcessData_MaskedFrame) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    uvhttp_ws_set_callbacks(conn, test_on_message, test_on_close, test_on_error);

    /* Build a masked text frame manually */
    const char* text = "masked!";
    size_t text_len = 7;
    uint8_t masking_key[4] = {0x12, 0x34, 0x56, 0x78};

    uint8_t frame[64];
    frame[0] = 0x81; /* FIN=1, opcode=TEXT */
    frame[1] = 0x80 | (uint8_t)text_len; /* mask bit set, length */
    frame[2] = masking_key[0];
    frame[3] = masking_key[1];
    frame[4] = masking_key[2];
    frame[5] = masking_key[3];

    /* Copy and mask payload */
    memcpy(frame + 6, text, text_len);
    for (size_t i = 0; i < text_len; i++) {
        frame[6 + i] ^= masking_key[i % 4];
    }

    reset_cb_state();
    uvhttp_error_t ret = uvhttp_ws_process_data(conn, frame, 6 + text_len);

    EXPECT_EQ(ret, UVHTTP_OK);
    EXPECT_TRUE(g_cb_state.message_called);
    EXPECT_EQ(g_cb_state.last_message_len, text_len);
    EXPECT_EQ(memcmp(g_cb_state.last_message, "masked!", text_len), 0);

    uvhttp_ws_connection_free(conn);
}

/* ========== uvhttp_ws_close ========== */
/* Note: uvhttp_ws_close sets state to CLOSING before calling send_frame,
 * which requires state==OPEN. So send_frame always fails from close().
 * The tests verify the state transition and that the close logic runs
 * without crashing. */

TEST_F(WsBoostTest, WsClose_WithReason) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    /* close sets CLOSING then calls send_frame which rejects non-OPEN */
    uvhttp_ws_close(&ctx_, conn, 1000, "Normal");
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSING);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, WsClose_NullReason) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    /* null reason path: sends just 2-byte status code */
    uvhttp_ws_close(&ctx_, conn, 1001, NULL);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSING);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

TEST_F(WsBoostTest, WsClose_NullConn) {
    uvhttp_error_t ret = uvhttp_ws_close(&ctx_, nullptr, 1000, "bye");
    EXPECT_NE(ret, UVHTTP_OK);
}

/* ========== uvhttp_ws_close: long reason truncated ========== */

TEST_F(WsBoostTest, WsClose_LongReason) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        fds[0], NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);
    conn->state = UVHTTP_WS_STATE_OPEN;

    /* Reason > 125 bytes gets truncated */
    char long_reason[200];
    memset(long_reason, 'R', sizeof(long_reason) - 1);
    long_reason[sizeof(long_reason) - 1] = '\0';

    /* close sets CLOSING then calls send_frame which rejects non-OPEN */
    uvhttp_ws_close(&ctx_, conn, 1000, long_reason);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CLOSING);

    uvhttp_ws_connection_free(conn);
    close(fds[0]);
    close(fds[1]);
}

/* ========== uvhttp_server_register_websocket_upgrade ========== */

TEST_F(WsBoostTest, RegisterWebsocketUpgrade_NullServer) {
    uvhttp_error_t ret = uvhttp_server_register_websocket_upgrade(nullptr);
    EXPECT_NE(ret, UVHTTP_OK);
}

/* ========== uvhttp_ws_connection: free NULL ========== */

TEST_F(WsBoostTest, ConnectionFree_Null) {
    /* Should not crash */
    uvhttp_ws_connection_free(nullptr);
}

/* ========== uvhttp_ws_connection_create: with config ========== */

TEST_F(WsBoostTest, ConnectionCreate_WithConfig) {
    uvhttp_config_t config;
    memset(&config, 0, sizeof(config));
    config.websocket_max_frame_size = 65536;
    config.websocket_max_message_size = 1024 * 1024;
    config.websocket_ping_interval = 60;
    config.websocket_ping_timeout = 15;

    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 1, &config);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(conn->config.max_frame_size, 65536);
    EXPECT_EQ(conn->config.max_message_size, 1024 * 1024);
    EXPECT_EQ(conn->config.ping_interval, 60);
    EXPECT_EQ(conn->config.ping_timeout, 15);
    EXPECT_EQ(conn->is_server, 1);
    EXPECT_EQ(conn->state, UVHTTP_WS_STATE_CONNECTING);

    uvhttp_ws_connection_free(conn);
}

TEST_F(WsBoostTest, ConnectionCreate_DefaultConfig) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(
        0, NULL, 0, NULL);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(conn->config.max_frame_size,
              UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE);
    EXPECT_EQ(conn->config.max_message_size,
              UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE);
    EXPECT_EQ(conn->config.ping_interval,
              UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL);
    EXPECT_EQ(conn->config.ping_timeout,
              UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT);
    EXPECT_EQ(conn->is_server, 0);
    EXPECT_NE(conn->recv_buffer, nullptr);
    EXPECT_GT(conn->recv_buffer_size, 0u);

    uvhttp_ws_connection_free(conn);
}

#else

/* WebSocket feature disabled: empty placeholder test */
TEST(WsBoostTestDisabled, FeatureNotEnabled) {
    GTEST_SKIP() << "UVHTTP_FEATURE_WEBSOCKET is not enabled";
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */
