/**
 * @file test_middleware.cpp
 * @brief 中间件系统单元测试
 *
 * 测试编译时中间件宏的正确性：
 * - UVHTTP_EXECUTE_MIDDLEWARE (CONTINUE/STOP 路径、上下文清理、多次使用)
 * - UVHTTP_DEFINE_MIDDLEWARE_CHAIN + UVHTTP_EXECUTE_MIDDLEWARE_CHAIN
 * - UVHTTP_DEFINE_MIDDLEWARE_HANDLER + UVHTTP_MIDDLEWARE_HANDLER
 * - 中间件错误码
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_middleware.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_error.h"
}

#include <stdlib.h>
#include <string.h>

/* ========== 辅助中间件 ========== */

static int g_mw_call_count = 0;

static int counting_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                               uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    g_mw_call_count++;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int blocking_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                               uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    return UVHTTP_MIDDLEWARE_STOP;
}

static int g_cleanup_called = 0;

static void test_cleanup(void* data) {
    free(data);
    g_cleanup_called = 1;
}

static int alloc_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                            uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    if (!ctx->data) {
        ctx->data = malloc(64);
        ctx->cleanup = test_cleanup;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int alloc_and_stop_middleware(uvhttp_request_t* req,
                                     uvhttp_response_t* resp,
                                     uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    if (!ctx->data) {
        ctx->data = malloc(32);
        ctx->cleanup = test_cleanup;
    }
    return UVHTTP_MIDDLEWARE_STOP;
}

static int g_route_handler_called = 0;

static int test_route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    g_route_handler_called++;
    return 0;
}

static int failing_route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    return -1;
}

/* 调用顺序追踪 */
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

/* ========== HANDLER 包装器定义 ========== */

UVHTTP_DEFINE_MIDDLEWARE_HANDLER(test_route_handler);
UVHTTP_DEFINE_MIDDLEWARE_HANDLER(failing_route_handler);

/* ========== 测试 Fixture ========== */

class MiddlewareTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mw_call_count = 0;
        g_route_handler_called = 0;
        g_cleanup_called = 0;
        call_order_idx = 0;
        memset(call_order, 0, sizeof(call_order));
    }
};

/* ========== UVHTTP_EXECUTE_MIDDLEWARE 基础测试 ========== */

TEST_F(MiddlewareTest, ExecuteMiddleware_ContinuePath) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);

    EXPECT_EQ(g_mw_call_count, 1);
}

TEST_F(MiddlewareTest, ExecuteMiddleware_MultipleContinue) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        counting_middleware,
        counting_middleware,
        counting_middleware);

    EXPECT_EQ(g_mw_call_count, 3);
}

TEST_F(MiddlewareTest, ExecuteMiddleware_StopPath) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        blocking_middleware,
        counting_middleware);

    EXPECT_EQ(g_mw_call_count, 0);
}

TEST_F(MiddlewareTest, ExecuteMiddleware_StopThenContinue) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        counting_middleware,
        blocking_middleware,
        counting_middleware);

    EXPECT_EQ(g_mw_call_count, 1);
}

/* ========== 上下文清理测试 ========== */

TEST_F(MiddlewareTest, ContextCleanup_ContinuePath) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, alloc_middleware);

    EXPECT_EQ(g_cleanup_called, 1);
}

TEST_F(MiddlewareTest, ContextCleanup_StopPath) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, alloc_and_stop_middleware);

    EXPECT_EQ(g_cleanup_called, 1);
}

/* ========== 同一函数多次使用 ========== */

TEST_F(MiddlewareTest, MultipleUsagePerFunction) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);
    EXPECT_EQ(g_mw_call_count, 1);

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);
    EXPECT_EQ(g_mw_call_count, 2);
}

TEST_F(MiddlewareTest, MultipleUsageWithOrderTracking) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first);
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_second);

    EXPECT_EQ(call_order[0], 1);
    EXPECT_EQ(call_order[1], 2);
}

/* ========== UVHTTP_DEFINE_MIDDLEWARE_CHAIN ========== */

TEST_F(MiddlewareTest, DefineChain_Execute) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(test_chain,
        counting_middleware,
        counting_middleware);

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, test_chain);
    EXPECT_EQ(g_mw_call_count, 2);
}

TEST_F(MiddlewareTest, DefineChain_StopMidway) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(blocking_chain,
        counting_middleware,
        blocking_middleware,
        counting_middleware);

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, blocking_chain);
    EXPECT_EQ(g_mw_call_count, 1);
}

TEST_F(MiddlewareTest, DefineChain_CleanupOnContinue) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(cleanup_chain, alloc_middleware);

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, cleanup_chain);
    EXPECT_EQ(g_cleanup_called, 1);
}

/* ========== UVHTTP_MIDDLEWARE_HANDLER 包装测试 ========== */

TEST_F(MiddlewareTest, HandlerWrapper_Continue) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(test_route_handler));

    EXPECT_EQ(g_route_handler_called, 1);
}

TEST_F(MiddlewareTest, HandlerWrapper_StopOnNonZero) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(failing_route_handler),
        counting_middleware);

    EXPECT_EQ(g_mw_call_count, 0);
}

TEST_F(MiddlewareTest, HandlerWrapper_MixedWithMiddleware) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        counting_middleware,
        UVHTTP_MIDDLEWARE_HANDLER(test_route_handler));

    EXPECT_EQ(g_mw_call_count, 1);
    EXPECT_EQ(g_route_handler_called, 1);
}

/* ========== 错误码测试 ========== */

TEST_F(MiddlewareTest, ErrorCodes_AreDistinct) {
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_STOPPED, UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY);
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_STOPPED, UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY, UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_LT(UVHTTP_ERROR_MIDDLEWARE_STOPPED, 0);
}

TEST_F(MiddlewareTest, ErrorCodes_DescriptionNotNull) {
    const char* desc = uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_STOPPED);
    EXPECT_NE(desc, nullptr);
    EXPECT_STRNE(desc, "");

    desc = uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY);
    EXPECT_NE(desc, nullptr);

    desc = uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_NE(desc, nullptr);
}

TEST_F(MiddlewareTest, ErrorCodes_CategoryString) {
    const char* cat = uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_STOPPED);
    EXPECT_NE(cat, nullptr);
    EXPECT_STRNE(cat, "");
}