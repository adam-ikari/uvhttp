#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/uvhttp_server_simple.h"
#include "include/uvhttp_request_simple.h"
#include "include/uvhttp_response_simple.h"

static void test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Test Response", 13);
}

int test_basic_integration() {
    printf("测试基本集成功能...\n");
    
    // 创建服务器
    struct uvhttp_server* server = uvhttp_server_new(NULL);
    if (!server) {
        printf("✗ 服务器创建失败\n");
        return -1;
    }
    
    // 设置处理器
    uvhttp_server_set_handler(server, test_handler);
    
    // 创建请求和响应
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    if (uvhttp_request_init(&request, (void*)0x1) != 0 ||
        uvhttp_response_init(&response, (void*)0x1) != 0) {
        printf("✗ 请求/响应初始化失败\n");
        uvhttp_server_free(server);
        return -1;
    }
    
    // 处理请求
    test_handler(&request, &response);
    
    // 验证响应
    if (response.status_code == 200 && 
        response.body_length == 13 &&
        strcmp(response.body, "Test Response") == 0) {
        printf("✓ 基本集成测试通过\n");
    } else {
        printf("✗ 基本集成测试失败\n");
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        uvhttp_server_free(server);
        return -1;
    }
    
    // 清理
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_server_free(server);
    
    return 0;
}

int test_multiple_responses() {
    printf("\n测试多响应处理...\n");
    
    uvhttp_response_t responses[5];
    
    // 初始化多个响应
    for (int i = 0; i < 5; i++) {
        if (uvhttp_response_init(&responses[i], (void*)0x1) != 0) {
            printf("✗ 多响应初始化失败\n");
            // 清理已初始化的
            for (int j = 0; j < i; j++) {
                uvhttp_response_cleanup(&responses[j]);
            }
            return -1;
        }
    }
    
    // 设置不同的状态码
    for (int i = 0; i < 5; i++) {
        uvhttp_response_set_status(&responses[i], 200 + i);
        uvhttp_response_set_header(&responses[i], "Content-Type", "text/plain");
        char body[50];
        snprintf(body, sizeof(body), "Response %d", i);
        uvhttp_response_set_body(&responses[i], body, strlen(body));
    }
    
    // 验证所有响应
    int success = 1;
    for (int i = 0; i < 5; i++) {
        if (responses[i].status_code != 200 + i || 
            responses[i].body_length == 0) {
            success = 0;
            break;
        }
    }
    
    if (success) {
        printf("✓ 多响应处理测试通过\n");
    } else {
        printf("✗ 多响应处理测试失败\n");
    }
    
    // 清理
    for (int i = 0; i < 5; i++) {
        uvhttp_response_cleanup(&responses[i]);
    }
    
    return success ? 0 : -1;
}

int main() {
    printf("=== 集成测试执行 ===\n");
    
    int failed = 0;
    int total = 2;
    
    if (test_basic_integration() != 0) failed++;
    if (test_multiple_responses() != 0) failed++;
    
    printf("\n=== 集成测试结果 ===\n");
    if (failed == 0) {
        printf("✅ 所有集成测试通过 (%d/%d)\n", total - failed, total);
    } else {
        printf("❌ 部分集成测试失败 (%d/%d 通过)\n", total - failed, total);
    }
    
    return failed;
}