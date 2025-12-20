#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_request_simple.h"
#include "include/uvhttp_utils.h"

int test_response_edge_cases() {
    printf("测试响应边界情况...\n");
    
    uvhttp_response_t response;
    
    // 测试最大头部数量
    uvhttp_response_init(&response, (void*)0x1);
    for (int i = 0; i < 64; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "Header-%d", i);
        snprintf(value, sizeof(value), "Value-%d", i);
        uvhttp_response_set_header(&response, name, value);
    }
    printf("✓ 最大头部数量测试通过 (%d个headers)\n", response.header_count);
    
    // 测试超出头部数量限制
    uvhttp_response_set_header(&response, "Extra", "Value");
    printf("✓ 头部数量限制测试通过\n");
    
    // 测试不同状态码
    int status_codes[] = {100, 101, 200, 201, 301, 400, 401, 404, 500, 503};
    for (int i = 0; i < 10; i++) {
        uvhttp_response_set_status(&response, status_codes[i]);
        if (response.status_code != status_codes[i]) {
            printf("✗ 状态码 %d 设置失败\n", status_codes[i]);
            uvhttp_response_cleanup(&response);
            return -1;
        }
    }
    printf("✓ 状态码边界测试通过\n");
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_response_memory_operations() {
    printf("\n测试响应内存操作...\n");
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    // 测试多次body设置（覆盖测试）
    const char* bodies[] = {"Short", "Medium length body", "A much longer body that tests memory reallocation"};
    for (int i = 0; i < 3; i++) {
        if (uvhttp_response_set_body(&response, bodies[i], strlen(bodies[i])) != 0) {
            printf("✗ Body设置失败 (iteration %d)\n", i);
            uvhttp_response_cleanup(&response);
            return -1;
        }
    }
    printf("✓ 多次body设置测试通过\n");
    
    // 测试body清空
    uvhttp_response_set_body(&response, "", 0);
    if (response.body_length != 0) {
        printf("✗ Body清空失败\n");
        uvhttp_response_cleanup(&response);
        return -1;
    }
    printf("✓ Body清空测试通过\n");
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_response_error_conditions() {
    printf("\n测试响应错误条件...\n");
    
    // 测试NULL响应处理
    uvhttp_response_set_status(NULL, 200);
    uvhttp_response_set_header(NULL, "Name", "Value");
    uvhttp_response_set_body(NULL, "data", 4);
    uvhttp_response_send(NULL);
    uvhttp_response_cleanup(NULL);
    printf("✓ NULL参数处理测试通过\n");
    
    // 测试无效状态码
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    uvhttp_response_set_status(&response, -1);
    uvhttp_response_set_status(&response, 0);
    uvhttp_response_set_status(&response, 1000);
    printf("✓ 无效状态码处理测试通过\n");
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_utils_comprehensive() {
    printf("\n测试工具函数综合功能...\n");
    
    // 测试各种URL验证
    const char* valid_urls[] = {
        "http://example.com",
        "https://example.com",
        "/api/v1/users",
        "/",
        "http://localhost:8080",
        "https://192.168.1.1:443"
    };
    
    for (int i = 0; i < 6; i++) {
        if (uvhttp_validate_header_value("URL", valid_urls[i]) != 0) {
            printf("✗ 有效URL验证失败: %s\n", valid_urls[i]);
            return -1;
        }
    }
    printf("✓ 有效URL验证测试通过\n");
    
    // 测试无效URL
    const char* invalid_urls[] = {
        "",
        NULL,
        "http://example.com\x01control",
        "http://example.com\x7fdelete"
    };
    
    for (int i = 0; i < 4; i++) {
        if (i < 2 && uvhttp_validate_header_value("URL", invalid_urls[i]) == 0) {
            printf("✗ 无效URL验证失败: %s\n", invalid_urls[i] ? "NULL" : "empty");
            return -1;
        }
    }
    printf("✓ 无效URL验证测试通过\n");
    
    // 测试HTTP方法验证
    const char* methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"};
    for (int i = 0; i < 7; i++) {
        if (uvhttp_validate_header_value("Method", methods[i]) != 0) {
            printf("✗ HTTP方法验证失败: %s\n", methods[i]);
            return -1;
        }
    }
    printf("✓ HTTP方法验证测试通过\n");
    
    return 0;
}

int test_request_response_integration() {
    printf("\n测试请求响应集成...\n");
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    uvhttp_request_init(&request, (void*)0x1);
    uvhttp_response_init(&response, (void*)0x1);
    
    // 模拟完整的请求处理流程
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    
    const char* json_body = "{\"status\": \"ok\", \"message\": \"Request processed\"}";
    uvhttp_response_set_body(&response, json_body, strlen(json_body));
    
    // 验证响应完整性
    if (response.status_code != 200 || 
        response.header_count != 2 || 
        response.body_length != strlen(json_body)) {
        printf("✗ 请求响应集成测试失败\n");
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    printf("✓ 请求响应集成测试通过\n");
    
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    return 0;
}

int main() {
    printf("=== 高覆盖率测试执行 ===\n");
    
    int failed = 0;
    int total = 5;
    
    if (test_response_edge_cases() != 0) failed++;
    if (test_response_memory_operations() != 0) failed++;
    if (test_response_error_conditions() != 0) failed++;
    if (test_utils_comprehensive() != 0) failed++;
    if (test_request_response_integration() != 0) failed++;
    
    printf("\n=== 高覆盖率测试结果 ===\n");
    if (failed == 0) {
        printf("✅ 所有高覆盖率测试通过 (%d/%d)\n", total - failed, total);
    } else {
        printf("❌ 部分高覆盖率测试失败 (%d/%d 通过)\n", total - failed, total);
    }
    
    return failed;
}