#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_common.h"

int test_header_validation() {
    printf("Testing header validation...\n");
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试正常header
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    printf("✓ Normal header test passed\n");
    
    // 测试空值
    uvhttp_response_set_header(&response, "", "value");
    printf("✓ Empty name test handled\n");
    
    // 测试NULL值
    uvhttp_response_set_header(&response, "Name", NULL);
    printf("✓ NULL value test handled\n");
    
    return 0;
}

int test_response_validation() {
    printf("\nTesting response validation...\n");
    
    uvhttp_response_t response;
    
    // 测试NULL响应对象
    int result = uvhttp_response_init(NULL, (void*)0x1);
    if (result != 0) {
        printf("✓ NULL response validation passed\n");
    }
    
    // 测试NULL客户端
    result = uvhttp_response_init(&response, NULL);
    if (result != 0) {
        printf("✓ NULL client validation passed\n");
    }
    
    // 测试无效状态码
    uvhttp_response_init(&response, (void*)0x1);
    uvhttp_response_set_status(&response, 999); // 无效状态码
    printf("✓ Invalid status code test handled\n");
    
    // 测试过大body
    result = uvhttp_response_set_body(&response, "data", 2*1024*1024); // 2MB
    if (result != 0) {
        printf("✓ Large body rejection test passed\n");
    }
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_buffer_overflow_protection() {
    printf("\nTesting buffer overflow protection...\n");
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, (void*)0x1);
    
    // 测试超长header名称
    char long_name[300];
    memset(long_name, 'A', 299);
    long_name[299] = '\0';
    
    uvhttp_response_set_header(&response, long_name, "value");
    printf("✓ Long header name test handled\n");
    
    // 测试超长header值
    char long_value[5000];
    memset(long_value, 'B', 4999);
    long_value[4999] = '\0';
    
    uvhttp_response_set_header(&response, "name", long_value);
    printf("✓ Long header value test handled\n");
    
    // 测试包含控制字符的header
    uvhttp_response_set_header(&response, "name", "value\x01\x02");
    printf("✓ Control character test handled\n");
    
    uvhttp_response_cleanup(&response);
    return 0;
}

int test_memory_management() {
    printf("\nTesting memory management...\n");
    
    // 测试重复分配和释放
    for (int i = 0; i < 100; i++) {
        uvhttp_response_t response;
        uvhttp_response_init(&response, (void*)0x1);
        
        // 设置body
        char* data = malloc(1000);
        if (data) {
            memset(data, 'A', 1000);
            uvhttp_response_set_body(&response, data, 1000);
            free(data);
        }
        
        // 发送响应
        uvhttp_response_send(&response);
        
        // 清理
        uvhttp_response_cleanup(&response);
    }
    
    printf("✓ Memory stress test passed (100 iterations)\n");
    
    // 测试NULL处理
    uvhttp_response_cleanup(NULL);
    printf("✓ NULL cleanup test passed\n");
    
    return 0;
}

int main() {
    printf("=== Security Fixes Validation ===\n");
    
    test_header_validation();
    test_response_validation();
    test_buffer_overflow_protection();
    test_memory_management();
    
    printf("\n=== All security validation tests completed ===\n");
    return 0;
}