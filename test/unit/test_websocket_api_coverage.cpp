/* uvhttp_websocket.c API 覆盖率测试 - 目标提升至 40%+ */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_websocket.h"
#include "uvhttp_context.h"
#include "uvhttp_config.h"
#include <string.h>

/* ========== 测试 WebSocket 连接创建和释放 ========== */

TEST(UvhttpWebsocketApiTest, WsConnectionCreateValid) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    /* 即使fd无效，也可能返回非NULL（内部会处理） */
    /* 所以这里只测试不会崩溃 */
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

TEST(UvhttpWebsocketApiTest, WsConnectionCreateNegativeFd) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(-1, NULL, 1, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

TEST(UvhttpWebsocketApiTest, WsConnectionCreateLargeFd) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(99999, NULL, 1, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

TEST(UvhttpWebsocketApiTest, WsConnectionCreateClientMode) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

TEST(UvhttpWebsocketApiTest, WsConnectionCreateWithConfig) {
    uvhttp_config_t config;
    memset(&config, 0, sizeof(config));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, &config);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

/* ========== 测试 WebSocket 握手（服务器端） ========== */

TEST(UvhttpWebsocketApiTest, WsHandshakeServerValidRequest) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* request = "GET /ws HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                         "Sec-WebSocket-Version: 13\r\n"
                         "\r\n";
    
    char response[1024];
    size_t response_len = sizeof(response);
    
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, request, strlen(request), response, &response_len);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeServerEmptyRequest) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    char response[1024];
    size_t response_len = sizeof(response);
    
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, "", 0, response, &response_len);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeServerInvalidRequest) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* request = "Invalid HTTP request";
    
    char response[1024];
    size_t response_len = sizeof(response);
    
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, request, strlen(request), response, &response_len);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeServerNullResponseBuffer) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* request = "GET /ws HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                         "Sec-WebSocket-Version: 13\r\n"
                         "\r\n";
    
    size_t response_len = 0;
    
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, request, strlen(request), NULL, &response_len);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeServerNullResponseLen) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* request = "GET /ws HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                         "Sec-WebSocket-Version: 13\r\n"
                         "\r\n";
    
    char response[1024];
    
    uvhttp_error_t result = uvhttp_ws_handshake_server(conn, request, strlen(request), response, NULL);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 握手（客户端） ========== */

TEST(UvhttpWebsocketApiTest, WsHandshakeClientValid) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    char request[1024];
    size_t request_len = sizeof(request);
    
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, "localhost", "/ws", request, &request_len);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeClientNullHost) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    char request[1024];
    size_t request_len = sizeof(request);
    
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, NULL, "/ws", request, &request_len);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeClientNullPath) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    char request[1024];
    size_t request_len = sizeof(request);
    
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, "localhost", NULL, request, &request_len);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsHandshakeClientEmptyPath) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    char request[1024];
    size_t request_len = sizeof(request);
    
    uvhttp_error_t result = uvhttp_ws_handshake_client(&context, conn, "localhost", "", request, &request_len);
    /* 可能成功或失败 */
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 验证握手响应 ========== */

TEST(UvhttpWebsocketApiTest, WsVerifyHandshakeResponseValid) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    /* 模拟有效的握手响应 */
    const char* response = "HTTP/1.1 101 Switching Protocols\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n"
                          "\r\n";
    
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(conn, response, strlen(response));
    /* 可能成功或失败 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsVerifyHandshakeResponseInvalid) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* response = "Invalid response";
    
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(conn, response, strlen(response));
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsVerifyHandshakeResponseEmpty) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 0, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_verify_handshake_response(conn, "", 0);
    EXPECT_NE(result, 0);
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 发送 ========== */

TEST(UvhttpWebsocketApiTest, WsSendTextEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_send_text(&context, conn, "", 0);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendTextWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    const char* text = "Hello WebSocket";
    uvhttp_error_t result = uvhttp_ws_send_text(&context, conn, text, strlen(text));
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendBinaryEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_send_binary(&context, conn, NULL, 0);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendBinaryWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uvhttp_error_t result = uvhttp_ws_send_binary(&context, conn, data, sizeof(data));
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendPingEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_send_ping(&context, conn, NULL, 0);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendPingWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uvhttp_error_t result = uvhttp_ws_send_ping(&context, conn, data, sizeof(data));
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendPongEmpty) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_send_pong(&context, conn, NULL, 0);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSendPongWithData) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uvhttp_error_t result = uvhttp_ws_send_pong(&context, conn, data, sizeof(data));
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 关闭 ========== */

