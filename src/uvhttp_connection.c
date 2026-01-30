#include "uvhttp_connection.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

/* ========== 编译时验证 ========== */
/* 验证结构体大小，确保内存布局优化不会被破坏 */
#ifdef __cplusplus
#    define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#    define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* ========== 结构体大小验证 ========== */

/* 验证结构体大小在合理范围内（允许用户自定义配置） */

/* 注意：结构体大小取决于 UVHTTP_INLINE_HEADERS_CAPACITY 等可配置常量 */

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_request_t) >= 65536,

                     "uvhttp_request_t size too small");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_request_t) < 2 * 1024 * 1024,

                     "uvhttp_request_t size exceeds 2MB limit, consider "
                     "reducing UVHTTP_INLINE_HEADERS_CAPACITY");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_response_t) >= 65536,

                     "uvhttp_response_t size too small");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_response_t) < 2 * 1024 * 1024,

                     "uvhttp_response_t size exceeds 2MB limit, consider "
                     "reducing UVHTTP_INLINE_HEADERS_CAPACITY");

// 用于安全连接重用的idle回调
static void on_idle_restart_read(uv_idle_t* handle);

/* 连接池获取函数实现 */
static void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size,
                            uv_buf_t* buf) {
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
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->request) {
        UVHTTP_LOG_ERROR("on_read: conn or conn->request is NULL\n");
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

    /* 检查缓冲区有效性 */
    if (!buf || !buf->base) {
        UVHTTP_LOG_ERROR("Invalid buffer in on_read\n");
        uvhttp_connection_close(conn);
        return;
    }

    /* 检查缓冲区边界，防止溢出 */
    if (conn->read_buffer_used + (size_t)nread > conn->read_buffer_size) {
        UVHTTP_LOG_ERROR("Read buffer overflow: %zu + %zd > %zu\n",
                         conn->read_buffer_used, nread, conn->read_buffer_size);
        uvhttp_connection_close(conn);
        return;
    }

    /* 单线程HTTP解析 - 无需同步机制 */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        UVHTTP_LOG_DEBUG("on_read: Parsing %zd bytes\n", nread);
        UVHTTP_LOG_DEBUG("on_read: parser->data = %p, conn = %p\n",
                         parser->data, conn);
        enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);

        if (err != HPE_OK) {
            const char* err_name = llhttp_errno_name(err);
            UVHTTP_LOG_ERROR("HTTP parse error: %d (%s)\n", err,
                             err_name ? err_name : "unknown");
            UVHTTP_LOG_ERROR("HTTP parse error reason: %s\n",
                             llhttp_get_error_reason(parser));
            uvhttp_log_safe_error(err, "http_parse", err_name);
            /* 解析错误时异步关闭连接 */
            uvhttp_connection_close(conn);
            return;
        }

        UVHTTP_LOG_DEBUG(
            "on_read: llhttp_execute success, parsing_complete = %d\n",
            conn->parsing_complete);
    } else {
        UVHTTP_LOG_ERROR("on_read: parser is NULL\n");
    }

    /* 更新已使用的缓冲区大小 */
    conn->read_buffer_used += nread;
}

