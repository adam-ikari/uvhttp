/**
 * @file test_websocket_frame_fixes.cpp
 * @brief Regression tests for WebSocket extended payload length handling.
 *
 * Covers two bugs found via real-client load testing (keep-alive + large
 * frames):
 *
 * 1. uvhttp_ws_parse_frame_header wrote extended lengths (126/127) into the
 *    7-bit payload_len bitfield, silently truncating them (200 -> 72). The
 *    actual length must go into payload_length; payload_len keeps the raw
 *    length code from the wire (0-125, 126, 127).
 *
 * 2. uvhttp_ws_build_frame computed total_size with header_size=2 before the
 *    extended-length branch, so frames with 126/127 codes were reported 2
 *    bytes short and senders transmitted a truncated frame (clients stalled
 *    waiting for the missing tail).
 */

#include <gtest/gtest.h>
#include <uvhttp.h>

#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

TEST(WsFrameHeaderParse, SmallPayloadLength) {
    uint8_t f[8] = {0x81, 0x85, 0, 0, 0, 0, 'a', 'b'}; /* masked, 5 bytes */
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 8, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)2);
    EXPECT_EQ(h.payload_length, (uint64_t)5);
    EXPECT_EQ(h.payload_len, (uint8_t)5);
    EXPECT_EQ(h.fin, (uint8_t)1);
    EXPECT_EQ(h.opcode, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_EQ(h.mask, (uint8_t)1);
}

TEST(WsFrameHeaderParse, Extended16BitPayloadLength) {
    /* 200-byte payload: 0x81 0xFE 00 C8 <mask> <payload...> */
    uint8_t f[8] = {0x81, 0xFE, 0x00, 0xC8, 0, 0, 0, 0};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 8, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)4);
    EXPECT_EQ(h.payload_length, (uint64_t)200); /* was truncated to 72 */
    EXPECT_EQ(h.payload_len, (uint8_t)126);     /* raw length code kept */
    EXPECT_EQ(h.opcode, UVHTTP_WS_OPCODE_TEXT);
}

TEST(WsFrameHeaderParse, Extended16BitMaxLength) {
    uint8_t f[10] = {0x81, 0xFE, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0}; /* 65535 */
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 10, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)4);
    EXPECT_EQ(h.payload_length, (uint64_t)65535);
    EXPECT_EQ(h.payload_len, (uint8_t)126);
}

TEST(WsFrameHeaderParse, Extended64BitPayloadLength) {
    /* 70000 bytes: 0x0000000000011170 */
    uint8_t f[16] = {0x81, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                     0x11, 0x70, 0, 0, 0, 0, 0, 0};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 16, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)10);
    EXPECT_EQ(h.payload_length, (uint64_t)70000);
    EXPECT_EQ(h.payload_len, (uint8_t)127); /* raw code kept */
    EXPECT_EQ(h.opcode, UVHTTP_WS_OPCODE_TEXT);
}

TEST(WsFrameHeaderParse, Extended64BitMaxLength) {
    /* length code 127 declaring 2^64-1: RFC 6455 §5.2 requires the most
     * significant bit of a 64-bit length to be 0, so this must now be
     * rejected (review M3). */
    uint8_t f[16] = {0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                     0xFF, 0xFF, 0, 0, 0, 0, 0, 0};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    EXPECT_NE(uvhttp_ws_parse_frame_header(f, 16, &h, &hsize), UVHTTP_OK);
}

TEST(WsFrameHeaderParse, Extended64BitMaxLegalLength) {
    /* 2^63-1 is the largest legal 64-bit length (MSB must be 0). */
    uint8_t f[16] = {0x82, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                     0xFF, 0xFF, 0, 0, 0, 0, 0, 0};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 16, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)10);
    EXPECT_EQ(h.payload_length, (uint64_t)INT64_MAX);
    EXPECT_EQ(h.payload_len, (uint8_t)127);
}

TEST(WsFrameHeaderParse, TooShortForExtendedHeader) {
    /* len < 4 cannot be a valid 126 frame */
    uint8_t f[3] = {0x81, 0xFE, 0x00};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    EXPECT_NE(uvhttp_ws_parse_frame_header(f, 3, &h, &hsize), UVHTTP_OK);
}

TEST(WsBuildFrame, SmallFrameTotalSizeAndBytes) {
    uint8_t buf[64];
    uint8_t payload[5] = {'h', 'e', 'l', 'l', 'o'};
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload, 5,
                                             UVHTTP_WS_OPCODE_TEXT, 0, 1);
    /* 2-byte header + 5 payload = 7 */
    ASSERT_EQ((int)r, 7);
    EXPECT_EQ(buf[0], (uint8_t)0x81);
    EXPECT_EQ(buf[1], (uint8_t)0x05);
    EXPECT_EQ(memcmp(buf + 2, payload, 5), 0);
}

TEST(WsBuildFrame, Extended16BitTotalSizeAndBytes) {
    uint8_t buf[256];
    uint8_t payload[200];
    memset(payload, 'x', sizeof(payload));
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             sizeof(payload),
                                             UVHTTP_WS_OPCODE_TEXT, 0, 1);
    /* 4-byte header + 200 payload = 204 (previously reported 202) */
    ASSERT_EQ((int)r, 204);
    EXPECT_EQ(buf[0], (uint8_t)0x81);
    EXPECT_EQ(buf[1], (uint8_t)0x7E); /* 126 length code, server (no mask) */
    EXPECT_EQ(buf[2], (uint8_t)0x00);
    EXPECT_EQ(buf[3], (uint8_t)0xC8);
    EXPECT_EQ(memcmp(buf + 4, payload, sizeof(payload)), 0);
}

TEST(WsBuildFrame, Extended16BitBoundary) {
    uint8_t buf[65536 + 16];
    uint8_t payload[65535];
    memset(payload, 'y', sizeof(payload));
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             sizeof(payload),
                                             UVHTTP_WS_OPCODE_BINARY, 0, 1);
    ASSERT_EQ((int)r, 4 + 65535);
    EXPECT_EQ(buf[0], (uint8_t)0x82);
    EXPECT_EQ(buf[1], (uint8_t)0x7E);
    EXPECT_EQ(buf[2], (uint8_t)0xFF);
    EXPECT_EQ(buf[3], (uint8_t)0xFF);
}

TEST(WsBuildFrame, Extended64BitTotalSizeAndBytes) {
    uint8_t buf[70016];
    uint8_t payload[70000];
    memset(payload, 'z', sizeof(payload));
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             sizeof(payload),
                                             UVHTTP_WS_OPCODE_BINARY, 0, 1);
    /* 10-byte header + 70000 payload */
    ASSERT_EQ((int)r, 10 + 70000);
    EXPECT_EQ(buf[0], (uint8_t)0x82);
    EXPECT_EQ(buf[1], (uint8_t)0x7F); /* 127 length code */
    EXPECT_EQ(buf[2], (uint8_t)0x00);
    EXPECT_EQ(buf[3], (uint8_t)0x00);
    EXPECT_EQ(buf[4], (uint8_t)0x00);
    EXPECT_EQ(buf[5], (uint8_t)0x00);
    EXPECT_EQ(buf[6], (uint8_t)0x00);
    EXPECT_EQ(buf[7], (uint8_t)0x01);
    EXPECT_EQ(buf[8], (uint8_t)0x11);
    EXPECT_EQ(buf[9], (uint8_t)0x70);
    EXPECT_EQ(memcmp(buf + 10, payload, sizeof(payload)), 0);
}

TEST(WsBuildFrame, InsufficientBufferRejected) {
    uint8_t buf[8]; /* too small for a 200-byte frame */
    uint8_t payload[200] = {0};
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             sizeof(payload),
                                             UVHTTP_WS_OPCODE_TEXT, 0, 1);
    EXPECT_NE(r, UVHTTP_OK);
}

/* ========== regression: 64-bit extended length overflow (CVE-class) ===== */
/* A 127-length-code frame declaring payload_length = 2^64-1 used to wrap
 * header_size + payload_length + 4 around size_t, pass the "enough data"
 * check with only the header buffered, and drive an out-of-bounds access in
 * uvhttp_ws_apply_mask. process_data must now reject it outright. */
TEST(WsFrameHeaderParse, ProcessDataRejectsHuge64BitLength) {
    uvhttp_ws_connection_t* conn =
        uvhttp_ws_connection_create(0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    /* FIN=1, TEXT, masked, 127 length code = 0xFFFFFFFFFFFFFFFF */
    uint8_t frame[14] = {
        0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00 /* masking key */
    };
    uvhttp_error_t r = uvhttp_ws_process_data(conn, frame, sizeof(frame));
    EXPECT_NE(r, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

/* A 127-length-code frame declaring more than max_frame_size must also be
 * rejected (not silently buffered forever). */
TEST(WsFrameHeaderParse, ProcessDataRejectsLengthOverMaxFrameSize) {
    uvhttp_ws_connection_t* conn =
        uvhttp_ws_connection_create(0, NULL, 1, NULL);
    ASSERT_NE(conn, nullptr);

    /* declared length = max_frame_size + 1 (default max is 16MB) */
    uint64_t huge = (uint64_t)conn->config.max_frame_size + 1;
    uint8_t frame[14] = {
        0x81, 0xFF,
        (uint8_t)((huge >> 56) & 0xFF), (uint8_t)((huge >> 48) & 0xFF),
        (uint8_t)((huge >> 40) & 0xFF), (uint8_t)((huge >> 32) & 0xFF),
        (uint8_t)((huge >> 24) & 0xFF), (uint8_t)((huge >> 16) & 0xFF),
        (uint8_t)((huge >> 8) & 0xFF),  (uint8_t)(huge & 0xFF),
        0x00, 0x00, 0x00, 0x00 /* masking key */
    };
    uvhttp_error_t r = uvhttp_ws_process_data(conn, frame, sizeof(frame));
    EXPECT_NE(r, UVHTTP_OK);

    uvhttp_ws_connection_free(conn);
}

/* ========== regression: recv_frame with extended payload lengths ========= */
/* recv_frame used to read only the first 2 header bytes before parsing, so
 * 126/127 frames failed with UVHTTP_ERROR_INVALID_PARAM before the extended
 * length was ever read. Exercise the full read path over a socketpair. */
TEST(WsRecvFrame, Extended16BitLength) {
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    uvhttp_ws_connection_t* conn =
        uvhttp_ws_connection_create(sv[0], NULL, 0, NULL);
    ASSERT_NE(conn, nullptr);

    uint8_t payload[200];
    memset(payload, 'a', sizeof(payload));
    uint8_t wire[256];
    int wire_len = uvhttp_ws_build_frame(NULL, wire, sizeof(wire), payload,
                                         sizeof(payload),
                                         UVHTTP_WS_OPCODE_BINARY, 0, 1);
    ASSERT_GT(wire_len, 0);
    ASSERT_EQ((int)write(sv[1], wire, (size_t)wire_len), wire_len);

    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    uvhttp_error_t r = uvhttp_ws_recv_frame(conn, &frame);
    EXPECT_EQ(r, UVHTTP_OK);
    EXPECT_EQ(frame.header.payload_length, (uint64_t)200);
    EXPECT_EQ(frame.payload_size, (size_t)200);
    if (frame.payload) {
        EXPECT_EQ(memcmp(frame.payload, payload, sizeof(payload)), 0);
        uvhttp_free(frame.payload);
    }

    uvhttp_ws_connection_free(conn);
    close(sv[0]);
    close(sv[1]);
}

TEST(WsRecvFrame, Extended64BitLength) {
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    uvhttp_ws_connection_t* conn =
        uvhttp_ws_connection_create(sv[0], NULL, 0, NULL);
    ASSERT_NE(conn, nullptr);

    uint8_t payload[70000];
    memset(payload, 'b', sizeof(payload));
    uint8_t wire[70016];
    int wire_len = uvhttp_ws_build_frame(NULL, wire, sizeof(wire), payload,
                                         sizeof(payload),
                                         UVHTTP_WS_OPCODE_BINARY, 0, 1);
    ASSERT_GT(wire_len, 0);
    ASSERT_EQ((int)write(sv[1], wire, (size_t)wire_len), wire_len);

    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    uvhttp_error_t r = uvhttp_ws_recv_frame(conn, &frame);
    EXPECT_EQ(r, UVHTTP_OK);
    EXPECT_EQ(frame.header.payload_length, (uint64_t)70000);
    if (frame.payload) {
        EXPECT_EQ(memcmp(frame.payload, payload, sizeof(payload)), 0);
        uvhttp_free(frame.payload);
    }

    uvhttp_ws_connection_free(conn);
    close(sv[0]);
    close(sv[1]);
}

/* build_frame must reject a payload length that would overflow total_size. */
TEST(WsBuildFrame, HugeLengthRejected) {
    uint8_t buf[32];
    uint8_t payload[8] = {0};
    (void)payload;
    long r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             SIZE_MAX,
                                             UVHTTP_WS_OPCODE_TEXT, 0, 1);
    EXPECT_NE(r, UVHTTP_OK);
}
