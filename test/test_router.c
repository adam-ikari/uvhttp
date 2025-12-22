/**
 * @file test_router.c
 * @brief 路由模块测试
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_router.h"
#include "../include/uvhttp_response.h"

static int test_handler_called = 0;
static uvhttp_request_t* last_request = NULL;
static uvhttp_response_t* last_response = NULL;

static int test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    test_handler_called++;
    last_request = request;
    last_response = response;
    return 0;
}

static int test_handler_404(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Not Found", 9);
    return 0;
}

TEST_FUNC(router_create_destroy) {
    uvhttp_router_t* router = uvhttp_router_new();
    TEST_ASSERT_NOT_NULL(router);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_add_route_simple) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    int result = uvhttp_router_add_route(router, "/", test_handler);
    TEST_ASSERT_EQ(0, result);
    
    result = uvhttp_router_add_route(router, "/api", test_handler);
    TEST_ASSERT_EQ(0, result);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_add_route_invalid) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    int result = uvhttp_router_add_route(NULL, "/", test_handler);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_router_add_route(router, NULL, test_handler);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_router_add_route(router, "/", NULL);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_find_handler_found) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/hello", test_handler);
    uvhttp_router_add_route(router, "/api/test", test_handler_404);
    
    /* 测试查找存在的路由 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/hello", "GET");
    TEST_ASSERT_NOT_NULL(handler);
    TEST_ASSERT_EQ(test_handler, handler);
    
    handler = uvhttp_router_find_handler(router, "/api/test", "POST");
    TEST_ASSERT_NOT_NULL(handler);
    TEST_ASSERT_EQ(test_handler_404, handler);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_find_handler_not_found) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/hello", test_handler);
    
    /* 测试查找不存在的路由 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/missing", "GET");
    TEST_ASSERT_NULL(handler);
    
    handler = uvhttp_router_find_handler(router, "/hello/world", "GET");
    TEST_ASSERT_NULL(handler);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_find_handler_different_methods) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route_method(router, "/api", UVHTTP_GET, test_handler);
    uvhttp_router_add_route_method(router, "/api", UVHTTP_POST, test_handler_404);
    
    /* 测试不同方法的路由 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api", "GET");
    TEST_ASSERT_NOT_NULL(handler);
    TEST_ASSERT_EQ(test_handler, handler);
    
    handler = uvhttp_router_find_handler(router, "/api", "POST");
    TEST_ASSERT_NOT_NULL(handler);
    TEST_ASSERT_EQ(test_handler_404, handler);
    
    handler = uvhttp_router_find_handler(router, "/api", "PUT");
    TEST_ASSERT_NULL(handler); /* PUT方法未注册 */
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_find_handler_invalid_params) {
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(NULL, "/", "GET");
    TEST_ASSERT_NULL(handler);
    
    uvhttp_router_t* router = uvhttp_router_new();
    handler = uvhttp_router_find_handler(router, NULL, "GET");
    TEST_ASSERT_NULL(handler);
    
    handler = uvhttp_router_find_handler(router, "/", NULL);
    TEST_ASSERT_NULL(handler);
    
    uvhttp_router_free(router);
    
    return 0;
}

TEST_FUNC(router_method_conversion) {
    /* 测试方法字符串转换 */
    uvhttp_method_t method = uvhttp_method_from_string("GET");
    TEST_ASSERT_EQ(UVHTTP_GET, method);
    
    method = uvhttp_method_from_string("POST");
    TEST_ASSERT_EQ(UVHTTP_POST, method);
    
    method = uvhttp_method_from_string("INVALID");
    TEST_ASSERT_EQ(UVHTTP_ANY, method);
    
    /* 测试方法转换回字符串 */
    const char* method_str = uvhttp_method_to_string(UVHTTP_GET);
    TEST_ASSERT_STREQ("GET", method_str);
    
    method_str = uvhttp_method_to_string(UVHTTP_POST);
    TEST_ASSERT_STREQ("POST", method_str);
    
    method_str = uvhttp_method_to_string((uvhttp_method_t)999);
    TEST_ASSERT_STREQ("UNKNOWN", method_str);
    
    return 0;
}

TEST_SUITE(router) {
    TEST_CASE(router_create_destroy);
    TEST_CASE(router_add_route_simple);
    TEST_CASE(router_add_route_invalid);
    TEST_CASE(router_find_handler_found);
    TEST_CASE(router_find_handler_not_found);
    TEST_CASE(router_find_handler_different_methods);
    TEST_CASE(router_find_handler_invalid_params);
    TEST_CASE(router_method_conversion);
    
    END_TEST_SUITE();
}