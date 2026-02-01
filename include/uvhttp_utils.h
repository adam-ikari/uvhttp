/**
 * @file uvhttp_utils.h
 * @brief Utility functions for UVHTTP
 *
 * This module provides utility functions including response handling,
 * string operations, and validation helpers.
 */

#ifndef UVHTTP_UTILS_H
#define UVHTTP_UTILS_H

#include "uvhttp_error.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
typedef struct uvhttp_response uvhttp_response_t;

/* 安全的字符串操作 */

/**
 * @brief Safe string copy with size checking
 *
 * @param dest Destination buffer
 * @param src Source string to copy
 * @param dest_size Size of destination buffer
 * @return int Number of characters copied (excluding null terminator)
 *
 * @note Ensures destination is null-terminated
 * @note Returns 0 if dest_size is 0
 */
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size);

/* 注意：输入验证函数已移动到 uvhttp_validation.h */
/* 请使用 #include "uvhttp_validation.h" 来访问验证函数 */

/* ============ 统一响应处理函数 ============ */

/**
 * @brief Send unified response with custom content
 *
 * @param response Response object
 * @param content Response body content
 * @param length Length of content in bytes
 * @param status_code HTTP status code (e.g., 200, 404)
 * @return uvhttp_error_t UVHTTP_OK on success, error code otherwise
 *
 * @note Caller is responsible for setting Content-Type header
 * @note Automatically sets status code and sends response
 */
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response,
                                            const char* content, size_t length,
                                            int status_code);

/**
 * @brief Send error response in JSON format
 *
 * @param response Response object
 * @param error_code Error code to include in response
 * @param error_message Error message
 * @param details Additional error details (can be NULL)
 * @return uvhttp_error_t UVHTTP_OK on success, error code otherwise
 *
 * @note Response format: {"error": {"code": X, "message": "...", "details":
 * "..."}}
 * @note Automatically sets Content-Type to application/json
 * @note Automatically sets status code based on error_code
 */
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response,
                                          int error_code,
                                          const char* error_message,
                                          const char* details);

/* ============ 验证函数 ============ */

/**
 * @brief Validate HTTP status code
 *
 * @param status_code Status code to validate
 * @return int TRUE if valid, FALSE otherwise
 *
 * @note Valid range: 100-599
 * @note Checks for standard HTTP status code ranges
 */
int uvhttp_is_valid_status_code(int status_code);

/* uvhttp_is_valid_content_type 已删除 - 完全未使用 */
/* uvhttp_is_valid_string_length 已删除 - 完全未使用 */
/* uvhttp_is_valid_ipv4 已删除 - 完全未使用 */
/* uvhttp_is_valid_ipv6 已删除 - 完全未使用 */

/**
 * @brief Validate IP address format (IPv4 or IPv6)
 *
 * @param ip IP address string to validate
 * @return int TRUE if valid, FALSE otherwise
 *
 * @note Supports both IPv4 (e.g., "192.168.1.1") and IPv6 (e.g., "::1")
 * @note Simplified validation, does not check all edge cases
 */
int uvhttp_is_valid_ip_address(const char* ip);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_UTILS_H */