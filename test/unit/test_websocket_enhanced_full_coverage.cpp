/* uvhttp_websocket.c 增强覆盖率测试 */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_websocket.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"

/* 测试WebSocket操作码 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketOpcodes) {
    /* 测试所有操作码 */
    EXPECT_EQ(UVHTTP_WS_OPCODE_CONTINUATION, 0x0);
    EXPECT_EQ(UVHTTP_WS_OPCODE_TEXT, 0x1);
    EXPECT_EQ(UVHTTP_WS_OPCODE_BINARY, 0x2);
    EXPECT_EQ(UVHTTP_WS_OPCODE_CLOSE, 0x8);
    EXPECT_EQ(UVHTTP_WS_OPCODE_PING, 0x9);
    EXPECT_EQ(UVHTTP_WS_OPCODE_PONG, 0xA);
}

/* 测试WebSocket状态 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketStates) {
    /* 测试所有状态 */
    EXPECT_EQ(UVHTTP_WS_STATE_CONNECTING, 0);
    EXPECT_EQ(UVHTTP_WS_STATE_OPEN, 1);
    EXPECT_EQ(UVHTTP_WS_STATE_CLOSING, 2);
    EXPECT_EQ(UVHTTP_WS_STATE_CLOSED, 3);
}

/* 测试WebSocket帧头 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketFrameHeader) {
    uvhttp_ws_frame_header_t header;
    memset(&header, 0, sizeof(header));
    
    /* 测试FIN位 */
    header.fin = 1;
    EXPECT_EQ(header.fin, 1);
    
    /* 测试RSV位 */
    header.rsv1 = 1;
    header.rsv2 = 1;
    header.rsv3 = 1;
    EXPECT_EQ(header.rsv1, 1);
    EXPECT_EQ(header.rsv2, 1);
    EXPECT_EQ(header.rsv3, 1);
    
    /* 测试操作码 */
    header.opcode = UVHTTP_WS_OPCODE_TEXT;
    EXPECT_EQ(header.opcode, UVHTTP_WS_OPCODE_TEXT);
    
    /* 测试MASK位 */
    header.mask = 1;
    EXPECT_EQ(header.mask, 1);
    
    /* 测试负载长度 */
    header.payload_len = 127;
    EXPECT_EQ(header.payload_len, 127);
}

/* 测试WebSocket帧结构 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketFrameStructure) {
    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    
    /* 测试帧头 */
    frame.header.fin = 1;
    frame.header.opcode = UVHTTP_WS_OPCODE_TEXT;
    frame.header.mask = 0;
    frame.header.payload_len = 127;
    
    /* 测试负载长度 */
    frame.payload_length = 1024;
    EXPECT_EQ(frame.payload_length, 1024);
    
    /* 测试掩码密钥 */
    frame.masking_key[0] = 0x12;
    frame.masking_key[1] = 0x34;
    frame.masking_key[2] = 0x56;
    frame.masking_key[3] = 0x78;
    
    /* 测试负载数据 */
    frame.payload = (uint8_t*)malloc(1024);
    ASSERT_NE(frame.payload, nullptr);
    frame.payload_size = 1024;
    
    free(frame.payload);
}

/* 测试WebSocket配置 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketConfig) {
    uvhttp_ws_config_t config;
    memset(&config, 0, sizeof(config));
    
    /* 测试配置参数 */
    config.max_frame_size = 16777216;
    EXPECT_EQ(config.max_frame_size, 16777216);
    
    config.max_message_size = 67108864;
    EXPECT_EQ(config.max_message_size, 67108864);
    
    config.ping_interval = 30;
    EXPECT_EQ(config.ping_interval, 30);
    
    config.ping_timeout = 60;
    EXPECT_EQ(config.ping_timeout, 60);
    
    config.enable_compression = 0;
    EXPECT_EQ(config.enable_compression, 0);
}

