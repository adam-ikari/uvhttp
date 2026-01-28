#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_features.h"
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
    .base_delay_ms = UVHTTP_DEFAULT_BASE_DELAY_MS,
    .max_delay_ms = UVHTTP_DEFAULT_MAX_DELAY_MS,
    .backoff_multiplier = 2.0
};

/* 错误统计 - 仅在启用统计功能时定义 */
#if UVHTTP_FEATURE_STATISTICS
static uvhttp_error_stats_t error_stats = {0};
#endif

/* 设置错误恢复配置 */
void uvhttp_set_error_recovery_config(int max_retries, int base_delay_ms, 
                                     int max_delay_ms, double backoff_multiplier) {
    recovery_config.max_retries = max_retries > 0 ? max_retries : 3;
    recovery_config.base_delay_ms = base_delay_ms > 0 ? base_delay_ms : UVHTTP_DEFAULT_BASE_DELAY_MS;
    recovery_config.max_delay_ms = max_delay_ms > 0 ? max_delay_ms : UVHTTP_DEFAULT_MAX_DELAY_MS;
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
        /* 可重试的连接错误 */
        case UVHTTP_ERROR_CONNECTION_ACCEPT:
        case UVHTTP_ERROR_CONNECTION_START:
        case UVHTTP_ERROR_CONNECTION_RESET:
        case UVHTTP_ERROR_CONNECTION_TIMEOUT:
        case UVHTTP_ERROR_CONNECTION_REFUSED:
        case UVHTTP_ERROR_CONNECTION_BROKEN:
        
        /* 可重试的协议错误 */
        case UVHTTP_ERROR_RESPONSE_SEND:
        case UVHTTP_ERROR_TLS_HANDSHAKE:
        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
        case UVHTTP_ERROR_WEBSOCKET_FRAME:
        case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:
        case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:
        
        /* 可重试的错误 */
        case UVHTTP_ERROR_LOG_WRITE:
            return TRUE;
        
        /* 不可重试的错误 */
        case UVHTTP_ERROR_INVALID_PARAM:
        case UVHTTP_ERROR_OUT_OF_MEMORY:
        case UVHTTP_ERROR_NOT_FOUND:
        case UVHTTP_ERROR_ALREADY_EXISTS:
        case UVHTTP_ERROR_NULL_POINTER:
        case UVHTTP_ERROR_BUFFER_TOO_SMALL:
        case UVHTTP_ERROR_SERVER_INIT:
        case UVHTTP_ERROR_SERVER_LISTEN:
        case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:
        case UVHTTP_ERROR_SERVER_NOT_RUNNING:
        case UVHTTP_ERROR_SERVER_INVALID_CONFIG:
        case UVHTTP_ERROR_CONNECTION_INIT:
        case UVHTTP_ERROR_CONNECTION_CLOSE:
        case UVHTTP_ERROR_REQUEST_INIT:
        case UVHTTP_ERROR_RESPONSE_INIT:
        case UVHTTP_ERROR_INVALID_HTTP_METHOD:
        case UVHTTP_ERROR_INVALID_HTTP_VERSION:
        case UVHTTP_ERROR_HEADER_TOO_LARGE:
        case UVHTTP_ERROR_BODY_TOO_LARGE:
        case UVHTTP_ERROR_MALFORMED_REQUEST:
        case UVHTTP_ERROR_FILE_TOO_LARGE:
        case UVHTTP_ERROR_IO_ERROR:
        case UVHTTP_ERROR_TLS_INIT:
        case UVHTTP_ERROR_TLS_CONTEXT:
        case UVHTTP_ERROR_TLS_CERT_LOAD:
        case UVHTTP_ERROR_TLS_KEY_LOAD:
        case UVHTTP_ERROR_TLS_VERIFY_FAILED:
        case UVHTTP_ERROR_TLS_EXPIRED:
        case UVHTTP_ERROR_TLS_NOT_YET_VALID:
        case UVHTTP_ERROR_ROUTER_INIT:
        case UVHTTP_ERROR_ROUTER_ADD:
        case UVHTTP_ERROR_ROUTE_NOT_FOUND:
        case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:
        case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:
        case UVHTTP_ERROR_ALLOCATOR_INIT:
        case UVHTTP_ERROR_ALLOCATOR_SET:
        case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:
        case UVHTTP_ERROR_WEBSOCKET_INIT:
        case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:
        case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:
        case UVHTTP_ERROR_WEBSOCKET_CLOSED:
        case UVHTTP_ERROR_CONFIG_PARSE:
        case UVHTTP_ERROR_CONFIG_INVALID:
        case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:
        case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:
        case UVHTTP_ERROR_LOG_INIT:
        case UVHTTP_ERROR_LOG_FILE_OPEN:
        case UVHTTP_ERROR_LOG_NOT_INITIALIZED:
            return FALSE;
        
        default:
            return FALSE;
    }
}

