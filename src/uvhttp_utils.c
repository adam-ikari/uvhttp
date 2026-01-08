#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <time.h>
#include "uvhttp_common.h"
#include "uvhttp_response.h"
#include "uvhttp_utils.h"

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

/* ============ 核心工具函数 ============ */

// 安全字符串复制函数
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    
    return 0;  // 成功返回0，失败返回-1
}

/* ============ 内部辅助函数 ============ */

// 内部辅助函数：验证状态码有效性
static int is_valid_status_code(int code) {
    return (code >= 100 && code <= 599);
}

// 内部辅助函数：验证字符串长度
static int is_valid_string_length(const char* str, size_t max_len) {
    if (!str) return 0;
    return (strlen(str) <= max_len);
}



/* ============ 统一响应处理函数 ============ */

/**
 * @brief 统一的响应发送函数 - 由使用者设置 Content-Type
 * @param response 响应对象
 * @param content 内容数据
 * @param length 内容长度（如果为0则自动计算）
 * @param status_code HTTP状态码（如果为0则使用响应对象中已有的状态码）
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response, 
                                          const char* content, 
                                          size_t length, 
                                          int status_code) {
    // 参数验证
    if (!response || !content) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证状态码（如果提供）
    if (status_code != 0 && !is_valid_status_code(status_code)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 处理长度参数
    if (length == 0) {
        length = strlen(content);
    }
    
    // 验证内容长度
    if (length == 0 || length > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 设置状态码（如果提供了有效状态码）
    if (status_code != 0) {
        uvhttp_response_set_status(response, status_code);
    }
    
    // 设置响应体
    uvhttp_error_t err = uvhttp_response_set_body(response, content, length);
    if (err != UVHTTP_OK) {
        return err;
    }
    
    // 发送响应
    return uvhttp_response_send(response);
}



/**
 * @brief 创建标准错误响应（JSON格式）
 * @param response 响应对象
 * @param error_code 错误代码
 * @param error_message 错误消息
 * @param details 详细信息（可选）
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response, 
                                         int error_code, 
                                         const char* error_message, 
                                         const char* details) {
    // 参数验证
    if (!response || !error_message) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证错误代码范围
    if (!is_valid_status_code(error_code)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证字符串长度，防止缓冲区溢出
    #define MAX_ERROR_MSG_LEN 200
    #define MAX_ERROR_DETAILS_LEN 400
    
    if (!is_valid_string_length(error_message, MAX_ERROR_MSG_LEN)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (details && !is_valid_string_length(details, MAX_ERROR_DETAILS_LEN)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 创建安全的 JSON 错误响应
    char error_json[1024];
    int json_len;
    
    if (details && strlen(details) > 0) {
        json_len = snprintf(error_json, sizeof(error_json), 
                "{\"error\":\"%s\",\"details\":\"%s\",\"code\":%d,\"timestamp\":%ld}",
                error_message, details, error_code, time(NULL));
    } else {
        json_len = snprintf(error_json, sizeof(error_json), 
                "{\"error\":\"%s\",\"code\":%d,\"timestamp\":%ld}",
                error_message, error_code, time(NULL));
    }
    
    // 验证 snprintf 是否成功
    if (json_len < 0 || json_len >= (int)sizeof(error_json)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    uvhttp_response_set_status(response, error_code);
    uvhttp_response_set_header(response, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(response, error_json, strlen(error_json));
    return uvhttp_response_send(response);
}

/* ============ 公共验证函数实现 ============ */

/**
 * @brief 验证 HTTP 状态码有效性
 * @param status_code 状态码
 * @return 1 表示有效，0 表示无效
 */
int uvhttp_is_valid_status_code(int status_code) {
    return (status_code >= 100 && status_code <= 599);
}

/**
 * @brief 验证 Content-Type 格式
 * @param content_type Content-Type 字符串
 * @return 1 表示有效，0 表示无效
 */
int uvhttp_is_valid_content_type(const char* content_type) {
    if (!content_type || strlen(content_type) == 0) {
        return 0;
    }
    
    // 基本格式验证：应该包含 '/'
    const char* slash = strchr(content_type, '/');
    if (!slash) {
        return 0;
    }
    
    // 检查是否有非法字符
    const char* invalid_chars = "\"\\()<>@,;:\\[]?=";
    for (const char* p = content_type; *p; p++) {
        if (strchr(invalid_chars, *p)) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief 验证字符串长度
 * @param str 字符串
 * @param max_len 最大长度
 * @return 1 表示有效，0 表示无效
 */
int uvhttp_is_valid_string_length(const char* str, size_t max_len) {
    if (!str) return 0;
    return (strlen(str) <= max_len);
}