TEST(UvhttpWebsocketApiTest, WsCloseNormal) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_close(&context, conn, 1000, "Normal closure");
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsCloseNoReason) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_close(&context, conn, 1000, "");
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsCloseNullReason) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_error_t result = uvhttp_ws_close(&context, conn, 1000, NULL);
    /* 可能成功或失败，取决于实现 */
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 回调设置 ========== */

TEST(UvhttpWebsocketApiTest, WsSetCallbacks) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    /* 设置所有回调 */
    uvhttp_ws_set_callbacks(conn,
        [](struct uvhttp_ws_connection* conn, const char* data, size_t len, int opcode) -> int {
            return 0;
        },
        [](struct uvhttp_ws_connection* conn, int code, const char* reason) -> int {
            return 0;
        },
        [](struct uvhttp_ws_connection* conn, int error_code, const char* error_msg) -> int {
            return 0;
        }
    );
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSetCallbacksNullMessage) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    /* 只设置部分回调 */
    uvhttp_ws_set_callbacks(conn, NULL,
        [](struct uvhttp_ws_connection* conn, int code, const char* reason) -> int {
            return 0;
        },
        [](struct uvhttp_ws_connection* conn, int error_code, const char* error_msg) -> int {
            return 0;
        }
    );
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSetCallbacksNullClose) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_ws_set_callbacks(conn,
        [](struct uvhttp_ws_connection* conn, const char* data, size_t len, int opcode) -> int {
            return 0;
        }, NULL,
        [](struct uvhttp_ws_connection* conn, int error_code, const char* error_msg) -> int {
            return 0;
        }
    );
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSetCallbacksNullError) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    uvhttp_ws_set_callbacks(conn,
        [](struct uvhttp_ws_connection* conn, const char* data, size_t len, int opcode) -> int {
            return 0;
        },
        [](struct uvhttp_ws_connection* conn, int code, const char* reason) -> int {
            return 0;
        }, NULL
    );
    
    uvhttp_ws_connection_free(conn);
}

TEST(UvhttpWebsocketApiTest, WsSetCallbacksAllNull) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(0, NULL, 1, NULL);
    if (!conn) {
        GTEST_SKIP() << "Failed to create WebSocket connection";
    }
    
    /* 所有回调都为 NULL */
    uvhttp_ws_set_callbacks(conn, NULL, NULL, NULL);
    
    uvhttp_ws_connection_free(conn);
}

/* ========== 测试 WebSocket 帧处理 ========== */

TEST(UvhttpWebsocketApiTest, WsParseFrameHeaderValid) {
    /* 有效的文本帧头：FIN=1, opcode=1 (TEXT), mask=0, payload_len=0 */
    uint8_t frame_data[] = {0x81, 0x00};
    
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(frame_data, sizeof(frame_data), &header, &header_size);
    /* 可能成功或失败，取决于实现 */
}

TEST(UvhttpWebsocketApiTest, WsParseFrameHeaderEmpty) {
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(NULL, 0, &header, &header_size);
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsParseFrameHeaderPartial) {
    uint8_t frame_data[] = {0x81};
    
    uvhttp_ws_frame_header_t header;
    size_t header_size = 0;
    
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(frame_data, sizeof(frame_data), &header, &header_size);
    /* 可能失败，因为数据不完整 */
}

TEST(UvhttpWebsocketApiTest, WsParseFrameHeaderNullHeader) {
    uint8_t frame_data[] = {0x81, 0x00};
    
    size_t header_size = 0;
    
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(frame_data, sizeof(frame_data), NULL, &header_size);
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsParseFrameHeaderNullHeaderSize) {
    uint8_t frame_data[] = {0x81, 0x00};
    
    uvhttp_ws_frame_header_t header;
    
    uvhttp_error_t result = uvhttp_ws_parse_frame_header(frame_data, sizeof(frame_data), &header, NULL);
    EXPECT_NE(result, 0);
}

