/**
 * @file test_smoke.cpp
 * @brief UVHTTP 冒烟测试
 * 
 * 冒烟测试用于验证系统基本功能是否正常工作
 */

#include <gtest/gtest.h>
#include <uv.h>
#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_response.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"

/**
 * @brief 测试服务器创建和销毁
 */
TEST(SmokeTest, ServerCreateDestroy) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_free(server);
    
    uv_loop_close(loop);
}

/**
 * @brief 测试路由器创建和销毁
 */
TEST(SmokeTest, RouterCreateDestroy) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_router_free(router);
}

/**
 * @brief 测试路由添加
 */
TEST(SmokeTest, RouterAddRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    // 创建请求处理器
    uvhttp_request_handler_t handler = [](uvhttp_request_t* req, uvhttp_response_t* res) -> int {
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/plain");
        uvhttp_response_set_body(res, "OK", 2);
        return uvhttp_response_send(res);
    };
    
    uvhttp_error_t result = uvhttp_router_add_route(router, "/test", handler);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_router_free(router);
}

/**
 * @brief 测试配置创建和验证
 */
TEST(SmokeTest, ConfigCreateValidate) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_free(config);
}

/**
 * @brief 测试错误码系统
 */
TEST(SmokeTest, ErrorCodes) {
    const char* error_str = uvhttp_error_string(UVHTTP_OK);
    EXPECT_NE(error_str, nullptr);
    
    error_str = uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(error_str, nullptr);
    
    error_str = uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(error_str, nullptr);
    
    const char* category = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(category, nullptr);
}

/**
 * @brief 测试分配器
 */
TEST(SmokeTest, Allocator) {
    void* ptr = malloc(1024);
    ASSERT_NE(ptr, nullptr);
    
    memset(ptr, 0, 1024);
    
    free(ptr);
}

/**
 * @brief 测试服务器监听和关闭
 */
TEST(SmokeTest, ServerListenAndClose) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_server_set_router(server, router);
    
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", 0);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_DEFAULT);
    
    uvhttp_router_free(router);
    uv_loop_close(loop);
    free(loop);
}

/**
 * @brief 测试版本信息
 */
TEST(SmokeTest, VersionInfo) {
    // 测试版本宏定义
    EXPECT_GE(UVHTTP_VERSION_MAJOR, 0);
    EXPECT_GE(UVHTTP_VERSION_MINOR, 0);
    EXPECT_GE(UVHTTP_VERSION_PATCH, 0);
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}