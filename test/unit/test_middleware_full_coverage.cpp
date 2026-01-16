#include <gtest/gtest.h>
#include "uvhttp_middleware.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 静态变量用于测试 */
static int g_middleware_call_count = 0;
static int g_middleware_stop = 0;
static int g_context_cleanup_count = 0;

/* 测试中间件处理函数 */
static int test_middleware_handler(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    g_middleware_call_count++;
    if (g_middleware_stop) {
        return UVHTTP_MIDDLEWARE_STOP;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 测试中间件处理函数 - 停止 */
static int test_middleware_handler_stop(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    g_middleware_call_count++;
    return UVHTTP_MIDDLEWARE_STOP;
}

/* 测试上下文清理函数 */
static void test_context_cleanup(void* data) {
    g_context_cleanup_count++;
}

/* 测试创建中间件 - 正常情况 */
TEST(UvhttpMiddlewareTest, CreateMiddlewareNormal) {
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    EXPECT_STREQ(middleware->path, "/api");
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(middleware->handler, test_middleware_handler);
    EXPECT_EQ(middleware->next, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试创建中间件 - NULL 路径 */
TEST(UvhttpMiddlewareTest, CreateMiddlewareNullPath) {
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        nullptr,
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    EXPECT_EQ(middleware->path, nullptr);
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试创建中间件 - NULL 处理函数 */
TEST(UvhttpMiddlewareTest, CreateMiddlewareNullHandler) {
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        nullptr,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    /* NULL 处理函数应该返回 NULL */
    EXPECT_EQ(middleware, nullptr);
}

/* 测试创建中间件 - 不同优先级 */
TEST(UvhttpMiddlewareTest, CreateMiddlewareDifferentPriorities) {
    uvhttp_http_middleware_t* low = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_LOW
    );
    
    uvhttp_http_middleware_t* normal = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    uvhttp_http_middleware_t* high = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    ASSERT_NE(low, nullptr);
    ASSERT_NE(normal, nullptr);
    ASSERT_NE(high, nullptr);
    
    EXPECT_EQ(low->priority, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    EXPECT_EQ(normal->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(high->priority, UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    
    uvhttp_http_middleware_destroy(low);
    uvhttp_http_middleware_destroy(normal);
    uvhttp_http_middleware_destroy(high);
}

/* 测试销毁中间件 - 正常情况 */
TEST(UvhttpMiddlewareTest, DestroyMiddlewareNormal) {
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    /* 测试销毁不会崩溃 */
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试销毁中间件 - NULL */
TEST(UvhttpMiddlewareTest, DestroyMiddlewareNull) {
    /* 测试销毁 NULL 不会崩溃 */
    uvhttp_http_middleware_destroy(nullptr);
}

/* 测试设置上下文 - 正常情况 */
TEST(UvhttpMiddlewareTest, SetContextNormal) {
    g_context_cleanup_count = 0;
    int* data = (int*)uvhttp_alloc(sizeof(int));
    *data = 42;
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_http_middleware_set_context(middleware, data, test_context_cleanup);
    
    EXPECT_EQ(middleware->context.data, data);
    EXPECT_EQ(middleware->context.cleanup, test_context_cleanup);
    
    uvhttp_http_middleware_destroy(middleware);
    EXPECT_EQ(g_context_cleanup_count, 1); /* 清理函数应该被调用 */
}

/* 测试设置上下文 - NULL 中间件 */
TEST(UvhttpMiddlewareTest, SetContextNullMiddleware) {
    int data = 42;
    
    /* 测试设置 NULL 中间件的上下文不会崩溃 */
    uvhttp_http_middleware_set_context(nullptr, &data, test_context_cleanup);
}

/* 测试设置上下文 - NULL 清理函数 */
TEST(UvhttpMiddlewareTest, SetContextNullCleanup) {
    int data = 42;
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_http_middleware_set_context(middleware, &data, nullptr);
    
    EXPECT_EQ(middleware->context.data, &data);
    EXPECT_EQ(middleware->context.cleanup, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试执行中间件链 - 单个中间件 */
TEST(UvhttpMiddlewareTest, ExecuteSingleMiddleware) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/test");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件 */
    int result = uvhttp_http_middleware_execute(middleware, &request, &response);
    
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    EXPECT_EQ(g_middleware_call_count, 1);
    
    uvhttp_http_middleware_destroy(middleware);
}

/* 测试执行中间件链 - 多个中间件 */
TEST(UvhttpMiddlewareTest, ExecuteMultipleMiddleware) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    uvhttp_http_middleware_t* m3 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_LOW
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    ASSERT_NE(m3, nullptr);
    
    /* 构建链表 */
    m1->next = m2;
    m2->next = m3;
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/test");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件链 */
    int result = uvhttp_http_middleware_execute(m1, &request, &response);
    
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    EXPECT_EQ(g_middleware_call_count, 3);
    
    uvhttp_http_middleware_destroy(m1);
    uvhttp_http_middleware_destroy(m2);
    uvhttp_http_middleware_destroy(m3);
}

/* 测试执行中间件链 - 停止执行 */
TEST(UvhttpMiddlewareTest, ExecuteMiddlewareStop) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler_stop,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    /* 构建链表 */
    m1->next = m2;
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/test");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件链 */
    int result = uvhttp_http_middleware_execute(m1, &request, &response);
    
    /* m1 被调用（返回 STOP），m2 没有被调用 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_STOP);
    EXPECT_EQ(g_middleware_call_count, 1);
    
    uvhttp_http_middleware_destroy(m1);
    uvhttp_http_middleware_destroy(m2);
}

/* 测试执行中间件链 - 路径匹配 */
TEST(UvhttpMiddlewareTest, ExecuteMiddlewarePathMatch) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/api/users",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    /* 构建链表 */
    m1->next = m2;
    
    /* 创建请求和响应 - 请求路径为 /api/users */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/users");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件链 */
    int result = uvhttp_http_middleware_execute(m1, &request, &response);
    
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    /* 两个中间件都应该被调用，因为 /api/users 前缀匹配 /api */
    EXPECT_EQ(g_middleware_call_count, 2);
    
    uvhttp_http_middleware_destroy(m1);
    uvhttp_http_middleware_destroy(m2);
}

/* 测试执行中间件链 - NULL 路径匹配所有 */
TEST(UvhttpMiddlewareTest, ExecuteMiddlewareNullPathMatchAll) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        nullptr,
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    ASSERT_NE(m1, nullptr);
    
    /* 创建请求和响应 - 任意路径 */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/any/path");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件链 */
    int result = uvhttp_http_middleware_execute(m1, &request, &response);
    
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    EXPECT_EQ(g_middleware_call_count, 1);
    
    uvhttp_http_middleware_destroy(m1);
}

/* 测试执行中间件链 - 路径不匹配 */
TEST(UvhttpMiddlewareTest, ExecuteMiddlewarePathNoMatch) {
    g_middleware_call_count = 0;
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/users",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    /* 构建链表 */
    m1->next = m2;
    
    /* 创建请求和响应 - 请求路径为 /other */
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/other");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 执行中间件链 */
    int result = uvhttp_http_middleware_execute(m1, &request, &response);
    
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    /* 没有中间件匹配，调用次数为 0 */
    EXPECT_EQ(g_middleware_call_count, 0);
    
    uvhttp_http_middleware_destroy(m1);
    uvhttp_http_middleware_destroy(m2);
}

/* 测试执行中间件链 - NULL 参数 */
TEST(UvhttpMiddlewareTest, ExecuteMiddlewareNullParams) {
    /* 测试 NULL 参数不会崩溃 */
    int result = uvhttp_http_middleware_execute(nullptr, nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
}

/* 测试向服务器添加中间件 */
TEST(UvhttpMiddlewareTest, AddMiddlewareToServer) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_error_t result = uvhttp_server_add_middleware(server, middleware);
    
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->middleware_chain, middleware);
    
    uvhttp_server_free(server);
}

/* 测试向服务器添加多个中间件 */
TEST(UvhttpMiddlewareTest, AddMultipleMiddlewareToServer) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/users",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    uvhttp_server_add_middleware(server, m1);
    uvhttp_server_add_middleware(server, m2);
    
    /* 高优先级的中间件应该在前面 */
    EXPECT_EQ(server->middleware_chain, m2);
    EXPECT_EQ(m2->next, m1);
    
    uvhttp_server_free(server);
}

/* 测试从服务器移除中间件 */
TEST(UvhttpMiddlewareTest, RemoveMiddlewareFromServer) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_http_middleware_t* middleware = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_server_add_middleware(server, middleware);
    
    uvhttp_error_t result = uvhttp_server_remove_middleware(server, "/api");
    
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->middleware_chain, nullptr);
    
    uvhttp_server_free(server);
}

/* 测试从服务器移除不存在的中间件 */
TEST(UvhttpMiddlewareTest, RemoveNonExistentMiddleware) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_error_t result = uvhttp_server_remove_middleware(server, "/api");
    
    EXPECT_EQ(result, UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND);
    
    uvhttp_server_free(server);
}

/* 测试清理服务器中间件链 */
TEST(UvhttpMiddlewareTest, CleanupServerMiddleware) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_http_middleware_t* m1 = uvhttp_http_middleware_create(
        "/api",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    uvhttp_http_middleware_t* m2 = uvhttp_http_middleware_create(
        "/users",
        test_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    ASSERT_NE(m1, nullptr);
    ASSERT_NE(m2, nullptr);
    
    uvhttp_server_add_middleware(server, m1);
    uvhttp_server_add_middleware(server, m2);
    
    /* 清理中间件链 */
    uvhttp_server_cleanup_middleware(server);
    
    EXPECT_EQ(server->middleware_chain, nullptr);
    
    uvhttp_server_free(server);
}

/* 测试清理空中间件链 */
TEST(UvhttpMiddlewareTest, CleanupEmptyMiddlewareChain) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    /* 清理空中间件链不会崩溃 */
    uvhttp_server_cleanup_middleware(server);
    
    EXPECT_EQ(server->middleware_chain, nullptr);
    
    uvhttp_server_free(server);
}

/* 测试清理 NULL 服务器的中间件链 */
TEST(UvhttpMiddlewareTest, CleanupNullServerMiddleware) {
    /* 清理 NULL 服务器的中间件链不会崩溃 */
    uvhttp_server_cleanup_middleware(nullptr);
}