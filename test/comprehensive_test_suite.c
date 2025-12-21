/**
 * @file comprehensive_test_suite.c
 * @brief UVHTTP 综合测试套件
 * 
 * 包含所有模块的单元测试、集成测试和性能测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

/* 包含所有需要测试的头文件 */
#include "../include/uvhttp.h"
#include "../include/uvhttp_request.h"
#include "../include/uvhttp_response.h"
#include "../include/uvhttp_router.h"
#include "../include/uvhttp_server.h"
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_error.h"
#include "../include/uvhttp_allocator.h"

/* 测试统计 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} test_stats_t;

static test_stats_t g_stats = {0, 0, 0};

/* 测试宏 */
#define TEST_ASSERT(condition, message) do { \
    g_stats.total_tests++; \
    if (condition) { \
        g_stats.passed_tests++; \
        printf("✓ %s\n", message); \
    } else { \
        g_stats.failed_tests++; \
        printf("✗ %s\n", message); \
    } \
} while(0)

#define TEST_SECTION(name) do { \
    printf("\n=== %s ===\n", name); \
} while(0)

/* ==================== 请求处理测试 ==================== */
void test_request_parsing() {
    TEST_SECTION("请求解析测试");
    
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* 测试请求初始化 */
    int result = uvhttp_request_init(&request, (void*)0x1);
    TEST_ASSERT(result == 0, "请求初始化");
    TEST_ASSERT(request.method == UVHTTP_GET, "默认方法设置");
    
    /* 测试 URL 设置 */
    strncpy(request.url, "/test/path", sizeof(request.url) - 1);
    TEST_ASSERT(strcmp(uvhttp_request_get_url(&request), "/test/path") == 0, "URL 获取");
    
    /* 测试 header 处理 */
    request.headers[0].name = "Content-Type";
    request.headers[0].value = "application/json";
    request.header_count = 1;
    
    const char* content_type = uvhttp_request_get_header(&request, "Content-Type");
    TEST_ASSERT(content_type != NULL, "Header 获取");
    TEST_ASSERT(strcmp(content_type, "application/json") == 0, "Header 值正确");
    
    /* 测试无效 header */
    const char* invalid = uvhttp_request_get_header(&request, "Invalid-Header");
    TEST_ASSERT(invalid == NULL, "无效 Header 返回 NULL");
    
    uvhttp_request_cleanup(&request);
}

/* ==================== 响应处理测试 ==================== */
void test_response_handling() {
    TEST_SECTION("响应处理测试");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 测试响应初始化 */
    int result = uvhttp_response_init(&response, (void*)0x1);
    TEST_ASSERT(result == 0, "响应初始化");
    TEST_ASSERT(response.status_code == 200, "默认状态码");
    
    /* 测试状态码设置 */
    uvhttp_response_set_status(&response, 404);
    TEST_ASSERT(response.status_code == 404, "状态码设置");
    
    /* 测试无效状态码 */
    uvhttp_response_set_status(&response, 999);
    TEST_ASSERT(response.status_code == 404, "无效状态码被拒绝");
    
    /* 测试 header 设置 */
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    TEST_ASSERT(response.header_count == 1, "Header 添加");
    
    /* 测试 body 设置 */
    const char* body = "<html><body>Hello</body></html>";
    result = uvhttp_response_set_body(&response, body, strlen(body));
    TEST_ASSERT(result == 0, "Body 设置");
    TEST_ASSERT(response.body_length == strlen(body), "Body 长度正确");
    
    /* 测试过大 body */
    char large_body[2 * 1024 * 1024]; /* 2MB */
    memset(large_body, 'A', sizeof(large_body) - 1);
    large_body[sizeof(large_body) - 1] = '\0';
    
    result = uvhttp_response_set_body(&response, large_body, strlen(large_body));
    TEST_ASSERT(result != 0, "过大 Body 被拒绝");
    
    uvhttp_response_cleanup(&response);
}

/* ==================== 路由系统测试 ==================== */
void test_router_system() {
    TEST_SECTION("路由系统测试");
    
    /* 创建路由器 */
    uvhttp_router_t* router = uvhttp_router_new();
    TEST_ASSERT(router != NULL, "路由器创建");
    
    /* 模拟请求处理器 */
    static int handler_called = 0;
    void test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
        handler_called++;
        uvhttp_response_set_status(response, 200);
        const char* body = "OK";
        uvhttp_response_set_body(response, body, strlen(body));
        uvhttp_response_send(response);
    }
    
    /* 添加路由 */
    int result = uvhttp_router_add_route(router, "/test", test_handler);
    TEST_ASSERT(result == 0, "路由添加");
    
    /* 测试路由查找 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/test");
    TEST_ASSERT(handler == test_handler, "路由查找");
    
    /* 测试不存在的路由 */
    handler = uvhttp_router_find_handler(router, "/nonexistent");
    TEST_ASSERT(handler == NULL, "不存在的路由");
    
    uvhttp_router_free(router);
}

