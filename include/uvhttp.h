#ifndef UVHTTP_H
#define UVHTTP_H

#include <uv.h>
#include "llhttp.h"
#include "uvhttp_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uvhttp_server uvhttp_server_t;
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;

typedef void (*uvhttp_request_handler_t)(uvhttp_request_t* request, uvhttp_response_t* response);

// Server functions
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
void uvhttp_server_free(uvhttp_server_t* server);
int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
void uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler);
void uvhttp_server_stop(uvhttp_server_t* server);

// TLS functions
int uvhttp_server_enable_tls(uvhttp_server_t* server, uvhttp_tls_context_t* tls_ctx);
int uvhttp_server_disable_tls(uvhttp_server_t* server);
int uvhttp_server_is_tls_enabled(uvhttp_server_t* server);

// Router functions  
uvhttp_router_t* uvhttp_router_new(void);
void uvhttp_router_free(uvhttp_router_t* router);
void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler);
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path);

// Request functions
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

// Response functions
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
void uvhttp_response_send(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif