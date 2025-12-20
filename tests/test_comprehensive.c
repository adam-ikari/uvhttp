#include "../deps/googletest/gtest_fixed.h"
#include "../include/uvhttp.h"
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_server.h"
#include "../include/uvhttp_request.h"
#include "../include/uvhttp_response.h"
#include "../include/uvhttp_router.h"
#include <stdlib.h>
#include <string.h>

// 测试工具函数
TEST(UtilsTest, SafeStrncpy) {
    char dest[100];
    
    EXPECT_EQ(safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
    
    EXPECT_EQ(safe_strncpy(NULL, "hello", sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, "hello", 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(UtilsTest, ValidateUrl) {
    EXPECT_EQ(validate_url("http://example.com", 18), 0);
    EXPECT_EQ(validate_url("/api/v1/users", 14), 0);
    EXPECT_EQ(validate_url("", 0), -1);
    EXPECT_EQ(validate_url(NULL, 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(UtilsTest, ValidateMethod) {
    EXPECT_EQ(validate_method("GET", 3), 0);
    EXPECT_EQ(validate_method("POST", 4), 0);
    EXPECT_EQ(validate_method(NULL, 3), -1);
    EXPECT_EQ(validate_method("", 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试请求模块
TEST(RequestTest, Initialization) {
    uvhttp_request_t request;
    
    EXPECT_EQ(uvhttp_request_init(&request, NULL), 0);
    EXPECT_EQ(request.method, UVHTTP_GET);
    EXPECT_NOTNULL_PTR(request.body);
    EXPECT_EQ(request.body_length, 0);
    
    uvhttp_request_cleanup(&request);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(RequestTest, MethodParsing) {
    uvhttp_request_t request;
    
    EXPECT_EQ(uvhttp_request_init(&request, NULL), 0);
    
    // 测试不同的方法
    request.method = UVHTTP_GET;
    EXPECT_STREQ(uvhttp_request_get_method(&request), "GET");
    
    request.method = UVHTTP_POST;
    EXPECT_STREQ(uvhttp_request_get_method(&request), "POST");
    
    uvhttp_request_cleanup(&request);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试响应模块
TEST(ResponseTest, Initialization) {
    uvhttp_response_t response;
    
    // 修复：传递有效的客户端指针
    EXPECT_EQ(uvhttp_response_init(&response, (void*)0x1), 0);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.finished, 0);
    
    uvhttp_response_cleanup(&response);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(ResponseTest, StatusAndHeaders) {
    uvhttp_response_t response;
    
    // 修复：传递有效的客户端指针
    EXPECT_EQ(uvhttp_response_init(&response, (void*)0x1), 0);
    
    // 测试状态码
    uvhttp_response_set_status(&response, 404);
    EXPECT_EQ(response.status_code, 404);
    
    // 测试头部
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    EXPECT_EQ(response.header_count, 1);
    EXPECT_STREQ(response.headers[0].name, "Content-Type");
    EXPECT_STREQ(response.headers[0].value, "application/json");
    
    uvhttp_response_cleanup(&response);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试服务器模块
TEST(ServerTest, CreationAndDestruction) {
    struct uvhttp_server* server = uvhttp_server_new(NULL);
    
    EXPECT_NOTNULL_PTR(server);
    EXPECT_EQ(server->is_listening, 0);
    EXPECT_EQ(server->active_connections, 0);
    
    uvhttp_server_free(server);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(ServerTest, HandlerManagement) {
    struct uvhttp_server* server = uvhttp_server_new(NULL);
    
    void test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
        uvhttp_response_set_status(response, 200);
    }
    
    uvhttp_server_set_handler(server, test_handler);
    EXPECT_EQ_PTR(server->handler, test_handler);
    
    uvhttp_server_free(server);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试路由模块
TEST(RouterTest, BasicOperations) {
    uvhttp_router_t* router = uvhttp_router_new();
    
    EXPECT_NOTNULL_PTR(router);
    EXPECT_EQ(router->route_count, 0);
    
    void test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
        uvhttp_response_set_status(response, 200);
    }
    
    EXPECT_EQ(uvhttp_router_add_route(router, "/test", test_handler), 0);
    EXPECT_EQ(router->route_count, 1);
    
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/test");
    EXPECT_EQ_PTR(handler, test_handler);
    
    handler = uvhttp_router_find_handler(router, "/nonexistent");
    EXPECT_EQ_PTR(handler, NULL);
    
    uvhttp_router_free(router);
    
TEST_CLEANUP_LABEL:
    return;
}

RUN_ALL_TESTS()