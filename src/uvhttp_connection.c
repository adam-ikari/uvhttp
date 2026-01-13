#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_error_handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

/* 连接池管理 - 优化连接复用 */
#define UVHTTP_CONNECTION_POOL_SIZE  100
#define UVHTTP_CONNECTION_POOL_TTL   30  // 连接池TTL（秒）

typedef struct uvhttp_connection_pool_entry {
    uvhttp_connection_t* connection;
    time_t last_used;
    struct uvhttp_connection_pool_entry* next;
} uvhttp_connection_pool_entry_t;

static uvhttp_connection_pool_entry_t* g_connection_pool = NULL;
static size_t g_pool_size = 0;

static void on_close(uv_handle_t* handle);

// 用于安全连接重用的idle回调
static void on_idle_restart_read(uv_idle_t* handle);

/* 连接池管理函数 */
static uvhttp_connection_t* uvhttp_connection_pool_acquire(void) __attribute__((unused));
static void uvhttp_connection_pool_release(uvhttp_connection_t* conn) __attribute__((unused));
static void uvhttp_connection_pool_cleanup(void) __attribute__((unused));

/* 连接池获取函数实现 */
static uvhttp_connection_t* uvhttp_connection_pool_acquire(void) {
    if (!g_connection_pool) {
        return NULL;
    }
    
    uvhttp_connection_pool_entry_t* entry = g_connection_pool;
    uvhttp_connection_t* conn = entry->connection;
    
    /* 检查连接是否仍然有效 */
    time_t now = time(NULL);
    if (now - entry->last_used > UVHTTP_CONNECTION_POOL_TTL) {
        /* 连接过期，从池中移除并销毁 */
        g_connection_pool = entry->next;
        uvhttp_connection_free(conn);
        uvhttp_free(entry);
        g_pool_size--;
        return NULL;
    }
    
    /* 从池中移除 */
    g_connection_pool = entry->next;
    uvhttp_free(entry);
    g_pool_size--;
    
    /* 重置连接状态 */
    conn->state = UVHTTP_CONN_STATE_NEW;
    conn->parsing_complete = 0;
    conn->keep_alive = 1;
    
    return conn;
}

/* 连接池释放函数实现 */
static void uvhttp_connection_pool_release(uvhttp_connection_t* conn) {
    if (!conn || g_pool_size >= UVHTTP_CONNECTION_POOL_SIZE) {
        uvhttp_connection_free(conn);
        return;
    }
    
    /* 创建池条目 */
    uvhttp_connection_pool_entry_t* entry = uvhttp_alloc(sizeof(uvhttp_connection_pool_entry_t));
    if (!entry) {
        uvhttp_connection_free(conn);
        return;
    }
    
    entry->connection = conn;
    entry->last_used = time(NULL);
    entry->next = g_connection_pool;
    g_connection_pool = entry;
    g_pool_size++;
}

/* 连接池清理函数实现 */
static void uvhttp_connection_pool_cleanup(void) {
    while (g_connection_pool) {
        uvhttp_connection_pool_entry_t* entry = g_connection_pool;
        g_connection_pool = entry->next;
        uvhttp_connection_free(entry->connection);
        uvhttp_free(entry);
        g_pool_size--;
    }
}

static void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)suggested_size;
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn || !conn->read_buffer) {
        buf->base = NULL;
        buf->len = 0;
        return;
    }
    
    size_t remaining = conn->read_buffer_size - conn->read_buffer_used;
    
    buf->base = conn->read_buffer + conn->read_buffer_used;
    buf->len = remaining;
}

/* 单线程事件驱动的读取回调
 * 这个函数在libuv事件循环线程中被调用，处理所有传入数据
 * 单线程模型优势：无需锁，数据访问安全，执行流可预测
 */
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    (void)buf; /* 避免未使用参数警告 */
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->request) {
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            uvhttp_log_safe_error(nread, "connection_read", NULL);
        }
        /* 异步关闭连接 - 在事件循环中安全执行 */
        uvhttp_connection_close(conn);
        return;
    }
    
    if (nread == 0) {
        return;
    }
    
    /* 单线程HTTP解析 - 无需同步机制 */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);
        
        if (err != HPE_OK) {
            uvhttp_log_safe_error(err, "http_parse", llhttp_errno_name(err));
            /* 解析错误时异步关闭连接 */
            uvhttp_connection_close(conn);
            return;
        }
    }
    
    /* 更新已使用的缓冲区大小 */
    conn->read_buffer_used += nread;
}

/* 重新开始读取新请求 - 用于keep-alive连接 */
int uvhttp_connection_restart_read(uvhttp_connection_t* conn) {
    if (!conn || !conn->request || !conn->response || !conn->request->parser || !conn->request->parser_settings) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 检查连接状态，确保连接没有在关闭过程中 */
    if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
        return UVHTTP_ERROR_CONNECTION_CLOSE;
    }
    
    /* 优化：先停止当前的读取（如果正在进行） */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);
    
    /* 优化：快速重置请求对象状态 */
    memset(conn->request, 0, sizeof(uvhttp_request_t));
    
    /* 重新初始化请求对象 */
    if (uvhttp_request_init(conn->request, &conn->tcp_handle) != 0) {
        UVHTTP_LOG_ERROR("Failed to reinitialize request object during connection restart\n");
        return UVHTTP_ERROR_REQUEST_INIT;
    }

    /* 重要：重新将parser->data设置为connection */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        parser->data = conn;
    }

    /* 完全重置响应对象状态 */
    memset(conn->response, 0, sizeof(uvhttp_response_t));
    
    /* 重新初始化响应对象 */
    if (uvhttp_response_init(conn->response, &conn->tcp_handle) != 0) {
        UVHTTP_LOG_ERROR("Failed to reinitialize response object during connection restart\n");
        return UVHTTP_ERROR_RESPONSE_INIT;
    }
    
    /* 重置HTTP/1.1状态标志 */
    conn->parsing_complete = 0;
    conn->content_length = 0;
    conn->body_received = 0;
    conn->keep_alive = 1;  // 继续保持连接
    
    /* 更新连接状态 */
    conn->state = UVHTTP_CONN_STATE_HTTP_READING;
    
    /* 重新开始读取以接收新请求 */
    int result = uv_read_start((uv_stream_t*)&conn->tcp_handle, 
                      on_alloc_buffer, on_read);
    
    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to restart reading on connection: %s\n", uv_strerror(result));
    }
    
    return result;
}

/* 单线程安全的连接关闭回调
 * 在libuv事件循环线程中执行，确保资源安全释放
 * 无需锁机制，因为所有操作都在同一线程中
 */
static void on_close(uv_handle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (conn) {
        /* 单线程安全的连接计数递减 */
        if (conn->server) {
            conn->server->active_connections--;
        }
        /* 释放连接资源 - 在事件循环线程中安全执行 */
        uvhttp_connection_free(conn);
    }
}

/* 创建新的HTTP连接对象（单线程事件驱动）
 * server: 所属的HTTP服务器
 * 返回: 连接对象，所有操作都在事件循环线程中处理
 * 
 * 单线程连接管理特点：
 * 1. 无需连接池锁机制
 * 2. 内存分配在单线程中进行，安全可靠
 * 3. 所有状态变更都在事件循环中串行化
 */
