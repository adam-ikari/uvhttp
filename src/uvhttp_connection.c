#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

static void on_close(uv_handle_t* handle);

static void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)suggested_size; // 避免未使用参数警告
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

static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    (void)buf; // 避免未使用参数警告
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->request) {
        return;
    }
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error: %s\n", uv_err_name(nread));
        }
        uvhttp_connection_close(conn);
        return;
    }
    
    if (nread == 0) {
        return;
    }
    
    // 使用request中的解析器处理数据
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);
        if (err != HPE_OK) {
            fprintf(stderr, "HTTP parse error: %s %s\n", 
                           llhttp_errno_name(err), 
                           llhttp_get_error_reason(parser));
            uvhttp_connection_close(conn);
            return;
        }
    }
}

static void on_close(uv_handle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (conn) {
        uvhttp_connection_free(conn);
    }
}

uvhttp_connection_t* uvhttp_connection_new(struct uvhttp_server* server) {
    if (!server) {
        return NULL;
    }
    
    uvhttp_connection_t* conn = uvhttp_malloc(sizeof(uvhttp_connection_t));
    if (!conn) {
        return NULL;
    }
    
    memset(conn, 0, sizeof(uvhttp_connection_t));
    
    conn->server = server;
    conn->state = UVHTTP_CONN_STATE_NEW;
    conn->tls_enabled = 0; // 简化版本暂时禁用TLS
    
    // HTTP/1.1优化：初始化默认值
    conn->keep_alive = 1;           // HTTP/1.1默认保持连接
    conn->chunked_encoding = 0;     // 默认不使用分块传输
    conn->content_length = 0;       // 默认无内容长度
    conn->body_received = 0;        // 已接收body长度
    conn->parsing_complete = 0;     // 解析未完成
    conn->current_header_is_important = 0; // 当前头部非关键字段
    
    // TCP初始化 - 完整实现
    if (uv_tcp_init(server->loop, &conn->tcp_handle) != 0) {
        uvhttp_free(conn);
        return NULL;
    }
    conn->tcp_handle.data = conn;
    
    // 分配读缓冲区
    conn->read_buffer_size = UVHTTP_READ_BUFFER_SIZE;
    conn->read_buffer = uvhttp_malloc(conn->read_buffer_size);
    if (!conn->read_buffer) {
        uvhttp_free(conn);
        return NULL;
    }
    
    // 创建请求和响应对象
    conn->request = uvhttp_malloc(sizeof(uvhttp_request_t));
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
    
    conn->response = uvhttp_malloc(sizeof(uvhttp_response_t));
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
        uvhttp_free(conn->response);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 设置解析器的data指针为连接对象
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        parser->data = conn;
    }
    
    return conn;
}

void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
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
    
    uvhttp_free(conn);
}

int uvhttp_connection_start(uvhttp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    // 开始HTTP读取 - 完整实现
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle, (uv_alloc_cb)on_alloc_buffer, (uv_read_cb)on_read) != 0) {
        uvhttp_connection_close(conn);
        return -1;
    }
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    
    // TLS处理
    if (conn->tls_enabled) {
        if (uvhttp_connection_tls_handshake_func(conn) != 0) {
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