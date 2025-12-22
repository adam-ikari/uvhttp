#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uvhttp.h"
#include "../include/uvhttp_utils.h"

int main() {
    printf("Testing UVHTTP basic functionality...\n");
    
    // 测试安全的字符串操作
    char dest[10];
    int result = safe_strncpy(dest, "hello", sizeof(dest));
    if (result != 0) {
        printf("FAIL: safe_strncpy failed\n");
        return 1;
    }
    
    if (strcmp(dest, "hello") != 0) {
        printf("FAIL: safe_strncpy incorrect result\n");
        return 1;
    }
    
    // 测试缓冲区溢出保护
    result = safe_strncpy(dest, "this is too long", sizeof(dest));
    if (result != 0) {
        printf("FAIL: safe_strncpy overflow handling failed\n");
        return 1;
    }
    
    if (strlen(dest) >= sizeof(dest)) {
        printf("FAIL: safe_strncpy overflow protection failed\n");
        return 1;
    }
    
    // 测试URL验证
    result = validate_url("/test", 5);
    if (result != 0) {
        printf("FAIL: validate_url failed for valid URL\n");
        return 1;
    }
    
    result = validate_url("/test\x00", 6);
    if (result == 0) {
        printf("FAIL: validate_url should reject NULL byte\n");
        return 1;
    }
    
    // 测试头部值验证
    result = validate_header_value("valid-value", 11);
    if (result != 0) {
        printf("FAIL: validate_header_value failed for valid value\n");
        return 1;
    }
    
    result = validate_header_value("invalid\x01value", 13);
    if (result == 0) {
        printf("FAIL: validate_header_value should reject control character\n");
        return 1;
    }
    
    // 测试HTTP方法验证
    result = validate_method("GET", 3);
    if (result != 0) {
        printf("FAIL: validate_method failed for GET\n");
        return 1;
    }
    
    result = validate_method("INVALID", 7);
    if (result == 0) {
        printf("FAIL: validate_method should reject invalid method\n");
        return 1;
    }
    
    printf("All basic tests passed!\n");
    printf("UVHTTP core utilities are working correctly.\n");
    
    return 0;
}