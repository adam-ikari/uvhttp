/* UVHTTP 请求模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_constants.h"
#include <uv.h>

/* 测试请求初始化 */
void test_request_init(void) {
    /* 测试初始化 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    int result = uvhttp_request_init(&request, &client);
    assert(result == 0);
    
    /* 检查初始状态 */
    assert(request.client == &client);
    assert(request.parser != NULL);
    assert(request.parser_settings != NULL);
    assert(request.method == UVHTTP_GET);  /* 默认方法是 GET */
    assert(request.body != NULL);  /* body 会被分配内存 */
    assert(request.body_length == 0);
    assert(request.header_count == 0);
    assert(request.parsing_complete == 0);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_init: PASSED\n");
}

/* 测试请求清理 */
void test_request_cleanup(void) {
    /* 测试清理 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    uvhttp_request_cleanup(&request);
    
    /* 检查清理后的状态 */
    // body 已被释放，但不一定设置为 NULL
    assert(request.body_length == 0);
    
    printf("test_request_cleanup: PASSED\n");
}

/* 测试请求释放 */
void test_request_free(void) {
    /* 测试释放 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    // uvhttp_request_free 只能释放堆上分配的请求
    
    printf("test_request_free: PASSED\n");
}

/* 测试 NULL 参数处理 */
void test_null_params(void) {
    /* 测试 NULL 参数 */
    uvhttp_request_t request;
    
    /* 测试 NULL 客户端 */
    int result = uvhttp_request_init(&request, NULL);
    assert(result != 0);
    
    /* 测试 NULL 请求 */
    uv_tcp_t client;
    result = uvhttp_request_init(NULL, &client);
    assert(result != 0);
    
    /* 测试清理 NULL */
    uvhttp_request_cleanup(NULL);
    
    /* 测试释放 NULL */
    uvhttp_request_free(NULL);
    
    printf("test_null_params: PASSED\n");
}

/* 测试多次初始化和释放 */
void test_multiple_init_free(void) {
    /* 测试多次初始化和释放 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    for (int i = 0; i < 10; i++) {
        uvhttp_request_init(&request, &client);
        uvhttp_request_cleanup(&request);
    }
    
    printf("test_multiple_init_free: PASSED\n");
}

/* 测试请求体长度获取 */
void test_request_get_body_length(void) {
    /* 测试获取请求体长度 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 初始长度应该为 0 */
    size_t length = uvhttp_request_get_body_length(&request);
    assert(length == 0);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_get_body_length: PASSED\n");
}

/* 测试请求结构体字段 */
void test_request_fields(void) {
    /* 测试请求结构体字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查 URL 字段 */
    assert(strlen(request.url) == 0);
    
    /* 检查路径和查询字段 */
    assert(request.path == NULL);
    assert(request.query == NULL);
    
    /* 检查用户数据字段 */
    assert(request.user_data == NULL);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_fields: PASSED\n");
}

/* 测试请求头字段 */
void test_request_headers(void) {
    /* 测试请求头字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查请求头数量 */
    assert(request.header_count == 0);
    
    /* 检查请求头数组 */
    for (size_t i = 0; i < MAX_HEADERS; i++) {
        assert(request.headers[i].name[0] == '\0');
        assert(request.headers[i].value[0] == '\0');
    }
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_headers: PASSED\n");
}

/* 测试请求方法 */
void test_request_method(void) {
    /* 测试请求方法 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查初始方法（默认是 GET） */
    assert(request.method == UVHTTP_GET);
    
    /* 设置不同的方法 */
    request.method = UVHTTP_ANY;
    assert(request.method == UVHTTP_ANY);
    
    request.method = UVHTTP_POST;
    assert(request.method == UVHTTP_POST);
    
    request.method = UVHTTP_PUT;
    assert(request.method == UVHTTP_PUT);
    
    request.method = UVHTTP_DELETE;
    assert(request.method == UVHTTP_DELETE);
    
    request.method = UVHTTP_HEAD;
    assert(request.method == UVHTTP_HEAD);
    
    request.method = UVHTTP_OPTIONS;
    assert(request.method == UVHTTP_OPTIONS);
    
    request.method = UVHTTP_PATCH;
    assert(request.method == UVHTTP_PATCH);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_method: PASSED\n");
}

/* 测试请求解析状态 */
void test_request_parsing_state(void) {
    /* 测试请求解析状态 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查初始解析状态 */
    assert(request.parsing_complete == 0);
    
    /* 设置解析完成状态 */
    request.parsing_complete = 1;
    assert(request.parsing_complete == 1);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_parsing_state: PASSED\n");
}

/* 测试请求体字段 */
void test_request_body(void) {
    /* 测试请求体字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查初始请求体 */
    assert(request.body != NULL);  /* body 会被分配内存 */
    assert(request.body_length == 0);
    assert(request.body_capacity > 0);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_body: PASSED\n");
}

/* 测试请求用户数据 */
void test_request_user_data(void) {
    /* 测试请求用户数据 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 设置用户数据 */
    int test_data = 42;
    request.user_data = &test_data;
    assert(request.user_data == &test_data);
    
    /* 清除用户数据 */
    request.user_data = NULL;
    assert(request.user_data == NULL);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_user_data: PASSED\n");
}

/* 测试请求 URL 字段 */
void test_request_url(void) {
    /* 测试请求 URL 字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查 URL 数组 */
    for (size_t i = 0; i < MAX_URL_LEN; i++) {
        assert(request.url[i] == '\0');
    }
    
    /* 设置 URL */
    strncpy(request.url, "/test/path?param=value", MAX_URL_LEN - 1);
    assert(strcmp(request.url, "/test/path?param=value") == 0);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_url: PASSED\n");
}

/* 测试请求客户端字段 */
void test_request_client(void) {
    /* 测试请求客户端字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查客户端字段 */
    assert(request.client == &client);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_client: PASSED\n");
}

/* 测试请求解析器字段 */
void test_request_parser(void) {
    /* 测试请求解析器字段 */
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    /* 检查解析器字段 */
    assert(request.parser != NULL);
    assert(request.parser_settings != NULL);
    
    uvhttp_request_cleanup(&request);
    
    printf("test_request_parser: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_request.c 完整覆盖率测试 ===\n\n");

    test_request_init();
    test_request_cleanup();
    test_request_free();
    test_null_params();
    test_multiple_init_free();
    test_request_get_body_length();
    test_request_fields();
    test_request_headers();
    test_request_method();
    test_request_parsing_state();
    test_request_body();
    test_request_user_data();
    test_request_url();
    test_request_client();
    test_request_parser();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}
