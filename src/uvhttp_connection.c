#include "uvhttp_connection.h"
#include "uvhttp_utils.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_tls.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* state_strings[] = {
    "NEW",
    "TLS_HANDSHAKE", 
    "HTTP_READING",
    "HTTP_PROCESSING",
    "HTTP_WRITING",
    "CLOSING"
};

// HTTP解析器回调函数
static int on_message_begin(llhttp_t* parser) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 重置解析状态
    conn->parsing_complete = 0;
    conn->content_length = 0;
    conn->body_received = 0;
    
    return 0;
}

static int on_url(llhttp_t* parser, const char* at, size_t length) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 确保URL长度不超过限制
    if (length >= MAX_URL_LEN) {
        return -1;
    }
    
    // 优化：直接复制，避免重复检查
    memcpy(conn->request->url, at, length);
    conn->request->url[length] = '\0';
    
    // HTTP/1.1优化：预解析URL中的路径和查询参数
    char* query_start = strchr(conn->request->url, '?');
    if (query_start) {
        *query_start = '\0';
        conn->request->path = conn->request->url;
        conn->request->query = query_start + 1;
        *query_start = '?'; // 恢复原始URL
    } else {
        conn->request->path = conn->request->url;
        conn->request->query = NULL;
    }
    
    return 0;
}

static int on_header_field(llhttp_t* parser, const char* at, size_t length) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 检查header数量限制
    if (conn->request->header_count >= MAX_HEADERS) {
        return -1;
    }
    
    uvhttp_header_t* header = &conn->request->headers[conn->request->header_count];
    
    // 优化：直接复制，减少函数调用
    size_t copy_len = length < UVHTTP_MAX_HEADER_NAME_SIZE - 1 ? length : UVHTTP_MAX_HEADER_NAME_SIZE - 1;
    memcpy(header->name, at, copy_len);
    header->name[copy_len] = '\0';
    
    // HTTP/1.1优化：提前检查关键头部字段
    if (strncasecmp(header->name, "connection", 10) == 0 ||
        strncasecmp(header->name, "content-length", 14) == 0 ||
        strncasecmp(header->name, "host", 4) == 0) {
        conn->current_header_is_important = 1;
    } else {
        conn->current_header_is_important = 0;
    }
    
    return 0;
}

static int on_header_value(llhttp_t* parser, const char* at, size_t length) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    uvhttp_header_t* header = &conn->request->headers[conn->request->header_count];
    
    // 优化：直接复制，减少函数调用
    size_t copy_len = length < UVHTTP_MAX_HEADER_VALUE_SIZE - 1 ? length : UVHTTP_MAX_HEADER_VALUE_SIZE - 1;
    memcpy(header->value, at, copy_len);
    header->value[copy_len] = '\0';
    
    // HTTP/1.1优化：处理关键头部字段
    if (conn->current_header_is_important) {
        if (strncasecmp(header->name, "connection", 10) == 0) {
            // 解析Connection头，决定是否保持连接
            if (strncasecmp(header->value, "keep-alive", 10) == 0) {
                conn->keep_alive = 1;
            } else if (strncasecmp(header->value, "close", 5) == 0) {
                conn->keep_alive = 0;
            }
        } else if (strncasecmp(header->name, "content-length", 14) == 0) {
            // 解析Content-Length头
            conn->content_length = strtoul(header->value, NULL, 10);
        } else if (strncasecmp(header->name, "transfer-encoding", 17) == 0) {
            // 检查分块传输编码
            if (strncasecmp(header->value, "chunked", 7) == 0) {
                conn->chunked_encoding = 1;
            }
        }
    }
    
    // 验证header
    if (uvhttp_validate_header_value(header->name, header->value) != 0) {
        return -1;
    }
    
    conn->request->header_count++;
    return 0;
}