/* 重新开始读取新请求 - 用于keep-alive连接 */
uvhttp_error_t uvhttp_connection_restart_read(uvhttp_connection_t* conn) {
    if (!conn || !conn->request || !conn->response || !conn->request->parser ||
        !conn->request->parser_settings) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查连接状态，确保连接没有在关闭过程中 */
    if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
        return UVHTTP_ERROR_CONNECTION_CLOSE;
    }

    /* 优化：先停止当前的读取（如果正在进行） */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);

    /* 性能优化：只重置必要的字段，避免清零整个结构体（280KB）
     *
     * 优化原理：
     * - 原方案：memset(conn->request, 0, sizeof(uvhttp_request_t)) 清零 280KB
     * - 新方案：只重置 10 个字段，共约 80 字节
     * - 性能提升：每次连接复用节省 279,920 字节的内存操作
     *
     * 注意事项：
     * - 必须确保所有状态字段都被正确重置
     * - 指针字段需要特殊处理（保持不变或释放）
     * - 大块内存（headers 数组）不需要清零，因为 header_count 已重置
     *
     * 字段重置清单：
     * - 热路径字段：method, parsing_complete, header_count
     * - 指针字段：path, query, body, user_data（设置为 NULL）
     * - 缓冲区字段：url（清零第一个字节）
     * - 大块内存：headers 数组（通过 header_count 标记无效）
     */

    /* 重置请求对象的热路径字段 */
    conn->request->method = UVHTTP_ANY;
    conn->request->parsing_complete = 0;
    conn->request->header_count = 0;
    conn->request->path = NULL;
    conn->request->query = NULL;
    conn->request->body = NULL;
    conn->request->body_length = 0;
    conn->request->body_capacity = 0;
    conn->request->user_data = NULL;

    /* 重置URL缓冲区 */
    conn->request->url[0] = '\0';

    /* 重置headers数组（只重置已使用的部分） */
    /* 注意：不需要清零整个headers数组，因为header_count已经重置为0 */

    /* 重置HTTP解析器 */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        llhttp_reset(parser);
        parser->data = conn;
    }

    /* 性能优化：只重置响应对象的热路径字段，避免清零整个结构体（278KB）
     *
     * 优化原理：
     * - 原方案：memset(conn->response, 0, sizeof(uvhttp_response_t)) 清零 278KB
     * - 新方案：只重置 10 个字段，共约 80 字节
     * - 性能提升：每次连接复用节省 277,920 字节的内存操作
     */

    /* 重置响应对象的热路径字段 */
    conn->response->status_code = 0;
    conn->response->headers_sent = 0;
    conn->response->sent = 0;
    conn->response->finished = 0;
    conn->response->keepalive = 0;
    conn->response->compress = 0;
    conn->response->cache_ttl = 0;
    conn->response->header_count = 0;
    conn->response->body_length = 0;
    conn->response->cache_expires = 0;

    /* 重置响应body */
    if (conn->response->body) {
        uvhttp_free(conn->response->body);
        conn->response->body = NULL;
    }

    /* 重置连接的HTTP/1.1状态标志 */
    conn->parsing_complete = 0;
    conn->content_length = 0;
    conn->body_received = 0;
    conn->keepalive = 1;        /* 继续保持连接 */
    conn->chunked_encoding = 0; /* 重置分块传输编码标志 */
    conn->current_header_is_important = 0;
    conn->parsing_header_field = 0;
    conn->need_restart_read = 0;

    /* 重置当前头部字段 */
    conn->current_header_field_len = 0;

    /* 更新连接状态 */
    conn->state = UVHTTP_CONN_STATE_HTTP_READING;

    /* 重新开始读取以接收新请求 */
    int result = uv_read_start((uv_stream_t*)&conn->tcp_handle, on_alloc_buffer,
                               on_read);

    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to restart reading on connection: %s\n",
                         uv_strerror(result));
    }

    return result;
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
uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                     uvhttp_connection_t** conn) {
    if (!server || !conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *conn = NULL;

    /* 单线程安全的内存分配 */
    uvhttp_connection_t* c = uvhttp_alloc(sizeof(uvhttp_connection_t));
    if (!c) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(c, 0, sizeof(uvhttp_connection_t));

    c->server = server;
    c->state = UVHTTP_CONN_STATE_NEW;
    c->tls_enabled = 0;        // 简化版本暂时禁用TLS
    c->need_restart_read = 0;  // 初始化为0，不需要重启读取

    // 初始化idle句柄用于安全的连接重用
    if (uv_idle_init(server->loop, &c->idle_handle) != 0) {
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->idle_handle.data = c;

    // 初始化超时定时器
    if (uv_timer_init(server->loop, &c->timeout_timer) != 0) {
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->timeout_timer.data = c;

    // HTTP/1.1优化：初始化默认值
    c->keepalive = 1;         /* HTTP/1.1默认保持连接 */
    c->chunked_encoding = 0;  /* 默认不使用分块传输 */
    c->close_pending = 0;     /* 初始化待关闭的 handle 计数 */
    c->content_length = 0;    /* 默认无内容长度 */
    c->body_received = 0;     // 已接收body长度
    c->parsing_complete = 0;  // 解析未完成
    c->current_header_is_important = 0;  // 当前头部非关键字段
    c->read_buffer_used = 0;             // 重置读缓冲区使用量

    // HTTP解析状态初始化
    memset(c->current_header_field, 0, sizeof(c->current_header_field));
    c->current_header_field_len = 0;
    c->parsing_header_field = 0;

    // TCP初始化 - 完整实现
    if (uv_tcp_init(server->loop, &c->tcp_handle) != 0) {
        /* 注意：uv_close 是异步的，不能在这里使用
         * 对于初始化失败的情况，直接释放内存即可
         * 因为这些 handle 还没有被添加到事件循环中 */
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->tcp_handle.data = c;

    // TCP 选项已在服务器级别统一设置（TCP_NODELAY 和 TCP_KEEPALIVE）
    // 避免重复设置以提高性能

    // 分配读缓冲区
    c->read_buffer_size = UVHTTP_READ_BUFFER_SIZE;
    c->read_buffer = uvhttp_alloc(c->read_buffer_size);
    if (!c->read_buffer) {
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    c->read_buffer_used = 0;

    // 创建请求和响应对象
    c->request = uvhttp_alloc(sizeof(uvhttp_request_t));
    if (!c->request) {
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // 正确初始化request对象（包含HTTP解析器）
    if (uvhttp_request_init(c->request, &c->tcp_handle) != 0) {
        uvhttp_free(c->request);
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }

    c->response = uvhttp_alloc(sizeof(uvhttp_response_t));
    if (!c->response) {
        uvhttp_request_cleanup(c->request);
        uvhttp_free(c->request);
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // 正确初始化response对象
    if (uvhttp_response_init(c->response, &c->tcp_handle) != 0) {
        uvhttp_request_cleanup(c->request);
        uvhttp_free(c->request);
        uvhttp_free(c->response);  // 直接释放，不需要cleanup（因为初始化失败）
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }

    // 设置解析器的data指针为连接对象
    llhttp_t* parser = (llhttp_t*)c->request->parser;
    if (parser) {
        parser->data = c;
    }

    *conn = c;
    return UVHTTP_OK;
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

    // 释放连接内存
    uvhttp_free(conn);
}

uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 开始HTTP读取 - 完整实现 */
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle,
                      (uv_alloc_cb)on_alloc_buffer, (uv_read_cb)on_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start reading on connection\n");
        uvhttp_connection_close(conn);
        return UVHTTP_ERROR_CONNECTION_START;
    }
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);

    /* TLS处理 */
    if (conn->tls_enabled) {
        if (uvhttp_connection_tls_handshake_func(conn) != 0) {
            UVHTTP_LOG_ERROR("TLS handshake failed\n");
            uvhttp_connection_close(conn);
            return UVHTTP_ERROR_CONNECTION_START;
        }
    }

    return UVHTTP_OK;
}

/* Handle 关闭回调（通用） */
static void on_handle_close(uv_handle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn) {
        return;
    }

    /* 减少待关闭的 handle 计数 */
    conn->close_pending--;

    /* 当所有 handle 都关闭后，释放连接 */
    if (conn->close_pending == 0) {
        /* 单线程安全的连接计数递减 */
        if (conn->server) {
            conn->server->active_connections--;
        }
        /* 释放连接资源 - 在事件循环线程中安全执行 */
        uvhttp_connection_free(conn);
    }
}

void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);

    /* 初始化待关闭的 handle 计数 */
    conn->close_pending = 0;

    /* 停止 idle handle（如果正在运行） */
    if (!uv_is_closing((uv_handle_t*)&conn->idle_handle)) {
        uv_idle_stop(&conn->idle_handle);
        uv_close((uv_handle_t*)&conn->idle_handle, on_handle_close);
        conn->close_pending++;
    }

    /* 停止超时定时器（如果正在运行） */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
        uv_close((uv_handle_t*)&conn->timeout_timer, on_handle_close);
        conn->close_pending++;
    }

    /* 关闭 TCP handle */
    if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        uv_close((uv_handle_t*)&conn->tcp_handle, on_handle_close);
        conn->close_pending++;
    }
}

