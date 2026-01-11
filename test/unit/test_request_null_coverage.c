/* uvhttp_request.c NULL参数覆盖率测试 */

#include "uvhttp_request.h"
#include <stdio.h>
#include <assert.h>

/* 测试request释放NULL */
void test_request_free_null(void) {
    uvhttp_request_free(NULL);

    printf("test_request_free_null: PASSED\n");
}

/* 测试request清理NULL */
void test_request_cleanup_null(void) {
    uvhttp_request_cleanup(NULL);

    printf("test_request_cleanup_null: PASSED\n");
}

/* 测试request获取方法NULL */
void test_request_get_method_null(void) {
    const char* method = uvhttp_request_get_method(NULL);
    assert(method == NULL);
    (void)method;

    printf("test_request_get_method_null: PASSED\n");
}

/* 测试request获取URL NULL */
void test_request_get_url_null(void) {
    const char* url = uvhttp_request_get_url(NULL);
    assert(url == NULL);
    (void)url;

    printf("test_request_get_url_null: PASSED\n");
}

/* 测试request获取路径NULL */
void test_request_get_path_null(void) {
    const char* path = uvhttp_request_get_path(NULL);
    assert(path == NULL);
    (void)path;

    printf("test_request_get_path_null: PASSED\n");
}

/* 测试request获取查询字符串NULL */
void test_request_get_query_string_null(void) {
    const char* query = uvhttp_request_get_query_string(NULL);
    assert(query == NULL);
    (void)query;

    printf("test_request_get_query_string_null: PASSED\n");
}

/* 测试request获取查询参数NULL */
void test_request_get_query_param_null(void) {
    const char* param = uvhttp_request_get_query_param(NULL, "test");
    assert(param == NULL);
    (void)param;

    printf("test_request_get_query_param_null: PASSED\n");
}

/* 测试request获取客户端IP NULL */
void test_request_get_client_ip_null(void) {
    const char* ip = uvhttp_request_get_client_ip(NULL);
    assert(ip == NULL);
    (void)ip;

    printf("test_request_get_client_ip_null: PASSED\n");
}

/* 测试request获取header NULL */
void test_request_get_header_null(void) {
    const char* header = uvhttp_request_get_header(NULL, "test");
    assert(header == NULL);
    (void)header;

    printf("test_request_get_header_null: PASSED\n");
}

/* 测试request获取body NULL */
void test_request_get_body_null(void) {
    const char* body = uvhttp_request_get_body(NULL);
    assert(body == NULL);
    (void)body;

    printf("test_request_get_body_null: PASSED\n");
}

/* 测试request获取body长度NULL */
void test_request_get_body_length_null(void) {
    size_t len = uvhttp_request_get_body_length(NULL);
    assert(len == 0);
    (void)len;

    printf("test_request_get_body_length_null: PASSED\n");
}

int main() {
    printf("=== uvhttp_request.c NULL参数覆盖率测试 ===\n\n");

    test_request_free_null();
    test_request_cleanup_null();
    test_request_get_method_null();
    test_request_get_url_null();
    test_request_get_path_null();
    test_request_get_query_string_null();
    test_request_get_query_param_null();
    test_request_get_client_ip_null();
    test_request_get_header_null();
    test_request_get_body_null();
    test_request_get_body_length_null();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}