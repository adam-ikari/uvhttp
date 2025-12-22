#ifndef UVHTTP_CONNECTION_H
#define UVHTTP_CONNECTION_H

#include <stdlib.h>
#include <stddef.h>
#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"
#include "llhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UVHTTP_CONN_STATE_NEW,
    UVHTTP_CONN_STATE_TLS_HANDSHAKE,
    UVHTTP_CONN_STATE_HTTP_READING,
    UVHTTP_CONN_STATE_HTTP_PROCESSING,
    UVHTTP_CONN_STATE_HTTP_WRITING,
    UVHTTP_CONN_STATE_CLOSING
} uvhttp_connection_state_t;

// 前向声明
typedef struct uvhttp_connection uvhttp_connection_t;

struct uvhttp_connection {
    struct uvhttp_server* server;
    uvhttp_request_t* request;
    uvhttp_response_t* response;
    
    // 网络连接
    uv_tcp_t tcp_handle;
    
    // TLS相关 - 简化版本
    void* ssl;
    int tls_enabled;
    
    // 连接状态
    uvhttp_connection_state_t state;
    
    // 读写缓冲区
    char* read_buffer;
    size_t read_buffer_size;
    size_t read_buffer_used;
    
    // HTTP解析器
    llhttp_t* http_parser;
    llhttp_settings_t* parser_settings;
    
    // HTTP/1.1优化字段
    int current_header_is_important;    // 当前头部是否为关键字段
    int keep_alive;                     // 是否保持连接
    int chunked_encoding;               // 是否使用分块传输
    size_t content_length;              // 内容长度
    size_t body_received;               // 已接收的body长度
    int parsing_complete;               // 解析是否完成
    
    // 解析状态
    int parsing_complete;
    size_t content_length;
    size_t body_received;
    
    // 错误处理
    int last_error;
};

// 连接管理函数
uvhttp_connection_t* uvhttp_connection_new(struct uvhttp_server* server);
void uvhttp_connection_free(uvhttp_connection_t* conn);
int uvhttp_connection_start(uvhttp_connection_t* conn);
void uvhttp_connection_close(uvhttp_connection_t* conn);

// TLS处理函数
int uvhttp_connection_start_tls_handshake(uvhttp_connection_t* conn);
int uvhttp_connection_tls_read(uvhttp_connection_t* conn);
int uvhttp_connection_tls_write(uvhttp_connection_t* conn, const void* data, size_t len);

// 状态管理
void uvhttp_connection_set_state(uvhttp_connection_t* conn, uvhttp_connection_state_t state);
const char* uvhttp_connection_get_state_string(uvhttp_connection_state_t state);

#ifdef __cplusplus
}
#endif

#endif