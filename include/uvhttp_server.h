#ifndef UVHTTP_SERVER_H
#    define UVHTTP_SERVER_H

#    include "uvhttp_allocator.h"
#    include "uvhttp_common.h"
#    include "uvhttp_config.h"
#    include "uvhttp_error.h"
#    include "uvhttp_platform.h"

#    include <assert.h>
#    include <uv.h>

/* 包含 uthash 头文件用于哈希表实现 */
#    include "uthash.h"

/* 白名单哈希表项 */
struct whitelist_item {
    char ip[INET_ADDRSTRLEN];
    UT_hash_handle hh;
};
#    include "uvhttp_features.h"

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;
typedef struct uvhttp_connection uvhttp_connection_t;

#    if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#    endif

#    if UVHTTP_FEATURE_WEBSOCKET
typedef struct uvhttp_ws_connection uvhttp_ws_connection_t;

/* WebSocket 连接节点 */
typedef struct ws_connection_node {
    uvhttp_ws_connection_t* ws_conn;
    char path[4096];
    uint64_t last_activity;  /* 最后活动时间（毫秒） */
    uint64_t last_ping_sent; /* 最后发送 Ping 的时间（毫秒） */
    int ping_pending;        /* 是否有待处理的 Ping */
    struct ws_connection_node* next;
} ws_connection_node_t;

/* WebSocket 连接管理器 */
typedef struct {
    ws_connection_node_t* connections; /* 连接链表 */
    int connection_count;              /* 连接计数 */
    uv_timer_t timeout_timer;          /* 超时检测定时器 */
    uv_timer_t heartbeat_timer;        /* 心跳检测定时器 */
    int timeout_seconds;               /* 超时时间（秒） */
    int heartbeat_interval;            /* 心跳间隔（秒） */
    uint64_t ping_timeout_ms;          /* Ping 超时时间（毫秒） */
    int enabled;                       /* 是否启用连接管理 */
    struct uvhttp_server* server;      /* 所属服务器 */
} ws_connection_manager_t;
#    endif

#    ifdef __cplusplus
extern "C" {
#    endif

#    define MAX_CONNECTIONS 1000

typedef struct uvhttp_server uvhttp_server_t;

// 服务器构建器结构体（统一API）
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_config_t* config;
    uv_loop_t* loop;
    int auto_cleanup;
} uvhttp_server_builder_t;
/* ========== 超时统计回调 ========== */
/**
 * @brief 超时统计回调函数类型
 *
 * 由应用层实现，用于统计和记录连接超时事件
 */
typedef void (*uvhttp_timeout_callback_t)(uvhttp_server_t* server,
                                          uvhttp_connection_t* conn,
                                          uint64_t timeout_ms, void* user_data);

struct uvhttp_server {
    /* ========== 缓存行1（0-63字节）：热路径字段 - 最频繁访问 ========== */
    /* 在 on_connection、连接管理中频繁访问 */
    int is_listening;                 /* 4 字节 - 是否正在监听 */
    int owns_loop;                    /* 4 字节 - 是否拥有循环 */
    int _padding1[2];                 /* 8 字节 - 填充到16字节 */
    size_t active_connections;        /* 8 字节 - 活跃连接数 */
    size_t max_connections;           /* 8 字节 - 最大连接数 */
    size_t max_message_size;          /* 8 字节 - 最大消息大小 */
    uvhttp_request_handler_t handler; /* 8 字节 - 请求处理器 */
    uvhttp_timeout_callback_t timeout_callback; /* 8 字节 - 超时统计回调 */
    void* timeout_callback_user_data; /* 8 字节 - 回调用户数据 */
    /* 缓存行1总计：64字节 */

    /* ========== 缓存行2（64-127字节）：核心指针字段 - 次频繁访问 ========== */
    /* 在服务器初始化、请求路由中频繁访问 */
    uv_loop_t* loop;                /* 8 字节 - 事件循环 */
    uvhttp_router_t* router;        /* 8 字节 - 路由器 */
    uvhttp_config_t* config;        /* 8 字节 - 配置 */
    struct uvhttp_context* context; /* 8 字节 - 应用上下文 */
    void* user_data;                /* 8 字节 - 应用数据 */
#    if UVHTTP_FEATURE_TLS
    uvhttp_tls_context_t* tls_ctx; /* 8 字节 - TLS 上下文 */
    int tls_enabled;               /* 4 字节 - TLS 是否启用 */
    int _padding2[3];              /* 12字节 - 填充到16字节 */
#    else
    int _padding2[4];  /* 16字节 - 填充到64字节 */
#    endif
    /* 缓存行2总计：约64字节（取决于 TLS 是否启用） */

    /* ========== 缓存行3（128-191字节）：libuv 句柄 ========== */
    /* libuv 内部结构体，大小固定 */
    uv_tcp_t tcp_handle; /* 约40-48字节 */
    int _padding3[3];    /* 12字节 - 填充到64字节 */
    /* 缓存行3总计：约64字节 */

    /* ========== 缓存行4（192-255字节）：WebSocket 相关 ========== */
#    if UVHTTP_FEATURE_WEBSOCKET
    void* ws_routes; /* 8 字节 - WebSocket 路由表（已废弃） */
    ws_connection_manager_t*
        ws_connection_manager; /* 8 字节 - WebSocket 连接管理器 */
    int _padding4[12];         /* 48字节 - 填充到64字节 */
#    else
    int _padding4[16]; /* 64字节 - 填充到64字节 */
#    endif
    /* 缓存行4总计：64字节 */

