/* uvhttp_router.c 扩展覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_router.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 简单的请求处理器 */
int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    return 0;
}

/* 另一个请求处理器 */
int another_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    return 0;
}

TEST(UvhttpRouterExtraCoverageTest, RouterNewAndFree) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_router_free(router);
    uvhttp_router_free(nullptr);
}

TEST(UvhttpRouterExtraCoverageTest, RouterAddRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加简单路由 */
    uvhttp_error_t result = uvhttp_router_add_route(router, "/api/users", simple_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 添加另一个路由 */
    result = uvhttp_router_add_route(router, "/api/posts", another_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 添加带参数的路由 */
    result = uvhttp_router_add_route(router, "/api/users/:id", simple_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    result = uvhttp_router_add_route(nullptr, "/api/test", simple_handler);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 路径 */
    result = uvhttp_router_add_route(router, nullptr, simple_handler);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 处理器 */
    result = uvhttp_router_add_route(router, "/api/test", nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterAddRouteMethod) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加 GET 路由 */
    uvhttp_error_t result = uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, simple_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 添加 POST 路由 */
    result = uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, another_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 添加 PUT 路由 */
    result = uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_PUT, simple_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 添加 DELETE 路由 */
    result = uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_DELETE, another_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    result = uvhttp_router_add_route_method(nullptr, "/api/test", UVHTTP_GET, simple_handler);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 路径 */
    result = uvhttp_router_add_route_method(router, nullptr, UVHTTP_GET, simple_handler);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 处理器 */
    result = uvhttp_router_add_route_method(router, "/api/test", UVHTTP_GET, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterFindHandler) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/api/users", simple_handler);
    uvhttp_router_add_route(router, "/api/posts", another_handler);
    
    /* 查找存在的路由 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api/users", "GET");
    EXPECT_NE(handler, nullptr);
    
    handler = uvhttp_router_find_handler(router, "/api/posts", "GET");
    EXPECT_NE(handler, nullptr);
    
    /* 查找不存在的路由 */
    handler = uvhttp_router_find_handler(router, "/api/comments", "GET");
    EXPECT_EQ(handler, nullptr);
    
    /* 测试 NULL 路由器 */
    handler = uvhttp_router_find_handler(nullptr, "/api/test", "GET");
    EXPECT_EQ(handler, nullptr);
    
    /* 测试 NULL 路径 */
    handler = uvhttp_router_find_handler(router, nullptr, "GET");
    EXPECT_EQ(handler, nullptr);
    
    /* 测试 NULL 方法 */
    handler = uvhttp_router_find_handler(router, "/api/test", nullptr);
    EXPECT_EQ(handler, nullptr);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterMatch) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/api/users", simple_handler);
    uvhttp_router_add_route(router, "/api/users/:id", another_handler);
    uvhttp_router_add_route(router, "/api/posts/:id/comments/:commentId", simple_handler);
    
    /* 匹配简单路由 */
    uvhttp_route_match_t match;
    uvhttp_error_t result = uvhttp_router_match(router, "/api/users", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    /* 匹配带参数的路由 */
    result = uvhttp_router_match(router, "/api/users/123", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    /* 匹配多个参数的路由 */
    result = uvhttp_router_match(router, "/api/posts/456/comments/789", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    /* 测试不存在的路由 */
    result = uvhttp_router_match(router, "/api/comments", "GET", &match);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    result = uvhttp_router_match(nullptr, "/api/test", "GET", &match);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 路径 */
    result = uvhttp_router_match(router, nullptr, "GET", &match);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 方法 */
    result = uvhttp_router_match(router, "/api/test", nullptr, &match);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 匹配结果 */
    result = uvhttp_router_match(router, "/api/test", "GET", nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, ParsePathParams) {
    uvhttp_param_t params[MAX_PARAMS];
    size_t param_count = 0;
    
    /* 解析带参数的路径（使用 :name:value 格式） */
    uvhttp_error_t result = uvhttp_parse_path_params("/api/users/:id:123/posts/:postId:456", params, &param_count);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 解析简单路径（无参数） */
    param_count = 0;
    result = uvhttp_parse_path_params("/api/users", params, &param_count);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试 NULL 路径 */
    result = uvhttp_parse_path_params(nullptr, params, &param_count);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 参数数组 */
    result = uvhttp_parse_path_params("/api/users/:id", nullptr, &param_count);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 参数计数 */
    result = uvhttp_parse_path_params("/api/users/:id", params, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpRouterExtraCoverageTest, MethodFromString) {
    uvhttp_method_t method;
    
    method = uvhttp_method_from_string("GET");
    EXPECT_EQ(method, UVHTTP_GET);
    
    method = uvhttp_method_from_string("POST");
    EXPECT_EQ(method, UVHTTP_POST);
    
    method = uvhttp_method_from_string("PUT");
    EXPECT_EQ(method, UVHTTP_PUT);
    
    method = uvhttp_method_from_string("DELETE");
    EXPECT_EQ(method, UVHTTP_DELETE);
    
    method = uvhttp_method_from_string("HEAD");
    EXPECT_EQ(method, UVHTTP_HEAD);
    
    method = uvhttp_method_from_string("OPTIONS");
    EXPECT_EQ(method, UVHTTP_OPTIONS);
    
    method = uvhttp_method_from_string("PATCH");
    EXPECT_EQ(method, UVHTTP_PATCH);
    
    /* 测试不支持的 HTTP 方法 */
    method = uvhttp_method_from_string("UNKNOWN");
    EXPECT_EQ(method, UVHTTP_ANY);
    
    /* 测试 NULL 方法 */
    method = uvhttp_method_from_string(nullptr);
    EXPECT_EQ(method, UVHTTP_ANY);
}

TEST(UvhttpRouterExtraCoverageTest, MethodToString) {
    const char* method;
    
    method = uvhttp_method_to_string(UVHTTP_GET);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_POST);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_PUT);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_DELETE);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_HEAD);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_OPTIONS);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_PATCH);
    ASSERT_NE(method, nullptr);
    
    method = uvhttp_method_to_string(UVHTTP_ANY);
    ASSERT_NE(method, nullptr);
}

TEST(UvhttpRouterExtraCoverageTest, RouterAddStaticRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加静态文件路由 */
    uvhttp_error_t result = uvhttp_router_add_static_route(router, "/static", (void*)0x12345678);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    result = uvhttp_router_add_static_route(nullptr, "/static", (void*)0x12345678);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 前缀路径 */
    result = uvhttp_router_add_static_route(router, nullptr, (void*)0x12345678);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterAddFallbackRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加回退路由 */
    uvhttp_error_t result = uvhttp_router_add_fallback_route(router, (void*)0x12345678);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    result = uvhttp_router_add_fallback_route(nullptr, (void*)0x12345678);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterMultipleRoutes) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加多个路由 */
    for (int i = 0; i < 10; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/api/route%d", i);
        uvhttp_router_add_route(router, path, simple_handler);
    }
    
    /* 验证路由可以找到 */
    for (int i = 0; i < 10; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/api/route%d", i);
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, path, "GET");
        EXPECT_NE(handler, nullptr);
    }
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterComplexPaths) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加复杂路径 */
    uvhttp_router_add_route(router, "/api/v1/users/:userId/posts/:postId/comments/:commentId", simple_handler);
    uvhttp_router_add_route(router, "/api/v2/users/:userId/profile", another_handler);
    
    /* 匹配复杂路径 */
    uvhttp_route_match_t match;
    uvhttp_error_t result = uvhttp_router_match(router, "/api/v1/users/123/posts/456/comments/789", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    result = uvhttp_router_match(router, "/api/v2/users/123/profile", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterBoundaryValues) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 测试根路径 */
    uvhttp_error_t result = uvhttp_router_add_route(router, "/", simple_handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试长路径 */
    char long_path[300];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    result = uvhttp_router_add_route(router, long_path, simple_handler);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterMemoryLeaks) {
    for (int i = 0; i < 100; i++) {
        uvhttp_router_t* router = uvhttp_router_new();
        ASSERT_NE(router, nullptr);
        
        uvhttp_router_add_route(router, "/api/users", simple_handler);
        uvhttp_router_add_route(router, "/api/posts", another_handler);
        uvhttp_router_add_route(router, "/api/comments", simple_handler);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterExtraCoverageTest, RouterDifferentMethods) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 为同一路径添加不同方法的路由 */
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, simple_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, another_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_PUT, simple_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_DELETE, another_handler);
    
    /* 验证不同方法都能找到正确的处理器 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api/users", "GET");
    EXPECT_NE(handler, nullptr);
    
    handler = uvhttp_router_find_handler(router, "/api/users", "POST");
    EXPECT_NE(handler, nullptr);
    
    handler = uvhttp_router_find_handler(router, "/api/users", "PUT");
    EXPECT_NE(handler, nullptr);
    
    handler = uvhttp_router_find_handler(router, "/api/users", "DELETE");
    EXPECT_NE(handler, nullptr);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterSpecialChars) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 测试特殊字符 */
    uvhttp_router_add_route(router, "/api/users-with-dash", simple_handler);
    uvhttp_router_add_route(router, "/api/users_with_underscore", another_handler);
    
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api/users-with-dash", "GET");
    EXPECT_NE(handler, nullptr);
    
    handler = uvhttp_router_find_handler(router, "/api/users_with_underscore", "GET");
    EXPECT_NE(handler, nullptr);
    
    uvhttp_router_free(router);
}

TEST(UvhttpRouterExtraCoverageTest, RouterMaxParams) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    /* 添加带最大参数数量的路由 */
    uvhttp_router_add_route(router, "/api/:p1/:p2/:p3/:p4/:p5/:p6/:p7/:p8/:p9/:p10/:p11/:p12/:p13/:p14/:p15/:p16", simple_handler);
    
    /* 匹配最大参数数量的路由 */
    uvhttp_route_match_t match;
    uvhttp_error_t result = uvhttp_router_match(router, "/api/1/2/3/4/5/6/7/8/9/10/11/12/13/14/15/16", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(match.handler, nullptr);
    
    uvhttp_router_free(router);
}
