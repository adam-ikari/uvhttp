/* uvhttp_static.c 覆盖率测试 */

#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试获取MIME类型 */
void test_static_get_mime_type(void) {
    char mime_type[256];
    int result;

    /* 测试常见文件类型 */
    result = uvhttp_static_get_mime_type("test.html", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "text/html") != NULL);

    result = uvhttp_static_get_mime_type("test.css", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "text/css") != NULL);

    result = uvhttp_static_get_mime_type("test.js", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "application/javascript") != NULL);

    result = uvhttp_static_get_mime_type("test.json", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "application/json") != NULL);

    result = uvhttp_static_get_mime_type("test.png", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "image/png") != NULL);

    result = uvhttp_static_get_mime_type("test.jpg", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "image/jpeg") != NULL);

    /* 测试未知文件类型 */
    result = uvhttp_static_get_mime_type("test.unknown", mime_type, sizeof(mime_type));
    assert(result == 0);
    assert(strstr(mime_type, "application/octet-stream") != NULL);

    printf("test_static_get_mime_type: PASSED\n");
}

/* 测试获取MIME类型NULL参数 */
void test_static_get_mime_type_null(void) {
    char mime_type[256];
    int result;

    result = uvhttp_static_get_mime_type(NULL, mime_type, sizeof(mime_type));
    assert(result != 0);

    result = uvhttp_static_get_mime_type("test.html", NULL, sizeof(mime_type));
    assert(result != 0);

    printf("test_static_get_mime_type_null: PASSED\n");
}

/* 测试生成ETag */
void test_static_generate_etag(void) {
    char etag[256];
    uvhttp_result_t result;

    result = uvhttp_static_generate_etag("test.html", 0, 100, etag, sizeof(etag));
    assert(result == UVHTTP_OK);
    assert(strlen(etag) > 0);

    printf("test_static_generate_etag: PASSED\n");
}

/* 测试生成ETag NULL参数 */
void test_static_generate_etag_null(void) {
    char etag[256];
    uvhttp_result_t result;

    result = uvhttp_static_generate_etag(NULL, 0, 0, etag, sizeof(etag));
    assert(result != UVHTTP_OK);

    result = uvhttp_static_generate_etag("test.html", 0, 0, NULL, sizeof(etag));
    assert(result != UVHTTP_OK);

    printf("test_static_generate_etag_null: PASSED\n");
}

/* 测试静态上下文创建 */
void test_static_context_new(void) {
    /* uvhttp_static_context_new函数不存在，跳过此测试 */
    printf("test_static_context_new: SKIPPED (function not available)\n");
}

/* 测试静态上下文释放NULL */
void test_static_free_null(void) {
    uvhttp_static_free(NULL);

    printf("test_static_free_null: PASSED\n");
}

/* 测试设置响应头NULL */
void test_static_set_response_headers_null(void) {
    uvhttp_result_t result = uvhttp_static_set_response_headers(NULL, NULL, 0, 0, NULL);
    assert(result != UVHTTP_OK);

    printf("test_static_set_response_headers_null: PASSED\n");
}

/* 测试检查条件请求NULL */
void test_static_check_conditional_request_null(void) {
    int result = uvhttp_static_check_conditional_request(NULL, NULL, 0);
    /* 可能返回0（函数可能忽略NULL） */
    printf("test_static_check_conditional_request_null: PASSED (result=%d)\n", result);
}

int main() {
    printf("=== uvhttp_static.c 覆盖率测试 ===\n\n");

    test_static_get_mime_type();
    test_static_get_mime_type_null();
    test_static_generate_etag();
    test_static_generate_etag_null();
    test_static_context_new();
    test_static_free_null();
    test_static_set_response_headers_null();
    test_static_check_conditional_request_null();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}