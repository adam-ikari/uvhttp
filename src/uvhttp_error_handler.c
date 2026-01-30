/**
 * @file uvhttp_error_handler.c
 * @brief 错误统计实现
 */

#include "uvhttp_error_handler.h"

#include "uvhttp_allocator.h"
#include "uvhttp_logging.h"

#include <string.h>
#include <time.h>

/* 错误统计 */
static uvhttp_error_stats_t g_error_stats = {0};

/* ========== 错误统计函数 ========== */

const uvhttp_error_stats_t* uvhttp_error_get_stats(void) {
    return &g_error_stats;
}

void uvhttp_error_reset_stats(void) {
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    UVHTTP_LOG_INFO("Error statistics reset");
}

/* ========== 内部错误报告函数 ========== */

void uvhttp_error_report_(uvhttp_error_t error_code, const char* message,
                          const char* function, const char* file, int line,
                          void* user_data) {
    (void)function;
    (void)file;
    (void)line;
    (void)user_data;

    /* 更新统计 */
    int index = (error_code < 0) ? -error_code : 0;
    if (index >= 0 && index < UVHTTP_ERROR_COUNT) {
        g_error_stats.error_counts[index]++;
    }
    g_error_stats.last_error_time = time(NULL);

    /* 格式化错误上下文 */
    char context_msg[512];
    snprintf(context_msg, sizeof(context_msg), "%s (%s)", message,
             uvhttp_error_string(error_code));

    /* 确保不超过目标缓冲区大小 */
    size_t copy_len = strlen(context_msg);
    if (copy_len >= sizeof(g_error_stats.last_error_context)) {
        copy_len = sizeof(g_error_stats.last_error_context) - 1;
    }
    memcpy(g_error_stats.last_error_context, context_msg, copy_len);
    g_error_stats.last_error_context[copy_len] = '\0';
}