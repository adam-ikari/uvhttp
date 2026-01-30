/* UVHTTP 错误处理辅助函数实现 */

#include "uvhttp_error_helpers.h"

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 敏感信息关键词列表 */
static const char* sensitive_keywords[] = {
    "password", "passwd",     "secret",  "key",     "token",
    "auth",     "credential", "private", "session", NULL};

/**
 * 检查字符串是否包含敏感信息
 */
static int contains_sensitive_info(const char* str) {
    if (!str)
        return FALSE;

    char lower_str[UVHTTP_ERROR_MESSAGE_BUFFER_SIZE];
    strncpy(lower_str, str, sizeof(lower_str) - 1);
    lower_str[sizeof(lower_str) - 1] = '\0';

    // 转换为小写进行比较
    for (char* p = lower_str; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }

    for (int i = 0; sensitive_keywords[i]; i++) {
        if (strstr(lower_str, sensitive_keywords[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

void uvhttp_handle_memory_failure(const char* context,
                                  void (*cleanup_func)(void*),
                                  void* cleanup_data) {
    if (context) {
        UVHTTP_LOG_ERROR("Memory allocation failed in %s\n", context);
    }

    if (cleanup_func && cleanup_data) {
        cleanup_func(cleanup_data);
    }
}

void uvhttp_handle_write_error(uv_write_t* req, int status,
                               const char* context) {
    if (!req)
        return;

    char safe_msg[UVHTTP_ERROR_CONTEXT_BUFFER_SIZE];
    const char* error_desc = uv_strerror(status);

    if (uvhttp_sanitize_error_message(error_desc, safe_msg, sizeof(safe_msg)) ==
        0) {
        UVHTTP_LOG_ERROR("Write error in %s: %s\n", context, safe_msg);
    } else {
        UVHTTP_LOG_ERROR("Write error in %s: (error %d)\n", context, status);
    }

    (void)context;
    uvhttp_free(req);
}

void uvhttp_log_safe_error(int error_code, const char* context,
                           const char* user_msg) {
    char safe_buffer[UVHTTP_ERROR_LOG_BUFFER_SIZE];
    const char* error_desc = error_code ? uv_strerror(error_code) : user_msg;

    if (uvhttp_sanitize_error_message(error_desc, safe_buffer,
                                      sizeof(safe_buffer)) == 0) {
        UVHTTP_LOG_ERROR("[%s] %s\n", context ? context : "unknown",
                         safe_buffer);
    } else {
        UVHTTP_LOG_ERROR("[%s] Error occurred (code: %d)\n",
                         context ? context : "unknown", error_code);
    }

    (void)context;
}

uvhttp_error_t uvhttp_sanitize_error_message(const char* message,
                                             char* safe_buffer,
                                             size_t buffer_size) {
    if (!message || !safe_buffer || buffer_size == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 检查是否包含敏感信息
    if (contains_sensitive_info(message)) {
        snprintf(safe_buffer, buffer_size, "Sensitive information hidden");
        return UVHTTP_OK;
    }

    // 复制消息，但限制长度
    size_t msg_len = strlen(message);

    // 处理小缓冲区（buffer_size < 4）
    if (buffer_size < 4) {
        strncpy(safe_buffer, message, buffer_size - 1);
        safe_buffer[buffer_size - 1] = '\0';
    } else if (msg_len >= buffer_size) {
        strncpy(safe_buffer, message, buffer_size - 4);
        safe_buffer[buffer_size - 4] = '\0';
        strcat(safe_buffer, "...");
    } else {
        strncpy(safe_buffer, message, buffer_size - 1);
        safe_buffer[buffer_size - 1] = '\0';
    }

    return UVHTTP_OK;
}
void uvhttp_safe_free(void** ptr, void (*free_func)(void*)) {
    if (!ptr || !*ptr)
        return;

    if (free_func) {
        free_func(*ptr);
    } else {
        uvhttp_free(*ptr);
    }

    *ptr = NULL;
}