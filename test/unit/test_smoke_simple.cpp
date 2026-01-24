/**
 * @file test_smoke_simple.cpp
 * @brief UVHTTP 冒烟测试（简化版）
 * 
 * 冒烟测试用于验证系统的基本功能是否正常工作。
 */

#include <gtest/gtest.h>
#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"

// 测试 1: 服务器创建和销毁
TEST(SmokeTest, ServerCreateDestroy) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_free(server);
}

// 测试 2: 路由器创建和销毁
TEST(SmokeTest, RouterCreateDestroy) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_router_free(router);
}

// 测试 3: 添加路由
TEST(SmokeTest, RouterAddRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    int result = uvhttp_router_add_route(router, "/test", [](uvhttp_request_t*, uvhttp_response_t*) { return 0; });
    EXPECT_EQ(result, 0);
    
    uvhttp_router_free(router);
}

// 测试 4: 配置创建和验证
TEST(SmokeTest, ConfigCreateValidate) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_connections = 100;
    config->max_requests_per_connection = 100;
    
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_free(config);
}

// 测试 5: 上下文创建和销毁
TEST(SmokeTest, ContextCreateDestroy) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    ASSERT_NE(context, nullptr);
    
    uvhttp_context_destroy(context);
}

// 测试 6: 错误码系统
TEST(SmokeTest, ErrorCodes) {
    const char* ok_str = uvhttp_error_string(UVHTTP_OK);
    ASSERT_NE(ok_str, nullptr);
    EXPECT_GT(strlen(ok_str), 0);
    
    const char* category = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NE(category, nullptr);
}

// 测试 7: 内存分配器
TEST(SmokeTest, Allocator) {
    void* ptr = uvhttp_alloc(1024);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_free(ptr);
}

// 测试 8: 响应构建
TEST(SmokeTest, ResponseBuild) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_response_set_status(&response, 200);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    EXPECT_EQ(result, UVHTTP_OK);
}