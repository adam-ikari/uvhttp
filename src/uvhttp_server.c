/*
 * UVHTTP 服务器模块
 *
 * 提供HTTP服务器的核心功能，包括连接管理、请求路由和响应处理
 * 基于libuv事件驱动架构实现高性能异步I/O
 */

#include "uvhttp_server.h"

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"
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
static void
write_503_response_cb(uv_write_t* req, int status) {
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
static void
on_connection(uv_stream_t* server_handle, int status) {
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
    size_t max_connections = UVHTTP_MAX_CONNECTIONS_DEFAULT;  // 默认值
    if (server->config) {
        max_connections = server->config->max_connections;
    } else {
        // 回退到全局配置（使用 server->context）
        uvhttp_context_t* context = server->context;
        const uvhttp_config_t* global_config =
            uvhttp_config_get_current(context);
        if (global_config) {
            max_connections = global_config->max_connections;
        }
    }

    if (server->active_connections >= max_connections) {
        UVHTTP_LOG_WARN("Connection limit reached: %zu/%zu\n",
                        server->active_connections, (size_t)max_connections);
        /* 创建临时连接以发送503响应 */
        uv_tcp_t* temp_client = uvhttp_alloc(sizeof(uv_tcp_t));
        if (!temp_client) {
            uvhttp_handle_memory_failure("temporary_client_allocation", NULL,
                                         NULL);
            return;
        }

        if (uv_tcp_init(server->loop, temp_client) != 0) {
            UVHTTP_LOG_ERROR("Failed to initialize temporary client\n");
            uvhttp_free(temp_client);
            return;
        }

        if (uv_accept(server_handle, (uv_stream_t*)temp_client) == 0) {
            /* 发送HTTP 503响应 - 使用静态常量避免重复分配 */
            static const char response_503[] = UVHTTP_VERSION_1_1
                " 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " UVHTTP_STRINGIFY(
                    UVHTTP_503_RESPONSE_CONTENT_LENGTH) "\r\n"
                                                        "Connection: close\r\n"
                                                        "\r\n"
                                                        "Service Unavailable";

            uv_write_t* write_req = uvhttp_alloc(sizeof(uv_write_t));
            if (write_req) {
                uv_buf_t buf =
                    uv_buf_init((char*)response_503, sizeof(response_503) - 1);

                int write_result =
                    uv_write(write_req, (uv_stream_t*)temp_client, &buf, 1,
                             write_503_response_cb);
                if (write_result < 0) {
                    UVHTTP_LOG_ERROR("Failed to send 503 response: %s\n",
                                     uv_strerror(write_result));
                    // 如果写入失败，立即释放write_req并关闭连接
                    uvhttp_free(write_req);
                    uv_close((uv_handle_t*)temp_client,
                             (uv_close_cb)uvhttp_free);
                    return;
                }
            } else {
                UVHTTP_LOG_ERROR(
                    "Failed to allocate write request for 503 response\n");
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
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t conn_result = uvhttp_connection_new(server, &conn);
    if (conn_result != UVHTTP_OK) {
        return;
    }

    /* 接受连接 */
    int accept_result =
        uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle);

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
    if (start_result == 0) {
        uvhttp_connection_start_timeout(conn);
    }

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
uvhttp_error_t
uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *server = NULL;

/* 初始化TLS模块（如果还没有初始化） */
#if UVHTTP_FEATURE_TLS
    UVHTTP_LOG_DEBUG("Initializing TLS module...");
    /* 使用全局变量以保持向后兼容性 */
    /* 新项目应使用 uvhttp_context 进行 TLS 配置 */
    UVHTTP_LOG_DEBUG("TLS module initialization skipped (using global "
                     "variables for backward compatibility)");
#endif
    UVHTTP_LOG_DEBUG("Allocating uvhttp_server_t, size=%zu",
                     sizeof(uvhttp_server_t));
    uvhttp_server_t* s = uvhttp_alloc(sizeof(uvhttp_server_t));
    if (!s) {
        UVHTTP_LOG_ERROR("Failed to allocate uvhttp_server_t");
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    UVHTTP_LOG_DEBUG("uvhttp_alloc success, server=%p", (void*)s);
    memset(s, 0, sizeof(uvhttp_server_t));

    // 初始化连接限制默认值
    s->max_connections = UVHTTP_MAX_CONNECTIONS_MAX;  // 默认最大连接数
    s->max_message_size = UVHTTP_MAX_BODY_SIZE;  // 默认最大消息大小1MB

// 初始化WebSocket路由表
#if UVHTTP_FEATURE_WEBSOCKET
    s->ws_routes = NULL;
    s->ws_connection_manager = NULL;
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
    // 初始化限流功能字段
    s->rate_limit_enabled = 0;
    s->rate_limit_max_requests = 0;
    s->rate_limit_window_seconds = 0;
    s->rate_limit_request_count = 0;
    s->rate_limit_window_start_time = 0;
    s->rate_limit_whitelist = NULL;
    s->rate_limit_whitelist_count = 0;
#endif

    // 如果没有提供loop，内部创建新循环
    if (loop) {
        s->loop = loop;
        s->owns_loop = 0;
    } else {
        s->loop = uvhttp_alloc(sizeof(uv_loop_t));
        if (!s->loop) {
            uvhttp_free(s);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        if (uv_loop_init(s->loop) != 0) {
            uvhttp_free(s->loop);
            uvhttp_free(s);
            return UVHTTP_ERROR_IO_ERROR;
        }
        s->owns_loop = 1;
    }

    if (uv_tcp_init(s->loop, &s->tcp_handle) != 0) {
        if (s->owns_loop) {
            uv_loop_close(s->loop);
            uvhttp_free(s->loop);
        }
        uvhttp_free(s);
        return UVHTTP_ERROR_IO_ERROR;
    }
    s->tcp_handle.data = s;
    s->active_connections = 0;
#if UVHTTP_FEATURE_TLS
    s->tls_enabled = 0;
    s->tls_ctx = NULL;
#endif

    *server = s;
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_free(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 关闭 TCP handle */
    if (!uv_is_closing((uv_handle_t*)&server->tcp_handle)) {
        uv_close((uv_handle_t*)&server->tcp_handle, NULL);
    }

    /* 运行循环多次以处理关闭回调
     * 修复：无论是否拥有循环，都需要运行循环处理关闭回调
     * 使用 UV_RUN_ONCE 而不是 UV_RUN_NOWAIT，确保回调被执行
     */
    if (server->loop) {
        for (int index = 0; index < UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS;
             index++) {
            uv_run(server->loop, UV_RUN_ONCE);
        }
    }

    /* 清理连接池 */
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

    /* 清理上下文 */
    if (server->context) {
        uvhttp_context_destroy(server->context);
        server->context = NULL;
    }

// 释放WebSocket路由表
#if UVHTTP_FEATURE_WEBSOCKET
    if (server->ws_routes) {
        ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
        while (current) {
            ws_route_entry_t* next = current->next;
            if (current->path) {
                uvhttp_free(current->path);
            }
            uvhttp_free(current);
            current = next;
        }
        server->ws_routes = NULL;
    }
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
    // 清理限流白名单
    if (server->rate_limit_whitelist) {
        for (size_t i = 0; i < server->rate_limit_whitelist_count; i++) {
            if (server->rate_limit_whitelist[i]) {
                uvhttp_free(server->rate_limit_whitelist[i]);
            }
        }
        uvhttp_free(server->rate_limit_whitelist);
        server->rate_limit_whitelist = NULL;
        server->rate_limit_whitelist_count = 0;
    }

    // 清理白名单哈希表
    struct whitelist_item *current, *tmp;
    HASH_ITER(hh, server->rate_limit_whitelist_hash, current, tmp) {
        HASH_DEL(server->rate_limit_whitelist_hash, current);
        uvhttp_free(current);
    }
    server->rate_limit_whitelist_hash = NULL;

    // 限流状态已嵌入到结构体中，无需额外清理
#endif

    // 如果拥有循环，需要关闭并释放
    if (server->owns_loop && server->loop) {
        uv_loop_close(server->loop);
        uvhttp_free(server->loop);
    }

    uvhttp_free(server);
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!host) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    struct sockaddr_in addr;
    uv_ip4_addr(host, port, &addr);

    /* Nginx 优化：绑定端口 */
    int ret =
        uv_tcp_bind(&server->tcp_handle, (const struct sockaddr*)&addr, 0);
    if (ret != 0) {
        UVHTTP_LOG_ERROR("uv_tcp_bind failed: %s\n", uv_strerror(ret));
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    /* TCP优化：设置TCP_NODELAY和TCP_KEEPALIVE */
    int enable = 1;
    uv_tcp_nodelay(&server->tcp_handle, enable);

    /* 设置keepalive */
    uv_tcp_keepalive(&server->tcp_handle, enable, UVHTTP_TCP_KEEPALIVE_TIMEOUT);

    /* 使用配置系统的backlog设置 */
    // 使用 server->context 而非 loop->data，避免独占 loop->data
    uvhttp_context_t* context = server->context;
    const uvhttp_config_t* config = NULL;

    if (context) {
        config = uvhttp_config_get_current(context);
    }

    int backlog = UVHTTP_BACKLOG;
    if (config && config->backlog > 0) {
        backlog = config->backlog;
    }

    ret = uv_listen((uv_stream_t*)&server->tcp_handle, backlog, on_connection);
    if (ret != 0) {
        UVHTTP_LOG_ERROR("uv_listen failed: %s\n", uv_strerror(ret));
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    server->is_listening = 1;
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_set_handler(uvhttp_server_t* server,
                          uvhttp_request_handler_t handler) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->handler = handler;
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_set_router(uvhttp_server_t* server, uvhttp_router_t* router) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->router = router;
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_set_context(uvhttp_server_t* server,
                          struct uvhttp_context* context) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->context = context;
    return UVHTTP_OK;
}

uvhttp_error_t
uvhttp_server_stop(uvhttp_server_t* server) {
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
uvhttp_error_t
uvhttp_server_enable_tls(uvhttp_server_t* server,
                         uvhttp_tls_context_t* tls_ctx) {
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

uvhttp_error_t
uvhttp_server_disable_tls(uvhttp_server_t* server) {
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

int
uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    return server ? server->tls_enabled : 0;
}
#else
uvhttp_error_t
uvhttp_server_enable_tls(uvhttp_server_t* server, void* tls_ctx) {
    (void)server;
    (void)tls_ctx;
    return UVHTTP_ERROR_INVALID_PARAM;
}

uvhttp_error_t
uvhttp_server_disable_tls(uvhttp_server_t* server) {
    (void)server;
    return UVHTTP_ERROR_INVALID_PARAM;
}

int
uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    (void)server;
    return 0;
}
#endif

// ========== 统一API实现 ==========

// 内部辅助函数
static uvhttp_error_t
create_simple_server_internal(const char* host, int port,
                              uvhttp_server_builder_t** server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_server_builder_t* simple =
        uvhttp_alloc(sizeof(uvhttp_server_builder_t));
    if (!simple) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(simple, 0, sizeof(uvhttp_server_builder_t));

    // 获取或创建事件循环
    simple->loop = uv_default_loop();
    if (!simple->loop) {
        uvhttp_free(simple);
        return UVHTTP_ERROR_IO_ERROR;
    }

    // 创建服务器
    uvhttp_error_t server_result =
        uvhttp_server_new(simple->loop, &simple->server);
    if (server_result != UVHTTP_OK) {
        uvhttp_free(simple);
        return server_result;
    }

    // 创建路由器
    uvhttp_error_t router_result = uvhttp_router_new(&simple->router);
    if (router_result != UVHTTP_OK) {
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        return router_result;
    }

    // 创建并设置默认配置
    uvhttp_error_t result = uvhttp_config_new(&simple->config);
    if (result != UVHTTP_OK) {
        uvhttp_router_free(simple->router);
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        return result;
    }

    simple->server->config = simple->config;
    simple->server->router = simple->router;
    simple->auto_cleanup = 1;

    // 启动监听
    if (uvhttp_server_listen(simple->server, host, port) != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to start server on %s:%d\n", host, port);
        uvhttp_config_free(simple->config);
        uvhttp_router_free(simple->router);
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    *server = simple;
    return UVHTTP_OK;
}

// 快速创建和启动服务器
uvhttp_error_t
uvhttp_server_create(const char* host, int port,
                     uvhttp_server_builder_t** server) {
    return create_simple_server_internal(host, port, server);
}

// 路由添加辅助函数
static uvhttp_server_builder_t*
add_route_internal(uvhttp_server_builder_t* server, const char* path,
                   uvhttp_method_t method, uvhttp_request_handler_t handler) {
    if (!server || !path || !handler)
        return server;

    uvhttp_router_add_route_method(server->router, path, method, handler);
    return server;
}

// 链式路由API
uvhttp_server_builder_t*
uvhttp_get(uvhttp_server_builder_t* server, const char* path,
           uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_GET, handler);
}

uvhttp_server_builder_t*
uvhttp_post(uvhttp_server_builder_t* server, const char* path,
            uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_POST, handler);
}

uvhttp_server_builder_t*
uvhttp_put(uvhttp_server_builder_t* server, const char* path,
           uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_PUT, handler);
}

uvhttp_server_builder_t*
uvhttp_delete(uvhttp_server_builder_t* server, const char* path,
              uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_DELETE, handler);
}

uvhttp_server_builder_t*
uvhttp_any(uvhttp_server_builder_t* server, const char* path,
           uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_ANY, handler);
}

// 简化配置API
uvhttp_server_builder_t*
uvhttp_set_max_connections(uvhttp_server_builder_t* server, int max_conn) {
    if (server && server->config) {
        server->config->max_connections = max_conn;
    }
    return server;
}

uvhttp_server_builder_t*
uvhttp_set_timeout(uvhttp_server_builder_t* server, int timeout) {
    if (server && server->config) {
        server->config->request_timeout = timeout;
        server->config->keepalive_timeout = timeout;
    }
    return server;
}

uvhttp_server_builder_t*
uvhttp_set_max_body_size(uvhttp_server_builder_t* server, size_t size) {
    if (server && server->config) {
        server->config->max_body_size = size;
    }
    return server;
}

// 便捷请求参数获取
const char*
uvhttp_get_param(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_query_param(request, name);
}

const char*
uvhttp_get_header(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_header(request, name);
}

const char*
uvhttp_get_body(uvhttp_request_t* request) {
    return uvhttp_request_get_body(request);
}

// 服务器运行和清理
int
uvhttp_server_run(uvhttp_server_builder_t* server) {
    if (!server || !server->loop)
        return -1;
    return uv_run(server->loop, UV_RUN_DEFAULT);
}

void
uvhttp_server_stop_simple(uvhttp_server_builder_t* server) {
    if (server && server->server) {
        uvhttp_server_stop(server->server);
    }
}

void
uvhttp_server_simple_free(uvhttp_server_builder_t* server) {
    if (!server)
        return;

    if (server->server) {
        uvhttp_server_free(server->server);
    }

    // 注意：router和config由server负责释放，不要重复释放

    uvhttp_free(server);
}

// 默认处理器（用于一键启动）
static int
default_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
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
             method, url, time(NULL));

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    return 0;
}

// 一键启动函数（最简API）
int
uvhttp_serve(const char* host, int port) {
    // 参数验证
    if (port < 1 || port > 65535) {
        fprintf(stderr, "错误: 端口号必须在 1-65535 范围内\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!host) {
        fprintf(stderr, "警告: host 参数为 NULL，使用默认值 0.0.0.0\n");
    }

    uvhttp_server_builder_t* server = NULL;
    uvhttp_error_t create_result = uvhttp_server_create(host, port, &server);
    if (create_result != UVHTTP_OK)
        return create_result;

    // 添加默认路由
    uvhttp_any(server, "/", default_handler);

    printf("UVHTTP 服务器运行在 http://%s:%d\n", host ? host : "0.0.0.0", port);
    printf("按 Ctrl+C 停止服务器\n");

    int run_result = uvhttp_server_run(server);
    uvhttp_server_simple_free(server);

    return run_result;
}

// ========== WebSocket 实现 ==========

#if UVHTTP_FEATURE_WEBSOCKET

// WebSocket握手验证（单线程安全）
// 注册WebSocket处理器（添加到服务器的路由表中）
uvhttp_error_t
uvhttp_server_register_ws_handler(uvhttp_server_t* server, const char* path,
                                  uvhttp_ws_handler_t* handler) {
    if (!server || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 创建新路由条目
    ws_route_entry_t* entry =
        (ws_route_entry_t*)uvhttp_alloc(sizeof(ws_route_entry_t));
    if (!entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // 分配并复制路径（使用 uvhttp_alloc 避免混用分配器）
    size_t path_len = strlen(path);
    entry->path = (char*)uvhttp_alloc(path_len + 1);
    if (!entry->path) {
        uvhttp_free(entry);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->path, path, path_len + 1);

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

// 查找WebSocket处理器（根据路径）
uvhttp_ws_handler_t*
uvhttp_server_find_ws_handler(uvhttp_server_t* server, const char* path) {
    if (!server || !path) {
        return NULL;
    }

    // 遍历WebSocket路由表
    ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
    while (current) {
        if (current->path && strcmp(current->path, path) == 0) {
            // 找到匹配的路径，返回处理器指针
            return &current->handler;
        }
        current = current->next;
    }

    // 未找到匹配的处理器
    return NULL;
}

// 发送WebSocket消息
uvhttp_error_t
uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn, const char* data,
                      size_t len) {
    if (!ws_conn || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 获取 context
    uvhttp_context_t* context = NULL;
    if (ws_conn->ssl) {
        // TLS 连接
        uvhttp_connection_t* conn = (uvhttp_connection_t*)ws_conn->user_data;
        if (conn && conn->server && conn->server->context) {
            context = conn->server->context;
        }
    }

    // 调用原生WebSocket API发送文本消息
    int result = uvhttp_ws_send_text(context, ws_conn, data, len);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

// 关闭WebSocket连接
uvhttp_error_t
uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code,
                       const char* reason) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 获取 context
    uvhttp_context_t* context = NULL;
    if (ws_conn->ssl) {
        // TLS 连接
        uvhttp_connection_t* conn = (uvhttp_connection_t*)ws_conn->user_data;
        if (conn && conn->server && conn->server->context) {
            context = conn->server->context;
        }
    }

    // 调用原生WebSocket API关闭连接
    int result = uvhttp_ws_close(context, ws_conn, code, reason);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

#endif  // UVHTTP_FEATURE_WEBSOCKET

// ========== 限流功能实现（核心功能） ==========

#if UVHTTP_FEATURE_RATE_LIMIT
// ========== 限流功能实现 ==========

// 限流参数限制
#    define MAX_RATE_LIMIT_REQUESTS 1000000      // 最大请求数：100万
#    define MAX_RATE_LIMIT_WINDOW_SECONDS 86400  // 最大时间窗口：24小时

// 启用限流功能
uvhttp_error_t
uvhttp_server_enable_rate_limit(uvhttp_server_t* server, int max_requests,
                                int window_seconds) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (max_requests <= 0 || max_requests > MAX_RATE_LIMIT_REQUESTS) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (window_seconds <= 0 || window_seconds > MAX_RATE_LIMIT_WINDOW_SECONDS) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 初始化限流状态
    server->rate_limit_enabled = 1;
    server->rate_limit_max_requests = max_requests;
    server->rate_limit_window_seconds = window_seconds;
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// 禁用限流功能
uvhttp_error_t
uvhttp_server_disable_rate_limit(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->rate_limit_enabled = 0;
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// 检查限流状态
uvhttp_error_t
uvhttp_server_check_rate_limit(uvhttp_server_t* server) {
    if (!server || !server->rate_limit_enabled) {
        return UVHTTP_OK;  // 限流未启用，允许请求
    }

    // 获取当前时间（毫秒）
    uint64_t current_time = uv_hrtime() / 1000000;
    uint64_t window_duration = server->rate_limit_window_seconds * 1000;

    // 检查时间窗口是否过期
    if (current_time - server->rate_limit_window_start_time >=
        window_duration) {
        // 重置计数器
        server->rate_limit_request_count = 0;
        server->rate_limit_window_start_time = current_time;
    }

    // 检查是否超过限制
    if (server->rate_limit_request_count >= server->rate_limit_max_requests) {
        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
    }

    // 增加计数
    server->rate_limit_request_count++;

    return UVHTTP_OK;
}

// 添加限流白名单IP地址
uvhttp_error_t
uvhttp_server_add_rate_limit_whitelist(uvhttp_server_t* server,
                                       const char* client_ip) {
    if (!server || !client_ip) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 验证IP地址格式
    if (!uvhttp_is_valid_ip_address(client_ip)) {
        return UVHTTP_ERROR_INVALID_PARAM;  // 无效的 IP 地址
    }

    // 检查是否已经存在于哈希表中（避免重复添加）
    struct whitelist_item* existing_item;
    HASH_FIND_STR(server->rate_limit_whitelist_hash, client_ip, existing_item);
    if (existing_item) {
        return UVHTTP_OK;  // 已经存在，无需重复添加
    }

    // 重新分配白名单数组
    size_t new_count = server->rate_limit_whitelist_count + 1;
    void** new_whitelist =
        uvhttp_realloc(server->rate_limit_whitelist, sizeof(void*) * new_count);
    if (!new_whitelist) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    server->rate_limit_whitelist = new_whitelist;
    server->rate_limit_whitelist_count = new_count;

    // 复制IP地址
    size_t ip_len = strlen(client_ip) + 1;
    char* ip_copy = uvhttp_alloc(ip_len);
    if (!ip_copy) {
        // 回退：恢复原来的数组大小
        server->rate_limit_whitelist_count = new_count - 1;
        void** old_whitelist = uvhttp_realloc(server->rate_limit_whitelist,
                                              sizeof(void*) * (new_count - 1));
        if (old_whitelist) {
            server->rate_limit_whitelist = old_whitelist;
        }
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(ip_copy, client_ip, ip_len);
    server->rate_limit_whitelist[new_count - 1] = ip_copy;

    // 添加到哈希表（用于O(1)查找）
    struct whitelist_item* hash_item =
        uvhttp_alloc(sizeof(struct whitelist_item));
    if (!hash_item) {
        // 回退：清理已分配的IP字符串
        uvhttp_free(ip_copy);
        server->rate_limit_whitelist_count = new_count - 1;
        void** old_whitelist = uvhttp_realloc(server->rate_limit_whitelist,
                                              sizeof(void*) * (new_count - 1));
        if (old_whitelist) {
            server->rate_limit_whitelist = old_whitelist;
        }
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    strncpy(hash_item->ip, client_ip, INET_ADDRSTRLEN - 1);
    hash_item->ip[INET_ADDRSTRLEN - 1] = '\0';
    HASH_ADD_STR(server->rate_limit_whitelist_hash, ip, hash_item);

    return UVHTTP_OK;
}

// 获取客户端限流状态
uvhttp_error_t
uvhttp_server_get_rate_limit_status(uvhttp_server_t* server,
                                    const char* client_ip, int* remaining,
                                    uint64_t* reset_time) {
    if (!server || !client_ip || !remaining) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!server->rate_limit_enabled) {
        *remaining = -1;  // 限流未启用
        return UVHTTP_OK;
    }

    *remaining =
        server->rate_limit_max_requests - server->rate_limit_request_count;

    if (reset_time) {
        uint64_t window_duration = server->rate_limit_window_seconds * 1000;
        *reset_time = server->rate_limit_window_start_time + window_duration;
    }

    return UVHTTP_OK;
}

// 清空所有限流状态
uvhttp_error_t
uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// 重置客户端限流状态
uvhttp_error_t
uvhttp_server_reset_rate_limit_client(uvhttp_server_t* server,
                                      const char* client_ip) {
    if (!server || !client_ip) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 简化实现：重置整个服务器的限流计数器
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = uv_hrtime() / 1000000;

    return UVHTTP_OK;
}
#endif /* UVHTTP_FEATURE_RATE_LIMIT */

#if !UVHTTP_FEATURE_TLS
// 空的 TLS 函数定义，用于禁用 TLS 时的链接
void
uvhttp_tls_context_free(void* ctx) {
    (void)ctx;
}
#endif

// ========== WebSocket 连接管理实现 ==========

#if UVHTTP_FEATURE_WEBSOCKET

/**
 * 超时检测定时器回调
 * 检查所有连接的活动时间，关闭超时连接
 */
static void
ws_timeout_timer_callback(uv_timer_t* handle) {
    if (!handle || !handle->data) {
        return;
    }

    ws_connection_manager_t* manager = (ws_connection_manager_t*)handle->data;
    uint64_t current_time = uv_hrtime() / 1000000; /* 转换为毫秒 */
    uint64_t timeout_ms = manager->timeout_seconds * 1000;

    ws_connection_node_t* current = manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        ws_connection_node_t* next = current->next;

        /* 检查连接是否超时 */
        if (current_time - current->last_activity > timeout_ms) {
            UVHTTP_LOG_WARN("WebSocket connection timeout, closing...\n");

            /* 关闭超时连接 */
            if (current->ws_conn) {
                uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                "Connection timeout");
            }

            /* 从链表中移除 */
            if (prev) {
                prev->next = next;
            } else {
                manager->connections = next;
            }

            /* 释放节点 */
            uvhttp_free(current);
            manager->connection_count--;
        } else {
            prev = current;
        }

        current = next;
    }
}

/**
 * 心跳检测定时器回调
 * 定期发送 Ping 帧以检测连接活跃状态
 */
static void
ws_heartbeat_timer_callback(uv_timer_t* handle) {
    if (!handle || !handle->data) {
        return;
    }

    ws_connection_manager_t* manager = (ws_connection_manager_t*)handle->data;
    uint64_t current_time = uv_hrtime() / 1000000; /* 转换为毫秒 */

    ws_connection_node_t* current = manager->connections;

    while (current) {
        if (current->ws_conn &&
            current->ws_conn->state == UVHTTP_WS_STATE_OPEN) {
            /* 检查是否需要发送 Ping */
            if (!current->ping_pending) {
                /* 发送 Ping 帧 */
                if (uvhttp_ws_send_ping(NULL, current->ws_conn, NULL, 0) == 0) {
                    current->last_ping_sent = current_time;
                    current->ping_pending = 1;
                }
            } else {
                /* 检查 Ping 是否超时（未收到 Pong 响应） */
                if (current_time - current->last_ping_sent >
                    manager->ping_timeout_ms) {
                    UVHTTP_LOG_WARN(
                        "WebSocket ping timeout, closing connection...\n");

                    /* 关闭无响应的连接 */
                    uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                    "Ping timeout");
                }
            }
        }

        current = current->next;
    }
}

/**
 * 启用 WebSocket 连接管理
 *
 * @param server 服务器实例
 * @param timeout_seconds 超时时间（秒），范围：10-3600
 * @param heartbeat_interval 心跳间隔（秒），范围：5-300
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t
uvhttp_server_ws_enable_connection_management(uvhttp_server_t* server,
                                              int timeout_seconds,
                                              int heartbeat_interval) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 参数验证 */
    if (timeout_seconds < 10 || timeout_seconds > 3600) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (heartbeat_interval < 5 || heartbeat_interval > 300) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经启用，先禁用 */
    if (server->ws_connection_manager) {
        uvhttp_error_t result =
            uvhttp_server_ws_disable_connection_management(server);
        if (result != UVHTTP_OK) {
            return result;
        }
    }

    /* 创建连接管理器 */
    ws_connection_manager_t* manager =
        uvhttp_alloc(sizeof(ws_connection_manager_t));
    if (!manager) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(manager, 0, sizeof(ws_connection_manager_t));
    manager->connections = NULL;
    manager->connection_count = 0;
    manager->timeout_seconds = timeout_seconds;
    manager->heartbeat_interval = heartbeat_interval;
    manager->ping_timeout_ms = 10000; /* 默认10秒 Ping 超时 */
    manager->enabled = 1;

    /* 初始化超时检测定时器 */
    int ret = uv_timer_init(server->loop, &manager->timeout_timer);
    if (ret != 0) {
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }
    manager->timeout_timer.data = manager;

    /* 初始化心跳检测定时器 */
    ret = uv_timer_init(server->loop, &manager->heartbeat_timer);
    if (ret != 0) {
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }
    manager->heartbeat_timer.data = manager;

    /* 启动定时器 */
    ret = uv_timer_start(&manager->timeout_timer, ws_timeout_timer_callback,
                         timeout_seconds * 1000, timeout_seconds * 1000);
    if (ret != 0) {
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }

    ret = uv_timer_start(&manager->heartbeat_timer, ws_heartbeat_timer_callback,
                         heartbeat_interval * 1000, heartbeat_interval * 1000);
    if (ret != 0) {
        uv_timer_stop(&manager->timeout_timer);
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }

    server->ws_connection_manager = manager;

    UVHTTP_LOG_INFO(
        "WebSocket connection management enabled: timeout=%ds, heartbeat=%ds\n",
        timeout_seconds, heartbeat_interval);

    return UVHTTP_OK;
}

/**
 * 禁用 WebSocket 连接管理
 *
 * @param server 服务器实例
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t
uvhttp_server_ws_disable_connection_management(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager) {
        return UVHTTP_OK; /* 未启用，直接返回成功 */
    }

    /* 停止定时器 */
    if (!uv_is_closing((uv_handle_t*)&manager->timeout_timer)) {
        uv_timer_stop(&manager->timeout_timer);
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
    }

    if (!uv_is_closing((uv_handle_t*)&manager->heartbeat_timer)) {
        uv_timer_stop(&manager->heartbeat_timer);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
    }

    /* 关闭所有连接 */
    ws_connection_node_t* current = manager->connections;
    while (current) {
        ws_connection_node_t* next = current->next;

        if (current->ws_conn) {
            uvhttp_ws_close(NULL, current->ws_conn, 1000, "Server shutdown");
        }

        uvhttp_free(current);
        current = next;
    }

    manager->connections = NULL;
    manager->connection_count = 0;
    manager->enabled = 0;

    /* 释放管理器 */
    uvhttp_free(manager);
    server->ws_connection_manager = NULL;

    UVHTTP_LOG_INFO("WebSocket connection management disabled\n");

    return UVHTTP_OK;
}

/**
 * 获取 WebSocket 连接总数
 *
 * @param server 服务器实例
 * @return 连接数量
 */
int
uvhttp_server_ws_get_connection_count(uvhttp_server_t* server) {
    if (!server || !server->ws_connection_manager) {
        return 0;
    }

    return server->ws_connection_manager->connection_count;
}

/**
 * 获取指定路径的 WebSocket 连接数量
 *
 * @param server 服务器实例
 * @param path 路径
 * @return 连接数量
 */
int
uvhttp_server_ws_get_connection_count_by_path(uvhttp_server_t* server,
                                              const char* path) {
    if (!server || !server->ws_connection_manager || !path) {
        return 0;
    }

    int count = 0;
    ws_connection_node_t* current = server->ws_connection_manager->connections;

    while (current) {
        if (strcmp(current->path, path) == 0) {
            count++;
        }
        current = current->next;
    }

    return count;
}

/**
 * 向指定路径的所有连接广播消息
 *
 * @param server 服务器实例
 * @param path 路径（NULL 表示广播到所有连接）
 * @param data 消息数据
 * @param len 消息长度
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t
uvhttp_server_ws_broadcast(uvhttp_server_t* server, const char* path,
                           const char* data, size_t len) {
    if (!server || !server->ws_connection_manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!data || len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    ws_connection_node_t* current = server->ws_connection_manager->connections;
    int sent_count = 0;

    while (current) {
        /* 检查路径是否匹配（如果指定了路径） */
        if (!path || strcmp(current->path, path) == 0) {
            if (current->ws_conn &&
                current->ws_conn->state == UVHTTP_WS_STATE_OPEN) {
                uvhttp_ws_send_text(NULL, current->ws_conn, data, len);
                sent_count++;
            }
        }

        current = current->next;
    }

    UVHTTP_LOG_DEBUG("WebSocket broadcast: sent to %d connections\n",
                     sent_count);

    return UVHTTP_OK;
}