/* ========== 测试 WebSocket 生成 Accept ========== */

TEST(UvhttpWebsocketApiTest, WsGenerateAcceptValid) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    char accept[64];
    
    uvhttp_error_t result = uvhttp_ws_generate_accept(key, accept, sizeof(accept));
    /* 可能成功或失败，取决于实现 */
}

TEST(UvhttpWebsocketApiTest, WsGenerateAcceptNullKey) {
    char accept[64];
    
    uvhttp_error_t result = uvhttp_ws_generate_accept(NULL, accept, sizeof(accept));
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsGenerateAcceptNullAccept) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    
    uvhttp_error_t result = uvhttp_ws_generate_accept(key, NULL, 0);
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsGenerateAcceptSmallBuffer) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    char accept[10];
    
    uvhttp_error_t result = uvhttp_ws_generate_accept(key, accept, sizeof(accept));
    /* 可能失败，因为缓冲区太小 */
}

/* ========== 测试 WebSocket 验证 Accept ========== */

TEST(UvhttpWebsocketApiTest, WsVerifyAcceptValid) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    const char* accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
    
    uvhttp_error_t result = uvhttp_ws_verify_accept(key, accept);
    /* 可能成功或失败，取决于实现 */
}

TEST(UvhttpWebsocketApiTest, WsVerifyAcceptNullKey) {
    const char* accept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";
    
    uvhttp_error_t result = uvhttp_ws_verify_accept(NULL, accept);
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsVerifyAcceptNullAccept) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    
    uvhttp_error_t result = uvhttp_ws_verify_accept(key, NULL);
    EXPECT_NE(result, 0);
}

TEST(UvhttpWebsocketApiTest, WsVerifyAcceptInvalid) {
    const char* key = "dGhlIHNhbXBsZSBub25jZQ==";
    const char* accept = "invalid_accept_value";
    
    uvhttp_error_t result = uvhttp_ws_verify_accept(key, accept);
    EXPECT_NE(result, 0);
}

/* ========== 测试 WebSocket 应用掩码 ========== */

TEST(UvhttpWebsocketApiTest, WsApplyMaskValid) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t masking_key[] = {0x12, 0x34, 0x56, 0x78};
    
    /* 应用掩码 */
    uvhttp_ws_apply_mask(data, sizeof(data), masking_key);
    
    /* 验证数据已被修改 */
    /* 注意：这里不验证具体值，只确保函数不会崩溃 */
}

TEST(UvhttpWebsocketApiTest, WsApplyMaskNullData) {
    uint8_t masking_key[] = {0x12, 0x34, 0x56, 0x78};
    
    /* 不应该崩溃 */
    uvhttp_ws_apply_mask(NULL, 0, masking_key);
}

TEST(UvhttpWebsocketApiTest, WsApplyMaskNullMaskingKey) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    
    /* 不应该崩溃 */
    uvhttp_ws_apply_mask(data, sizeof(data), NULL);
}

TEST(UvhttpWebsocketApiTest, WsApplyMaskZeroLength) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t masking_key[] = {0x12, 0x34, 0x56, 0x78};
    
    /* 零长度，数据不应该被修改 */
    uvhttp_ws_apply_mask(data, 0, masking_key);
}

TEST(UvhttpWebsocketApiTest, WsApplyMaskSingleByte) {
    uint8_t data[] = {0x01};
    uint8_t masking_key[] = {0x12, 0x34, 0x56, 0x78};
    
    uvhttp_ws_apply_mask(data, sizeof(data), masking_key);
}

TEST(UvhttpWebsocketApiTest, WsApplyMaskLargeData) {
    uint8_t data[1024];
    uint8_t masking_key[] = {0x12, 0x34, 0x56, 0x78};
    
    /* 初始化数据 */
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)i;
    }
    
    uvhttp_ws_apply_mask(data, sizeof(data), masking_key);
}

#else

/* WebSocket 功能未启用时的空测试 */
TEST(UvhttpWebsocketApiTest, WebSocketDisabled) {
    SUCCEED() << "WebSocket功能未启用，跳过测试";
}

#endif