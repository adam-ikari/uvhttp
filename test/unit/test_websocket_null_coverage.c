/* uvhttp_websocket_native.c NULL参数覆盖率测试 */

#if UVHTTP_FEATURE_WEBSOCKET

#include "uvhttp_websocket_native.h"
#include <stdio.h>
#include <assert.h>

/* 测试WebSocket连接创建NULL */
void test_ws_connection_new_null(void) {
    uvhttp_ws_connection_t* conn = uvhttp_ws_connection_create(-1, NULL, 0);
    /* 即使fd无效，也可能返回非NULL（内部会处理） */
    /* 所以这里只测试不会崩溃 */
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }

    printf("test_ws_connection_new_null: PASSED\n");
}

/* 测试WebSocket连接释放NULL */
void test_ws_connection_free_null(void) {
    uvhttp_ws_connection_free(NULL);

    printf("test_ws_connection_free_null: PASSED\n");
}

/* 测试WebSocket握手客户端NULL */
void test_ws_handshake_client_null(void) {
    char request[1024];
    size_t request_len = sizeof(request);
    int result = uvhttp_ws_handshake_client(NULL, "ws://example.com", "/", request, &request_len);
    assert(result != 0);
    (void)result;

    printf("test_ws_handshake_client_null: PASSED\n");
}

/* 测试WebSocket握手服务端NULL */
void test_ws_handshake_server_null(void) {
    char response[1024];
    size_t response_len = sizeof(response);
    int result = uvhttp_ws_handshake_server(NULL, "GET /ws HTTP/1.1", 0, response, &response_len);
    assert(result != 0);
    (void)result;

    printf("test_ws_handshake_server_null: PASSED\n");
}

/* 测试WebSocket验证握手响应NULL */
void test_ws_verify_handshake_response_null(void) {
    int result = uvhttp_ws_verify_handshake_response(NULL, NULL, 0);
    assert(result != 0);
    (void)result;

    printf("test_ws_verify_handshake_response_null: PASSED\n");
}

/* 测试WebSocket接收帧NULL */
void test_ws_recv_frame_null(void) {
    uvhttp_ws_frame_t frame;
    int result = uvhttp_ws_recv_frame(NULL, &frame);
    assert(result != 0);
    (void)result;

    printf("test_ws_recv_frame_null: PASSED\n");
}

/* 测试WebSocket发送帧NULL */
void test_ws_send_frame_null(void) {
    int result = uvhttp_ws_send_frame(NULL, NULL, 0, UVHTTP_WS_OPCODE_TEXT);
    assert(result != 0);
    (void)result;

    printf("test_ws_send_frame_null: PASSED\n");
}

/* 测试WebSocket发送文本NULL */
void test_ws_send_text_null(void) {
    int result = uvhttp_ws_send_text(NULL, NULL, 0);
    assert(result != 0);
    (void)result;

    printf("test_ws_send_text_null: PASSED\n");
}

/* 测试WebSocket发送二进制NULL */
void test_ws_send_binary_null(void) {
    int result = uvhttp_ws_send_binary(NULL, NULL, 0);
    assert(result != 0);
    (void)result;

    printf("test_ws_send_binary_null: PASSED\n");
}

/* 测试WebSocket发送Ping NULL */
void test_ws_send_ping_null(void) {
    int result = uvhttp_ws_send_ping(NULL, NULL, 0);
    assert(result != 0);
    (void)result;

    printf("test_ws_send_ping_null: PASSED\n");
}

/* 测试WebSocket发送Pong NULL */
void test_ws_send_pong_null(void) {
    int result = uvhttp_ws_send_pong(NULL, NULL, 0);
    assert(result != 0);
    (void)result;

    printf("test_ws_send_pong_null: PASSED\n");
}

/* 测试WebSocket获取状态字符串 */
void test_ws_get_state_string(void) {
    /* WebSocket没有get_state_string函数，跳过此测试 */
    printf("test_ws_get_state_string: SKIPPED (function not available)\n");
}

/* 测试WebSocket关闭NULL */
void test_ws_close_null(void) {
    int result = uvhttp_ws_close(NULL, 1000, "");
    assert(result != 0);
    (void)result;

    printf("test_ws_close_null: PASSED\n");
}

int main() {
    printf("=== uvhttp_websocket_native.c NULL参数覆盖率测试 ===\n\n");

    test_ws_connection_new_null();
    test_ws_connection_free_null();
    test_ws_handshake_client_null();
    test_ws_handshake_server_null();
    test_ws_verify_handshake_response_null();
    test_ws_recv_frame_null();
    test_ws_send_frame_null();
    test_ws_send_text_null();
    test_ws_send_binary_null();
    test_ws_send_ping_null();
    test_ws_send_pong_null();
    test_ws_close_null();
    test_ws_get_state_string();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}

#else

int main() {
    printf("=== WebSocket功能未启用，跳过测试 ===\n");
    return 0;
}

#endif