/**
 * 关闭指定路径的所有连接
 *
 * @param server 服务器实例
 * @param path 路径（NULL 表示关闭所有连接）
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t
uvhttp_server_ws_close_all(uvhttp_server_t* server, const char* path) {
    if (!server || !server->ws_connection_manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    int closed_count = 0;
    ws_connection_node_t* current = server->ws_connection_manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        ws_connection_node_t* next = current->next;

        /* 检查路径是否匹配（如果指定了路径） */
        if (!path || strcmp(current->path, path) == 0) {
            /* 关闭连接 */
            if (current->ws_conn) {
                uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                "Server closed connection");
            }

            /* 从链表中移除 */
            if (prev) {
                prev->next = next;
            } else {
                server->ws_connection_manager->connections = next;
            }

            /* 释放节点 */
            uvhttp_free(current);
            server->ws_connection_manager->connection_count--;
            closed_count++;
        } else {
            prev = current;
        }

        current = next;
    }

    UVHTTP_LOG_DEBUG("WebSocket close_all: closed %d connections\n",
                     closed_count);

    return UVHTTP_OK;
}

/**
 * 内部函数：添加 WebSocket 连接到管理器
 */
void
uvhttp_server_ws_add_connection(uvhttp_server_t* server,
                                uvhttp_ws_connection_t* ws_conn,
                                const char* path) {
    if (!server || !ws_conn || !path) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    /* 创建连接节点 */
    ws_connection_node_t* node = uvhttp_alloc(sizeof(ws_connection_node_t));
    if (!node) {
        UVHTTP_LOG_ERROR("Failed to allocate WebSocket connection node\n");
        return;
    }

    memset(node, 0, sizeof(ws_connection_node_t));
    node->ws_conn = ws_conn;
    strncpy(node->path, path, sizeof(node->path) - 1);
    node->path[sizeof(node->path) - 1] = '\0';
    node->last_activity = uv_hrtime() / 1000000; /* 转换为毫秒 */
    node->last_ping_sent = 0;
    node->ping_pending = 0;
    node->next = NULL;

    /* 添加到链表头部 */
    node->next = manager->connections;
    manager->connections = node;
    manager->connection_count++;

    UVHTTP_LOG_DEBUG("WebSocket connection added: path=%s, total=%d\n", path,
                     manager->connection_count);
}

