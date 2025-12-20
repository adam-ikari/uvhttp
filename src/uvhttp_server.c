#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <uv.h>



static void on_connection(uv_stream_t* server_handle, int status) {
    if (status < 0) {
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
        return;
    }
    
    if (!server_handle || !server_handle->data) {
        fprintf(stderr, "Invalid server handle or data\n");
        return;
    }
    
    uvhttp_server_t* server = (uvhttp_server_t*)server_handle->data;
    
    /* 检查连接数限制 */
    if (server->active_connections >= MAX_CONNECTIONS) {
        fprintf(stderr, "Connection limit reached: %d/%d\n", 
                server->active_connections, MAX_CONNECTIONS);
        
        /* 创建临时连接以发送503响应 */
        uv_tcp_t* temp_client = uvhttp_malloc(sizeof(uv_tcp_t));
        if (!temp_client) {
            fprintf(stderr, "Failed to allocate temporary client\n");
            return;
        }
        
        if (uv_tcp_init(server->loop, temp_client) != 0) {
            fprintf(stderr, "Failed to initialize temporary client\n");
            uvhttp_free(temp_client);
            return;
        }
        
        if (uv_accept(server_handle, (uv_stream_t*)temp_client) == 0) {
            /* 发送HTTP 503响应 */
            const char* response_503 = 
                UVHTTP_VERSION_1_1 " 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 19\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Service Unavailable";
                
            uv_write_t* write_req = uvhttp_malloc(sizeof(uv_write_t));
            if (write_req) {
                uv_buf_t buf = uv_buf_init((char*)response_503, strlen(response_503));
                int write_result = uv_write(write_req, (uv_stream_t*)temp_client, &buf, 1, NULL);
                if (write_result < 0) {
                    fprintf(stderr, "Failed to send 503 response: %s\n", uv_strerror(write_result));
                    uvhttp_free(write_req);
                }
            }
        } else {
            fprintf(stderr, "Failed to accept temporary connection\n");
        }
        
        /* 关闭临时连接 */
        uv_close((uv_handle_t*)temp_client, (uv_close_cb)uvhttp_free);
        return;
    }
    
    /* 创建新的连接对象 */
    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    if (!conn) {
        return;
    }
    
    /* 接受连接 */
    if (uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    /* 初始化请求和响应对象 */
    if (uvhttp_request_init(conn->request, &conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    if (uvhttp_response_init(conn->response, &conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    /* 增加活跃连接计数 */
    server->active_connections++;
    
    /* 开始连接处理（TLS握手或HTTP读取） */
    if (uvhttp_connection_start(conn) != 0) {
        uvhttp_connection_close(conn);
        return;
    }
}



uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop) {
    /* 初始化TLS模块（如果还没有初始化） - 暂时禁用 */
    /* if (uvhttp_tls_init() != UVHTTP_OK) {
        return NULL;
    } */
    
    uvhttp_server_t* server = uvhttp_malloc(sizeof(uvhttp_server_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(uvhttp_server_t));
    
    // 如果没有提供loop，内部创建新循环
    if (loop) {
        server->loop = loop;
        server->owns_loop = 0;
    } else {
        server->loop = uvhttp_malloc(sizeof(uv_loop_t));
        if (!server->loop) {
                    uvhttp_free(server);
                    return NULL;        }
        if (uv_loop_init(server->loop) != 0) {
            uvhttp_free(server->loop);
                uvhttp_free(server);            return NULL;
        }
        server->owns_loop = 1;
    }
    
    if (uv_tcp_init(server->loop, &server->tcp_handle) != 0) {
        if (server->owns_loop) {
            uv_loop_close(server->loop);
            uvhttp_free(server->loop);
        }
        uvhttp_free(server);
        return NULL;
    }
    server->tcp_handle.data = server;
    server->active_connections = 0;
    server->tls_enabled = 0;
    server->tls_ctx = NULL;
    
    return server;
}

uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (server->router) {
        uvhttp_router_free(server->router);
    }
    if (server->tls_ctx) {
        // uvhttp_tls_context_free(server->tls_ctx);  // 暂时禁用
    }
    
    // 如果拥有循环，需要关闭并释放
    if (server->owns_loop && server->loop) {
        uv_loop_close(server->loop);
        uvhttp_free(server->loop);
    }
    
    uvhttp_free(server);
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port) {
    struct sockaddr_in addr;
    uv_ip4_addr(host, port, &addr);
    
    int ret = uv_tcp_bind(&server->tcp_handle, (const struct sockaddr*)&addr, 0);
    if (ret != 0) {
        return UVHTTP_ERROR_SERVER_LISTEN;
    }
    
    ret = uv_listen((uv_stream_t*)&server->tcp_handle, UVHTTP_MAX_CONNECTIONS, on_connection);
    if (ret != 0) {
        return UVHTTP_ERROR_SERVER_LISTEN;
    }
    
    server->is_listening = 1;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    server->handler = handler;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (server->is_listening) {
        uv_close((uv_handle_t*)&server->tcp_handle, NULL);
        server->is_listening = 0;
        return UVHTTP_OK;
    }
    
    return UVHTTP_ERROR_SERVER_STOP;
}

uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server, uvhttp_tls_context_t* tls_ctx) {
    if (!server || !tls_ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (server->tls_ctx) {
        // uvhttp_tls_context_free(server->tls_ctx);  // 暂时禁用
    }
    
    server->tls_ctx = tls_ctx;
    server->tls_enabled = 1;
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (server->tls_ctx) {
        // uvhttp_tls_context_free(server->tls_ctx);  // 暂时禁用
        server->tls_ctx = NULL;
    }
    
    server->tls_enabled = 0;
    
    return UVHTTP_OK;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    return server ? server->tls_enabled : 0;
}