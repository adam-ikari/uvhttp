/*
 * UVHTTP WebSocket 中间件
 * 将 WebSocket 功能解耦为独立的中间件模块
 */

#if UVHTTP_FEATURE_WEBSOCKET

#ifndef UVHTTP_WEBSOCKET_MIDDLEWARE_H
#define UVHTTP_WEBSOCKET_MIDDLEWARE_H

#include "uvhttp_middleware.h"
#include "uvhttp_websocket_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket 中间件配置 */
typedef struct {
    int max_connections;      /* 最大连接数 */
    int max_message_size;     /* 最大消息大小 */
    int ping_interval;        /* Ping 间隔（秒） */
    int ping_timeout;         /* Ping 超时（秒） */
    int enable_compression;   /* 是否启用压缩 */
} uvhttp_ws_middleware_config_t;

/* WebSocket 中间件回调 */
typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn, void* user_data);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode, void* user_data);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn, void* user_data);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code, void* user_data);
    void* user_data;
} uvhttp_ws_middleware_callbacks_t;

/* WebSocket 中间件结构 */
typedef struct {
    const char* path;                              /* WebSocket 路径 */
    uvhttp_ws_middleware_config_t config;          /* 配置 */
    uvhttp_ws_middleware_callbacks_t callbacks;    /* 回调函数 */
    uvhttp_http_middleware_t* http_middleware;     /* HTTP 中间件封装 */
    void* internal_data;                           /* 内部数据 */
} uvhttp_ws_middleware_t;

/* 创建 WebSocket 中间件 */
uvhttp_ws_middleware_t* uvhttp_ws_middleware_create(
    const char* path,
    const uvhttp_ws_middleware_config_t* config
);

/* 销毁 WebSocket 中间件 */
void uvhttp_ws_middleware_destroy(uvhttp_ws_middleware_t* middleware);

/* 设置回调函数 */
void uvhttp_ws_middleware_set_callbacks(
    uvhttp_ws_middleware_t* middleware,
    const uvhttp_ws_middleware_callbacks_t* callbacks
);

/* 获取 HTTP 中间件（用于注册到服务器） */
uvhttp_http_middleware_t* uvhttp_ws_middleware_get_http_middleware(
    uvhttp_ws_middleware_t* middleware
);

/* 发送消息（便捷函数） */
int uvhttp_ws_middleware_send(
    uvhttp_ws_middleware_t* middleware,
    uvhttp_ws_connection_t* ws_conn,
    const char* data,
    size_t len
);

/* 关闭连接（便捷函数） */
int uvhttp_ws_middleware_close(
    uvhttp_ws_middleware_t* middleware,
    uvhttp_ws_connection_t* ws_conn,
    int code,
    const char* reason
);

/* 获取当前连接数 */
int uvhttp_ws_middleware_get_connection_count(
    uvhttp_ws_middleware_t* middleware
);

/* 广播消息到所有连接 */
int uvhttp_ws_middleware_broadcast(
    uvhttp_ws_middleware_t* middleware,
    const char* data,
    size_t len
);

/* 默认配置 */
#define UVHTTP_WS_MIDDLEWARE_DEFAULT_CONFIG { \
    .max_connections = 1000, \
    .max_message_size = 1024 * 1024, \
    .ping_interval = 30, \
    .ping_timeout = 60, \
    .enable_compression = 0 \
}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_FEATURE_WEBSOCKET */

#endif /* UVHTTP_WEBSOCKET_MIDDLEWARE_H */