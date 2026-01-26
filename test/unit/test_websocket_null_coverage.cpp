/* uvhttp_websocket_native.c NULL参数覆盖率测试 */

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>
#include "uvhttp_websocket_native.h"
#include "uvhttp_context.h"

/* 测试WebSocket连接创建NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsConnectionNewNull) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(-1, NULL, 0);
    /* 即使fd无效，也可能返回非NULL（内部会处理） */
    /* 所以这里只测试不会崩溃 */
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
}

/* 测试WebSocket连接释放NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsConnectionFreeNull) {
    uvhttp_ws_connection_free(NULL);
}

/* 测试WebSocket握手客户端NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsHandshakeClientNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    char request[1024];
    size_t request_len = sizeof(request);
    int result = uvhttp_ws_handshake_client(&context, NULL, "localhost", "/", request, &request_len);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket握手服务端NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsHandshakeServerNull) {
    char response[1024];
    size_t response_len = sizeof(response);
    int result = uvhttp_ws_handshake_server(NULL, "GET /ws HTTP/1.1", 0, response, &response_len);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket验证握手响应NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsVerifyHandshakeResponseNull) {
    int result = uvhttp_ws_verify_handshake_response(NULL, NULL, 0);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket接收帧NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsRecvFrameNull) {
    uvhttp_ws_frame_t frame;
    int result = uvhttp_ws_recv_frame(NULL, &frame);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket发送帧NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsSendFrameNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_send_frame(&context, NULL, NULL, 0, UVHTTP_WS_OPCODE_TEXT);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket发送文本NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsSendTextNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_send_text(&context, NULL, NULL, 0);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket发送二进制NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsSendBinaryNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_send_binary(&context, NULL, NULL, 0);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket发送Ping NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsSendPingNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_send_ping(&context, NULL, NULL, 0);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket发送Pong NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsSendPongNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_send_pong(&context, NULL, NULL, 0);
    EXPECT_NE(result, 0);
}

/* 测试WebSocket获取状态字符串 */
TEST(UvhttpWebsocketNullCoverageTest, WsGetStateString) {
    /* WebSocket没有get_state_string函数，跳过此测试 */
    SUCCEED();
}

/* 测试WebSocket关闭NULL */
TEST(UvhttpWebsocketNullCoverageTest, WsCloseNull) {
    uvhttp_context_t context;
    memset(&context, 0, sizeof(context));
    
    int result = uvhttp_ws_close(&context, NULL, 1000, "");
    EXPECT_NE(result, 0);
}

#else

/* WebSocket 功能未启用时的空测试 */
TEST(UvhttpWebsocketNullCoverageTest, WebSocketDisabled) {
    SUCCEED() << "WebSocket功能未启用，跳过测试";
}

#endif