void uvhttp_connection_set_state(uvhttp_connection_t* conn,
                                 uvhttp_connection_state_t state) {
    if (conn) {
        conn->state = state;
    }
}

uvhttp_error_t uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn) {
    // 简化版本不支持TLS
    (void)conn;
    return UVHTTP_ERROR_NOT_SUPPORTED;
}

uvhttp_error_t uvhttp_connection_tls_write(uvhttp_connection_t* conn,
                                           const void* data, size_t len) {
    // 简化版本不支持TLS
    (void)conn;
    (void)data;
    (void)len;
    return UVHTTP_ERROR_NOT_SUPPORTED;
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
uvhttp_error_t uvhttp_connection_schedule_restart_read(
    uvhttp_connection_t* conn) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // 使用idle句柄在下一个事件循环中安全重启读取
    conn->idle_handle.data = conn;

    if (uv_idle_start(&conn->idle_handle, on_idle_restart_read) != 0) {
        UVHTTP_LOG_ERROR(
            "Failed to start idle handle for connection restart\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    return UVHTTP_OK;
}

#if UVHTTP_FEATURE_WEBSOCKET

/* WebSocket数据读取回调
 * 在WebSocket握手成功后，使用此回调处理WebSocket帧数据
 */
static void on_websocket_read(uv_stream_t* stream, ssize_t nread,
                              const uv_buf_t* buf) {
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
    uvhttp_ws_connection_t* ws_conn =
        (uvhttp_ws_connection_t*)conn->ws_connection;
    int result =
        uvhttp_ws_process_data(ws_conn, (const uint8_t*)buf->base, nread);
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
static int on_websocket_close(uvhttp_ws_connection_t* ws_conn, int code,
                              const char* reason) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 从wrapper中获取连接对象和用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (!wrapper || !wrapper->conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
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
    return UVHTTP_OK;
}

/* WebSocket错误回调 */
static int on_websocket_error(uvhttp_ws_connection_t* ws_conn, int error_code,
                              const char* error_msg) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    UVHTTP_LOG_ERROR("WebSocket error: %s (code: %d)\n", error_msg, error_code);

    /* 从wrapper中获取用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->user_handler) {
        /* 调用用户注册的错误回调 */
        if (wrapper->user_handler->on_error) {
            int result =
                wrapper->user_handler->on_error(ws_conn, error_code, error_msg);
            if (result != 0) {
                UVHTTP_LOG_ERROR("User on_error callback failed: %d\n", result);
            }
            return result;
        }
    }

    return UVHTTP_ERROR_IO_ERROR;
}

/* WebSocket消息回调 */
static int on_websocket_message(uvhttp_ws_connection_t* ws_conn,
                                const char* data, size_t len, int opcode) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 更新连接活动时间 */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->conn && wrapper->conn->server) {
        uvhttp_server_ws_update_activity(wrapper->conn->server, ws_conn);
    }

    /* 从wrapper中获取用户处理器 */
    if (!wrapper || !wrapper->user_handler) {
        /* 没有用户处理器，忽略消息 */
        return UVHTTP_OK;
    }

    /* 调用用户注册的消息回调 */
    if (wrapper->user_handler->on_message) {
        int result =
            wrapper->user_handler->on_message(ws_conn, data, len, opcode);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_message callback failed: %d\n", result);
            return result;
        }
    }

    return UVHTTP_OK;
}

