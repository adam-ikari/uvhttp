#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include <uv.h>
#include <assert.h>
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

#if UVHTTP_FEATURE_MIDDLEWARE
#include "uvhttp_middleware.h"
#endif

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;

#if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#endif

#if UVHTTP_FEATURE_WEBSOCKET
typedef struct uvhttp_ws_connection uvhttp_ws_connection_t;

/* WebSocket 连接节点 */
typedef struct ws_connection_node {
    uvhttp_ws_connection_t* ws_conn;
    char path[4096];
    uint64_t last_activity;      /* 最后活动时间（毫秒） */
    uint64_t last_ping_sent;     /* 最后发送 Ping 的时间（毫秒） */
    int ping_pending;            /* 是否有待处理的 Ping */
    struct ws_connection_node* next;
} ws_connection_node_t;

/* WebSocket 连接管理器 */
typedef struct {
    ws_connection_node_t* connections;  /* 连接链表 */
    int connection_count;               /* 连接计数 */
    uv_timer_t timeout_timer;           /* 超时检测定时器 */
    uv_timer_t heartbeat_timer;         /* 心跳检测定时器 */
    int timeout_seconds;                /* 超时时间（秒） */
    int heartbeat_interval;             /* 心跳间隔（秒） */
    uint64_t ping_timeout_ms;           /* Ping 超时时间（毫秒） */
    int enabled;                        /* 是否启用连接管理 */
    struct uvhttp_server* server;       /* 所属服务器 */
} ws_connection_manager_t;
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
    /* 热路径字段（频繁访问）- 优化内存局部性 */
    int is_listening;                         /* 4 字节 - 是否正在监听 */
    int owns_loop;                            /* 4 字节 - 是否拥有循环 */
    size_t active_connections;                /* 8 字节 - 活跃连接数 */
    
    /* 指针字段（8字节对齐） */
    uv_loop_t* loop;                           /* 8 字节 */
    uvhttp_request_handler_t handler;         /* 8 字节 */
    uvhttp_router_t* router;                   /* 8 字节 */
    uvhttp_config_t* config;                   /* 8 字节 */
    
    /* 网络连接（16字节对齐） */
    uv_tcp_t tcp_handle;                       /* 8 字节 */
    
    /* HTTP/1.1优化字段 */
    size_t max_connections;                     /* 8 字节 */
    size_t max_message_size;                    /* 8 字节 */
    
    /* TLS相关 */
#if UVHTTP_FEATURE_TLS
    uvhttp_tls_context_t* tls_ctx;             /* 8 字节 */
    int tls_enabled;                            /* 4 字节 */
#endif
    
    /* 中间件链 */
#if UVHTTP_FEATURE_MIDDLEWARE
    uvhttp_http_middleware_t* middleware_chain; /* 8 字节 */
#endif
    
    /* WebSocket相关 */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_routes;                           /* 8 字节 - 已废弃 */
    ws_connection_manager_t* ws_connection_manager; /* 8 字节 */
#endif
    
    /* 限流功能 */
#if UVHTTP_FEATURE_RATE_LIMIT
    int rate_limit_enabled;                      /* 4 字节 */
    int rate_limit_max_requests;                   /* 4 字节 */
    int rate_limit_window_seconds;                 /* 4 字节 */
    int rate_limit_request_count;                  /* 4 字节 */
    uint64_t rate_limit_window_start_time;        /* 8 字节 */
    void** rate_limit_whitelist;                 /* 8 字节 */
    size_t rate_limit_whitelist_count;             /* 8 字节 */
    struct whitelist_item* rate_limit_whitelist_hash; /* 8 字节 */
#endif
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_server_t, loop) % 8 == 0,
                      "loop pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_server_t, router) % 8 == 0,
                      "router pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_server_t, config) % 8 == 0,
                      "config pointer not 8-byte aligned");

/* 验证size_t对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_server_t, active_connections) % 8 == 0,
                      "active_connections not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_server_t, max_connections) % 8 == 0,
                      "max_connections not 8-byte aligned");

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

#if UVHTTP_FEATURE_MIDDLEWARE
// ========== 中间件 API ==========
uvhttp_error_t uvhttp_server_add_middleware(uvhttp_server_t* server, uvhttp_http_middleware_t* middleware);
uvhttp_error_t uvhttp_server_remove_middleware(uvhttp_server_t* server, const char* path);
void uvhttp_server_cleanup_middleware(uvhttp_server_t* server);
#endif

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
uvhttp_error_t uvhttp_quick_response(uvhttp_response_t* response, int status, const char* content_type, const char* body);
uvhttp_error_t uvhttp_html_response(uvhttp_response_t* response, const char* html_body);
uvhttp_error_t uvhttp_file_response(uvhttp_response_t* response, const char* file_path);

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
#include "uvhttp_websocket_auth.h"

typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg);
    void* user_data;
} uvhttp_ws_handler_t;

uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server, const char* path, uvhttp_ws_handler_t* handler);
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len);
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code, const char* reason);

/* WebSocket 认证 API */
uvhttp_error_t uvhttp_server_ws_set_auth_config(
    uvhttp_server_t* server,
    const char* path,
    uvhttp_ws_auth_config_t* config
);

uvhttp_ws_auth_config_t* uvhttp_server_ws_get_auth_config(
    uvhttp_server_t* server,
    const char* path
);

uvhttp_error_t uvhttp_server_ws_enable_token_auth(
    uvhttp_server_t* server,
    const char* path,
    uvhttp_ws_token_validator_callback validator,
    void* user_data
);

uvhttp_error_t uvhttp_server_ws_add_ip_to_whitelist(
    uvhttp_server_t* server,
    const char* path,
    const char* ip
);

uvhttp_error_t uvhttp_server_ws_add_ip_to_blacklist(
    uvhttp_server_t* server,
    const char* path,
    const char* ip
);

/* 连接管理 API */
uvhttp_error_t uvhttp_server_ws_enable_connection_management(
    uvhttp_server_t* server,
    int timeout_seconds,
    int heartbeat_interval
);

uvhttp_error_t uvhttp_server_ws_disable_connection_management(
    uvhttp_server_t* server
);

int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server);

int uvhttp_server_ws_get_connection_count_by_path(
    uvhttp_server_t* server,
    const char* path
);

uvhttp_error_t uvhttp_server_ws_broadcast(
    uvhttp_server_t* server,
    const char* path,
    const char* data,
    size_t len
);

uvhttp_error_t uvhttp_server_ws_close_all(
    uvhttp_server_t* server,
    const char* path
);

/* 内部函数（由 uvhttp_connection 调用） */
void uvhttp_server_ws_add_connection(
    uvhttp_server_t* server,
    uvhttp_ws_connection_t* ws_conn,
    const char* path
);

void uvhttp_server_ws_remove_connection(
    uvhttp_server_t* server,
    uvhttp_ws_connection_t* ws_conn
);

void uvhttp_server_ws_update_activity(
    uvhttp_server_t* server,
    uvhttp_ws_connection_t* ws_conn
);
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