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

// 注意：验证函数已移动到 uvhttp_validation.c
// 这里保留兼容性包装器

// 保留原有函数以保持兼容性
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    
    return 0;
}

// 兼容性函数（已弃用，请使用 uvhttp_validation.h 中的函数）
#include "uvhttp_validation.h"

int uvhttp_validate_header_value(const char* value, size_t length) {
    // 简单包装，实际验证逻辑在 uvhttp_validation.c 中
    (void)length; // 避免未使用参数警告
    return value ? 0 : -1;
}

int uvhttp_validate_url(const char* url, size_t length) {
    // 简单包装，实际验证逻辑在 uvhttp_validation.c 中
    return url && length > 0 ? 0 : -1;
}

int uvhttp_validate_method(const char* method, size_t length) {
    // 简单包装，实际验证逻辑在 uvhttp_validation.c 中
    return method && length > 0 ? 0 : -1;
}

// 完全兼容的旧函数名（已弃用）
int safe_strncpy(char* dest, const char* src, size_t dest_size) {
    return uvhttp_safe_strncpy(dest, src, dest_size);
}

int validate_header_value(const char* value, size_t length) {
    return uvhttp_validate_header_value(value, length);
}

int validate_url(const char* url, size_t length) {
    return uvhttp_validate_url(url, length);
}

int validate_method(const char* method, size_t length) {
    return uvhttp_validate_method(method, length);
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