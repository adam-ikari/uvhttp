#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include <uv.h>
#include "uvhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CONNECTIONS 1000

struct uvhttp_server {
    uv_loop_t* loop;
    uv_tcp_t tcp_handle;
    uvhttp_request_handler_t handler;
    uvhttp_router_t* router;
    uvhttp_tls_context_t* tls_ctx;
    int is_listening;
    int tls_enabled;
    size_t active_connections;
};

// 内部函数声明
int uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);
int uvhttp_response_init(uvhttp_response_t* response, uv_tcp_t* client);
void uvhttp_response_cleanup(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif