#include "uvhttp_connection.h"
#include "uvhttp_utils.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
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
    
    memcpy(conn->request->url, at, length);
    conn->request->url[length] = '\0';
    
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
    
    // 复制header名称
    if (uvhttp_safe_strcpy(header->name, UVHTTP_MAX_HEADER_NAME_SIZE, "") != 0) {
        return -1;
    }
    size_t copy_len = length < UVHTTP_MAX_HEADER_NAME_SIZE - 1 ? length : UVHTTP_MAX_HEADER_NAME_SIZE - 1;
    memcpy(header->name, at, copy_len);
    header->name[copy_len] = '\0';
    
    return 0;
}

static int on_header_value(llhttp_t* parser, const char* at, size_t length) {
    // 使用官方API获取data字段
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    uvhttp_header_t* header = &conn->request->headers[conn->request->header_count];
    
    // 复制header值
    if (uvhttp_safe_strcpy(header->value, UVHTTP_MAX_HEADER_VALUE_SIZE, "") != 0) {
        return -1;
    }
    size_t copy_len = length < UVHTTP_MAX_HEADER_VALUE_SIZE - 1 ? length : UVHTTP_MAX_HEADER_VALUE_SIZE - 1;
    memcpy(header->value, at, copy_len);
    header->value[copy_len] = '\0';
    
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
    conn->request->method = (int)llhttp_get_method(parser);
    
    // 标记解析完成
    conn->parsing_complete = 1;
    
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
    llhttp_init((llhttp_t*)conn->http_parser, HTTP_REQUEST, conn->parser_settings);
    // 设置解析器数据指针 - 使用官方API
    conn->http_parser->data = conn;
    
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
    
    // 简化版本跳过TCP初始化
    
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
    
    memset(conn->request, 0, sizeof(uvhttp_request_t));
    
    conn->response = uvhttp_malloc(sizeof(uvhttp_response_t));
    if (!conn->response) {
        uvhttp_free(conn->request);
        uvhttp_free(conn->http_parser);
        uvhttp_free(conn->parser_settings);
        uvhttp_free(conn->read_buffer);
        uvhttp_free(conn);
        return NULL;
    }
    
    memset(conn->response, 0, sizeof(uvhttp_response_t));
    
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

int uvhttp_connection_start(uvhttp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    // 简化版本直接开始HTTP读取
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    return 0; // 成功
}

int uvhttp_connection_start_tls_handshake(uvhttp_connection_t* conn) {
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

void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    
    // 简化版本直接调用关闭回调
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





static void on_close(void* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle;
    if (conn) {
        if (conn->server) {
            conn->server->active_connections--;
        }
        uvhttp_connection_free(conn);
    }
}