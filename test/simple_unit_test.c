#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp_response.h"
#include "uvhttp_utils.h"

int test_response_init() {
    printf("测试响应初始化...\n");
    
    uvhttp_response_t response;
    
    // 测试正常初始化
    int result = uvhttp_response_init(&response, (void*)0x1);
    if (result == 0 && response.status_code == 200) {
        printf("✓ 响应初始化测试通过\n");
    } else {
        printf("✗ 响应初始化测试失败\n");
        return -1;
    }
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_response_validation() {
    printf("测试响应验证...\n");
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    // 测试状态码设置
    uvhttp_response_set_status(&response, 404);
    if (response.status_code == 404) {
        printf("✓ 状态码设置测试通过\n");
    } else {
        printf("✗ 状态码设置测试失败\n");
        return -1;
    }
    
    // 测试header设置
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    if (response.header_count == 1) {
        printf("✓ Header设置测试通过\n");
    } else {
        printf("✗ Header设置测试失败\n");
        return -1;
    }
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_response_body() {
    printf("测试响应body设置...\n");
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    // 测试NULL body
    int result = uvhttp_response_set_body(&response, NULL, 100);
    if (result != 0) {
        printf("✓ NULL body拒绝测试通过\n");
    } else {
        printf("✗ NULL body拒绝测试失败\n");
        return -1;
    }
    
    // 测试空body
    result = uvhttp_response_set_body(&response, "data", 0);
    if (result != 0) {
        printf("✓ 空body拒绝测试通过\n");
    } else {
        printf("✗ 空body拒绝测试失败\n");
        return -1;
    }
    
    // 测试超大body (2MB)
    char* large_body = malloc(2 * 1024 * 1024);
    if (large_body) {
        result = uvhttp_response_set_body(&response, large_body, 2 * 1024 * 1024);
        if (result != 0) {
            printf("✓ 超大body拒绝测试通过\n");
        } else {
            printf("✗ 超大body拒绝测试失败\n");
            free(large_body);
            return -1;
        }
        free(large_body);
    }
    
    // 测试正常body
    const char* test_data = "Hello World";
    result = uvhttp_response_set_body(&response, test_data, strlen(test_data));
    if (result == 0 && response.body_length == strlen(test_data)) {
        printf("✓ 正常body设置测试通过\n");
    } else {
        printf("✗ 正常body设置测试失败\n");
        return -1;
    }
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_response_send() {
    printf("测试响应发送...\n");
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    // 设置基本响应
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    uvhttp_response_set_body(&response, "Test Response", 14);
    
    // 测试发送功能（不会实际发送，只测试不崩溃）
    printf("  测试响应发送（模拟）...\n");
    uvhttp_response_send(&response);
    
    // 检查响应是否标记为完成
    if (response.finished == 1) {
        printf("✓ 响应发送测试通过\n");
    } else {
        printf("✗ 响应发送测试失败\n");
        return -1;
    }
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_utils_functions() {
    printf("测试工具函数...\n");
    
    // 测试安全字符串复制
    char dest[100];
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), "hello");
    if (result == 0 && strcmp(dest, "hello") == 0) {
        printf("✓ 安全字符串复制测试通过\n");
    } else {
        printf("✗ 安全字符串复制测试失败\n");
        return -1;
    }
    
    // 测试header验证
    result = uvhttp_validate_header_value("Content-Type", "text/plain");
    if (result == 0) {
        printf("✓ Header验证测试通过\n");
    } else {
        printf("✗ Header验证测试失败\n");
        return -1;
    }
    
    return 0;
}

int main() {
    printf("=== 单元测试执行 ===\n\n");
    
    int failed = 0;
    int total = 5;
    
    if (test_response_init() != 0) failed++;
    if (test_response_validation() != 0) failed++;
    if (test_response_body() != 0) failed++;
    if (test_response_send() != 0) failed++;
    if (test_utils_functions() != 0) failed++;
    
    printf("\n=== 测试结果 ===\n");
    if (failed == 0) {
        printf("✓ 所有测试通过 (%d/%d)\n", total - failed, total);
    } else {
        printf("✗ 部分测试失败 (%d/%d 通过)\n", total - failed, total);
    }
    
    return failed;
}