/* ==================== 内存管理测试 ==================== */
void test_memory_management() {
    TEST_SECTION("内存管理测试");
    
    /* 重置内存统计 */
    #ifdef UVHTTP_ENABLE_MEMORY_DEBUG
    uvhttp_reset_memory_stats();
    #endif
    
    /* 测试基本分配 */
    void* ptr1 = uvhttp_malloc(1024);
    TEST_ASSERT(ptr1 != NULL, "内存分配");
    
    void* ptr2 = uvhttp_malloc(2048);
    TEST_ASSERT(ptr2 != NULL, "多次分配");
    
    /* 测试重新分配 */
    void* ptr3 = uvhttp_realloc(ptr1, 4096);
    TEST_ASSERT(ptr3 != NULL, "内存重新分配");
    
    /* 测试清零分配 */
    void* ptr4 = uvhttp_calloc(10, 100);
    TEST_ASSERT(ptr4 != NULL, "清零分配");
    
    /* 验证内容为零 */
    char* zero_check = (char*)ptr4;
    int all_zero = 1;
    for (int i = 0; i < 1000; i++) {
        if (zero_check[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    TEST_ASSERT(all_zero, "清零分配内容正确");
    
    /* 释放内存 */
    uvhttp_free(ptr2);
    uvhttp_free(ptr3);
    uvhttp_free(ptr4);
    
    /* 检查内存泄漏 */
    #ifdef UVHTTP_ENABLE_MEMORY_DEBUG
    int has_leaks = uvhttp_check_memory_leaks();
    TEST_ASSERT(!has_leaks, "无内存泄漏");
    #endif
}

/* ==================== 错误处理测试 ==================== */
void test_error_handling() {
    TEST_SECTION("错误处理测试");
    
    /* 测试错误字符串转换 */
    const char* error_msg = uvhttp_error_string(UVHTTP_OK);
    TEST_ASSERT(strcmp(error_msg, "Success") == 0, "成功错误消息");
    
    error_msg = uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM);
    TEST_ASSERT(strcmp(error_msg, "Invalid parameter") == 0, "参数错误消息");
    
    error_msg = uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    TEST_ASSERT(strcmp(error_msg, "Out of memory") == 0, "内存错误消息");
    
    /* 测试错误日志 */
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "测试错误");
    
    /* 测试错误恢复配置 */
    uvhttp_set_error_recovery_config(5, 50, 2000, 1.5);
    
    /* 测试最频繁错误 */
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error();
    TEST_ASSERT(most_frequent == UVHTTP_ERROR_INVALID_PARAM, "最频繁错误统计");
}

/* ==================== 工具函数测试 ==================== */
void test_utility_functions() {
    TEST_SECTION("工具函数测试");
    
    /* 测试 URL 解码 */
    char decoded[256];
    const char* encoded = "Hello%20World%21";
    int result = uvhttp_url_decode(encoded, decoded, sizeof(decoded));
    TEST_ASSERT(result == 0, "URL 解码");
    TEST_ASSERT(strcmp(decoded, "Hello World!") == 0, "解码结果正确");
    
    /* 测试 base64 编码 */
    const char* input = "UVHTTP Test";
    char encoded_output[64];
    result = uvhttp_base64_encode((const unsigned char*)input, strlen(input), 
                                 encoded_output, sizeof(encoded_output));
    TEST_ASSERT(result > 0, "Base64 编码");
    
    /* 测试字符串验证 */
    result = uvhttp_validate_header_value("Content-Type", "text/html");
    TEST_ASSERT(result == 0, "有效 Header 验证");
    
    result = uvhttp_validate_header_value("Bad\nHeader", "value");
    TEST_ASSERT(result != 0, "无效 Header 拒绝");
    
    /* 测试安全字符串复制 */
    char dest[10];
    result = uvhttp_safe_strcpy(dest, sizeof(dest), "1234567890");
    TEST_ASSERT(result == 0, "安全字符串复制（合适长度）");
    
    result = uvhttp_safe_strcpy(dest, sizeof(dest), "123456789012345");
    TEST_ASSERT(result != 0, "安全字符串复制（超长）");
}

/* ==================== 性能测试 ==================== */
void test_performance() {
    TEST_SECTION("性能测试");
    
    const int iterations = 10000;
    clock_t start, end;
    
    /* 测试内存分配性能 */
    start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_malloc(1024);
        uvhttp_free(ptr);
    }
    end = clock();
    
    double alloc_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("内存分配性能: %d 次分配/释放耗时 %.2f ms (平均 %.3f μs)\n", 
           iterations, alloc_time, alloc_time * 1000 / iterations);
    
    TEST_ASSERT(alloc_time < 1000, "内存分配性能达标");
    
    /* 测试字符串处理性能 */
    char buffer[256];
    start = clock();
    for (int i = 0; i < iterations; i++) {
        uvhttp_safe_strcpy(buffer, sizeof(buffer), "Performance test string");
    }
    end = clock();
    
    double str_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("字符串处理性能: %d 次操作耗时 %.2f ms (平均 %.3f μs)\n", 
           iterations, str_time, str_time * 1000 / iterations);
    
    TEST_ASSERT(str_time < 500, "字符串处理性能达标");
}