uvhttp_connection_t* uvhttp_connection_new(struct uvhttp_server* server) {
    if (!server) {
        return NULL;
    }
    
    /* 单线程安全的内存分配 */
    uvhttp_connection_t* conn = uvhttp_alloc(sizeof(uvhttp_connection_t));
    if (!conn) {
        return NULL;
    }
    
    memset(conn, 0, sizeof(uvhttp_connection_t));
    
    conn->server = server;
    conn->state = UVHTTP_CONN_STATE_NEW;
    conn->tls_enabled = 0; // 简化版本暂时禁用TLS
    conn->need_restart_read = 0; // 初始化为0，不需要重启读取
    
    // 初始化idle句柄用于安全的连接重用
    if (uv_idle_init(server->loop, &conn->idle_handle) != 0) {
        uvhttp_free(conn);
        return NULL;
    }
    conn->idle_handle.data = conn;
    
    // HTTP/1.1优化：初始化默认值
    conn->keep_alive = 1;           // HTTP/1.1默认保持连接
    conn->chunked_encoding = 0;     // 默认不使用分块传输
    conn->content_length = 0;       // 默认无内容长度
    conn->body_received = 0;        // 已接收body长度
    conn->parsing_complete = 0;     // 解析未完成
    conn->current_header_is_important = 0; // 当前头部非关键字段
    conn->read_buffer_used = 0;     // 重置读缓冲区使用量
    
    // HTTP解析状态初始化
    memset(conn->current_header_field, 0, sizeof(conn->current_header_field));
    conn->current_header_field_len = 0;
    conn->parsing_header_field = 0;
    
    // TCP初始化 - 完整实现
    if (uv_tcp_init(server->loop, &conn->tcp_handle) != 0) {
        uvhttp_free(conn);
        return NULL;
    }
    conn->tcp_handle.data = conn;
    
    // TCP 选项已在服务器级别统一设置（TCP_NODELAY 和 TCP_KEEPALIVE）
    // 避免重复设置以提高性能
    
    // 分配读缓冲区
    conn->read_buffer_size = UVHTTP_READ_BUFFER_SIZE;
    conn->read_buffer = uvhttp_alloc(conn->read_buffer_size);
    if (!conn->read_buffer) {
        uvhttp_free(conn);
        return NULL;
    }
    conn->read_buffer_used = 0;
    
    // 创建请求和响应对象
    conn->request = uvhttp_alloc(sizeof(uvhttp_request_t));
    if (!conn->request) {
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 正确初始化request对象（包含HTTP解析器）
    if (uvhttp_request_init(conn->request, &conn->tcp_handle) != 0) {
        uvhttp_free(conn->request);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    conn->response = uvhttp_alloc(sizeof(uvhttp_response_t));
    if (!conn->response) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 正确初始化response对象
    if (uvhttp_response_init(conn->response, &conn->tcp_handle) != 0) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        uvhttp_free(conn->response);  // 直接释放，不需要cleanup（因为初始化失败）
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 设置解析器的data指针为连接对象
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        parser->data = conn;
    }
    
    // 初始化内存池
    conn->mempool = uvhttp_mempool_create();
    if (!conn->mempool) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    return conn;
}

void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    // 清理请求和响应数据
    if (conn->request) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
    }
    
    if (conn->response) {
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
    }
    
    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
    }
    
    // 释放内存池
    if (conn->mempool) {
        uvhttp_mempool_destroy(conn->mempool);
    }
    
    // 释放连接内存
    uvhttp_free(conn);
}

int uvhttp_connection_start(uvhttp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    // 开始HTTP读取 - 完整实现
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle, (uv_alloc_cb)on_alloc_buffer, (uv_read_cb)on_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start reading on connection\n");
        uvhttp_connection_close(conn);
        return -1;
    }
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    
    // TLS处理
    if (conn->tls_enabled) {
        if (uvhttp_connection_tls_handshake_func(conn) != 0) {
            UVHTTP_LOG_ERROR("TLS handshake failed\n");
            uvhttp_connection_close(conn);
            return -1;
        }
    }
    
    return 0;
}

void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    uv_close((uv_handle_t*)&conn->tcp_handle, on_close);
}

void uvhttp_connection_set_state(uvhttp_connection_t* conn, uvhttp_connection_state_t state) {
    if (conn) {
        conn->state = state;
    }
}

int uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn) {
    // 简化版本不支持TLS
    (void)conn;
    return -1;
}

int uvhttp_connection_tls_write(uvhttp_connection_t* conn, const void* data, size_t len) {
    // 简化版本不支持TLS
    (void)conn;
    (void)data;
    (void)len;
    return -1;
}

/* 用于安全重启读取的idle回调函数
 * 在下一个事件循环中执行，避免在写入完成回调中直接操作状态
 */
static void on_idle_restart_read(uv_idle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn) {
        return;
    }
    
    // 停止idle句柄
    uv_idle_stop(handle);
    
    // 检查连接状态
    if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
        return;
    }
    
    // 执行连接重启
    if (uvhttp_connection_restart_read(conn) != 0) {
        // 重启失败，关闭连接
        uvhttp_connection_close(conn);
    }
}

// 启动安全的连接重用
int uvhttp_connection_schedule_restart_read(uvhttp_connection_t* conn) {
    if (!conn) {
        return -1;
    }

    // 使用idle句柄在下一个事件循环中安全重启读取
    conn->idle_handle.data = conn;

    if (uv_idle_start(&conn->idle_handle, on_idle_restart_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start idle handle for connection restart\n");
        return -1;
    }

    return 0;
}

#if UVHTTP_FEATURE_WEBSOCKET

/* WebSocket数据读取回调
 * 在WebSocket握手成功后，使用此回调处理WebSocket帧数据
 */
static void on_websocket_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->ws_connection) {
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            UVHTTP_LOG_ERROR("WebSocket read error: %s\n", uv_strerror(nread));
        }
        uvhttp_connection_websocket_close(conn);
        return;
    }

    if (nread == 0) {
        return;
    }

    /* 处理WebSocket帧数据 */
    uvhttp_ws_connection_t* ws_conn = (uvhttp_ws_connection_t*)conn->ws_connection;
    int result = uvhttp_ws_process_data(ws_conn, (const uint8_t*)buf->base, nread);
    if (result != 0) {
        UVHTTP_LOG_ERROR("WebSocket data processing failed: %d\n", result);
        uvhttp_connection_websocket_close(conn);
    }
}

