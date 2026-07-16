/* UVHTTP Error Handling Helper Functions Implementation */

#include "uvhttp_error_helpers.h"

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_logging.h"
#include "uvhttp_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sensitive information keyword list */
static const char* sensitive_keywords[] = {
    "password", "passwd",     "secret",  "key",     "token",
    "auth",     "credential", "private", "session", NULL};

/**
 * Check if string contains sensitive information
 */
static int contains_sensitive_info(const char* str) {
    if (!str)
        return FALSE;

    char lower_str[UVHTTP_ERROR_MESSAGE_BUFFER_SIZE];
    uvhttp_safe_strncpy(lower_str, str, sizeof(lower_str));

    // Convert to lowercase for comparison
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
    /* NOTE: do NOT use uv_strerror(status) here. For status values outside the
     * libuv errno map (this includes status==0 on the success path AND any
     * other non-mapped/internal code), uv_strerror falls through to
     * uv__unknown_err_code, which uv__strdup's "Unknown system error N" and
     * never frees it - a real leak observed under ASan. uv_strerror_r writes
     * the description into a caller-provided buffer and never allocates,
     * regardless of the status value. This mirrors the reasoning in
     * uvhttp_log_safe_error. */
    char err_desc[UVHTTP_ERROR_CONTEXT_BUFFER_SIZE];
    uv_strerror_r(status, err_desc, sizeof(err_desc));

    if (uvhttp_sanitize_error_message(err_desc, safe_msg, sizeof(safe_msg)) ==
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
    /* 注意: error_code 是 UVHTTP 错误码, 必须用 uvhttp_error_string 解析,
     * 不能用 uv_strerror。后者对未知(非 libuv errno)的错误码会 strdup 出
     * "Unknown system error N" 且永不释放(泄漏), 同时 UVHTTP 错误码也会被
     * 误报成 libuv 的未知系统错误。uvhttp_error_string 返回静态字符串字面量,
     * 无内存分配。 */
    const char* error_desc = error_code ? uvhttp_error_string(error_code) : user_msg;

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

    // Check if contains sensitive information
    if (contains_sensitive_info(message)) {
        snprintf(safe_buffer, buffer_size, "Sensitive information hidden");
        return UVHTTP_OK;
    }

    // Copy message, but limit length
    size_t msg_len = strlen(message);

    // Handle small buffer (buffer_size < 4)
    if (buffer_size < 4) {
        uvhttp_safe_strncpy(safe_buffer, message, buffer_size);
    } else if (msg_len >= buffer_size) {
        // Copy buffer_size - 4 characters, then add "..."
        size_t copy_len = buffer_size - 4;
        for (size_t i = 0; i < copy_len; i++) {
            safe_buffer[i] = message[i];
        }
        safe_buffer[copy_len] = '.';
        safe_buffer[copy_len + 1] = '.';
        safe_buffer[copy_len + 2] = '.';
        safe_buffer[buffer_size - 1] = '\0';
    } else {
        uvhttp_safe_strncpy(safe_buffer, message, buffer_size);
    }

    return UVHTTP_OK;
}
