#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include <uv.h>
#include "uvhttp_error.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"
#include "uvhttp_allocator.h"

/* 包含 uthash 头文件用于哈希表实现 */
#include "uthash.h"

/* 白名单哈希表项 */
struct whitelist_item {
    char ip[INET_ADDRSTRLEN];
    UT_hash_handle hh;
};
#include "uvhttp_features.h"

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;

#if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#endif

#if UVHTTP_FEATURE_WEBSOCKET
typedef struct uvhttp_ws_connection uvhttp_ws_connection_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CONNECTIONS 1000

typedef struct uvhttp_server uvhttp_server_t;



/* 前向声明 */
typedef struct uvhttp_http_middleware uvhttp_http_middleware_t;

// 服务器构建器结构体（统一API）
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_config_t* config;
    uv_loop_t* loop;
    int auto_cleanup;
} uvhttp_server_builder_t;

struct uvhttp_server {
    uv_loop_t* loop;
    uv_tcp_t tcp_handle;
    uvhttp_request_handler_t handler;
    uvhttp_router_t* router;
    int is_listening;
#if UVHTTP_FEATURE_TLS
    uvhttp_tls_context_t* tls_ctx;
    int tls_enabled;
#endif
    size_t active_connections;
    int owns_loop;  /* 是否拥有循环（内部创建的） */
    uvhttp_config_t* config;  /* 服务器配置 */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_routes;  /* WebSocket路由表（已废弃，使用中间件） */
#endif
    uvhttp_http_middleware_t* middleware_chain;  /* 中间件链 */
    
#if UVHTTP_FEATURE_RATE_LIMIT
    /* 限流功能（核心功能 - 固定窗口算法） */
    int rate_limit_enabled;                          /* 是否启用限流 */
    int rate_limit_max_requests;                       /* 最大请求数 */
    int rate_limit_window_seconds;                     /* 时间窗口（秒） */
    int rate_limit_request_count;                      /* 当前窗口内的请求数 */
    uint64_t rate_limit_window_start_time;            /* 窗口开始时间（毫秒） */
    void** rate_limit_whitelist;                       /* 限流白名单路径数组 */
    size_t rate_limit_whitelist_count;                   /* 白名单路径数量 */
    struct whitelist_item* rate_limit_whitelist_hash;  /* 白名单哈希表 */
#endif
};

/* API函数 */
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);  /* loop可为NULL，内部创建新循环 */
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
#if UVHTTP_FEATURE_TLS
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server, uvhttp_tls_context_t* tls_ctx);
uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server);
#endif
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler);
uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server, uvhttp_router_t* router);

// ========== 中间件 API ==========
uvhttp_error_t uvhttp_server_add_middleware(uvhttp_server_t* server, uvhttp_http_middleware_t* middleware);
uvhttp_error_t uvhttp_server_remove_middleware(uvhttp_server_t* server, const char* path);
void uvhttp_server_cleanup_middleware(uvhttp_server_t* server);

#if UVHTTP_FEATURE_RATE_LIMIT
// ========== 限流 API（核心功能） ==========

/**
 * 启用服务器级别的限流功能
 * 
 * @param server 服务器实例
 * @param max_requests 时间窗口内允许的最大请求数（范围：1-1000000）
 * @param window_seconds 时间窗口（秒，范围：1-86400）
 * @return UVHTTP_OK 成功，其他值表示失败
 * 
 * 注意：
 * - 限流功能对所有请求生效（服务器级别限流）
 * - 限流状态在服务器级别管理，所有客户端共享计数器
 * - 适用于防止 DDoS 攻击，不适用于按客户端限流
 * - 建议在调用 uvhttp_server_listen 之前调用
 * - 使用固定窗口算法实现
 */
uvhttp_error_t uvhttp_server_enable_rate_limit(
    uvhttp_server_t* server,
    int max_requests,
    int window_seconds
);

/**
 * 禁用限流功能
 * 
 * @param server 服务器实例
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t* server);

/**
 * 检查限流状态（内部使用）
 * 
 * @param server 服务器实例
 * @return UVHTTP_OK 允许请求，UVHTTP_ERROR_RATE_LIMIT_EXCEEDED 超过限流
 */
uvhttp_error_t uvhttp_server_check_rate_limit(uvhttp_server_t* server);

/**
 * 添加限流白名单IP地址（不受限流限制）
 * 
 * @param server 服务器实例
 * @param client_ip 客户端IP地址（如 "127.0.0.1"）
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(
    uvhttp_server_t* server,
    const char* client_ip
);

/**
 * 获取服务器的限流状态
 * 
 * @param server 服务器实例
 * @param client_ip 客户端IP地址（当前未使用，保留参数以备将来扩展）
 * @param remaining 剩余请求数（输出）
 * @param reset_time 重置时间戳（输出，毫秒）
 * @return UVHTTP_OK 成功，其他值表示失败
 * 
 * 注意：
 * - 当前实现为服务器级别限流，client_ip 参数未使用
 * - 返回的是服务器的总体限流状态，不是特定客户端的状态
 */
uvhttp_error_t uvhttp_server_get_rate_limit_status(
    uvhttp_server_t* server,
    const char* client_ip,
    int* remaining,
    uint64_t* reset_time
);

/**
 * 重置服务器的限流状态
 * 
 * @param server 服务器实例
 * @param client_ip 客户端IP地址（当前未使用，保留参数以备将来扩展）
 * @return UVHTTP_OK 成功，其他值表示失败
 * 
 * 注意：
 * - 当前实现为服务器级别限流，client_ip 参数未使用
 * - 重置的是服务器的总体限流计数器，不是特定客户端的计数器
 */
uvhttp_error_t uvhttp_server_reset_rate_limit_client(
    uvhttp_server_t* server,
    const char* client_ip
);

/**
 * 清空所有限流状态
 * 
 * @param server 服务器实例
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server);
#endif /* UVHTTP_FEATURE_RATE_LIMIT */

// ========== 统一API函数 ==========

// 快速创建和启动服务器
uvhttp_server_builder_t* uvhttp_server_create(const char* host, int port);

// 链式路由API
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler);

// 简化配置API
uvhttp_server_builder_t* uvhttp_set_max_connections(uvhttp_server_builder_t* server, int max_conn);
uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server, int timeout);
uvhttp_server_builder_t* uvhttp_set_max_body_size(uvhttp_server_builder_t* server, size_t size);

// 快速响应API
void uvhttp_quick_response(uvhttp_response_t* response, int status, const char* content_type, const char* body);
void uvhttp_html_response(uvhttp_response_t* response, const char* html_body);
void uvhttp_file_response(uvhttp_response_t* response, const char* file_path);

// 便捷请求参数获取
const char* uvhttp_get_param(uvhttp_request_t* request, const char* name);
const char* uvhttp_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_get_body(uvhttp_request_t* request);

// 服务器运行和清理
int uvhttp_server_run(uvhttp_server_builder_t* server);
void uvhttp_server_stop_simple(uvhttp_server_builder_t* server);
void uvhttp_server_simple_free(uvhttp_server_builder_t* server);

// 一键启动函数（最简API）
int uvhttp_serve(const char* host, int port);

// WebSocket API
#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket_native.h"

typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    void* user_data;
} uvhttp_ws_handler_t;

uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server, const char* path, uvhttp_ws_handler_t* handler);
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len);
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code, const char* reason);
#endif

// 内部函数声明
uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);

// TLS函数声明 (暂时禁用)
// uvhttp_error_t uvhttp_tls_init(void);
// void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif