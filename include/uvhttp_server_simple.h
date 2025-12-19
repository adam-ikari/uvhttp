#ifndef UVHTTP_SERVER_SIMPLE_H
#define UVHTTP_SERVER_SIMPLE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 请求处理函数类型
typedef void (*uvhttp_request_handler_t)(uvhttp_request_t* request, uvhttp_response_t* response);

// 前向声明
typedef struct uvhttp_router uvhttp_router_t;

// 简化的服务器结构
struct uvhttp_server {
    void* loop;  // 替换uv_loop_t*
    void* tcp_handle;  // 替换uv_tcp_t*
    uvhttp_request_handler_t handler;
    uvhttp_router_t* router;
    
    int is_listening;
    size_t active_connections;
};

// 服务器函数
struct uvhttp_server* uvhttp_server_new(void* loop);
void uvhttp_server_free(struct uvhttp_server* server);
int uvhttp_server_listen(struct uvhttp_server* server, const char* host, int port);
void uvhttp_server_set_handler(struct uvhttp_server* server, uvhttp_request_handler_t handler);
void uvhttp_server_stop(struct uvhttp_server* server);

#ifdef __cplusplus
}
#endif

#endif