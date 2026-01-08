/*
 * UVHTTP 服务器模块
 *
 * 提供HTTP服务器的核心功能，包括连接管理、请求路由和响应处理
 * 基于libuv事件驱动架构实现高性能异步I/O
 */

#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_tls.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket_native.h"
#endif

// WebSocket路由条目前向声明
#if UVHTTP_FEATURE_WEBSOCKET
typedef struct ws_route_entry {
    char* path;
    uvhttp_ws_handler_t handler;
    struct ws_route_entry* next;
} ws_route_entry_t;
#endif



/**
 * 503响应写完成回调函数
 * 
 * 处理503 Service Unavailable响应发送完成后的清理工作
 * 
 * @param req 写请求对象
 * @param status 写操作状态
 */
static void write_503_response_cb(uv_write_t* req, int status) {
    uvhttp_handle_write_error(req, status, "503_response");
}

/**
 * 单线程事件驱动连接处理回调
 * 
 * 这是libuv事件循环的核心回调函数，处理所有新连接
 * 单线程模型优势：无需锁，数据访问安全，执行流可预测
 * 
 * @param server_handle 服务器句柄
 * @param status 连接状态
 */
static void on_connection(uv_stream_t* server_handle, int status) {
    if (status < 0) {
        uvhttp_log_safe_error(status, "connection_accept", NULL);
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
            uvhttp_handle_memory_failure("temporary_client_allocation", NULL, NULL);
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
                "Content-Length: " UVHTTP_STRINGIFY(UVHTTP_503_RESPONSE_CONTENT_LENGTH) "\r\n"
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
    int accept_result = uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle);
    
    if (accept_result != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    /* 请求和响应对象已在连接创建时初始化 */
    
    /* 单线程安全的连接计数递增 */
    server->active_connections++;
    
    /* 开始连接处理（TLS握手或HTTP读取）
     * 所有后续处理都通过libuv回调在事件循环中异步进行
     */
    int start_result = uvhttp_connection_start(conn);
    
    if (start_result != 0) {
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
    #if UVHTTP_FEATURE_TLS
        UVHTTP_LOG_DEBUG("Initializing TLS module...");
        if (uvhttp_tls_init() != UVHTTP_TLS_OK) {
            UVHTTP_LOG_ERROR("Failed to initialize TLS module");
            return NULL;
        }
        UVHTTP_LOG_DEBUG("TLS module initialized successfully");
    #endif
        UVHTTP_LOG_DEBUG("Allocating uvhttp_server_t, size=%zu", sizeof(uvhttp_server_t));
        uvhttp_server_t* server = uvhttp_malloc(sizeof(uvhttp_server_t));
        if (!server) {
            UVHTTP_LOG_ERROR("Failed to allocate uvhttp_server_t");
            return NULL;
        }
        UVHTTP_LOG_DEBUG("uvhttp_malloc success, server=%p", (void*)server);
    memset(server, 0, sizeof(uvhttp_server_t));
    
    // 初始化WebSocket路由表
    #if UVHTTP_FEATURE_WEBSOCKET
    server->ws_routes = NULL;
    #endif
    
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
#if UVHTTP_FEATURE_TLS
    server->tls_enabled = 0;
    server->tls_ctx = NULL;
#endif
    
    return server;
}

uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 清理中间件链 - 零开销设计 */
    uvhttp_server_cleanup_middleware(server);
    
    if (server->router) {
        uvhttp_router_free(server->router);
    }
#if UVHTTP_FEATURE_TLS
    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
    }
#endif
    if (server->config) {
        uvhttp_config_free(server->config);
    }
    
    // 释放WebSocket路由表
    #if UVHTTP_FEATURE_WEBSOCKET
    if (server->ws_routes) {
        ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
        while (current) {
            ws_route_entry_t* next = current->next;
            if (current->path) {
                free(current->path);
            }
            uvhttp_free(current);
            current = next;
        }
        server->ws_routes = NULL;
    }
    #endif
    
    // 如果拥有循环，需要关闭并释放
    if (server->owns_loop && server->loop) {
        uv_loop_close(server->loop);
        uvhttp_free(server->loop);
    }
    
    uvhttp_free(server);
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!host) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

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

uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server, uvhttp_router_t* router) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    server->router = router;
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

#if UVHTTP_FEATURE_TLS
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
#else
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server, void* tls_ctx) {
    (void)server; (void)tls_ctx;
    return UVHTTP_ERROR_INVALID_PARAM;
}

uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server) {
    (void)server;
    return UVHTTP_ERROR_INVALID_PARAM;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    (void)server;
    return 0;
}
#endif

// ========== 统一API实现 ==========

// 内部辅助函数
static uvhttp_server_builder_t* create_simple_server_internal(const char* host, int port) {
    uvhttp_server_builder_t* simple = uvhttp_malloc(sizeof(uvhttp_server_builder_t));
    if (!simple) return NULL;

    memset(simple, 0, sizeof(uvhttp_server_builder_t));
    
    // 获取或创建事件循环
    simple->loop = uv_default_loop();
    if (!simple->loop) {
        uvhttp_free(simple);
        return NULL;
    }
    
    // 创建服务器
    simple->server = uvhttp_server_new(simple->loop);
    if (!simple->server) {
        uvhttp_free(simple);
        return NULL;
    }
    
    // 创建路由器
    simple->router = uvhttp_router_new();
    if (!simple->router) {
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        return NULL;
    }
    
    // 创建并设置默认配置
    simple->config = uvhttp_config_new();
    if (!simple->config) {
        uvhttp_router_free(simple->router);
        uvhttp_server_free(simple->server);
        free(simple);
        return NULL;
    }
    
    uvhttp_config_set_defaults(simple->config);
    simple->server->config = simple->config;
    simple->server->router = simple->router;
    simple->auto_cleanup = 1;
    
    // 启动监听
    if (uvhttp_server_listen(simple->server, host, port) != UVHTTP_OK) {
        uvhttp_config_free(simple->config);
            uvhttp_router_free(simple->router);
            uvhttp_server_free(simple->server);
            uvhttp_free(simple);        return NULL;
    }
    
    return simple;
}

// 快速创建和启动服务器
uvhttp_server_builder_t* uvhttp_server_create(const char* host, int port) {
    return create_simple_server_internal(host, port);
}

// 路由添加辅助函数
static uvhttp_server_builder_t* add_route_internal(uvhttp_server_builder_t* server,
                                                   const char* path,
                                                   uvhttp_method_t method,
                                                   uvhttp_request_handler_t handler) {
    if (!server || !path || !handler) return server;
    
    uvhttp_router_add_route_method(server->router, path, method, handler);
    return server;
}

// 链式路由API
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_GET, handler);
}

uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_POST, handler);
}

uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_PUT, handler);
}

uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_DELETE, handler);
}

uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server, const char* path, uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_ANY, handler);
}

// 简化配置API
uvhttp_server_builder_t* uvhttp_set_max_connections(uvhttp_server_builder_t* server, int max_conn) {
    if (server && server->config) {
        server->config->max_connections = max_conn;
    }
    return server;
}

uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server, int timeout) {
    if (server && server->config) {
        server->config->request_timeout = timeout;
        server->config->keepalive_timeout = timeout;
    }
    return server;
}

uvhttp_server_builder_t* uvhttp_set_max_body_size(uvhttp_server_builder_t* server, size_t size) {
    if (server && server->config) {
        server->config->max_body_size = size;
    }
    return server;
}

// 快速响应API
void uvhttp_quick_response(uvhttp_response_t* response, int status, const char* content_type, const char* body) {
    if (!response) return;
    
    uvhttp_response_set_status(response, status);
    if (content_type) {
        uvhttp_response_set_header(response, "Content-Type", content_type);
    }
    if (body) {
        uvhttp_response_set_body(response, body, strlen(body));
    }
    uvhttp_response_send(response);
}



void uvhttp_html_response(uvhttp_response_t* response, const char* html_body) {
    uvhttp_quick_response(response, 200, "text/html", html_body);
}