static int on_body(llhttp_t* parser, const char* at, size_t length) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 检查body大小限制（1MB）
    if (conn->body_received + length > UVHTTP_MAX_BODY_SIZE) {
        return -1;
    }
    
    // 重新分配内存以容纳body
    char* new_body = uvhttp_realloc(conn->request->body, conn->body_received + length);
    if (!new_body) {
        // 保留原有内存，避免内存泄漏
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    conn->request->body = new_body;
    memcpy(conn->request->body + conn->body_received, at, length);
    conn->body_received += length;
    conn->request->body_length = conn->body_received;
    
    return 0;
}

static int on_message_complete(llhttp_t* parser) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 设置HTTP方法
    conn->request->method = (uvhttp_method_t)llhttp_get_method(parser);
    
    // HTTP/1.1优化：根据HTTP版本和Connection头决定keep-alive
    int http_major = llhttp_get_http_major(parser);
    int http_minor = llhttp_get_http_minor(parser);
    
    if (http_major == 1 && http_minor == 1) {
        // HTTP/1.1默认保持连接，除非明确指定close
        if (conn->keep_alive != 0) {
            conn->keep_alive = 1;
        }
    } else if (http_major == 1 && http_minor == 0) {
        // HTTP/1.0默认关闭连接，除非明确指定keep-alive
        if (conn->keep_alive != 1) {
            conn->keep_alive = 0;
        }
    }
    
    // 标记解析完成
    conn->parsing_complete = 1;
    
    // 触发HTTP请求处理
    if (conn->server && conn->server->router && conn->request && conn->response) {
        // 安全检查路径
        if (!conn->request->path) {
            conn->request->path = "/";
        }
        
        // 使用我们自己的方法转换
        const char* method_str = uvhttp_method_to_string((uvhttp_method_t)llhttp_get_method(parser));
        if (!method_str) {
            method_str = "GET"; // 默认方法
        }
        
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(
            conn->server->router, 
            conn->request->path, 
            method_str
        );
        
        if (handler) {
            handler(conn->request, conn->response);
        } else {
            // 404 Not Found
            uvhttp_response_set_status(conn->response, 404);
            uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(conn->response, "Not Found", 9);
            uvhttp_response_send(conn->response);
        }
    }
    
    return 0;
}

// 初始化HTTP解析器
static int init_http_parser(uvhttp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    // 分配解析器设置
    conn->parser_settings = uvhttp_malloc(sizeof(llhttp_settings_t));
    if (!conn->parser_settings) {
        return -1;
    }
    
    llhttp_settings_init(conn->parser_settings);
    
    // 设置回调函数 - 使用正确的llhttp API
    conn->parser_settings->on_message_begin = on_message_begin;
    conn->parser_settings->on_url = on_url;
    conn->parser_settings->on_header_field = on_header_field;
    conn->parser_settings->on_header_value = on_header_value;
    conn->parser_settings->on_body = on_body;
    conn->parser_settings->on_message_complete = on_message_complete;
    
    // 分配解析器 - 使用固定大小避免不完整类型
    conn->http_parser = uvhttp_calloc(1, UVHTTP_PARSER_INTERNAL_SIZE);
    if (!conn->http_parser) {
        uvhttp_free(conn->parser_settings);
        conn->parser_settings = NULL;
        return -1;
    }
    
    // 初始化解析器并设置连接上下文
    llhttp_t* parser = (llhttp_t*)conn->http_parser;
    llhttp_init(parser, HTTP_REQUEST, conn->parser_settings);
    // 设置解析器数据指针 - 使用官方API
    parser->data = conn;
    
    return 0;
}

static void on_close(void* handle);

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
        uvhttp_free(conn->read_buffer);
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
    
    // 初始化HTTP解析器
    if (init_http_parser(conn) != 0) {
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 创建请求和响应对象
    conn->request = uvhttp_malloc(sizeof(uvhttp_request_t));
    if (!conn->request) {
        uvhttp_free(conn->http_parser);
        uvhttp_free(conn->parser_settings);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 正确初始化request对象
    if (uvhttp_request_init(conn->request, &conn->tcp_handle) != 0) {
        uvhttp_free(conn->request);
        uvhttp_free(conn->http_parser);
        uvhttp_free(conn->parser_settings);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    conn->response = uvhttp_malloc(sizeof(uvhttp_response_t));
    if (!conn->response) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        uvhttp_free(conn->http_parser);
        uvhttp_free(conn->parser_settings);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    // 正确初始化response对象
    if (uvhttp_response_init(conn->response, &conn->tcp_handle) != 0) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        uvhttp_free(conn->response);
        uvhttp_free(conn->http_parser);
        uvhttp_free(conn->parser_settings);
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
    
    if (conn->request) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
    }
    
    if (conn->response) {
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
    }
    
    if (conn->http_parser) {
        uvhttp_free(conn->http_parser);
    }
    
    if (conn->parser_settings) {
        uvhttp_free(conn->parser_settings);
    }
    
    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
    }
    
    uvhttp_free(conn);
}

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
    if (!conn) {
        return;
    }
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            // 连接错误
            uvhttp_connection_close(conn);
        }
        return;
    }
    
    if (nread == 0) {
        return;
    }
    
    // 处理接收到的数据
    if (conn->http_parser) {
        llhttp_t* parser = (llhttp_t*)conn->http_parser;
        int result = llhttp_execute(parser, buf->base, nread);
        if (result != HPE_OK) {
            // 解析错误
            uvhttp_connection_close(conn);
            return;
        }
    }
    
    conn->read_buffer_used = 0;
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





int uvhttp_connection_tls_write(uvhttp_connection_t* conn, const void* data, size_t len) {
    // 简化版本不支持TLS
    (void)conn;
    (void)data;
    (void)len;
    return -1;
}

void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    
    // 连接关闭处理 - 完整实现
    if (conn->server) {
        conn->server->active_connections--;
    }
    
    // 清理资源
    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
        conn->read_buffer = NULL;
    }
    
    if (conn->http_parser) {
        uvhttp_free(conn->http_parser);
        conn->http_parser = NULL;
    }
    
    if (conn->parser_settings) {
        uvhttp_free(conn->parser_settings);
        conn->parser_settings = NULL;
    }
    
    if (conn->request) {
        uvhttp_request_free(conn->request);
        conn->request = NULL;
    }
    
    if (conn->response) {
        uvhttp_response_free(conn->response);
        conn->response = NULL;
    }
    
    // TLS清理
    if (conn->tls_enabled && conn->ssl) {
        uvhttp_connection_tls_cleanup(conn);
    }
    on_close(conn);
}

void uvhttp_connection_set_state(uvhttp_connection_t* conn, uvhttp_connection_state_t state) {
    if (conn) {
        conn->state = state;
    }
}

const char* uvhttp_connection_get_state_string(uvhttp_connection_state_t state) {
    if (state >= 0 && state < sizeof(state_strings) / sizeof(state_strings[0])) {
        return state_strings[state];
    }
    return "UNKNOWN";
}





// TLS适配函数 - 注意这个函数与uvhttp_tls.h中的函数名冲突
// 这里实现connection版本的包装函数
int uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn) {
    if (!conn || !conn->ssl) {
        return -1;
    }
    // 调用实际的TLS握手函数
    uvhttp_tls_error_t result = uvhttp_tls_handshake((SSL*)conn->ssl);
    return (result == UVHTTP_TLS_OK) ? 0 : -1;
}

void uvhttp_connection_tls_cleanup(uvhttp_connection_t* conn) {
    if (conn && conn->ssl) {
        SSL_free((SSL*)conn->ssl);
        conn->ssl = NULL;
    }
}

static void on_close(void* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle;
    if (conn) {
        if (conn->server) {
            conn->server->active_connections--;
        }
        uvhttp_connection_free(conn);
    }
}