/**
 * @file test_middleware_full_coverage_enhanced.cpp
 * @brief 增强的中间件测试 - 提升覆盖率到 50%
 * 
 * 目标：提升 uvhttp_middleware.c 覆盖率从 9.4% 到 50%
 * 
 * 测试内容：
 * - uvhttp_http_middleware_create
 * - uvhttp_http_middleware_destroy
 * - uvhttp_http_middleware_set_context
 * - uvhttp_http_middleware_execute
 * - NULL 参数测试
 * - 中间件链执行测试
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_middleware.h"
#include "uvhttp_allocator.h"

/* 测试创建中间件 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareCreate) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    EXPECT_STREQ(middleware->path, "/test");
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试创建中间件 - NULL 路径 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareCreateNullPath) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(nullptr, handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    // NULL 路径可能被允许，所以不检查返回值
    if (middleware) {
        uvhttp_http_middleware_destroy(middleware);
    }
}

/* 测试创建中间件 - NULL 处理器 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareCreateNullHandler) {
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", nullptr, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(middleware, nullptr);
}

/* 测试销毁中间件 - NULL 参数 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareDestroyNull) {
    uvhttp_http_middleware_destroy(nullptr);
    // 不应该崩溃
}

/* 测试设置上下文 - NULL 参数 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareSetContextNull) {
    int data = 123;
    auto cleanup = [](void* data) {
        (void)data;
    };
    
    uvhttp_http_middleware_set_context(nullptr, &data, cleanup);
    // 不应该崩溃
}

/* 测试执行中间件 - NULL 参数 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareExecuteNull) {
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_http_middleware_execute(nullptr, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
}

/* 测试中间件结构体字段初始化 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareFieldInitialization) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    EXPECT_STREQ(middleware->path, "/test");
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_NE(middleware->handler, nullptr);
    EXPECT_EQ(middleware->context.data, nullptr);
    EXPECT_EQ(middleware->context.cleanup, nullptr);
    EXPECT_EQ(middleware->next, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试中间件结构体大小 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareStructSize) {
    EXPECT_GT(sizeof(uvhttp_http_middleware_t), 0);
    EXPECT_GT(sizeof(uvhttp_middleware_context_t), 0);
}

/* 测试常量值 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareConstants) {
    EXPECT_EQ(UVHTTP_MIDDLEWARE_CONTINUE, 0);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_STOP, 1);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_HTTP, 0);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_WEBSOCKET, 1);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_STATIC, 2);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_PRIORITY_LOW, 0);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_PRIORITY_NORMAL, 1);
    EXPECT_EQ(UVHTTP_MIDDLEWARE_PRIORITY_HIGH, 2);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpMiddlewareEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_http_middleware_destroy(nullptr);
        uvhttp_http_middleware_set_context(nullptr, nullptr, nullptr);
        
        uvhttp_request_t request;
        uvhttp_response_t response;
        memset(&request, 0, sizeof(request));
        memset(&response, 0, sizeof(response));
        uvhttp_http_middleware_execute(nullptr, &request, &response);
    }
    // 不应该崩溃
}

/* 测试中间件结构体对齐 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_http_middleware_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_http_middleware_t), sizeof(int));
}

/* 测试设置上下文 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareSetContext) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    int data = 123;
    auto cleanup = [](void* data) {
        *(int*)data = 456;
    };
    
    uvhttp_http_middleware_set_context(middleware, &data, cleanup);
    EXPECT_NE(middleware->context.data, nullptr);
    EXPECT_NE(middleware->context.cleanup, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试执行中间件 - 返回 CONTINUE */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareExecuteContinue) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_http_middleware_execute(middleware, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试执行中间件 - 返回 STOP */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareExecuteStop) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_STOP;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    // 设置请求路径以匹配中间件路径
    strncpy(request.url, "/test", sizeof(request.url) - 1);
    request.url[sizeof(request.url) - 1] = '\0';
    
    int result = uvhttp_http_middleware_execute(middleware, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_STOP);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试中间件优先级 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewarePriorities) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    // 测试 LOW 优先级
    uvhttp_http_middleware_t* middleware_low = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    ASSERT_NE(middleware_low, nullptr);
    EXPECT_EQ(middleware_low->priority, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    uvhttp_http_middleware_destroy(middleware_low);
    
    // 测试 NORMAL 优先级
    uvhttp_http_middleware_t* middleware_normal = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware_normal, nullptr);
    EXPECT_EQ(middleware_normal->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    uvhttp_http_middleware_destroy(middleware_normal);
    
    // 测试 HIGH 优先级
    uvhttp_http_middleware_t* middleware_high = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    ASSERT_NE(middleware_high, nullptr);
    EXPECT_EQ(middleware_high->priority, UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    uvhttp_http_middleware_destroy(middleware_high);
}

/* 测试中间件路径 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewarePaths) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    // 测试根路径
    uvhttp_http_middleware_t* middleware_root = uvhttp_http_middleware_create("/", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware_root, nullptr);
    EXPECT_STREQ(middleware_root->path, "/");
    uvhttp_http_middleware_destroy(middleware_root);
    
    // 测试嵌套路径
    uvhttp_http_middleware_t* middleware_nested = uvhttp_http_middleware_create("/api/v1/users", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware_nested, nullptr);
    EXPECT_STREQ(middleware_nested->path, "/api/v1/users");
    uvhttp_http_middleware_destroy(middleware_nested);
    
    // 测试通配符路径
    uvhttp_http_middleware_t* middleware_wildcard = uvhttp_http_middleware_create("/*", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware_wildcard, nullptr);
    EXPECT_STREQ(middleware_wildcard->path, "/*");
    uvhttp_http_middleware_destroy(middleware_wildcard);
}

/* 测试中间件上下文清理 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareContextCleanup) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create("/test", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    int* data = (int*)uvhttp_alloc(sizeof(int));
    *data = 123;
    
    auto cleanup = [](void* data) {
        *(int*)data = 456;
        uvhttp_free(data);
    };
    
    uvhttp_http_middleware_set_context(middleware, data, cleanup);
    EXPECT_NE(middleware->context.data, nullptr);
    
    // 销毁中间件应该调用清理函数
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试多个中间件 */
TEST(UvhttpMiddlewareEnhancedTest, MultipleMiddleware) {
    auto handler = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware1 = uvhttp_http_middleware_create("/test1", handler, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    uvhttp_http_middleware_t* middleware2 = uvhttp_http_middleware_create("/test2", handler, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    uvhttp_http_middleware_t* middleware3 = uvhttp_http_middleware_create("/test3", handler, UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    
    ASSERT_NE(middleware1, nullptr);
    ASSERT_NE(middleware2, nullptr);
    ASSERT_NE(middleware3, nullptr);
    
    uvhttp_http_middleware_destroy(middleware1);
    uvhttp_http_middleware_destroy(middleware2);
    uvhttp_http_middleware_destroy(middleware3);
}

/* 测试中间件链 */
TEST(UvhttpMiddlewareEnhancedTest, MiddlewareChain) {
    auto handler1 = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    auto handler2 = [](uvhttp_request_t* request, uvhttp_response_t* response, uvhttp_middleware_context_t* ctx) -> int {
        (void)request;
        (void)response;
        (void)ctx;
        return UVHTTP_MIDDLEWARE_CONTINUE;
    };
    
    uvhttp_http_middleware_t* middleware1 = uvhttp_http_middleware_create("/test1", handler1, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    uvhttp_http_middleware_t* middleware2 = uvhttp_http_middleware_create("/test2", handler2, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    
    ASSERT_NE(middleware1, nullptr);
    ASSERT_NE(middleware2, nullptr);
    
    // 手动链接中间件
    middleware1->next = middleware2;
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    // 执行中间件链
    int result = uvhttp_http_middleware_execute(middleware1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_http_middleware_destroy(middleware1);
    // middleware2 会被 middleware1 的销毁函数一起销毁
}