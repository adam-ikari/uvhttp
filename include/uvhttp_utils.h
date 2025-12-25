#ifndef UVHTTP_UTILS_H
#define UVHTTP_UTILS_H

#include <stddef.h>
#include "uvhttp_error.h"

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
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response, 
                                          const char* content, 
                                          size_t length, 
                                          int status_code);

// 便捷响应发送函数
uvhttp_error_t uvhttp_send_json_response(uvhttp_response_t* response, 
                                        const char* json_content, 
                                        int status_code);

uvhttp_error_t uvhttp_send_html_response(uvhttp_response_t* response, 
                                        const char* html_content, 
                                        int status_code);

uvhttp_error_t uvhttp_send_text_response(uvhttp_response_t* response, 
                                        const char* text_content, 
                                        int status_code);

// 错误响应
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response, 
                                         int error_code, 
                                         const char* error_message, 
                                         const char* details);

/* ============ 验证函数 ============ */

// 验证 HTTP 状态码有效性
int uvhttp_is_valid_status_code(int status_code);

// 验证 Content-Type 格式
int uvhttp_is_valid_content_type(const char* content_type);

// 验证字符串长度
int uvhttp_is_valid_string_length(const char* str, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif