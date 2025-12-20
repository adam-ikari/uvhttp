#ifndef UVHTTP_UTILS_H
#define UVHTTP_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 安全的字符串操作
int safe_strncpy(char* dest, const char* src, size_t dest_size);

// 输入验证函数
int validate_header_value(const char* value, size_t length);
int validate_url(const char* url, size_t length);
int validate_method(const char* method, size_t length);

// JSON工具函数
char* uvhttp_escape_json_string(const char* str);

#ifdef __cplusplus
}
#endif

#endif