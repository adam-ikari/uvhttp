#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_connection.h"
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
    uvhttp_server_t* server;
    uvhttp_request_t* request;
    uvhttp_response_t* response;
} uvhttp_connection_t;

static void on_connection(uv_stream_t* server_handle, int status) {
    if (status < 0) {
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
        return;
    }
    
    uvhttp_server_t* server = (uvhttp_server_t*)server_handle->data;
    
    // 检查连接数限制
    if (server->active_connections >= MAX_CONNECTIONS) {
        fprintf(stderr, "Connection limit reached, rejecting new connection\n");
        
        // 创建临时连接以发送503响应
        uv_tcp_t* temp_client = malloc(sizeof(uv_tcp_t));
        if (!temp_client) {
            fprintf(stderr, "Failed to allocate temporary client\n");
            return;
        }
        
        uv_tcp_init(server->loop, temp_client);
        
        if (uv_accept(server_handle, (uv_stream_t*)temp_client) == 0) {
            // 发送HTTP 503响应
            const char* response_503 = 
                "HTTP/1.1 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 19\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Service Unavailable";
                
            uv_write_t* write_req = malloc(sizeof(uv_write_t));
            if (write_req) {
                uv_buf_t buf = uv_buf_init((char*)response_503, strlen(response_503));
                uv_write(write_req, (uv_stream_t*)temp_client, &buf, 1, NULL);
            }
        }
        
        // 关闭临时连接
        uv_close((uv_handle_t*)temp_client, (uv_close_cb)free);
        return;
    }
    
    // 创建新的连接对象
    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    if (!conn) {
        fprintf(stderr, "Failed to create connection\n");
        return;
    }
    
    // 接受连接
    if (uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        fprintf(stderr, "Failed to accept connection\n");
        return;
    }
    
    // 初始化请求和响应对象
    if (uvhttp_request_init(conn->request, &conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        fprintf(stderr, "Failed to initialize request\n");
        return;
    }
    
    if (uvhttp_response_init(conn->response, &conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        fprintf(stderr, "Failed to initialize response\n");
        return;
    }
    
    // 增加活跃连接计数
    server->active_connections++;
    
    // 开始连接处理（TLS握手或HTTP读取）
    if (uvhttp_connection_start(conn) != 0) {
        fprintf(stderr, "Failed to start connection\n");
        uvhttp_connection_close(conn);
        return;
    }
}



uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop) {
    // 初始化TLS模块（如果还没有初始化）
    if (uvhttp_tls_init() != 0) {
        fprintf(stderr, "Failed to initialize TLS module\n");
        return NULL;
    }
    
    uvhttp_server_t* server = malloc(sizeof(uvhttp_server_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(uvhttp_server_t));
    
    server->loop = loop;
    if (uv_tcp_init(loop, &server->tcp_handle) != 0) {
        free(server);
        return NULL;
    }
    server->tcp_handle.data = server;
    server->active_connections = 0;
    server->tls_enabled = 0;
    server->tls_ctx = NULL;
    
    return server;
}

void uvhttp_server_free(uvhttp_server_t* server) {
    if (server->router) {
        uvhttp_router_free(server->router);
    }
    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
    }
    free(server);
}

int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port) {
    struct sockaddr_in addr;
    uv_ip4_addr(host, port, &addr);
    
    int ret = uv_tcp_bind(&server->tcp_handle, (const struct sockaddr*)&addr, 0);
    if (ret != 0) {
        return ret;
    }
    
    ret = uv_listen((uv_stream_t*)&server->tcp_handle, 128, on_connection);
    if (ret != 0) {
        return ret;
    }
    
    server->is_listening = 1;
    return 0;
}

void uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler) {
    server->handler = handler;
}

void uvhttp_server_stop(uvhttp_server_t* server) {
    if (server->is_listening) {
        uv_close((uv_handle_t*)&server->tcp_handle, NULL);
        server->is_listening = 0;
    }
}

int uvhttp_server_enable_tls(uvhttp_server_t* server, uvhttp_tls_context_t* tls_ctx) {
    if (!server || !tls_ctx) {
        return -1;
    }
    
    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
    }
    
    server->tls_ctx = tls_ctx;
    server->tls_enabled = 1;
    
    return 0;
}

int uvhttp_server_disable_tls(uvhttp_server_t* server) {
    if (!server) {
        return -1;
    }
    
    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
        server->tls_ctx = NULL;
    }
    
    server->tls_enabled = 0;
    
    return 0;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    return server ? server->tls_enabled : 0;
}