    /* ========== 缓存行5-6（256-383字节）：限流功能 ========== */
#    if UVHTTP_FEATURE_RATE_LIMIT
    int rate_limit_enabled;        /* 4 字节 - 限流是否启用 */
    int rate_limit_max_requests;   /* 4 字节 - 最大请求数 */
    int rate_limit_window_seconds; /* 4 字节 - 时间窗口（秒） */
    int rate_limit_request_count;  /* 4 字节 - 当前请求数 */
    uint64_t rate_limit_window_start_time; /* 8 字节 - 窗口开始时间 */
    void** rate_limit_whitelist;           /* 8 字节 - 白名单数组 */
    size_t rate_limit_whitelist_count;     /* 8 字节 - 白名单数量 */
    struct whitelist_item*
        rate_limit_whitelist_hash; /* 8 字节 - 白名单哈希表 */
    int _padding5[8];              /* 32字节 - 填充到64字节 */
#    else
    int _padding5[16]; /* 64字节 - 填充到64字节 */
#    endif
    /* 缓存行5总计：64字节 */
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, loop, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, router, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, config, UVHTTP_POINTER_ALIGNMENT);

/* 验证size_t对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, active_connections,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, max_connections,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* API函数 */
/**
 * @brief 创建新的 HTTP 服务器
 * @param loop 事件循环，可为 NULL（内部创建新循环）
 * @param server 输出参数，用于接收服务器指针
 * @return UVHTTP_OK 成功，其他值表示失败
 * @note 成功时，*server 被设置为有效的服务器对象，必须使用 uvhttp_server_free
 * 释放
 * @note 失败时，*server 被设置为 NULL
 */
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server);
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host,
                                    int port);
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
#    if UVHTTP_FEATURE_TLS
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server,
                                        uvhttp_tls_context_t* tls_ctx);
uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server);
#    endif
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server,
                                         uvhttp_request_handler_t handler);
uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server,
                                        uvhttp_router_t* router);
uvhttp_error_t uvhttp_server_set_context(uvhttp_server_t* server,
                                         struct uvhttp_context* context);

#    if UVHTTP_FEATURE_RATE_LIMIT
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
uvhttp_error_t uvhttp_server_enable_rate_limit(uvhttp_server_t* server,
                                               int max_requests,
                                               int window_seconds);

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
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(uvhttp_server_t* server,
                                                      const char* client_ip);

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
uvhttp_error_t uvhttp_server_get_rate_limit_status(uvhttp_server_t* server,
                                                   const char* client_ip,
                                                   int* remaining,
                                                   uint64_t* reset_time);

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
uvhttp_error_t uvhttp_server_reset_rate_limit_client(uvhttp_server_t* server,
                                                     const char* client_ip);

/**
 * 清空所有限流状态
 *
 * @param server 服务器实例
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server);
#    endif /* UVHTTP_FEATURE_RATE_LIMIT */

// ========== 统一API函数 ==========

// 快速创建和启动服务器
/**
 * 创建并启动简单服务器（链式API）
 *
 * @param host 监听地址
 * @param port 监听端口
 * @param server 输出参数，返回创建的服务器构建器
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_server_create(const char* host, int port,
                                    uvhttp_server_builder_t** server);

// 链式路由API
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server,
                                     const char* path,
                                     uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);

// 简化配置API
uvhttp_server_builder_t* uvhttp_set_max_connections(
    uvhttp_server_builder_t* server, int max_conn);
uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server,
                                            int timeout);
uvhttp_server_builder_t* uvhttp_set_max_body_size(
    uvhttp_server_builder_t* server, size_t size);

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
#    if UVHTTP_FEATURE_WEBSOCKET
#        include "uvhttp_websocket.h"

typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data,
                      size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code,
                    const char* error_msg);
    void* user_data;
    /* 超时统计回调 */
    uvhttp_timeout_callback_t timeout_callback; /* 8 字节 - 超时统计回调 */
    void* timeout_callback_user_data; /* 8 字节 - 回调用户数据 */
} uvhttp_ws_handler_t;

uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server,
                                                 const char* path,
                                                 uvhttp_ws_handler_t* handler);
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn,
                                     const char* data, size_t len);
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code,
                                      const char* reason);

/* 内部函数 */
uvhttp_ws_handler_t* uvhttp_server_find_ws_handler(uvhttp_server_t* server,
                                                   const char* path);

/* 连接管理 API */
uvhttp_error_t uvhttp_server_ws_enable_connection_management(
    uvhttp_server_t* server, int timeout_seconds, int heartbeat_interval);

uvhttp_error_t uvhttp_server_ws_disable_connection_management(
    uvhttp_server_t* server);

int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server);

int uvhttp_server_ws_get_connection_count_by_path(uvhttp_server_t* server,
                                                  const char* path);

uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server,
                                          const char* path, const char* data,
                                          size_t len);

uvhttp_error_t uvhttp_server_ws_close_all(uvhttp_server_t* server,
                                          const char* path);

/* 内部函数（由 uvhttp_connection 调用） */
void uvhttp_server_ws_add_connection(uvhttp_server_t* server,
                                     uvhttp_ws_connection_t* ws_conn,
                                     const char* path);

void uvhttp_server_ws_remove_connection(uvhttp_server_t* server,
                                        uvhttp_ws_connection_t* ws_conn);

void uvhttp_server_ws_update_activity(uvhttp_server_t* server,
                                      uvhttp_ws_connection_t* ws_conn);
#    endif

// 内部函数声明
uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);

// TLS函数声明 (暂时禁用)
// uvhttp_error_t uvhttp_tls_init(void);
// void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx);

#    ifdef __cplusplus
}
#    endif

#endif
uvhttp_error_t uvhttp_server_set_timeout_callback(
    uvhttp_server_t* server, uvhttp_timeout_callback_t callback,
    void* user_data);