/* WebSocket连接包装器 - 用于保存用户处理器和连接对象 */
typedef struct {
    uvhttp_connection_t* conn;
    uvhttp_ws_handler_t* user_handler;
} uvhttp_ws_wrapper_t;

/* WebSocket连接关闭回调 */
static int on_websocket_close(uvhttp_ws_connection_t* ws_conn, int code, const char* reason) {
    if (!ws_conn) {
        return -1;
    }

    /* 从wrapper中获取连接对象和用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (!wrapper || !wrapper->conn) {
        return -1;
    }

    /* 调用用户注册的关闭回调 */
    if (wrapper->user_handler && wrapper->user_handler->on_close) {
        int result = wrapper->user_handler->on_close(ws_conn);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_close callback failed: %d\n", result);
        }
    }

    /* 释放wrapper */
    uvhttp_free(wrapper);
    ws_conn->user_data = NULL;

    (void)code;
    (void)reason;
    return 0;
}

/* WebSocket错误回调 */
static int on_websocket_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    if (!ws_conn) {
        return -1;
    }

    UVHTTP_LOG_ERROR("WebSocket error: %s (code: %d)\n", error_msg, error_code);

    /* 从wrapper中获取用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->user_handler) {
        /* 调用用户注册的错误回调 */
        if (wrapper->user_handler->on_error) {
            int result = wrapper->user_handler->on_error(ws_conn, error_code, error_msg);
            if (result != 0) {
                UVHTTP_LOG_ERROR("User on_error callback failed: %d\n", result);
            }
            return result;
        }
    }

    return -1;
}

/* WebSocket消息回调 */
static int on_websocket_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    if (!ws_conn) {
        return -1;
    }

    /* 更新连接活动时间 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->conn && wrapper->conn->server) {
        uvhttp_server_ws_update_activity(wrapper->conn->server, ws_conn);
    }

    /* 从wrapper中获取用户处理器 */
    if (!wrapper || !wrapper->user_handler) {
        /* 没有用户处理器，忽略消息 */
        return 0;
    }

    /* 调用用户注册的消息回调 */
    if (wrapper->user_handler->on_message) {
        int result = wrapper->user_handler->on_message(ws_conn, data, len, opcode);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_message callback failed: %d\n", result);
            return result;
        }
    }

    return 0;
}

/* 处理WebSocket握手
 * 在握手响应发送后调用，创建WebSocket连接对象并设置回调
 */
