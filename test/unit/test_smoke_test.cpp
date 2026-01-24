/**
 * @file test_smoke.cpp
 * @brief UVHTTP 冒烟测试
 * 
 * 冒烟测试用于验证系统的基本功能是否正常工作。
 * 这些测试应该在每次代码变更后快速运行，确保没有破坏核心功能。
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// 测试结果
static int g_tests_passed = 0;
static int g_tests_failed = 0;

// 测试宏
#define TEST_START(name) \
    printf("[TEST] %s\n", name);

#define TEST_PASS() \
    do { \
        g_tests_passed++; \
        printf("  [PASS]\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        g_tests_failed++; \
        printf("  [FAIL] %s\n", msg); \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, msg) \
    ASSERT_TRUE((ptr) != NULL, msg)

#define ASSERT_EQ(a, b, msg) \
    ASSERT_TRUE((a) == (b), msg)

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static int g_server_port = 18080;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

// 简单处理器
int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* body = "OK";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, 2);
    uvhttp_response_send(response);
    
    return 0;
}

// 测试 1: 服务器创建和销毁
void test_server_create_destroy() {
    TEST_START("Server Create/Destroy");
    
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NOT_NULL(loop, "Failed to get default loop");
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NOT_NULL(server, "Failed to create server");
    
    uvhttp_server_free(server);
    
    TEST_PASS();
}

// 测试 2: 路由器创建和销毁
void test_router_create_destroy() {
    TEST_START("Router Create/Destroy");
    
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NOT_NULL(router, "Failed to create router");
    
    uvhttp_router_free(router);
    
    TEST_PASS();
}

// 测试 3: 添加和移除路由
void test_router_add_remove() {
    TEST_START("Router Add/Remove");
    
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NOT_NULL(router, "Failed to create router");
    
    int result = uvhttp_router_add_route(router, "/test", simple_handler);
    ASSERT_EQ(result, 0, "Failed to add route");
    
    uvhttp_router_free(router);
    
    TEST_PASS();
}

// 测试 4: 服务器启动和停止
void test_server_start_stop() {
    TEST_START("Server Start/Stop");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NOT_NULL(server, "Failed to create server");
    
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NOT_NULL(router, "Failed to create router");
    
    uvhttp_router_add_route(router, "/", simple_handler);
    server->router = router;
    
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", g_server_port);
    ASSERT_EQ(result, UVHTTP_OK, "Failed to start server");
    
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    TEST_PASS();
}

// 测试 5: 配置创建和验证
void test_config_create_validate() {
    TEST_START("Config Create/Validate");
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NOT_NULL(config, "Failed to create config");
    
    // 设置基本配置
    config->max_connections = 100;
    config->max_requests_per_connection = 100;
    
    uvhttp_error_t result = uvhttp_config_validate(config);
    ASSERT_EQ(result, UVHTTP_OK, "Config validation failed");
    
    uvhttp_config_free(config);
    
    TEST_PASS();
}

// 测试 6: 上下文创建和销毁
void test_context_create_destroy() {
    TEST_START("Context Create/Destroy");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    ASSERT_NOT_NULL(context, "Failed to create context");
    
    uvhttp_context_destroy(context);
    
    TEST_PASS();
}

// 测试 7: 错误码系统
void test_error_codes() {
    TEST_START("Error Codes");
    
    const char* ok_str = uvhttp_error_string(UVHTTP_OK);
    ASSERT_NOT_NULL(ok_str, "Failed to get error string for UVHTTP_OK");
    
    const char* category = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NOT_NULL(category, "Failed to get error category");
    
    TEST_PASS();
}

// 测试 8: 内存分配器
void test_allocator() {
    TEST_START("Allocator");
    
    void* ptr = uvhttp_alloc(1024);
    ASSERT_NOT_NULL(ptr, "Failed to allocate memory");
    
    uvhttp_free(ptr);
    
    TEST_PASS();
}

// 测试 9: 响应构建
void test_response_build() {
    TEST_START("Response Build");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_response_t response;
    
    // 初始化响应（模拟）
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_response_set_status(&response, 200);
    ASSERT_EQ(result, UVHTTP_OK, "Failed to set status");
    
    result = uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    ASSERT_EQ(result, UVHTTP_OK, "Failed to set header");
    
    TEST_PASS();
}

// 测试 10: 特性宏
void test_feature_macros() {
    TEST_START("Feature Macros");
    
#ifdef UVHTTP_FEATURE_WEBSOCKET
    printf("  WebSocket: Enabled\n");
#else
    printf("  WebSocket: Disabled\n");
#endif

#ifdef UVHTTP_FEATURE_TLS
    printf("  TLS: Enabled\n");
#else
    printf("  TLS: Disabled\n");
#endif

#ifdef UVHTTP_FEATURE_STATIC_FILES
    printf("  Static Files: Enabled\n");
#else
    printf("  Static Files: Disabled\n");
#endif
    
    TEST_PASS();
}

// 主函数
int main() {
    printf("========================================\n");
    printf("      UVHTTP Smoke Tests\n");
    printf("========================================\n\n");
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 运行测试
    test_server_create_destroy();
    test_router_create_destroy();
    test_router_add_remove();
    test_server_start_stop();
    test_config_create_validate();
    test_context_create_destroy();
    test_error_codes();
    test_allocator();
    test_response_build();
    test_feature_macros();
    
    // 打印结果
    printf("\n========================================\n");
    printf("      Test Results\n");
    printf("========================================\n");
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_failed);
    printf("Total:  %d\n", g_tests_passed + g_tests_failed);
    
    if (g_tests_failed > 0) {
        printf("\n[FAILED] Some tests failed!\n");
        return 1;
    } else {
        printf("\n[PASSED] All tests passed!\n");
        return 0;
    }
}
