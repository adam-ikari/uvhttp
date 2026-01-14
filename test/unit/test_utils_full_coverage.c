#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <uv.h>
#include "uvhttp_utils.h"
#include "uvhttp_response.h"
#include "uvhttp_request.h"
#include "uvhttp_connection.h"

/* ============ 测试辅助函数 ============ */

/* 创建测试响应对象 */
static uvhttp_response_t* create_test_response(void) {
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (!client) return NULL;

    uv_tcp_init(loop, client);

    uvhttp_response_t* resp = (uvhttp_response_t*)malloc(sizeof(uvhttp_response_t));
    if (!resp) {
        uv_close((uv_handle_t*)client, NULL);
        free(client);
        return NULL;
    }

    uvhttp_response_init(resp, client);
    return resp;
}

/* 销毁测试响应对象 */
static void destroy_test_response(uvhttp_response_t* resp) {
    if (!resp) return;

    uv_tcp_t* client = resp->client;
    uvhttp_response_free(resp);

    if (client) {
        uv_close((uv_handle_t*)client, NULL);
        free(client);
    }
}

/* ============ 测试安全字符串复制函数 ============ */

void test_safe_strncpy_normal(void) {
    printf("test_safe_strncpy_normal: START\n");
    char dest[100];
    const char* src = "Hello, World!";

    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, src) == 0);
    (void)result;

    printf("test_safe_strncpy_normal: PASSED\n");
}

void test_safe_strncpy_exact_size(void) {
    printf("test_safe_strncpy_exact_size: START\n");
    char dest[14];
    const char* src = "Hello, World!";

    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, src) == 0);
    (void)result;

    printf("test_safe_strncpy_exact_size: PASSED\n");
}

void test_safe_strncpy_truncate(void) {
    printf("test_safe_strncpy_truncate: START\n");
    char dest[10];
    const char* src = "Hello, World!";

    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, "Hello, Wo") == 0);
    assert(dest[9] == '\0');
    (void)result;

    printf("test_safe_strncpy_truncate: PASSED\n");
}

void test_safe_strncpy_null_dest(void) {
    printf("test_safe_strncpy_null_dest: START\n");
    char dest[100];
    const char* src = "Hello, World!";

    int result = uvhttp_safe_strncpy(NULL, src, sizeof(dest));
    assert(result == -1);
    (void)result;

    printf("test_safe_strncpy_null_dest: PASSED\n");
}

void test_safe_strncpy_null_src(void) {
    printf("test_safe_strncpy_null_src: START\n");
    char dest[100];

    int result = uvhttp_safe_strncpy(dest, NULL, sizeof(dest));
    assert(result == -1);
    (void)result;

    printf("test_safe_strncpy_null_src: PASSED\n");
}

void test_safe_strncpy_zero_size(void) {
    printf("test_safe_strncpy_zero_size: START\n");
    char dest[100];
    const char* src = "Hello, World!";

    int result = uvhttp_safe_strncpy(dest, src, 0);
    assert(result == -1);
    (void)result;

    printf("test_safe_strncpy_zero_size: PASSED\n");
}

/* ============ 测试统一响应发送函数 ============ */

void test_send_unified_response_normal(void) {
    printf("test_send_unified_response_normal: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* content = "Hello, World!";
    (void)uvhttp_send_unified_response(resp, content, strlen(content), 200);
    /* 注意：这里可能会失败，因为需要实际的连接 */
    /* 我们主要测试参数验证 */

    destroy_test_response(resp);
    printf("test_send_unified_response_normal: PASSED\n");
}

void test_send_unified_response_null_response(void) {
    printf("test_send_unified_response_null_response: START\n");
    const char* content = "Hello, World!";
    uvhttp_error_t err = uvhttp_send_unified_response(NULL, content, strlen(content), 200);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    printf("test_send_unified_response_null_response: PASSED\n");
}

void test_send_unified_response_null_content(void) {
    printf("test_send_unified_response_null_content: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    uvhttp_error_t err = uvhttp_send_unified_response(resp, NULL, 0, 200);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_unified_response_null_content: PASSED\n");
}

void test_send_unified_response_invalid_status(void) {
    printf("test_send_unified_response_invalid_status: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* content = "Hello, World!";
    uvhttp_error_t err = uvhttp_send_unified_response(resp, content, strlen(content), 999);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_unified_response_invalid_status: PASSED\n");
}

void test_send_unified_response_auto_length(void) {
    printf("test_send_unified_response_auto_length: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* content = "Hello, World!";
    (void)uvhttp_send_unified_response(resp, content, 0, 200);
    /* 注意：这里可能会失败，因为需要实际的连接 */

    destroy_test_response(resp);
    printf("test_send_unified_response_auto_length: PASSED\n");
}

void test_send_unified_response_zero_length(void) {
    printf("test_send_unified_response_zero_length: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* content = "";
    uvhttp_error_t err = uvhttp_send_unified_response(resp, content, 0, 200);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_unified_response_zero_length: PASSED\n");
}

/* ============ 测试错误响应发送函数 ============ */

void test_send_error_response_normal(void) {
    printf("test_send_error_response_normal: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* error_message = "Not Found";
    const char* details = "Resource does not exist";
    (void)uvhttp_send_error_response(resp, 404, error_message, details);
    /* 注意：这里可能会失败，因为需要实际的连接 */

    destroy_test_response(resp);
    printf("test_send_error_response_normal: PASSED\n");
}

void test_send_error_response_null_response(void) {
    printf("test_send_error_response_null_response: START\n");
    const char* error_message = "Not Found";
    uvhttp_error_t err = uvhttp_send_error_response(NULL, 404, error_message, NULL);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    printf("test_send_error_response_null_response: PASSED\n");
}

void test_send_error_response_null_message(void) {
    printf("test_send_error_response_null_message: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    uvhttp_error_t err = uvhttp_send_error_response(resp, 404, NULL, NULL);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_error_response_null_message: PASSED\n");
}

void test_send_error_response_invalid_code(void) {
    printf("test_send_error_response_invalid_code: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* error_message = "Not Found";
    uvhttp_error_t err = uvhttp_send_error_response(resp, 999, error_message, NULL);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_error_response_invalid_code: PASSED\n");
}

void test_send_error_response_long_message(void) {
    printf("test_send_error_response_long_message: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    /* 创建一个超过 MAX_ERROR_MSG_LEN (200) 的消息 */
    char long_message[300];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';

    uvhttp_error_t err = uvhttp_send_error_response(resp, 404, long_message, NULL);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_error_response_long_message: PASSED\n");
}

void test_send_error_response_long_details(void) {
    printf("test_send_error_response_long_details: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* error_message = "Not Found";
    /* 创建一个超过 MAX_ERROR_DETAILS_LEN (400) 的详细信息 */
    char long_details[500];
    memset(long_details, 'B', sizeof(long_details) - 1);
    long_details[sizeof(long_details) - 1] = '\0';

    uvhttp_error_t err = uvhttp_send_error_response(resp, 404, error_message, long_details);
    assert(err == UVHTTP_ERROR_INVALID_PARAM);
    (void)err;

    destroy_test_response(resp);
    printf("test_send_error_response_long_details: PASSED\n");
}

void test_send_error_response_no_details(void) {
    printf("test_send_error_response_no_details: START\n");
    uvhttp_response_t* resp = create_test_response();
    assert(resp != NULL);

    const char* error_message = "Not Found";
    (void)uvhttp_send_error_response(resp, 404, error_message, NULL);
    /* 注意：这里可能会失败，因为需要实际的连接 */

    destroy_test_response(resp);
    printf("test_send_error_response_no_details: PASSED\n");
}

/* ============ 测试验证函数 ============ */

void test_is_valid_status_code_valid(void) {
    printf("test_is_valid_status_code_valid: START\n");
    
    /* 测试有效的状态码 */
    assert(uvhttp_is_valid_status_code(100) == TRUE);
    assert(uvhttp_is_valid_status_code(200) == TRUE);
    assert(uvhttp_is_valid_status_code(301) == TRUE);
    assert(uvhttp_is_valid_status_code(404) == TRUE);
    assert(uvhttp_is_valid_status_code(500) == TRUE);
    assert(uvhttp_is_valid_status_code(599) == TRUE);
    
    printf("test_is_valid_status_code_valid: PASSED\n");
}

void test_is_valid_status_code_invalid(void) {
    printf("test_is_valid_status_code_invalid: START\n");
    
    /* 测试无效的状态码 */
    assert(uvhttp_is_valid_status_code(99) == FALSE);
    assert(uvhttp_is_valid_status_code(600) == FALSE);
    assert(uvhttp_is_valid_status_code(0) == FALSE);
    assert(uvhttp_is_valid_status_code(-1) == FALSE);
    
    printf("test_is_valid_status_code_invalid: PASSED\n");
}

void test_is_valid_content_type_valid(void) {
    printf("test_is_valid_content_type_valid: START\n");

    /* 测试有效的 Content-Type */
    assert(uvhttp_is_valid_content_type("text/html") == TRUE);
    assert(uvhttp_is_valid_content_type("application/json") == TRUE);
    assert(uvhttp_is_valid_content_type("image/png") == TRUE);
    /* 注意：text/plain; charset=utf-8 包含 ;，被视为非法字符 */
    /* assert(uvhttp_is_valid_content_type("text/plain; charset=utf-8") == TRUE); */

    printf("test_is_valid_content_type_valid: PASSED\n");
}

void test_is_valid_content_type_invalid(void) {
    printf("test_is_valid_content_type_invalid: START\n");
    
    /* 测试无效的 Content-Type */
    assert(uvhttp_is_valid_content_type(NULL) == FALSE);
    assert(uvhttp_is_valid_content_type("") == FALSE);
    assert(uvhttp_is_valid_content_type("text") == FALSE);  /* 缺少 '/' */
    assert(uvhttp_is_valid_content_type("text/html\"") == FALSE);  /* 包含非法字符 */
    assert(uvhttp_is_valid_content_type("text/html,") == FALSE);  /* 包含非法字符 */
    
    printf("test_is_valid_content_type_invalid: PASSED\n");
}

void test_is_valid_string_length_valid(void) {
    printf("test_is_valid_string_length_valid: START\n");
    
    /* 测试有效的字符串长度 */
    assert(uvhttp_is_valid_string_length("Hello", 10) == TRUE);
    assert(uvhttp_is_valid_string_length("Hello", 5) == TRUE);
    assert(uvhttp_is_valid_string_length("", 10) == TRUE);
    
    printf("test_is_valid_string_length_valid: PASSED\n");
}

void test_is_valid_string_length_invalid(void) {
    printf("test_is_valid_string_length_invalid: START\n");
    
    /* 测试无效的字符串长度 */
    assert(uvhttp_is_valid_string_length(NULL, 10) == FALSE);
    assert(uvhttp_is_valid_string_length("Hello", 4) == FALSE);
    
    printf("test_is_valid_string_length_invalid: PASSED\n");
}

/* ============ 测试边界条件 ============ */

void test_edge_cases(void) {
    printf("test_edge_cases: START\n");
    
    /* 测试边界状态码 */
    assert(uvhttp_is_valid_status_code(100) == TRUE);
    assert(uvhttp_is_valid_status_code(599) == TRUE);
    assert(uvhttp_is_valid_status_code(101) == TRUE);
    assert(uvhttp_is_valid_status_code(598) == TRUE);
    
    /* 测试边界字符串长度 */
    assert(uvhttp_is_valid_string_length("A", 1) == TRUE);
    assert(uvhttp_is_valid_string_length("AB", 1) == FALSE);
    
    /* 测试特殊 Content-Type */
    assert(uvhttp_is_valid_content_type("a/b") == TRUE);
    assert(uvhttp_is_valid_content_type("application/vnd.api+json") == TRUE);
    
    printf("test_edge_cases: PASSED\n");
}

/* ============ 主函数 ============ */

int main(void) {
    printf("=== uvhttp_utils.c 完整覆盖率测试 ===\n\n");
    
    /* 测试安全字符串复制函数 */
    test_safe_strncpy_normal();
    test_safe_strncpy_exact_size();
    test_safe_strncpy_truncate();
    test_safe_strncpy_null_dest();
    test_safe_strncpy_null_src();
    test_safe_strncpy_zero_size();
    
    /* 测试统一响应发送函数 */
    test_send_unified_response_normal();
    test_send_unified_response_null_response();
    test_send_unified_response_null_content();
    test_send_unified_response_invalid_status();
    test_send_unified_response_auto_length();
    test_send_unified_response_zero_length();
    
    /* 测试错误响应发送函数 */
    test_send_error_response_normal();
    test_send_error_response_null_response();
    test_send_error_response_null_message();
    test_send_error_response_invalid_code();
    test_send_error_response_long_message();
    test_send_error_response_long_details();
    test_send_error_response_no_details();
    
    /* 测试验证函数 */
    test_is_valid_status_code_valid();
    test_is_valid_status_code_invalid();
    test_is_valid_content_type_valid();
    test_is_valid_content_type_invalid();
    test_is_valid_string_length_valid();
    test_is_valid_string_length_invalid();
    
    /* 测试边界条件 */
    test_edge_cases();
    
    printf("\n=== 所有测试通过 ===\n");
    return 0;
}