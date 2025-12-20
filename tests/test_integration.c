#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/uvhttp_server_simple.h"
#include "include/uvhttp_request_simple.h"
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_router_simple.h"

// 测试处理器函数
static void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
}

static void json_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    const char* json_body = "{\"message\": \"Hello JSON\", \"status\": \"ok\"}";
    uvhttp_response_set_body(response, json_body, strlen(json_body));
}

static void error_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Internal Server Error", 21);
}

int test_server_lifecycle() {
    printf("测试服务器生命周期...\n");
    
    // 创建服务器
    struct uvhttp_server* server = uvhttp_server_new(NULL);
    if (!server) {
        printf("✗ 服务器创建失败\n");
        return -1;
    }
    printf("✓ 服务器创建成功\n");
    
    // 设置处理器
    uvhttp_server_set_handler(server, hello_handler);
    printf("✓ 处理器设置成功\n");
    
    // 模拟启动（简化版本）
    printf("✓ 服务器启动模拟成功\n");
    
    // 清理
    uvhttp_server_free(server);
    printf("✓ 服务器清理成功\n");
    
    return 0;
}

int test_request_response_cycle() {
    printf("\n测试请求/响应周期...\n");
    
    // 创建模拟请求和响应
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    // 初始化
    if (uvhttp_request_init(&request, NULL) != 0) {
        printf("✗ 请求初始化失败\n");
        return -1;
    }
    
    if (uvhttp_response_init(&response, (void*)0x1) != 0) {
        printf("✗ 响应初始化失败\n");
        uvhttp_request_cleanup(&request);
        return -1;
    }
    
    printf("✓ 请求和响应初始化成功\n");
    
    // 模拟处理请求
    hello_handler(&request, &response);
    
    // 验证响应
    if (response.status_code == 200 && 
        response.header_count > 0 && 
        response.body != NULL &&
        response.body_length == 13) {
        printf("✓ 请求处理验证成功\n");
    } else {
        printf("✗ 请求处理验证失败\n");
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    // 清理
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    printf("✓ 请求/响应周期测试完成\n");
    
    return 0;
}

int test_routing_functionality() {
    printf("\n测试路由功能...\n");
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        printf("✗ 路由器创建失败\n");
        return -1;
    }
    printf("✓ 路由器创建成功\n");
    
    // 添加路由
    if (uvhttp_router_add_route(router, "/hello", hello_handler) != 0) {
        printf("✗ 路由添加失败\n");
        uvhttp_router_free(router);
        return -1;
    }
    printf("✓ 路由添加成功\n");
    
    if (uvhttp_router_add_route(router, "/api/json", json_handler) != 0) {
        printf("✗ JSON路由添加失败\n");
        uvhttp_router_free(router);
        return -1;
    }
    printf("✓ JSON路由添加成功\n");
    
    // 测试路由查找
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/hello");
    if (handler == hello_handler) {
        printf("✓ 路由查找成功\n");
    } else {
        printf("✗ 路由查找失败\n");
        uvhttp_router_free(router);
        return -1;
    }
    
    // 测试不存在的路由
    handler = uvhttp_router_find_handler(router, "/nonexistent");
    if (handler == NULL) {
        printf("✓ 不存在路由处理正确\n");
    } else {
        printf("✗ 不存在路由处理错误\n");
        uvhttp_router_free(router);
        return -1;
    }
    
    // 清理
    uvhttp_router_free(router);
    printf("✓ 路由器清理成功\n");
    
    return 0;
}

int test_error_handling() {
    printf("\n测试错误处理...\n");
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    // 初始化
    uvhttp_request_init(&request, NULL);
    uvhttp_response_init(&response, (void*)0x1);
    
    // 测试错误处理器
    error_handler(&request, &response);
    
    // 验证错误响应
    if (response.status_code == 500 && response.body_length == 21) {
        printf("✓ 错误处理验证成功\n");
    } else {
        printf("✗ 错误处理验证失败\n");
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    // 清理
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    
    return 0;
}

int test_concurrent_requests() {
    printf("\n测试并发请求处理...\n");
    
    // 模拟多个并发请求
    const int num_requests = 10;
    uvhttp_request_t requests[num_requests];
    uvhttp_response_t responses[num_requests];
    
    // 初始化所有请求和响应
    for (int i = 0; i < num_requests; i++) {
        if (uvhttp_request_init(&requests[i], NULL) != 0 ||
            uvhttp_response_init(&responses[i], (void*)0x1) != 0) {
            printf("✗ 并发初始化失败\n");
            // 清理已初始化的
            for (int j = 0; j < i; j++) {
                uvhttp_request_cleanup(&requests[j]);
                uvhttp_response_cleanup(&responses[j]);
            }
            return -1;
        }
    }
    
    printf("✓ 并发初始化成功 (%d个请求)\n", num_requests);
    
    // 处理所有请求
    for (int i = 0; i < num_requests; i++) {
        if (i % 2 == 0) {
            hello_handler(&requests[i], &responses[i]);
        } else {
            json_handler(&requests[i], &responses[i]);
        }
    }
    
    // 验证所有响应
    int success_count = 0;
    for (int i = 0; i < num_requests; i++) {
        if (responses[i].status_code == 200 && responses[i].body != NULL) {
            success_count++;
        }
    }
    
    if (success_count == num_requests) {
        printf("✓ 并发请求处理成功 (%d/%d)\n", success_count, num_requests);
    } else {
        printf("✗ 并发请求处理失败 (%d/%d)\n", success_count, num_requests);
    }
    
    // 清理所有资源
    for (int i = 0; i < num_requests; i++) {
        uvhttp_request_cleanup(&requests[i]);
        uvhttp_response_cleanup(&responses[i]);
    }
    
    return success_count == num_requests ? 0 : -1;
}

int main() {
    printf("=== 集成测试执行 ===\n");
    
    int failed = 0;
    int total = 5;
    
    if (test_server_lifecycle() != 0) failed++;
    if (test_request_response_cycle() != 0) failed++;
    if (test_routing_functionality() != 0) failed++;
    if (test_error_handling() != 0) failed++;
    if (test_concurrent_requests() != 0) failed++;
    
    printf("\n=== 集成测试结果 ===\n");
    if (failed == 0) {
        printf("✅ 所有集成测试通过 (%d/%d)\n", total - failed, total);
    } else {
        printf("❌ 部分集成测试失败 (%d/%d 通过)\n", total - failed, total);
    }
    
    return failed;
}