/* ==================== 集成测试 ==================== */
void test_integration() {
    TEST_SECTION("集成测试");
    
    /* 创建完整的服务器-请求-响应流程 */
    uv_loop_t* loop = uv_default_loop();
    TEST_ASSERT(loop != NULL, "事件循环创建");
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    TEST_ASSERT(server != NULL, "服务器创建");
    
    uvhttp_router_t* router = uvhttp_router_new();
    TEST_ASSERT(router != NULL, "路由器创建");
    
    /* 设置请求处理器 */
    static int request_processed = 0;
    void integration_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
        request_processed++;
        
        /* 获取请求信息 */
        const char* method = uvhttp_request_get_method(request);
        const char* url = uvhttp_request_get_url(request);
        const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
        
        TEST_ASSERT(method != NULL, "集成测试：获取方法");
        TEST_ASSERT(url != NULL, "集成测试：获取 URL");
        
        /* 设置响应 */
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        
        const char* body = "Integration Test OK";
        uvhttp_response_set_body(response, body, strlen(body));
        uvhttp_response_send(response);
    }
    
    /* 添加路由 */
    int result = uvhttp_router_add_route(router, "/test", integration_handler);
    TEST_ASSERT(result == 0, "集成测试：路由添加");
    
    /* 配置服务器 */
    server->router = router;
    server->max_connections = 100;
    
    /* 模拟请求处理（不需要真正启动服务器） */
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    uvhttp_request_init(&request, (void*)0x1);
    uvhttp_response_init(&response, (void*)0x1);
    
    /* 设置请求数据 */
    strncpy(request.url, "/test", sizeof(request.url) - 1);
    request.headers[0].name = "User-Agent";
    request.headers[0].value = "UVHTTP-Test/1.0";
    request.header_count = 1;
    
    /* 查找并执行处理器 */
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/test");
    TEST_ASSERT(handler != NULL, "集成测试：处理器查找");
    
    if (handler) {
        handler(&request, &response);
        TEST_ASSERT(request_processed == 1, "集成测试：请求处理");
        TEST_ASSERT(response.status_code == 200, "集成测试：响应状态");
    }
    
    /* 清理 */
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_router_free(router);
    uvhttp_server_free(server);
    uv_loop_close(loop);
}

/* ==================== 主测试函数 ==================== */
int main() {
    printf("🧪 UVHTTP 综合测试套件\n");
    printf("========================\n");
    
    /* 运行所有测试 */
    test_request_parsing();
    test_response_handling();
    test_router_system();
    test_memory_management();
    test_error_handling();
    test_utility_functions();
    test_performance();
    test_integration();
    
    /* 输出测试结果 */
    printf("\n========================\n");
    printf("📊 测试结果统计\n");
    printf("========================\n");
    printf("总测试数: %d\n", g_stats.total_tests);
    printf("通过: %d\n", g_stats.passed_tests);
    printf("失败: %d\n", g_stats.failed_tests);
    printf("成功率: %.1f%%\n", 
           g_stats.total_tests > 0 ? (double)g_stats.passed_tests / g_stats.total_tests * 100.0 : 0.0);
    
    /* 输出内存统计 */
    #ifdef UVHTTP_ENABLE_MEMORY_DEBUG
    size_t total_allocated, current_allocated, allocation_count, free_count;
    uvhttp_get_memory_stats(&total_allocated, &current_allocated, 
                           &allocation_count, &free_count);
    printf("\n📈 内存使用统计\n");
    printf("========================\n");
    printf("总分配: %zu 字节\n", total_allocated);
    printf("当前使用: %zu 字节\n", current_allocated);
    printf("分配次数: %zu\n", allocation_count);
    printf("释放次数: %zu\n", free_count);
    
    if (uvhttp_check_memory_leaks()) {
        printf("⚠️  检测到内存泄漏\n");
    } else {
        printf("✅ 无内存泄漏\n");
    }
    #endif
    
    /* 输出错误统计 */
    size_t error_counts[UVHTTP_ERROR_MAX];
    time_t last_error_time;
    const char* last_error_context;
    uvhttp_get_error_stats(error_counts, &last_error_time, &last_error_context);
    
    printf("\n🚨 错误统计\n");
    printf("========================\n");
    if (last_error_time > 0) {
        printf("最后错误: %s\n", last_error_context);
    } else {
        printf("无错误记录\n");
    }
    
    /* 判断测试是否通过 */
    if (g_stats.failed_tests == 0) {
        printf("\n✅ 所有测试通过！\n");
        return 0;
    } else {
        printf("\n❌ 有 %d 个测试失败\n", g_stats.failed_tests);
        return 1;
    }
}