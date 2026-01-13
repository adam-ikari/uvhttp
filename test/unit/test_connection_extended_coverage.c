/* uvhttp_connection.c 扩展覆盖率测试 */

#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <uv.h>

/* 测试 TLS 握手函数 */
void test_connection_tls_handshake_func(void) {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    assert(conn != NULL);

    /* 测试未启用TLS的情况 */
    int result = uvhttp_connection_tls_handshake_func(conn);
    assert(result == -1);

    uvhttp_connection_free(conn);
    uvhttp_free(server);

    printf("test_connection_tls_handshake_func: PASSED\n");
}

/* 测试连接状态转换 */
void test_connection_state_transitions(void) {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    assert(conn != NULL);
    assert(conn->state == UVHTTP_CONN_STATE_NEW);

    /* 测试状态转换 */
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    assert(conn->state == UVHTTP_CONN_STATE_HTTP_READING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    assert(conn->state == UVHTTP_CONN_STATE_HTTP_PROCESSING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    assert(conn->state == UVHTTP_CONN_STATE_CLOSING);

    uvhttp_connection_free(conn);
    uvhttp_free(server);

    printf("test_connection_state_transitions: PASSED\n");
}

/* 测试连接重启读取 */
void test_connection_restart_read_with_null(void) {
    /* 测试NULL连接 */
    int result = uvhttp_connection_restart_read(NULL);
    assert(result == -1);

    printf("test_connection_restart_read_with_null: PASSED\n");
}

/* 测试连接调度重启读取 */
void test_connection_schedule_restart_read_with_null(void) {
    /* 测试NULL连接 */
    int result = uvhttp_connection_schedule_restart_read(NULL);
    assert(result == -1);

    printf("test_connection_schedule_restart_read_with_null: PASSED\n");
}

/* 测试连接启动 */
void test_connection_start_with_null(void) {
    /* 测试NULL连接 */
    int result = uvhttp_connection_start(NULL);
    assert(result == -1);

    printf("test_connection_start_with_null: PASSED\n");
}

int main() {
    printf("=== uvhttp_connection.c 扩展覆盖率测试 ===\n\n");

    test_connection_tls_handshake_func();
    test_connection_state_transitions();
    test_connection_restart_read_with_null();
    test_connection_schedule_restart_read_with_null();
    test_connection_start_with_null();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}