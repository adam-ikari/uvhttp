#include "include/uvhttp_utils.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("Testing security fixes...\n");
    
    // 测试安全的字符串复制
    char dest[10];
    int result = safe_strncpy(dest, "hello", sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, "hello") == 0);
    
    // 测试缓冲区溢出保护
    result = safe_strncpy(dest, "this is too long", sizeof(dest));
    assert(result == 0);
    assert(strlen(dest) == 9); // 应该被截断
    assert(dest[9] == '\0');
    
    // 测试URL验证
    result = validate_url("/test", 5);
    assert(result == 0);
    
    result = validate_url("/test\x00", 6); // 包含NULL字节
    assert(result == -1);
    
    // 测试头部值验证
    result = validate_header_value("valid-value", 11);
    assert(result == 0);
    
    result = validate_header_value("invalid\x01value", 13); // 包含控制字符
    assert(result == -1);
    
    // 测试HTTP方法验证
    result = validate_method("GET", 3);
    assert(result == 0);
    
    result = validate_method("INVALID", 7);
    assert(result == -1);
    
    printf("All security fix tests passed!\n");
    return 0;
}