/* 带重试的错误处理函数 */
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*),
                                     void* context, const char* operation_name) {
    (void)operation_name;  /* 预留参数，用于日志记录 */
    uvhttp_error_t last_error = UVHTTP_OK;
    
    for (int attempt = 0; attempt <= recovery_config.max_retries; attempt++) {
        last_error = operation(context);
        
        if (last_error == UVHTTP_OK) {
            /* 成功，重置失败计数 */
            return UVHTTP_OK;
        }
        
#if UVHTTP_FEATURE_STATISTICS
        /* 记录错误统计 */
        int index = (last_error < 0) ? -last_error : 0;
        if (index >= 0 && index < UVHTTP_ERROR_COUNT) {
            error_stats.error_counts[index]++;
        }
        error_stats.last_error_time = time(NULL);
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s (attempt %d)", operation_name, attempt + 1);
#endif
        
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
#if UVHTTP_FEATURE_STATISTICS
    /* 转换负数错误码为正数索引 */
    int index = (error < 0) ? -error : 0;
    
    if (index >= 0 && index < UVHTTP_ERROR_COUNT) {
        error_stats.error_counts[index]++;
    }
    error_stats.last_error_time = time(NULL);
    
    if (context) {
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s: %s", context, uvhttp_error_string(error));
    } else {
        snprintf(error_stats.last_error_context, sizeof(error_stats.last_error_context),
                "%s", uvhttp_error_string(error));
    }
#else
    (void)error;
    (void)context;
#endif
}

/* 获取错误统计 */
#if UVHTTP_FEATURE_STATISTICS
void uvhttp_get_error_stats(uvhttp_context_t* context, size_t* error_counts, time_t* last_error_time, 
                           const char** last_error_context) {
    /* 向后兼容：当 context 为 NULL 时使用静态全局变量 */
    uvhttp_error_stats_t* stats = NULL;
    
    if (context) {
        stats = (uvhttp_error_stats_t*)context->error_stats;
    } else {
        /* v2.0.0: 向后兼容，context 为 NULL 时使用静态全局变量 */
        stats = &error_stats;
    }
    
    if (!stats) {
        return;
    }
    
    if (error_counts) {
        memcpy(error_counts, stats->error_counts, sizeof(stats->error_counts));
    }
    if (last_error_time) {
        *last_error_time = stats->last_error_time;
    }
    if (last_error_context) {
        *last_error_context = stats->last_error_context;
    }
}
#else
void uvhttp_get_error_stats(uvhttp_context_t* context, size_t* error_counts, time_t* last_error_time, 
                           const char** last_error_context) {
    (void)context;
    (void)error_counts;
    (void)last_error_time;
    (void)last_error_context;
}
#endif

/* 重置错误统计 */
#if UVHTTP_FEATURE_STATISTICS
void uvhttp_reset_error_stats(uvhttp_context_t* context) {
    /* 向后兼容：当 context 为 NULL 时使用静态全局变量 */
    uvhttp_error_stats_t* stats = NULL;
    
    if (context) {
        stats = (uvhttp_error_stats_t*)context->error_stats;
    } else {
        /* v2.0.0: 向后兼容，context 为 NULL 时使用静态全局变量 */
        stats = &error_stats;
    }
    
    if (stats) {
        memset(stats, 0, sizeof(*stats));
    }
}
#else
void uvhttp_reset_error_stats(uvhttp_context_t* context) {
    (void)context;
}
#endif

/* 获取最频繁的错误 */
#if UVHTTP_FEATURE_STATISTICS
uvhttp_error_t uvhttp_get_most_frequent_error(uvhttp_context_t* context) {
    /* 向后兼容：当 context 为 NULL 时使用静态全局变量 */
    uvhttp_error_stats_t* stats = NULL;
    
    if (context) {
        stats = (uvhttp_error_stats_t*)context->error_stats;
    } else {
        /* v2.0.0: 向后兼容，context 为 NULL 时使用静态全局变量 */
        stats = &error_stats;
    }
    
    if (!stats) {
        return UVHTTP_OK;
    }
    
    size_t max_count = 0;
    int max_index = 0;
    
    for (int i = 0; i < UVHTTP_ERROR_COUNT; i++) {
        if (stats->error_counts[i] > max_count) {
            max_count = stats->error_counts[i];
            max_index = i;
        }
    }
    
    /* 返回负数错误码 */
    return -max_index;
}
#else
uvhttp_error_t uvhttp_get_most_frequent_error(uvhttp_context_t* context) {
    (void)context;
    return UVHTTP_OK;
}
#endif

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
                        
                        /* Configuration errors */                case UVHTTP_ERROR_CONFIG_PARSE:
                    return "Configuration parse error";
                case UVHTTP_ERROR_CONFIG_INVALID:
                    return "Invalid configuration";
                
                default:
                
                                    return "Unknown error";
                
                    }
                
                }
                
                
                
                /* 获取错误分类字符串 */
                
                const char* uvhttp_error_category_string(uvhttp_error_t error) {
                
                    if (error == UVHTTP_OK) {
                
                        return "Success";
                
                    }
                
                    
                
                    /* 通用错误 */
                
                    if (error >= -8 && error <= -1) {
                
                        return "General Error";
                
                    }
                
                    
                
                    /* 服务器错误 */
                
                    if (error >= -106 && error <= -100) {
                
                        return "Server Error";
                
                    }
                
                    
                
                    /* 连接错误 */
                
                    if (error >= -207 && error <= -200) {
                
                        return "Connection Error";
                
                    }
                
                    
                
                    /* 请求/响应错误 */
                
                    if (error >= -307 && error <= -300) {
                
                        return "Request/Response Error";
                
                    }
                
                    
                
                    /* TLS 错误 */
                
                    if (error >= -407 && error <= -400) {
                
                        return "TLS Error";
                
                    }
                
                    
                
                    /* 路由错误 */
                
                    if (error >= -504 && error <= -500) {
                
                        return "Router Error";
                
                    }
                
                    
                
                    /* 分配器错误 */
                
                    if (error >= -602 && error <= -600) {
                
                        return "Allocator Error";
                
                    }
                
                    
                
                    /* WebSocket 错误 */
                
                    if (error >= -707 && error <= -700) {
                
                        return "WebSocket Error";
                
                    }
                
                    
                
                    /* HTTP/2 错误 */
                
                    if (error >= -805 && error <= -800) {
                
                        return "HTTP/2 Error";
                
                    }
                
                    
                
                    /* 配置错误 */
                
                    if (error >= -903 && error <= -900) {
                
                        return "Configuration Error";
                
                    }
                
                    
                
                    /* 中间件错误 */
                
                    if (error >= -1003 && error <= -1000) {
                
                        return "Middleware Error";
                
                    }
                
                    
                
                    /* 日志错误 */
                
                    if (error >= -1103 && error <= -1100) {
                
                        return "Logging Error";
                
                    }
                
                    
                
                    return "Unknown Error";
                
                }
                
                
                
                /* 获取错误描述 */
                
                const char* uvhttp_error_description(uvhttp_error_t error) {
                
                    switch (error) {
                
                        case UVHTTP_OK:
                
                            return "Operation completed successfully";
                
                        
                
                        /* General errors */
                
                        case UVHTTP_ERROR_INVALID_PARAM:
                
                            return "One or more parameters are invalid";
                
                        case UVHTTP_ERROR_OUT_OF_MEMORY:
                
                            return "Failed to allocate memory";
                
                        case UVHTTP_ERROR_NOT_FOUND:
                
                            return "Requested resource was not found";
                
                        case UVHTTP_ERROR_ALREADY_EXISTS:
                
                            return "Resource already exists";
                
                        case UVHTTP_ERROR_NULL_POINTER:
                
                            return "Null pointer encountered";
                
                        case UVHTTP_ERROR_BUFFER_TOO_SMALL:
                
                            return "Buffer is too small to hold the data";
                
                        case UVHTTP_ERROR_TIMEOUT:
                
                            return "Operation timed out";
                
                        case UVHTTP_ERROR_CANCELLED:
                
                            return "Operation was cancelled";
                
                        
                
                        /* Server errors */
                
                        case UVHTTP_ERROR_SERVER_INIT:
                
                            return "Failed to initialize server";
                
                        case UVHTTP_ERROR_SERVER_LISTEN:
                
                            return "Failed to listen on the specified port";
                
                        case UVHTTP_ERROR_SERVER_STOP:
                
                            return "Failed to stop server";
                
                        case UVHTTP_ERROR_CONNECTION_LIMIT:
                
                            return "Maximum connection limit reached";
                
                        case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:
                
                            return "Server is already running";
                
                        case UVHTTP_ERROR_SERVER_NOT_RUNNING:
                
                            return "Server is not running";
                
                        case UVHTTP_ERROR_SERVER_INVALID_CONFIG:
                
                            return "Invalid server configuration";
                
                        
                
                        /* Connection errors */
                
                        case UVHTTP_ERROR_CONNECTION_INIT:
                
                            return "Failed to initialize connection";
                
                        case UVHTTP_ERROR_CONNECTION_ACCEPT:
                
                            return "Failed to accept incoming connection";
                
                        case UVHTTP_ERROR_CONNECTION_START:
                
                            return "Failed to start connection";
                
                        case UVHTTP_ERROR_CONNECTION_CLOSE:
                
                            return "Failed to close connection";
                
                        case UVHTTP_ERROR_CONNECTION_RESET:
                
                            return "Connection was reset by peer";
                
                        case UVHTTP_ERROR_CONNECTION_TIMEOUT:
                
                            return "Connection timed out";
                
                        case UVHTTP_ERROR_CONNECTION_REFUSED:
                
                            return "Connection was refused";
                
                        case UVHTTP_ERROR_CONNECTION_BROKEN:
                
                            return "Connection is broken";
                
                        
                
                        /* Request/Response errors */
                
                        case UVHTTP_ERROR_REQUEST_INIT:
                
                            return "Failed to initialize request";
                
                        case UVHTTP_ERROR_RESPONSE_INIT:
                
                            return "Failed to initialize response";
                
                        case UVHTTP_ERROR_RESPONSE_SEND:
                
                            return "Failed to send response";
                
                        case UVHTTP_ERROR_INVALID_HTTP_METHOD:
                
                            return "Invalid HTTP method";
                
                        case UVHTTP_ERROR_INVALID_HTTP_VERSION:
                
                            return "Invalid HTTP version";
                
                        case UVHTTP_ERROR_HEADER_TOO_LARGE:
                
                            return "HTTP headers are too large";
                
                        case UVHTTP_ERROR_BODY_TOO_LARGE:

                            return "Request body is too large";

                        case UVHTTP_ERROR_MALFORMED_REQUEST:

                            return "Malformed HTTP request";

                        case UVHTTP_ERROR_FILE_TOO_LARGE:

                            return "File is too large";

                        case UVHTTP_ERROR_IO_ERROR:

                            return "I/O error";



                        /* TLS errors */
                
                        case UVHTTP_ERROR_TLS_INIT:
                
                            return "Failed to initialize TLS";
                
                        case UVHTTP_ERROR_TLS_CONTEXT:
                
                            return "Failed to create TLS context";
                
                        case UVHTTP_ERROR_TLS_HANDSHAKE:
                
                            return "TLS handshake failed";
                
                        case UVHTTP_ERROR_TLS_CERT_LOAD:
                
                            return "Failed to load TLS certificate";
                
                        case UVHTTP_ERROR_TLS_KEY_LOAD:
                
                            return "Failed to load TLS private key";
                
                        case UVHTTP_ERROR_TLS_VERIFY_FAILED:
                
                            return "TLS certificate verification failed";
                
                        case UVHTTP_ERROR_TLS_EXPIRED:
                
                            return "TLS certificate has expired";
                
                        case UVHTTP_ERROR_TLS_NOT_YET_VALID:
                
                            return "TLS certificate is not yet valid";
                
                        
                
                        /* Router errors */
                
                        case UVHTTP_ERROR_ROUTER_INIT:
                
                            return "Failed to initialize router";
                
                        case UVHTTP_ERROR_ROUTER_ADD:
                
                            return "Failed to add route";
                
                        case UVHTTP_ERROR_ROUTE_NOT_FOUND:
                
                            return "No matching route found";
                
                        case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:
                
                            return "Route already exists";
                
                        case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:
                
                            return "Invalid route pattern";
                
                        
                
                        /* Allocator errors */
                
                        case UVHTTP_ERROR_ALLOCATOR_INIT:
                
                            return "Failed to initialize allocator";
                
                        case UVHTTP_ERROR_ALLOCATOR_SET:
                
                            return "Failed to set allocator";
                
                        case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:
                
                            return "Allocator is not initialized";
                
                        
                
                        /* WebSocket errors */
                
                        case UVHTTP_ERROR_WEBSOCKET_INIT:
                
                            return "Failed to initialize WebSocket";
                
                        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
                
                            return "WebSocket handshake failed";
                
                        case UVHTTP_ERROR_WEBSOCKET_FRAME:
                
                            return "WebSocket frame processing failed";
                
                        case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:
                
                            return "WebSocket message is too large";
                
                        case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:
                
                            return "Invalid WebSocket opcode";
                
                        case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:
                
                            return "WebSocket is not connected";
                
                        case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:
                
                            return "WebSocket is already connected";
                
                        case UVHTTP_ERROR_WEBSOCKET_CLOSED:
                
                            return "WebSocket connection is closed";
                                            
                                            
                                            
                                                    /* Configuration errors */
                                            
                                                    case UVHTTP_ERROR_CONFIG_PARSE:                
                            return "Failed to parse configuration";
                
                        case UVHTTP_ERROR_CONFIG_INVALID:
                
                            return "Invalid configuration";
                
                        case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:
                
                            return "Configuration file not found";
                
                        case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:
                
                            return "Missing required configuration";
                
                        
                
                        /* Middleware errors */
                
                        /* Logging errors */
                
                        case UVHTTP_ERROR_LOG_INIT:
                
                            return "Failed to initialize logging";
                
                        case UVHTTP_ERROR_LOG_WRITE:
                
                            return "Failed to write log";
                
                        case UVHTTP_ERROR_LOG_FILE_OPEN:
                
                            return "Failed to open log file";
                
                        case UVHTTP_ERROR_LOG_NOT_INITIALIZED:
                
                            return "Logging is not initialized";
                
                        
                
                        default:
                
                            return "Unknown error";
                
                    }
                
                }
                
                
                
                /* 获取错误修复建议 */
                
                const char* uvhttp_error_suggestion(uvhttp_error_t error) {
                
                    switch (error) {
                
                        case UVHTTP_OK:
                
                            return "No action needed";
                
                        
                
                        /* General errors */
                
                        case UVHTTP_ERROR_INVALID_PARAM:
                
                            return "Check the parameters passed to the function";
                
                        case UVHTTP_ERROR_OUT_OF_MEMORY:
                
                            return "Free up memory or increase available memory";
                
                        case UVHTTP_ERROR_NOT_FOUND:
                
                            return "Verify the resource exists and the path is correct";
                
                        case UVHTTP_ERROR_ALREADY_EXISTS:
                
                            return "Use a different name or remove the existing resource";
                
                        case UVHTTP_ERROR_NULL_POINTER:
                
                            return "Ensure all pointers are properly initialized";
                
                        case UVHTTP_ERROR_BUFFER_TOO_SMALL:
                
                            return "Allocate a larger buffer";
                
                        case UVHTTP_ERROR_TIMEOUT:
                
                            return "Increase timeout or optimize operation";
                
                        case UVHTTP_ERROR_CANCELLED:
                
                            return "Check if operation was intentionally cancelled";
                
                        
                
                        /* Server errors */
                
                        case UVHTTP_ERROR_SERVER_INIT:
                
                            return "Check system resources and configuration";
                
                        case UVHTTP_ERROR_SERVER_LISTEN:
                
                            return "Check if port is available and you have permissions";
                
                        case UVHTTP_ERROR_SERVER_STOP:
                
                            return "Ensure server is running before stopping";
                
                        case UVHTTP_ERROR_CONNECTION_LIMIT:
                
                            return "Increase connection limit or reduce concurrent connections";
                
                        case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:
                
                            return "Stop the existing server before starting a new one";
                
                        case UVHTTP_ERROR_SERVER_NOT_RUNNING:
                
                            return "Start the server before performing this operation";
                
                        case UVHTTP_ERROR_SERVER_INVALID_CONFIG:
                
                            return "Review and correct server configuration";
                
                        
                
                        /* Connection errors */
                
                        case UVHTTP_ERROR_CONNECTION_INIT:
                
                            return "Check network configuration and availability";
                
                        case UVHTTP_ERROR_CONNECTION_ACCEPT:
                
                            return "Retry the connection attempt";
                
                        case UVHTTP_ERROR_CONNECTION_START:
                
                            return "Verify server is running and accessible";
                
                        case UVHTTP_ERROR_CONNECTION_CLOSE:
                
                            return "Ensure connection is still active";
                
                        case UVHTTP_ERROR_CONNECTION_RESET:
                
                            return "Check network stability and retry";
                
                        case UVHTTP_ERROR_CONNECTION_TIMEOUT:
                
                            return "Increase timeout or check network connectivity";
                
                        case UVHTTP_ERROR_CONNECTION_REFUSED:
                
                            return "Verify server is running and accepting connections";
                
                        case UVHTTP_ERROR_CONNECTION_BROKEN:
                
                            return "Re-establish the connection";
                
                        
                
                        /* Request/Response errors */
                
                        case UVHTTP_ERROR_REQUEST_INIT:
                
                            return "Check request parameters and format";
                
                        case UVHTTP_ERROR_RESPONSE_INIT:
                
                            return "Verify response configuration";
                
                        case UVHTTP_ERROR_RESPONSE_SEND:
                
                            return "Retry sending the response";
                
                        case UVHTTP_ERROR_INVALID_HTTP_METHOD:
                
                            return "Use a valid HTTP method (GET, POST, PUT, DELETE, etc.)";
                
                        case UVHTTP_ERROR_INVALID_HTTP_VERSION:
                
                            return "Use HTTP/1.1 or HTTP/2";
                
                        case UVHTTP_ERROR_HEADER_TOO_LARGE:
                
                            return "Reduce header size or increase limit";
                
                        case UVHTTP_ERROR_BODY_TOO_LARGE:

                            return "Reduce body size or increase limit";

                        case UVHTTP_ERROR_MALFORMED_REQUEST:

                            return "Check request format and syntax";

                        case UVHTTP_ERROR_FILE_TOO_LARGE:

                            return "Use a smaller file or increase max_file_size limit";

                        case UVHTTP_ERROR_IO_ERROR:

                            return "Check file permissions and disk space";



                        /* TLS errors */
                
                        case UVHTTP_ERROR_TLS_INIT:
                
                            return "Check TLS library installation";
                
                        case UVHTTP_ERROR_TLS_CONTEXT:
                
                            return "Verify TLS configuration";
                
                        case UVHTTP_ERROR_TLS_HANDSHAKE:
                
                            return "Check certificates and TLS configuration";
                
                        case UVHTTP_ERROR_TLS_CERT_LOAD:
                
                            return "Verify certificate file exists and is readable";
                
                        case UVHTTP_ERROR_TLS_KEY_LOAD:
                
                            return "Verify key file exists and is readable";
                
                        case UVHTTP_ERROR_TLS_VERIFY_FAILED:
                
                            return "Check certificate chain and validity";
                
                        case UVHTTP_ERROR_TLS_EXPIRED:
                
                            return "Renew the certificate";
                
                        case UVHTTP_ERROR_TLS_NOT_YET_VALID:
                
                            return "Check system time or wait for certificate validity";
                
                        
                
                        /* Router errors */
                
                        case UVHTTP_ERROR_ROUTER_INIT:
                
                            return "Check router configuration";
                
                        case UVHTTP_ERROR_ROUTER_ADD:
                
                            return "Verify route pattern is valid";
                
                        case UVHTTP_ERROR_ROUTE_NOT_FOUND:
                
                            return "Register the route or check URL";
                
                        case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:
                
                            return "Use a different route pattern";
                
                        case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:
                
                            return "Use a valid route pattern";
                
                        
                
                        /* Allocator errors */
                
                        case UVHTTP_ERROR_ALLOCATOR_INIT:
                
                            return "Check allocator configuration";
                
                        case UVHTTP_ERROR_ALLOCATOR_SET:
                
                            return "Verify allocator type is supported";
                
                        case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:
                
                            return "Initialize the allocator before use";
                
                        
                
                        /* WebSocket errors */
                
                        case UVHTTP_ERROR_WEBSOCKET_INIT:
                
                            return "Check WebSocket configuration";
                
                        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
                
                            return "Verify WebSocket URL and protocol";
                
                        case UVHTTP_ERROR_WEBSOCKET_FRAME:
                
                            return "Check WebSocket message format";
                
                        case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:
                
                            return "Reduce message size or increase limit";
                
                        case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:
                
                            return "Use valid WebSocket opcode";
                
                        case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:
                
                            return "Establish WebSocket connection first";
                
                        case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:
                
                            return "Close existing connection before opening new one";
                
                        case UVHTTP_ERROR_WEBSOCKET_CLOSED:
                
                            return "Re-establish WebSocket connection";
                                            
                                            
                                            
                                                    /* Configuration errors */
                                            
                                                    case UVHTTP_ERROR_CONFIG_PARSE:                
                            return "Check configuration file syntax";
                
                        case UVHTTP_ERROR_CONFIG_INVALID:
                
                            return "Review and correct configuration values";
                
                        case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:
                
                            return "Ensure configuration file exists";
                
                        case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:
                
                            return "Add missing required configuration";
                
                        
                
                        /* Middleware errors */
                
                        /* Logging errors */
                
                        case UVHTTP_ERROR_LOG_INIT:
                
                            return "Check logging configuration";
                
                        case UVHTTP_ERROR_LOG_WRITE:
                
                            return "Check disk space and permissions";
                
                        case UVHTTP_ERROR_LOG_FILE_OPEN:
                
                            return "Verify log file path and permissions";
                
                        case UVHTTP_ERROR_LOG_NOT_INITIALIZED:
                
                            return "Initialize logging before use";
                
                        
                
                        default:
                
                            return "Refer to error code documentation";
                
                    }
                
                }
                
                
                
                /* 检查错误是否可恢复 */
                
                int uvhttp_error_is_recoverable(uvhttp_error_t error) {
                
                    return is_retryable_error(error);
                
                }