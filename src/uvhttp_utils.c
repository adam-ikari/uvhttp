#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "uvhttp_common.h"

// 安全的字符串复制函数 - 匹配头文件声明
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0) return -1;
    
    size_t src_len = strnlen(src, dest_size - 1);
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    
    return 0;
}

// 验证header值是否有效 - 匹配头文件声明
int uvhttp_validate_header_value(const char* name, const char* value) {
    if (!name || !value) return -1;
    
    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    
    // 检查长度限制
    if (name_len > MAX_HEADER_NAME_LEN || value_len > MAX_HEADER_VALUE_LEN) {
        return -1;
    }
    
    // 验证header名称
    for (size_t i = 0; i < name_len; i++) {
        char c = name[i];
        if (!isalnum(c) && c != '-' && c != '_') {
            return -1;
        }
    }
    
    // 验证header值 - 检查控制字符
    for (size_t i = 0; i < value_len; i++) {
        if (value[i] < 0x20 || value[i] == 0x7F) {
            return -1;
        }
    }
    
    return 0;
}

// 保留原有函数以保持兼容性
int safe_strncpy(char* dest, const char* src, size_t dest_size) {
    return uvhttp_safe_strcpy(dest, dest_size, src);
}

// 验证HTTP头值是否安全
int validate_header_value(const char* value, size_t length) {
    if (!value) return -1;
    
    for (size_t i = 0; i < length; i++) {
        // 检查控制字符和NULL字节
        if (value[i] < 0x20 || value[i] == 0x7F) {
            return -1; // 无效字符
        }
    }
    return 0;
}

// 验证URL是否安全
int validate_url(const char* url, size_t length) {
    if (!url || length == 0) return -1;
    
    // 检查URL长度
    if (length > 2048) return -1;
    
    for (size_t i = 0; i < length; i++) {
        char c = url[i];
        // 允许的字符：字母、数字、-._~:/?#[]@!$&'()*+,;=%
        if (!isalnum(c) && c != '-' && c != '.' && c != '_' && c != '~' && 
            c != ':' && c != '/' && c != '?' && c != '#' && c != '[' && 
            c != ']' && c != '@' && c != '!' && c != '$' && c != '&' && 
            c != '\'' && c != '(' && c != ')' && c != '*' && c != '+' && 
            c != ',' && c != ';' && c != '=' && c != '%') {
            return -1;
        }
    }
    return 0;
}

// 验证HTTP方法
int validate_method(const char* method, size_t length) {
    if (!method || length == 0) return -1;
    
    const char* valid_methods[] = {
        "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH",
        "CONNECT", "TRACE", "COPY", "LOCK", "MKCOL", "MOVE", "PROPFIND",
        "PROPPATCH", "SEARCH", "UNLOCK", "BIND", "REBIND", "UNBIND",
        "ACL", "REPORT", "MKACTIVITY", "CHECKOUT", "MERGE", "MSEARCH",
        "NOTIFY", "SUBSCRIBE", "UNSUBSCRIBE", "PURGE", "MKCALENDAR",
        "LINK", "UNLINK", "SOURCE", "PRI", "DESCRIBE", "ANNOUNCE",
        "SETUP", "PLAY", "PAUSE", "TEARDOWN", "GET_PARAMETER", 
        "SET_PARAMETER", "REDIRECT", "RECORD", "FLUSH"
    };
    
    for (size_t i = 0; i < sizeof(valid_methods) / sizeof(valid_methods[0]); i++) {
        if (strncmp(method, valid_methods[i], length) == 0 && 
            strlen(valid_methods[i]) == length) {
            return 0;
        }
    }
    
    return -1;
}