int uvhttp_connection_handle_websocket_handshake(uvhttp_connection_t* conn, const char* ws_key) {
    if (!conn || !ws_key) {
        return -1;
    }

    /* 获取请求路径 */
    const char* path = conn->request ? conn->request->url : NULL;
    if (!path) {
        UVHTTP_LOG_ERROR("Failed to get request path for WebSocket handshake\n");
        return -1;
    }

    /* 获取客户端 IP 地址 */
    char client_ip[64] = {0};
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    if (uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)&addr, &addr_len) == 0) {
        uv_ip4_name(&addr, client_ip, sizeof(client_ip));
    }

    /* 从查询参数或头部获取 Token */
    char token[256] = {0};
    if (conn->request) {
        /* 尝试从查询参数获取 */
        const char* query = conn->request->query;
        if (query && strstr(query, "token=")) {
            const char* token_start = strstr(query, "token=") + 6;
            const char* token_end = strchr(token_start, '&');
            if (token_end) {
                size_t token_len = token_end - token_start;
                if (token_len < sizeof(token)) {
                    strncpy(token, token_start, token_len);
                    token[token_len] = '\0';
                }
            } else {
                strncpy(token, token_start, sizeof(token) - 1);
                token[sizeof(token) - 1] = '\0';
            }
        }

        /* 如果查询参数中没有，尝试从头部获取 */
        if (token[0] == '\0') {
            const char* auth_header = uvhttp_request_get_header(conn->request, "Authorization");
            if (auth_header && strncmp(auth_header, "Bearer ", 7) == 0) {
                strncpy(token, auth_header + 7, sizeof(token) - 1);
                token[sizeof(token) - 1] = '\0';
            }
        }
    }

    /* 执行认证检查 */
    uvhttp_ws_auth_result_t auth_result = UVHTTP_WS_AUTH_SUCCESS;
    if (conn->server) {
        auth_result = uvhttp_server_ws_authenticate(conn->server, path, client_ip, token);
    }

    if (auth_result != UVHTTP_WS_AUTH_SUCCESS) {
        UVHTTP_LOG_WARN("WebSocket authentication failed for path %s: %s\n",
                       path, uvhttp_ws_auth_result_string(auth_result));
        return -1;
    }

    /* 查找用户注册的WebSocket处理器 */
    uvhttp_ws_handler_t* user_handler = NULL;
    if (conn->server) {
        user_handler = uvhttp_server_find_ws_handler(conn->server, path);
    }

    if (!user_handler) {
        UVHTTP_LOG_WARN("No WebSocket handler found for path: %s\n", path);
        /* 继续创建连接，但使用默认回调 */
    }

    /* 获取TCP文件描述符 */
    int fd = 0;
    if (uv_fileno((uv_handle_t*)&conn->tcp_handle, &fd) != 0) {
        UVHTTP_LOG_ERROR("Failed to get file descriptor for WebSocket connection\n");
        return -1;
    }

    /* 创建WebSocket连接对象 */
    uvhttp_ws_connection_t* ws_conn = uvhttp_ws_connection_create(fd, NULL, 1);
    if (!ws_conn) {
        UVHTTP_LOG_ERROR("Failed to create WebSocket connection object\n");
        return -1;
    }

    /* 保存WebSocket Key（用于验证） */
    strncpy(ws_conn->client_key, ws_key, sizeof(ws_conn->client_key) - 1);
    ws_conn->client_key[sizeof(ws_conn->client_key) - 1] = '\0';

    /* 创建wrapper以保存连接对象和用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = uvhttp_alloc(sizeof(uvhttp_ws_wrapper_t));
    if (!wrapper) {
        UVHTTP_LOG_ERROR("Failed to allocate WebSocket wrapper\n");
        uvhttp_ws_connection_free(ws_conn);
        return -1;
    }
    wrapper->conn = conn;
    wrapper->user_handler = user_handler;

    /* 设置wrapper为WebSocket连接的user_data */
    ws_conn->user_data = wrapper;

    /* 设置回调函数（内部回调会调用用户回调） */
    uvhttp_ws_set_callbacks(ws_conn, on_websocket_message, on_websocket_close, on_websocket_error);

    /* 保存到连接对象 */
    conn->ws_connection = ws_conn;
    conn->is_websocket = 1;

    /* 调用用户注册的连接回调 */
    if (user_handler && user_handler->on_connect) {
        int result = user_handler->on_connect(ws_conn);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_connect callback failed: %d\n", result);
            /* 连接回调失败，关闭连接 */
            uvhttp_connection_websocket_close(conn);
            return -1;
        }
    }

    /* 切换到WebSocket数据读取模式 */
    uvhttp_connection_switch_to_websocket(conn);

    /* 添加到连接管理器 */
    if (conn->server) {
        uvhttp_server_ws_add_connection(conn->server, ws_conn, path);
    }

    UVHTTP_LOG_DEBUG("WebSocket handshake completed for path: %s\n", path);
    return 0;
}

/* 切换到WebSocket数据处理模式
 * 停止HTTP读取，启动WebSocket帧读取
 */
void uvhttp_connection_switch_to_websocket(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    /* 停止HTTP读取 */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);

    /* 更新连接状态 */
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;

    /* 启动WebSocket数据读取 */
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle, on_alloc_buffer, on_websocket_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start WebSocket reading\n");
        uvhttp_connection_close(conn);
        return;
    }

    UVHTTP_LOG_DEBUG("Switched to WebSocket mode for connection\n");
}

/* 关闭WebSocket连接 */
void uvhttp_connection_websocket_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    /* 从连接管理器中移除 */
    if (conn->server && conn->ws_connection) {
        uvhttp_server_ws_remove_connection(conn->server,
                                          (uvhttp_ws_connection_t*)conn->ws_connection);
    }

    /* 释放WebSocket连接对象 */
    if (conn->ws_connection) {
        uvhttp_ws_connection_free((uvhttp_ws_connection_t*)conn->ws_connection);
        conn->ws_connection = NULL;
    }

    conn->is_websocket = 0;

    /* 关闭底层TCP连接 */
    uvhttp_connection_close(conn);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */