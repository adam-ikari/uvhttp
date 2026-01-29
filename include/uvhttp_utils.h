#ifndef UVHTTP_UTILS_H
#define UVHTTP_UTILS_H

#include "uvhttp_error.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_response uvhttp_response_t;

// 安全的字符串操作
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size);

// 注意：输入验证函数已移动到 uvhttp_validation.h
// 请使用 #include "uvhttp_validation.h" 来访问验证函数

/* ============ 统一响应处理函数 ============ */

// 统一响应发送 - 由使用者设置 Content-Type
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response, const char* content,
                                            size_t length, int status_code);

// 错误响应 - 统一处理，返回 JSON 格式错误
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response, int error_code,
                                          const char* error_message, const char* details);

/* ============ 验证函数 ============ */

// 验证 HTTP 状态码有效性
int uvhttp_is_valid_status_code(int status_code);

// 验证 Content-Type 格式
int uvhttp_is_valid_content_type(const char* content_type);

// 验证字符串长度
int uvhttp_is_valid_string_length(const char* str, size_t max_len);

// 验证 IPv4 地址格式
int uvhttp_is_valid_ipv4(const char* ip);

// 验证 IPv6 地址格式
int uvhttp_is_valid_ipv6(const char* ip);

// 验证 IP 地址格式（IPv4 或 IPv6）
int uvhttp_is_valid_ip_address(const char* ip);

#ifdef __cplusplus
}
#endif

#endif