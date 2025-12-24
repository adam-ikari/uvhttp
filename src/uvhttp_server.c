#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_tls.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_config.h"
#include "uvhttp_config.h"
#include "uvhttp_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>



/* 写完成回调函数 - 用于释放503响应的write_req资源 */
static void write_503_response_cb(uv_write_t* req, int status) {
    (void)status;  // 避免未使用参数警告
    // 写操作完成后释放write_req
    if (req) {
        uvhttp_free(req);
    }
}

/* 单线程事件驱动连接处理回调
 * 这是libuv事件循环的核心回调函数，所有新连接都在这个单线程中处理
 * 无需锁机制，因为libuv保证所有回调都在同一个事件循环线程中执行
 */
static void on_connection(uv_stream_t* server_handle, int status) {
    if (status < 0) {
        UVHTTP_LOG_ERROR("Connection error: %s\n", uv_strerror(status));
        return;
    }
    
    if (!server_handle || !server_handle->data) {
        UVHTTP_LOG_ERROR("Invalid server handle or data\n");
        return;
    }
    
    uvhttp_server_t* server = (uvhttp_server_t*)server_handle->data;
    
    /* 单线程连接数检查 - 使用服务器特定配置 */
    size_t max_connections = UVHTTP_MAX_CONNECTIONS;  // 默认值
    if (server->config) {
        max_connections = server->config->max_connections;
    } else {
        // 回退到全局配置
        const uvhttp_config_t* global_config = uvhttp_config_get_current();
        if (global_config) {
            max_connections = global_config->max_connections;
        }
    }
    
    if (server->active_connections >= max_connections) {
        UVHTTP_LOG_WARN("Connection limit reached: %zu/%d\n", 
                server->active_connections, max_connections);
        
        /* 创建临时连接以发送503响应 */
        uv_tcp_t* temp_client = uvhttp_malloc(sizeof(uv_tcp_t));
        if (!temp_client) {
            UVHTTP_LOG_ERROR("Failed to allocate temporary client\n");
            return;
        }
        
        if (uv_tcp_init(server->loop, temp_client) != 0) {
            UVHTTP_LOG_ERROR("Failed to initialize temporary client\n");
            uvhttp_free(temp_client);
            return;
        }
        
        if (uv_accept(server_handle, (uv_stream_t*)temp_client) == 0) {
            /* 发送HTTP 503响应 - 使用静态常量避免重复分配 */
            static const char response_503[] = 
                UVHTTP_VERSION_1_1 " 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 19\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Service Unavailable";
                
            uv_write_t* write_req = uvhttp_malloc(sizeof(uv_write_t));
            if (write_req) {
                uv_buf_t buf = uv_buf_init((char*)response_503, sizeof(response_503) - 1);
                
                int write_result = uv_write(write_req, (uv_stream_t*)temp_client, &buf, 1, 
                    write_503_response_cb);
                if (write_result < 0) {
                    UVHTTP_LOG_ERROR("Failed to send 503 response: %s\n", uv_strerror(write_result));
                    // 如果写入失败，立即释放write_req并关闭连接
                    uvhttp_free(write_req);
                    uv_close((uv_handle_t*)temp_client, (uv_close_cb)uvhttp_free);
                    return;
                }
            } else {
                UVHTTP_LOG_ERROR("Failed to allocate write request for 503 response\n");
                uv_close((uv_handle_t*)temp_client, (uv_close_cb)uvhttp_free);
                return;
            }
        } else {
            UVHTTP_LOG_ERROR("Failed to accept temporary connection\n");
            uvhttp_free(temp_client);
        }
        
        return;
    }
    
    /* 创建新的连接对象 - 单线程分配，无需同步 */
    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    if (!conn) {
        return;
    }
    
    /* 接受连接 */
    if (uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    /* 请求和响应对象已在连接创建时初始化 */
    
    /* 单线程安全的连接计数递增 */
    server->active_connections++;
    
    /* 开始连接处理（TLS握手或HTTP读取）
     * 所有后续处理都通过libuv回调在事件循环中异步进行
     */
    if (uvhttp_connection_start(conn) != 0) {
        uvhttp_connection_close(conn);
        return;
    }
}



/* 创建基于单线程事件驱动的HTTP服务器
 * loop: libuv事件循环，如果为NULL则创建新的事件循环
 * 返回: 服务器对象，所有操作都在单个事件循环线程中进行
 * 
 * 单线程设计优势：
 * 1. 无需锁机制，避免死锁和竞态条件
 * 2. 内存访问更安全，无需原子操作
 * 3. 性能可预测，避免线程切换开销
 * 4. 调试简单，执行流清晰
 */
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop) {
    /* 初始化TLS模块（如果还没有初始化） */
    /* TODO: TLS模块需要完全实现后启用
    if (uvhttp_tls_init() != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to initialize TLS module");
        return NULL;
    }
    */
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
        uvhttp_tls_context_free(server->tls_ctx);
    }
    if (server->config) {
        uvhttp_config_free(server->config);
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
        UVHTTP_LOG_ERROR("uv_tcp_bind failed: %s\n", uv_strerror(ret));
        return UVHTTP_ERROR_SERVER_LISTEN;
    }
    
    /* 使用配置系统的backlog设置 */
    const uvhttp_config_t* config = uvhttp_config_get_current();
    int backlog = config ? config->backlog : UVHTTP_BACKLOG;
    
    ret = uv_listen((uv_stream_t*)&server->tcp_handle, backlog, on_connection);
    if (ret != 0) {
        UVHTTP_LOG_ERROR("uv_listen failed: %s\n", uv_strerror(ret));
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
        uvhttp_tls_context_free(server->tls_ctx);
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
        uvhttp_tls_context_free(server->tls_ctx);
        server->tls_ctx = NULL;
    }
    
    server->tls_enabled = 0;
    
    return UVHTTP_OK;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    return server ? server->tls_enabled : 0;
}