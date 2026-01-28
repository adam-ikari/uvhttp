/**
 * @file uvhttp_error_handler.c
 * @brief 统一错误处理框架实现
 */

#include "uvhttp_error_handler.h"
#include "uvhttp_allocator.h"
#include "uvhttp_platform.h"
#include "uvhttp_logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* 全局错误处理配置 */
uvhttp_error_config_t g_error_config = {
    .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
    .customHandler = NULL,
    .enableRecovery = 1,
    .maxRetries = 3,
    .baseDelayMs = 100,
    .maxDelayMs = 5000,
    .backoffMultiplier = 2.0
};

/* 错误统计 */
static struct {
    size_t error_counts[UVHTTP_ERROR_COUNT];
    time_t last_error_time;
    char last_error_context[256];
    size_t total_errors;
} g_error_stats = {0};

/* 日志级别字符串 */


/* 内部函数声明 */
static void default_error_handler(const uvhttp_error_context_t* context);
static int calculate_retry_delay(int attempt);
static void sleep_ms(int ms);

/* 初始化错误处理系统 */
void uvhttp_error_init(void) {
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    UVHTTP_LOG_INFO("Error handling system initialized");
}

/* 清理错误处理系统 */
void uvhttp_error_cleanup(void) {
    UVHTTP_LOG_INFO("Error handling system cleanup");
    UVHTTP_LOG_INFO("Total errors handled: %zu", g_error_stats.total_errors);
}

/* 设置错误处理配置 */
void uvhttp_error_set_config(const uvhttp_error_config_t* config) {
    if (config) {
        g_error_config = *config;
        UVHTTP_LOG_INFO("Error handling configuration updated");
    }
}

/* 内部错误报告函数 */
void uvhttp_error_report_(uvhttp_error_t error_code, 
                         const char* message,
                         const char* function,
                         const char* file,
                         int line,
                         void* user_data) {
    /* 更新统计 */
    int index = (error_code < 0) ? -error_code : 0;
    if (index >= 0 && index < UVHTTP_ERROR_COUNT) {
        g_error_stats.error_counts[index]++;
    }
    g_error_stats.total_errors++;
    g_error_stats.last_error_time = time(NULL);
    
    /* 格式化错误上下文 */
    char context_msg[512];
    snprintf(context_msg, sizeof(context_msg),
             "%s:%d in %s() - %s (%s)",
             file, line, function, message, uvhttp_error_string(error_code));
    
    /* 确保不超过目标缓冲区大小 */
    size_t copy_len = strlen(context_msg);
    if (copy_len >= sizeof(g_error_stats.last_error_context)) {
        copy_len = sizeof(g_error_stats.last_error_context) - 1;
    }
    memcpy(g_error_stats.last_error_context, context_msg, copy_len);
    g_error_stats.last_error_context[copy_len] = '\0';
    
    /* 创建错误上下文 */
    uvhttp_error_context_t context = {
        .error_code = error_code,
        .function = function,
        .file = file,
        .line = line,
        .message = message,
        .timestamp = time(NULL),
        .user_data = user_data
    };
    
    /* 调用错误处理器 */
    if (g_error_config.customHandler) {
        g_error_config.customHandler(&context);
    } else {
        default_error_handler(&context);
    }
}

/* 默认错误处理器 */
static void default_error_handler(const uvhttp_error_context_t* context) {
    /* 对于严重错误，尝试恢复 */
    if (context->error_code <= UVHTTP_ERROR_SERVER_STOP && 
        g_error_config.enableRecovery) {
        uvhttp_error_attempt_recovery(context);
    }
    
    /* 对于致命错误，可能需要退出 */
    if (context->error_code == UVHTTP_ERROR_OUT_OF_MEMORY ||
        context->error_code == UVHTTP_ERROR_SERVER_INIT) {
        /* 致命错误，考虑优雅关闭 */
    }
}

/* 错误恢复尝试 */
uvhttp_error_t uvhttp_error_attempt_recovery(const uvhttp_error_context_t* context) {
    if (!g_error_config.enableRecovery) {
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
            for (int attempt = 0; attempt < g_error_config.maxRetries; attempt++) {
                int delay = calculate_retry_delay(attempt);
                UVHTTP_LOG_INFO("Retry attempt %d after %d ms", attempt + 1, delay);
                sleep_ms(delay);
                
                /* 这里应该有实际的重试逻辑 */
                /* 暂时返回成功，实际实现需要根据具体错误类型 */
                if (attempt == g_error_config.maxRetries - 1) {
                    UVHTTP_LOG_WARN("Recovery failed after %d attempts", 
                                   g_error_config.maxRetries);
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
static int calculate_retry_delay(int attempt) {
    int delay = g_error_config.baseDelayMs;
    for (int i = 0; i < attempt; i++) {
        delay *= g_error_config.backoffMultiplier;
    }
    return (delay > g_error_config.maxDelayMs) ? 
           g_error_config.maxDelayMs : delay;
}

/* 毫秒级睡眠 */
static void sleep_ms(int ms) {
    uvhttp_sleep_ms(ms);
}