/**
 * 内部函数：从管理器中移除 WebSocket 连接
 */
void
uvhttp_server_ws_remove_connection(uvhttp_server_t* server,
                                   uvhttp_ws_connection_t* ws_conn) {
    if (!server || !ws_conn) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    ws_connection_node_t* current = manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        if (current->ws_conn == ws_conn) {
            /* 从链表中移除 */
            if (prev) {
                prev->next = current->next;
            } else {
                manager->connections = current->next;
            }

            /* 释放节点 */
            uvhttp_free(current);
            manager->connection_count--;

            UVHTTP_LOG_DEBUG("WebSocket connection removed: total=%d\n",
                             manager->connection_count);
            return;
        }

        prev = current;
        current = current->next;
    }
}

/**
 * 内部函数：更新 WebSocket 连接活动时间
 */
void
uvhttp_server_ws_update_activity(uvhttp_server_t* server,
                                 uvhttp_ws_connection_t* ws_conn) {
    if (!server || !ws_conn) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    ws_connection_node_t* current = manager->connections;

    while (current) {
        if (current->ws_conn == ws_conn) {
            current->last_activity = uv_hrtime() / 1000000; /* 转换为毫秒 */
            current->ping_pending = 0; /* 清除待处理的 Ping 标记 */
            return;
        }

        current = current->next;
    }
}

/* ========== WebSocket 认证 API ========== */

#endif /* UVHTTP_FEATURE_WEBSOCKET */
uvhttp_error_t
uvhttp_server_set_timeout_callback(uvhttp_server_t* server,
                                   uvhttp_timeout_callback_t callback,
                                   void* user_data) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->timeout_callback = callback;
    server->timeout_callback_user_data = user_data;

    return UVHTTP_OK;
}