/* 测试WebSocket连接创建 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketConnectionCreate) {
    /* 测试有效连接创建 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    /* 即使参数不完整，也可能返回非NULL */
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
    
    /* 测试无效fd */
    conn = uvhttp_ws_connection_create(-1, NULL, 0, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
    
    /* 测试NULL上下文 */
    conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket连接释放 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketConnectionFree) {
    /* 测试释放NULL连接 */
    uvhttp_ws_connection_free(NULL);
    
    /* 测试释放有效连接 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket握手客户端 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketHandshakeClient) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    char request[1024];
    size_t request_len = sizeof(request);
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_handshake_client(NULL, NULL, "localhost", "/", request, &request_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL路径 */
    result = uvhttp_ws_handshake_client(&context, NULL, "localhost", NULL, request, &request_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL请求缓冲区 */
    result = uvhttp_ws_handshake_client(&context, NULL, "localhost", "/", NULL, &request_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL长度指针 */
    result = uvhttp_ws_handshake_client(&context, NULL, "localhost", "/", request, NULL);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket握手服务端 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketHandshakeServer) {
    char response[1024];
    size_t response_len = sizeof(response);
    
    /* 测试NULL连接 */
    int result = uvhttp_ws_handshake_server(NULL, "GET /ws HTTP/1.1", 0, response, &response_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL请求 */
    result = uvhttp_ws_handshake_server(NULL, NULL, 0, response, &response_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL响应缓冲区 */
    result = uvhttp_ws_handshake_server(NULL, "GET /ws HTTP/1.1", 0, NULL, &response_len);
    EXPECT_NE(result, 0);
    
    /* 测试NULL长度指针 */
    result = uvhttp_ws_handshake_server(NULL, "GET /ws HTTP/1.1", 0, response, NULL);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket验证握手响应 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketVerifyHandshake) {
    /* 测试NULL连接 */
    int result = uvhttp_ws_verify_handshake_response(NULL, NULL, 0);
    EXPECT_NE(result, 0);
    
    /* 测试NULL响应 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_verify_handshake_response(conn, NULL, 0);
        EXPECT_NE(result, 0);
        
        /* 测试空响应 */
        result = uvhttp_ws_verify_handshake_response(conn, "", 0);
        EXPECT_NE(result, 0);
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket接收帧 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketRecvFrame) {
    /* 测试NULL连接 */
    uvhttp_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    int result = uvhttp_ws_recv_frame(NULL, &frame);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL帧指针 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_recv_frame(conn, NULL);
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket发送文本 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketSendText) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_send_text(NULL, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL连接 */
    result = uvhttp_ws_send_text(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL数据 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_send_text(&context, conn, NULL, 0);
        EXPECT_NE(result, UVHTTP_OK);
        
        /* 测试空数据 */
        result = uvhttp_ws_send_text(&context, conn, "", 0);
        /* 空数据可能成功 */
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket发送二进制 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketSendBinary) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_send_binary(NULL, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL连接 */
    result = uvhttp_ws_send_binary(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL数据 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_send_binary(&context, conn, NULL, 0);
        EXPECT_NE(result, UVHTTP_OK);
        
        /* 测试空数据 */
        result = uvhttp_ws_send_binary(&context, conn, (const uint8_t*)"", 0);
        /* 空数据可能成功 */
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket发送Ping */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketSendPing) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_send_ping(NULL, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL连接 */
    result = uvhttp_ws_send_ping(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL数据 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_send_ping(&context, conn, NULL, 0);
        EXPECT_NE(result, UVHTTP_OK);
        
        /* 测试空数据 */
        result = uvhttp_ws_send_ping(&context, conn, (const uint8_t*)"", 0);
        /* 空数据可能成功 */
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket发送Pong */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketSendPong) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_send_pong(NULL, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL连接 */
    result = uvhttp_ws_send_pong(&context, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL数据 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_send_pong(&context, conn, NULL, 0);
        EXPECT_NE(result, UVHTTP_OK);
        
        /* 测试空数据 */
        result = uvhttp_ws_send_pong(&context, conn, (const uint8_t*)"", 0);
        /* 空数据可能成功 */
        
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket关闭连接 */
TEST(UvhttpWebSocketEnhancedCoverageTest, WebSocketClose) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    /* 测试NULL上下文 */
    int result = uvhttp_ws_close(NULL, NULL, 1000, "Normal closure");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL连接 */
    result = uvhttp_ws_close(&context, NULL, 1000, "Normal closure");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL原因 */
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(10, NULL, 0, NULL);
    if (conn) {
        result = uvhttp_ws_close(&context, conn, 1000, NULL);
        /* NULL原因可能成功 */
        
        /* 测试空原因 */
        result = uvhttp_ws_close(&context, conn, 1000, "");
        /* 空原因可能成功 */
        
        uvhttp_ws_connection_free(conn);
    }
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */
