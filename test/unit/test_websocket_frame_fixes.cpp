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
    uint8_t f[16] = {0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                     0xFF, 0xFF, 0, 0, 0, 0, 0, 0};
    uvhttp_ws_frame_header_t h;
    size_t hsize = 0;
    ASSERT_EQ(uvhttp_ws_parse_frame_header(f, 16, &h, &hsize), UVHTTP_OK);
    EXPECT_EQ(hsize, (size_t)10);
    EXPECT_EQ(h.payload_length, (uint64_t)UINT64_MAX);
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
    uvhttp_error_t r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload, 5,
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
    uvhttp_error_t r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
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
    uvhttp_error_t r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
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
    uvhttp_error_t r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
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
    uvhttp_error_t r = uvhttp_ws_build_frame(NULL, buf, sizeof(buf), payload,
                                             sizeof(payload),
                                             UVHTTP_WS_OPCODE_TEXT, 0, 1);
    EXPECT_NE(r, UVHTTP_OK);
}
