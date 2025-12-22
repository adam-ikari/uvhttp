#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "uvhttp_common.h"

// 安全的字符串复制函数 - 匹配头文件声明
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
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
    if (name_len == 0 || name_len > MAX_HEADER_NAME_LEN || 
        value_len == 0 || value_len > MAX_HEADER_VALUE_LEN) {
        return -1;
    }
    
    // 验证header名称 - 防止HTTP响应分割攻击
    for (size_t i = 0; i < name_len; i++) {
        char c = name[i];
        // 只允许字母、数字、连字符和下划线
        if (!isalnum(c) && c != '-' && c != '_') {
            return -1;
        }
        // 防止换行符注入
        if (c == '\r' || c == '\n') {
            return -1;
        }
    }
    
    // 验证header值 - 检查控制字符和注入攻击
    for (size_t i = 0; i < value_len; i++) {
        char c = value[i];
        // 禁止控制字符（除了tab）
        if ((c < 0x20 && c != '\t') || c == 0x7F) {
            return -1;
        }
        // 防止HTTP响应分割攻击
        if (c == '\r' || c == '\n') {
            return -1;
        }
    }
    
    // 检查常见的危险header名称
    if (strcasecmp(name, "Location") == 0 || 
        strcasecmp(name, "Set-Cookie") == 0 ||
        strcasecmp(name, "Refresh") == 0) {
        // 这些header需要额外的URL验证
        for (size_t i = 0; i < value_len; i++) {
            if (value[i] == '\r' || value[i] == '\n') {
                return -1;
            }
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
    if (length > UVHTTP_MAX_URL_SIZE) return -1;
    
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

// JSON字符串转义函数
char* uvhttp_escape_json_string(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    size_t escaped_len = len;
    
    // 计算转义后的长度
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                escaped_len++; // 每个需要转义的字符增加1个字符
                break;
            default:
                if (str[i] < 0x20) {
                    escaped_len += UVHTTP_ESCAPE_SEQUENCE_LENGTH; // 控制字符转义为 \uXXXX 格式
                }
                break;
        }
    }
    
    char* escaped = uvhttp_malloc(escaped_len + 1);
    if (!escaped) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\b':
                escaped[j++] = '\\';
                escaped[j++] = 'b';
                break;
            case '\f':
                escaped[j++] = '\\';
                escaped[j++] = 'f';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                if (str[i] < 0x20) {
                    // 控制字符转义为 \uXXXX 格式
                    j += snprintf(escaped + j, escaped_len - j + 1, "\\u%04x", (unsigned char)str[i]);
                } else {
                    escaped[j++] = str[i];
                }
                break;
        }
    }
    
    escaped[j] = '\0';
    return escaped;
}