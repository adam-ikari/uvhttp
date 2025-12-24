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

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CONNECTIONS 1000

typedef struct uvhttp_server uvhttp_server_t;

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

// 内部函数声明
uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);

// TLS函数声明 (暂时禁用)
// uvhttp_error_t uvhttp_tls_init(void);
// void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx);

// Router函数声明
void uvhttp_router_free(uvhttp_router_t* router);

#ifdef __cplusplus
}
#endif

#endif