/* 处理WebSocket握手
 * 在握手响应发送后调用，创建WebSocket连接对象并设置回调
 */
uvhttp_error_t uvhttp_connection_handle_websocket_handshake(
    uvhttp_connection_t* conn, const char* ws_key) {
    if (!conn || !ws_key) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 获取请求路径 */
    const char* path = conn->request ? conn->request->url : NULL;
    if (!path) {
        UVHTTP_LOG_ERROR(
            "Failed to get request path for WebSocket handshake\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 获取客户端 IP 地址 */
    char client_ip[UVHTTP_CLIENT_IP_BUFFER_SIZE] = {0};
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    if (uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)&addr,
                           &addr_len) == 0) {
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
                    /* 使用安全的字符串复制函数 */
                    if (uvhttp_safe_strcpy(token, token_len + 1, token_start) !=
                        0) {
                        token[0] = '\0';
                    } else {
                        token[token_len] = '\0';
                    }
                }
            } else {
                /* 使用安全的字符串复制函数 */
                if (uvhttp_safe_strcpy(token, sizeof(token), token_start) !=
                    0) {
                    token[0] = '\0';
                }
            }
        }
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
        UVHTTP_LOG_ERROR(
            "Failed to get file descriptor for WebSocket connection\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* 创建WebSocket连接对象 */
    uvhttp_ws_connection_t* ws_conn =
        uvhttp_ws_connection_create(fd, NULL, 1, conn->server->config);
    if (!ws_conn) {
        UVHTTP_LOG_ERROR("Failed to create WebSocket connection object\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* 保存WebSocket Key（用于验证） */
    strncpy(ws_conn->client_key, ws_key, sizeof(ws_conn->client_key) - 1);
    ws_conn->client_key[sizeof(ws_conn->client_key) - 1] = '\0';

    /* 创建wrapper以保存连接对象和用户处理器 */
    uvhttp_ws_wrapper_t* wrapper = uvhttp_alloc(sizeof(uvhttp_ws_wrapper_t));
    if (!wrapper) {
        UVHTTP_LOG_ERROR("Failed to allocate WebSocket wrapper\n");
        uvhttp_ws_connection_free(ws_conn);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    wrapper->conn = conn;
    wrapper->user_handler = user_handler;

    /* 设置wrapper为WebSocket连接的user_data */
    ws_conn->user_data = wrapper;

    /* 设置回调函数（内部回调会调用用户回调） */
    uvhttp_ws_set_callbacks(ws_conn, on_websocket_message, on_websocket_close,
                            on_websocket_error);

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
            return UVHTTP_ERROR_IO_ERROR;
        }
    }

    /* 切换到WebSocket数据读取模式 */
    uvhttp_connection_switch_to_websocket(conn);

    /* 添加到连接管理器 */
    if (conn->server) {
        uvhttp_server_ws_add_connection(conn->server, ws_conn, path);
    }

    UVHTTP_LOG_DEBUG("WebSocket handshake completed for path: %s\n", path);
    return UVHTTP_OK;
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
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle, on_alloc_buffer,
                      on_websocket_read) != 0) {
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
        uvhttp_server_ws_remove_connection(
            conn->server, (uvhttp_ws_connection_t*)conn->ws_connection);
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
/* 连接超时回调函数 */
static void connection_timeout_cb(uv_timer_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn || !conn->server) {
        return;
    }

    /* 获取超时时间，如果 config 为 NULL 则使用默认值 */
    int timeout_ms = UVHTTP_CONNECTION_TIMEOUT_DEFAULT * 1000;
    if (conn->server->config) {
        timeout_ms = conn->server->config->connection_timeout * 1000;
    }

    /* 触发应用层超时统计回调 */
    if (conn->server->timeout_callback) {
        conn->server->timeout_callback(
            conn->server, conn, timeout_ms,
            conn->server->timeout_callback_user_data);
    }

    UVHTTP_LOG_WARN("Connection timeout, closing connection...\n");
    uvhttp_connection_close(conn);
}