void uvhttp_file_response(uvhttp_response_t* response, const char* file_path) {
    // 简单的文件响应实现
    FILE* file = fopen(file_path, "r");
    if (!file) {
        uvhttp_quick_response(response, 404, "text/plain", "File not found");
        return;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size_long = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size_long < 0) {
        fclose(file);
        uvhttp_quick_response(response, 500, "text/plain", "File size error");
        return;
    }
    
    size_t file_size = (size_t)file_size_long;
    
    // 读取文件内容
    char* content = uvhttp_malloc(file_size + 1);
    if (!content) {
        fclose(file);
        uvhttp_quick_response(response, 500, "text/plain", "Internal server error");
        return;
    }
    
    if (fread(content, 1, file_size, file) != file_size) {
        uvhttp_free(content);
        uvhttp_quick_response(response, 500, "text/plain", "File read error");
        return;
    }
    content[file_size] = '\0';
    fclose(file);
    
    // 设置内容类型
    const char* content_type = "text/plain";
    if (strstr(file_path, ".html")) content_type = "text/html";
    else if (strstr(file_path, ".css")) content_type = "text/css";
    else if (strstr(file_path, ".js")) content_type = "application/javascript";
    else if (strstr(file_path, ".json")) content_type = "application/json";
    else if (strstr(file_path, ".png")) content_type = "image/png";
    else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) content_type = "image/jpeg";
    
    uvhttp_quick_response(response, 200, content_type, content);
    free(content);
}

// 便捷请求参数获取
const char* uvhttp_get_param(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_query_param(request, name);
}

const char* uvhttp_get_header(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_header(request, name);
}

const char* uvhttp_get_body(uvhttp_request_t* request) {
    return uvhttp_request_get_body(request);
}

// 服务器运行和清理
int uvhttp_server_run(uvhttp_server_builder_t* server) {
    if (!server || !server->loop) return -1;
    return uv_run(server->loop, UV_RUN_DEFAULT);
}

void uvhttp_server_stop_simple(uvhttp_server_builder_t* server) {
    if (server && server->server) {
        uvhttp_server_stop(server->server);
    }
}

void uvhttp_server_simple_free(uvhttp_server_builder_t* server) {
    if (!server) return;
    
    if (server->server) {
        uvhttp_server_free(server->server);
    }
    
    // 注意：router和config由server负责释放，不要重复释放
    
    uvhttp_free(server);
}

// 默认处理器（用于一键启动）
static int default_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* method = uvhttp_request_get_method(request);
    const char* url = uvhttp_request_get_url(request);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
        "UVHTTP 统一API服务器\n\n"
        "请求信息:\n"
        "- 方法: %s\n"
        "- URL: %s\n"
        "- 时间: %ld\n"
        "\n欢迎使用 UVHTTP 统一API!",
        method, url, time(NULL)
    );
    
    uvhttp_quick_response(response, 200, "text/plain", response_body);
    return 0;
}

// 一键启动函数（最简API）
int uvhttp_serve(const char* host, int port) {
    uvhttp_server_builder_t* server = uvhttp_server_create(host, port);
    if (!server) return -1;
    
    // 添加默认路由
    uvhttp_any(server, "/", default_handler);
    
    printf("UVHTTP 服务器运行在 http://%s:%d\n", host ? host : "0.0.0.0", port);
    printf("按 Ctrl+C 停止服务器\n");
    
    int result = uvhttp_server_run(server);
    uvhttp_server_simple_free(server);
    
    return result;
}

// ========== WebSocket 实现 ==========

#if UVHTTP_FEATURE_WEBSOCKET

// WebSocket握手验证（单线程安全）
// 注册WebSocket处理器（添加到服务器的路由表中）
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server, const char* path, uvhttp_ws_handler_t* handler) {
    if (!server || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 创建新路由条目
    ws_route_entry_t* entry = (ws_route_entry_t*)uvhttp_malloc(sizeof(ws_route_entry_t));
    if (!entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    entry->path = strdup(path);
    if (!entry->path) {
        uvhttp_free(entry);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // 复制handler
    memcpy(&entry->handler, handler, sizeof(uvhttp_ws_handler_t));
    entry->next = NULL;

    // 添加到服务器的WebSocket路由表（单线程安全）
    if (!server->ws_routes) {
        server->ws_routes = entry;
    } else {
        ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
        while (current->next) {
            current = current->next;
        }
        current->next = entry;
    }

    return UVHTTP_OK;
}

// 发送WebSocket消息
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len) {
    if (!ws_conn || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 调用原生WebSocket API发送文本消息
    int result = uvhttp_ws_send_text(ws_conn, data, len);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

// 关闭WebSocket连接
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code, const char* reason) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 调用原生WebSocket API关闭连接
    int result = uvhttp_ws_close(ws_conn, code, reason);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

#endif // UVHTTP_FEATURE_WEBSOCKET