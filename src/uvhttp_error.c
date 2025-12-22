#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* 错误恢复配置 */
typedef struct {
    int max_retries;
    int base_delay_ms;
    int max_delay_ms;
    double backoff_multiplier;
} uvhttp_error_recovery_config_t;

static uvhttp_error_recovery_config_t recovery_config = {
    .max_retries = 3,
    .base_delay_ms = 100,
    .max_delay_ms = 5000,
    .backoff_multiplier = 2.0
};

/* 错误统计 */
typedef struct {
    size_t error_counts[UVHTTP_ERROR_MAX];
    time_t last_error_time;
    char last_error_context[256];
} uvhttp_error_stats_t;

static uvhttp_error_stats_t error_stats = {0};

/* 设置错误恢复配置 */
void uvhttp_set_error_recovery_config(int max_retries, int base_delay_ms, 
                                     int max_delay_ms, double backoff_multiplier) {
    recovery_config.max_retries = max_retries > 0 ? max_retries : 3;
    recovery_config.base_delay_ms = base_delay_ms > 0 ? base_delay_ms : 100;
    recovery_config.max_delay_ms = max_delay_ms > 0 ? max_delay_ms : 5000;
    recovery_config.backoff_multiplier = backoff_multiplier > 1.0 ? backoff_multiplier : 2.0;
}

/* 计算重试延迟（指数退避） */
static int calculate_retry_delay(int attempt) {
    int delay = recovery_config.base_delay_ms;
    for (int i = 0; i < attempt; i++) {
        delay *= recovery_config.backoff_multiplier;
    }
    return delay > recovery_config.max_delay_ms ? recovery_config.max_delay_ms : delay;
}

/* 执行重试延迟 */
static void retry_delay(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/* 判断错误是否可重试 */
static int is_retryable_error(uvhttp_error_t error) {
    switch (error) {
        case UVHTTP_ERROR_CONNECTION_ACCEPT:
        case UVHTTP_ERROR_CONNECTION_START:
        case UVHTTP_ERROR_RESPONSE_SEND:
        case UVHTTP_ERROR_TLS_HANDSHAKE:
        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
            return 1;
        
        case UVHTTP_ERROR_INVALID_PARAM:
        case UVHTTP_ERROR_OUT_OF_MEMORY:
        case UVHTTP_ERROR_NOT_FOUND:
        case UVHTTP_ERROR_CONNECTION_LIMIT:
        case UVHTTP_ERROR_SERVER_INIT:
        case UVHTTP_ERROR_ROUTER_INIT:
            return 0;
        
        default:
            return 0;
    }
}

/* 带重试的错误处理函数 */
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*), 
                                     void* context, const char* operation_name) {
    uvhttp_error_t last_error = UVHTTP_OK;
    
    for (int attempt = 0; attempt <= recovery_config.max_retries; attempt++) {
        last_error = operation(context);
        
        if (last_error == UVHTTP_OK) {
            /* 成功，重置失败计数 */
            return UVHTTP_OK;
        }
        
        /* 记录错误统计 */
        if (last_error < UVHTTP_ERROR_MAX) {
            error_stats.error_counts[last_error]++;
        }
        error_stats.last_error_time = time(NULL);
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s (attempt %d)", operation_name, attempt + 1);
        
        /* 如果是最后一次尝试或错误不可重试，返回错误 */
        if (attempt == recovery_config.max_retries || !is_retryable_error(last_error)) {
            break;
        }
        
        /* 计算并执行延迟 */
        int delay = calculate_retry_delay(attempt);
        retry_delay(delay);
    }
    
    return last_error;
}

/* 记录错误 */
void uvhttp_log_error(uvhttp_error_t error, const char* context) {
    if (error < UVHTTP_ERROR_MAX) {
        error_stats.error_counts[error]++;
    }
    error_stats.last_error_time = time(NULL);
    
    if (context) {
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s: %s", context, uvhttp_error_string(error));
    } else {
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s", uvhttp_error_string(error));
    }
}

/* 获取错误统计 */
void uvhttp_get_error_stats(size_t* error_counts, time_t* last_error_time, 
                           const char** last_error_context) {
    if (error_counts) {
        memcpy(error_counts, error_stats.error_counts, sizeof(error_stats.error_counts));
    }
    if (last_error_time) {
        *last_error_time = error_stats.last_error_time;
    }
    if (last_error_context) {
        *last_error_context = error_stats.last_error_context;
    }
}

/* 重置错误统计 */
void uvhttp_reset_error_stats(void) {
    memset(&error_stats, 0, sizeof(error_stats));
}

/* 获取最频繁的错误 */
uvhttp_error_t uvhttp_get_most_frequent_error(void) {
    size_t max_count = 0;
    uvhttp_error_t most_frequent = UVHTTP_OK;
    
    for (int i = 0; i < UVHTTP_ERROR_MAX; i++) {
        if (error_stats.error_counts[i] > max_count) {
            max_count = error_stats.error_counts[i];
            most_frequent = (uvhttp_error_t)i;
        }
    }
    
    return most_frequent;
}

const char* uvhttp_error_string(uvhttp_error_t error) {
    switch (error) {
        case UVHTTP_OK:
            return "Success";
        
        /* General errors */
        case UVHTTP_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case UVHTTP_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case UVHTTP_ERROR_NOT_FOUND:
            return "Not found";
        case UVHTTP_ERROR_ALREADY_EXISTS:
            return "Already exists";
        
        /* Server errors */
        case UVHTTP_ERROR_SERVER_INIT:
            return "Server initialization failed";
        case UVHTTP_ERROR_SERVER_LISTEN:
            return "Server listen failed";
        case UVHTTP_ERROR_SERVER_STOP:
            return "Server stop failed";
        case UVHTTP_ERROR_CONNECTION_LIMIT:
            return "Connection limit reached";
        
        /* Connection errors */
        case UVHTTP_ERROR_CONNECTION_INIT:
            return "Connection initialization failed";
        case UVHTTP_ERROR_CONNECTION_ACCEPT:
            return "Connection accept failed";
        case UVHTTP_ERROR_CONNECTION_START:
            return "Connection start failed";
        case UVHTTP_ERROR_CONNECTION_CLOSE:
            return "Connection close failed";
        
        /* Request/Response errors */
        case UVHTTP_ERROR_REQUEST_INIT:
            return "Request initialization failed";
        case UVHTTP_ERROR_RESPONSE_INIT:
            return "Response initialization failed";
        case UVHTTP_ERROR_RESPONSE_SEND:
            return "Response send failed";
        
        /* TLS errors */
        case UVHTTP_ERROR_TLS_INIT:
            return "TLS initialization failed";
        case UVHTTP_ERROR_TLS_CONTEXT:
            return "TLS context creation failed";
        case UVHTTP_ERROR_TLS_HANDSHAKE:
            return "TLS handshake failed";
        
        /* Router errors */
        case UVHTTP_ERROR_ROUTER_INIT:
            return "Router initialization failed";
        case UVHTTP_ERROR_ROUTER_ADD:
            return "Router add failed";
        
        /* Allocator errors */
        case UVHTTP_ERROR_ALLOCATOR_INIT:
            return "Allocator initialization failed";
        case UVHTTP_ERROR_ALLOCATOR_SET:
            return "Allocator set failed";
        
        /* WebSocket errors */
        case UVHTTP_ERROR_WEBSOCKET_INIT:
            return "WebSocket initialization failed";
        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
            return "WebSocket handshake failed";
        case UVHTTP_ERROR_WEBSOCKET_FRAME:
            return "WebSocket frame processing failed";
        
        /* HTTP/2 errors */

        
        default:
            return "Unknown error";
    }
}