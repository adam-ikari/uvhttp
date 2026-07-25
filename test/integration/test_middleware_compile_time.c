/**
 * @file test_middleware_compile_time.c
 * @brief 中间件编译时特性集成测试
 *
 * 验证中间件宏在真实编译环境下正确工作：
 * - 多次使用不冲突
 * - 与路由处理器集成
 * - 链式执行顺序正确
 * - 上下文清理正确
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp_middleware.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

/* --- 调用顺序追踪 --- */

static int call_order[8];
static int call_order_idx = 0;

static int mw_first(uvhttp_request_t* req, uvhttp_response_t* resp,
                     uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 1;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_second(uvhttp_request_t* req, uvhttp_response_t* resp,
                      uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 2;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_blocker(uvhttp_request_t* req, uvhttp_response_t* resp,
                       uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 3;
    return UVHTTP_MIDDLEWARE_STOP;
}

static int mw_after_block(uvhttp_request_t* req, uvhttp_response_t* resp,
                           uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 4;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* --- 上下文清理 --- */

static int g_cleanup_called = 0;

static void test_cleanup(void* data) {
    free(data);
    g_cleanup_called = 1;
}

static int mw_with_cleanup(uvhttp_request_t* req, uvhttp_response_t* resp,
                            uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    ctx->data = malloc(32);
    ctx->cleanup = test_cleanup;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_with_cleanup_stop(uvhttp_request_t* req,
                                 uvhttp_response_t* resp,
                                 uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    ctx->data = malloc(32);
    ctx->cleanup = test_cleanup;
    return UVHTTP_MIDDLEWARE_STOP;
}

/* --- 路由处理器 --- */

static int g_handler_result = 0;

static int route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    g_handler_result = 42;
    return 0;
}

UVHTTP_DEFINE_MIDDLEWARE_HANDLER(route_handler);

/* --- 测试函数 --- */

static void test_execute_continue(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first, mw_second);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: execute_continue\n");
}

static void test_execute_stop(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first, mw_blocker, mw_after_block);

    assert(call_order[0] == 1);
    assert(call_order[1] == 3);
    assert(call_order[2] == 0);
    printf("  PASS: execute_stop\n");
}

static void test_cleanup_continue(void) {
    g_cleanup_called = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_with_cleanup);

    assert(g_cleanup_called == 1);
    printf("  PASS: cleanup_continue\n");
}

static void test_cleanup_stop(void) {
    g_cleanup_called = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_with_cleanup_stop);

    assert(g_cleanup_called == 1);
    printf("  PASS: cleanup_stop\n");
}

static void test_multiple_usage(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first);
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_second);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: multiple_usage\n");
}

static void test_handler_wrapper(void) {
    g_handler_result = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(route_handler));

    assert(g_handler_result == 42);
    printf("  PASS: handler_wrapper\n");
}

static void test_chain(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(test_chain, mw_first, mw_second);

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, test_chain);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: chain\n");
}

static void test_chain_stop(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(block_chain,
        mw_first, mw_blocker, mw_after_block);

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, block_chain);

    assert(call_order[0] == 1);
    assert(call_order[1] == 3);
    assert(call_order[2] == 0);
    printf("  PASS: chain_stop\n");
}

int main(void) {
    printf("Middleware compile-time integration tests:\n");
    test_execute_continue();
    test_execute_stop();
    test_cleanup_continue();
    test_cleanup_stop();
    test_multiple_usage();
    test_handler_wrapper();
    test_chain();
    test_chain_stop();
    printf("All tests passed!\n");
    return 0;
}