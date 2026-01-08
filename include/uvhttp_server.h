#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include <uv.h>
#include "uvhttp_error.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"

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
    void* ws_routes;  /* WebSocket路由表 */
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