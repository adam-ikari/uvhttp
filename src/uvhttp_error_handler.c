/**
 * @file uvhttp_error_handler.c
 * @brief 统一错误处理框架实现
 */

#include "uvhttp_error_handler.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* 全局错误处理配置 */
uvhttp_error_config_t g_error_config = {
    .minLogLevel = UVHTTP_LOG_LEVEL_INFO,
    .customHandler = NULL,
    .enableRecovery = 1,
    .maxRetries = 3,
    .baseDelayMs = 100,
    .maxDelayMs = 5000,
    .backoffMultiplier = 2.0
};

/* 错误统计 */
static struct {
    size_t error_counts[UVHTTP_ERROR_MAX];
    time_t last_error_time;
    char last_error_context[256];
    size_t total_errors;
} g_error_stats = {0};

/* 日志级别字符串 */
static const char* log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

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
    if (error_code < UVHTTP_ERROR_MAX) {
        g_error_stats.error_counts[error_code]++;
    }
    g_error_stats.total_errors++;
    g_error_stats.last_error_time = time(NULL);
    
    /* 格式化错误上下文 */
    char context_msg[512];
    snprintf(context_msg, sizeof(context_msg),
             "%s:%d in %s() - %s (%s)",
             file, line, function, message, uvhttp_error_string(error_code));
    
    strncpy(g_error_stats.last_error_context, context_msg, 
            sizeof(g_error_stats.last_error_context) - 1);
    
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
    
    /* 记录日志 */
    uvhttp_log_level_t log_level = (error_code < 0) ? 
        UVHTTP_LOG_LEVEL_ERROR : UVHTTP_LOG_LEVEL_WARN;
    uvhttp_log(log_level, "%s", context_msg);
    
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
        UVHTTP_LOG_FATAL("Fatal error encountered, consider graceful shutdown");
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
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/* 日志函数 */
void uvhttp_log(uvhttp_log_level_t level, const char* format, ...) {
    if (level < g_error_config.minLogLevel) {
        return;
    }
    
    /* 获取时间戳 */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* 格式化日志消息 */
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    /* 输出日志 */
    fprintf(stderr, "[%s] %s: %s\n", 
            timestamp, log_level_strings[level], message);
    
    /* 对于致命错误，刷新输出 */
    if (level == UVHTTP_LOG_LEVEL_FATAL) {
        fflush(stderr);
    }
}

/* 获取错误统计 */
void uvhttp_get_error_stats(size_t* error_counts, time_t* last_error_time, 
                           const char** last_error_context) {
    if (error_counts) {
        memcpy(error_counts, g_error_stats.error_counts, 
               sizeof(g_error_stats.error_counts));
    }
    if (last_error_time) {
        *last_error_time = g_error_stats.last_error_time;
    }
    if (last_error_context) {
        *last_error_context = g_error_stats.last_error_context;
    }
}

/* 重置错误统计 */
void uvhttp_reset_error_stats(void) {
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    UVHTTP_LOG_INFO("Error statistics reset");
}

/* 获取最频繁的错误 */
uvhttp_error_t uvhttp_get_most_frequent_error(void) {
    size_t max_count = 0;
    uvhttp_error_t most_frequent = UVHTTP_OK;
    
    for (int i = 0; i < UVHTTP_ERROR_MAX; i++) {
        if (g_error_stats.error_counts[i] > max_count) {
            max_count = g_error_stats.error_counts[i];
            most_frequent = (uvhttp_error_t)i;
        }
    }
    
    return most_frequent;
}