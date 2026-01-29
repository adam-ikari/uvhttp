/**
 * @file uvhttp_error_handler.c
 * @brief 错误统计和恢复机制实现
 */

#include "uvhttp_error_handler.h"

#include "uvhttp_allocator.h"
#include "uvhttp_logging.h"
#include "uvhttp_platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 全局错误恢复配置 */
uvhttp_error_recovery_config_t g_error_recovery_config = {
    .custom_handler = NULL,
    .enable_recovery = 1,
    .max_retries = 3,
    .base_delay_ms = 100,
    .max_delay_ms = 5000,
    .backoff_multiplier = 2.0};

/* 错误统计 */
static uvhttp_error_stats_t g_error_stats = {0};

/* 内部函数声明 */
static int calculate_retry_delay(int attempt);
static void sleep_ms(int ms);

/* ========== 错误统计函数 ========== */

const uvhttp_error_stats_t*
uvhttp_error_get_stats(void) {
    return &g_error_stats;
}

void
uvhttp_error_reset_stats(void) {
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    UVHTTP_LOG_INFO("Error statistics reset");
}

/* ========== 错误恢复函数 ========== */

void
uvhttp_error_recovery_init(void) {
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    UVHTTP_LOG_INFO("Error recovery system initialized");
}

void
uvhttp_error_cleanup(void) {
    UVHTTP_LOG_INFO("Error recovery system cleanup");
    /* 计算总错误数 */
    size_t total = 0;
    for (int i = 0; i < UVHTTP_ERROR_COUNT; i++) {
        total += g_error_stats.error_counts[i];
    }
    UVHTTP_LOG_INFO("Total errors handled: %zu", total);
}

void
uvhttp_error_set_recovery_config(const uvhttp_error_recovery_config_t* config) {
    if (config) {
        g_error_recovery_config = *config;
        UVHTTP_LOG_INFO("Error recovery configuration updated");
    }
}

/* ========== 内部错误报告函数 ========== */

void
uvhttp_error_report_(uvhttp_error_t error_code, const char* message,
                     const char* function, const char* file, int line,
                     void* user_data) {
    /* 更新统计 */
    int index = (error_code < 0) ? -error_code : 0;
    if (index >= 0 && index < UVHTTP_ERROR_COUNT) {
        g_error_stats.error_counts[index]++;
    }
    g_error_stats.last_error_time = time(NULL);

    /* 格式化错误上下文 */
    char context_msg[512];
    snprintf(context_msg, sizeof(context_msg), "%s:%d in %s() - %s (%s)", file,
             line, function, message, uvhttp_error_string(error_code));

    /* 确保不超过目标缓冲区大小 */
    size_t copy_len = strlen(context_msg);
    if (copy_len >= sizeof(g_error_stats.last_error_context)) {
        copy_len = sizeof(g_error_stats.last_error_context) - 1;
    }
    memcpy(g_error_stats.last_error_context, context_msg, copy_len);
    g_error_stats.last_error_context[copy_len] = '\0';

    /* 调用自定义错误处理器（如果配置） */
    if (g_error_recovery_config.custom_handler) {
        uvhttp_error_context_t context = {.error_code = error_code,
                                          .function = function,
                                          .file = file,
                                          .line = line,
                                          .message = message,
                                          .timestamp = time(NULL),
                                          .user_data = user_data};
        g_error_recovery_config.custom_handler(&context);
    }
}

/* 错误恢复尝试 */
uvhttp_error_t
uvhttp_error_attempt_recovery(const uvhttp_error_context_t* context) {
    if (!g_error_recovery_config.enable_recovery) {
        return context->error_code;
    }

    UVHTTP_LOG_INFO("Attempting error recovery for %s",
                    uvhttp_error_string(context->error_code));

    /* 根据错误类型进行恢复 */
    switch (context->error_code) {
    case UVHTTP_ERROR_CONNECTION_ACCEPT:
    case UVHTTP_ERROR_CONNECTION_START:
    case UVHTTP_ERROR_RESPONSE_SEND:
        /* 网络相关错误，可以重试 */
        for (int attempt = 0; attempt < g_error_recovery_config.max_retries;
             attempt++) {
            int delay = calculate_retry_delay(attempt);
            UVHTTP_LOG_INFO("Retry attempt %d after %d ms", attempt + 1, delay);
            sleep_ms(delay);

            /* 这里应该有实际的重试逻辑 */
            /* 暂时返回成功，实际实现需要根据具体错误类型 */
            if (attempt == g_error_recovery_config.max_retries - 1) {
                UVHTTP_LOG_WARN("Recovery failed after %d attempts",
                                g_error_recovery_config.max_retries);
                return context->error_code;
            }
        }
        break;

    case UVHTTP_ERROR_OUT_OF_MEMORY:
/* 内存不足，尝试垃圾回收 */
#ifdef UVHTTP_ENABLE_MEMORY_DEBUG
        uvhttp_reset_memory_stats();
#endif
        UVHTTP_LOG_INFO("Memory recovery attempted");
        break;

    default:
        /* 其他错误不进行恢复 */
        break;
    }

    return UVHTTP_OK;
}

/* 计算重试延迟 */
static int
calculate_retry_delay(int attempt) {
    int delay = g_error_recovery_config.base_delay_ms;
    for (int i = 0; i < attempt; i++) {
        delay *= g_error_recovery_config.backoff_multiplier;
    }
    return (delay > g_error_recovery_config.max_delay_ms)
               ? g_error_recovery_config.max_delay_ms
               : delay;
}

/* 毫秒级睡眠 */
static void
sleep_ms(int ms) {
    uvhttp_sleep_ms(ms);
}