/* 启动连接超时定时器 */
uvhttp_error_t uvhttp_connection_start_timeout(uvhttp_connection_t* conn) {
    if (!conn || !conn->server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 停止旧的定时器（如果有） */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* 获取超时时间，如果 config 为 NULL 则使用默认值 */
    int timeout_ms = UVHTTP_CONNECTION_TIMEOUT_DEFAULT * 1000;
    if (conn->server->config) {
        timeout_ms = conn->server->config->connection_timeout * 1000;
    }

    /* 启动定时器 */
    if (uv_timer_start(&conn->timeout_timer, connection_timeout_cb, timeout_ms,
                       0) != 0) {
        UVHTTP_LOG_ERROR("Failed to start connection timeout timer\n");
        return UVHTTP_ERROR_CONNECTION_TIMEOUT;
    }

    return UVHTTP_OK;
}

/* 启动连接超时定时器（自定义超时时间） */
uvhttp_error_t uvhttp_connection_start_timeout_custom(uvhttp_connection_t* conn,
                                                      int timeout_seconds) {
    if (!conn || !conn->server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 验证超时时间范围 */
    if (timeout_seconds < UVHTTP_CONNECTION_TIMEOUT_MIN ||
        timeout_seconds > UVHTTP_CONNECTION_TIMEOUT_MAX) {
        UVHTTP_LOG_ERROR("Invalid timeout value: %d seconds\n",
                         timeout_seconds);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查整数溢出 */
    if (timeout_seconds > INT_MAX / 1000) {
        UVHTTP_LOG_ERROR("Timeout value too large: %d seconds\n",
                         timeout_seconds);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 停止旧的定时器（如果有） */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* 启动定时器 */
    if (uv_timer_start(&conn->timeout_timer, connection_timeout_cb,
                       timeout_seconds * 1000, 0) != 0) {
        UVHTTP_LOG_ERROR("Failed to start connection timeout timer\n");
        return UVHTTP_ERROR_CONNECTION_TIMEOUT;
    }

    return UVHTTP_OK;
}
