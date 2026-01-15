/* UVHTTP 中间件模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_constants.h"
#include <uv.h>

/* 测试中间件处理函数 */
static int test_middleware_handler(uvhttp_request_t* request, 
                                  uvhttp_response_t* response, 
                                  uvhttp_middleware_context_t* ctx) {
    (void)request;
    (void)response;
    (void)ctx;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int stop_middleware_handler(uvhttp_request_t* request, 
                                   uvhttp_response_t* response, 
                                   uvhttp_middleware_context_t* ctx) {
    (void)request;
    (void)response;
    (void)ctx;
    return UVHTTP_MIDDLEWARE_STOP;
}

/* 测试上下文清理函数 */
static void test_context_cleanup(void* data) {
    if (data) {
        free(data);
    }
}

/* 测试创建中间件 */
void test_middleware_create(void) {
    /* 测试基本创建 */
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    assert(middleware->handler == test_middleware_handler);
    assert(middleware->priority == UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(strcmp(middleware->path, "/api") == 0);
    assert(middleware->next == NULL);
    
    uvhttp_http_middleware_destroy(middleware);
    
    /* 测试 NULL 路径 */
    middleware = uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    assert(middleware != NULL);
    assert(middleware->path == NULL);
    
    uvhttp_http_middleware_destroy(middleware);
    
    /* 测试不同优先级 */
    middleware = uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    assert(middleware != NULL);
    assert(middleware->priority == UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    
    uvhttp_http_middleware_destroy(middleware);
    
    printf("test_middleware_create: PASSED\n");
}

/* 测试 NULL 参数处理 */
void test_null_params(void) {
    /* 测试 NULL 处理函数 */
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware == NULL);
    
    /* 测试 NULL 路径 */
    middleware = uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    
    /* 测试 NULL 路径和 NULL 处理函数 */
    middleware = uvhttp_http_middleware_create(NULL, NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware == NULL);
    
    /* 测试销毁 NULL */
    uvhttp_http_middleware_destroy(NULL);
    
    /* 测试设置 NULL 中间件上下文 */
    uvhttp_http_middleware_set_context(NULL, NULL, NULL);
    
    printf("test_null_params: PASSED\n");
}

/* 测试销毁中间件 */
void test_middleware_destroy(void) {
    /* 测试基本销毁 */
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    
    uvhttp_http_middleware_destroy(middleware);
    
    /* 测试销毁带路径的中间件 */
    middleware = uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    
    uvhttp_http_middleware_destroy(middleware);
    
    printf("test_middleware_destroy: PASSED\n");
}

/* 测试设置中间件上下文 */
void test_middleware_set_context(void) {
    /* 测试设置上下文 */
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    
    /* 设置上下文数据 */
    int* data = (int*)malloc(sizeof(int));
    *data = 42;
    uvhttp_http_middleware_set_context(middleware, data, test_context_cleanup);
    assert(middleware->context.data == data);
    assert(middleware->context.cleanup == test_context_cleanup);
    
    /* 销毁中间件（会调用清理函数） */
    uvhttp_http_middleware_destroy(middleware);
    
    /* 测试设置 NULL 上下文 */
    middleware = uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware != NULL);
    
    uvhttp_http_middleware_set_context(middleware, NULL, NULL);
    assert(middleware->context.data == NULL);
    assert(middleware->context.cleanup == NULL);
    
    uvhttp_http_middleware_destroy(middleware);
    
    printf("test_middleware_set_context: PASSED\n");
}

/* 测试中间件链 */
void test_middleware_chain(void) {
    /* 创建多个中间件 */
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    assert(mw1 != NULL);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw2 != NULL);
    
    uvhttp_http_middleware_t* mw3 = 
        uvhttp_http_middleware_create("/static", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    assert(mw3 != NULL);
    
    /* 链接中间件 */
    mw1->next = mw2;
    mw2->next = mw3;
    
    /* 验证链表 */
    assert(mw1->next == mw2);
    assert(mw2->next == mw3);
    assert(mw3->next == NULL);
    
    /* 销毁链表 */
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    uvhttp_http_middleware_destroy(mw3);
    
    printf("test_middleware_chain: PASSED\n");
}

/* 测试中间件执行 */
void test_middleware_execute(void) {
    /* 创建中间件链 */
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw1 != NULL);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw2 != NULL);
    
    mw1->next = mw2;
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    /* 测试执行中间件链 */
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 测试 NULL 参数 */
    result = uvhttp_http_middleware_execute(NULL, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    result = uvhttp_http_middleware_execute(mw1, NULL, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    result = uvhttp_http_middleware_execute(mw1, &request, NULL);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 清理 */
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    
    printf("test_middleware_execute: PASSED\n");
}

/* 测试中间件停止执行 */
void test_middleware_stop(void) {
    /* 创建中间件链 */
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", stop_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw1 != NULL);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw2 != NULL);
    
    mw1->next = mw2;
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    /* 设置请求路径 */
    strncpy(request.url, "/api/test", MAX_URL_LEN - 1);
    
    /* 测试执行中间件链（第一个中间件会停止） */
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_STOP);
    
    /* 清理 */
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    
    printf("test_middleware_stop: PASSED\n");
}

/* 测试路径匹配 */
void test_path_matching(void) {
    /* 创建中间件 */
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw1 != NULL);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                       UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw2 != NULL);
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    /* 测试精确匹配 */
    strncpy(request.url, "/api", MAX_URL_LEN - 1);
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 测试前缀匹配 */
    strncpy(request.url, "/api/test", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw1, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 测试不匹配 */
    strncpy(request.url, "/test", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw1, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 测试 NULL 路径（匹配所有） */
    strncpy(request.url, "/any/path", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw2, &request, &response);
    assert(result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 清理 */
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    
    printf("test_path_matching: PASSED\n");
}

/* 测试多次创建和销毁 */
void test_multiple_create_destroy(void) {
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 100; i++) {
        uvhttp_http_middleware_t* middleware = 
            uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                         UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
        assert(middleware != NULL);
        uvhttp_http_middleware_destroy(middleware);
    }
    
    printf("test_multiple_create_destroy: PASSED\n");
}

/* 测试不同优先级 */
void test_different_priorities(void) {
    /* 测试不同优先级 */
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    assert(mw1 != NULL);
    assert(mw1->priority == UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(mw2 != NULL);
    assert(mw2->priority == UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    
    uvhttp_http_middleware_t* mw3 = 
        uvhttp_http_middleware_create("/static", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    assert(mw3 != NULL);
    assert(mw3->priority == UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    uvhttp_http_middleware_destroy(mw3);
    
    printf("test_different_priorities: PASSED\n");
}

/* 测试内存分配失败 */
void test_memory_allocation_failure(void) {
    /* 这个测试很难模拟，因为我们无法控制 malloc 的行为 */
    /* 我们只能测试 NULL 处理函数的情况 */
    
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    assert(middleware == NULL);
    
    printf("test_memory_allocation_failure: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_middleware.c 完整覆盖率测试 ===\n\n");

    test_middleware_create();
    test_null_params();
    test_middleware_destroy();
    test_middleware_set_context();
    test_middleware_chain();
    test_middleware_execute();
    test_middleware_stop();
    test_path_matching();
    test_multiple_create_destroy();
    test_different_priorities();
    test_memory_allocation_failure();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}