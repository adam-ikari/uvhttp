#ifndef UVHTTP_CONNECTION_H
#define UVHTTP_CONNECTION_H

#include <stdlib.h>
#include <stddef.h>
#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"
#include "uvhttp_mempool.h"
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
    /* 热路径字段（频繁访问）- 优化内存局部性 */
    uvhttp_connection_state_t state;        /* 4 字节 - 连接状态 */
    int parsing_complete;                    /* 4 字节 - 解析是否完成 */
    int keep_alive;                         /* 4 字节 - 是否保持连接 */
    int chunked_encoding;                    /* 4 字节 - 是否使用分块传输 */
    
    /* 指针字段（8字节对齐） */
    struct uvhttp_server* server;          /* 8 字节 */
    uvhttp_request_t* request;               /* 8 字节 */
    uvhttp_response_t* response;             /* 8 字节 */
    uvhttp_mempool_t* mempool;              /* 8 字节 - 内存池 */
    
    /* 网络连接（16字节对齐） */
    uv_tcp_t tcp_handle;                      /* 8 字节 */
    uv_idle_t idle_handle;                    /* 8 字节 */
    
    /* HTTP/1.1优化字段 */
    int current_header_is_important;         /* 4 字节 */
    int parsing_header_field;                 /* 4 字节 */
    int need_restart_read;                    /* 4 字节 */
    int tls_enabled;                          /* 4 字节 */
    
    /* 大块内存字段（8字节对齐） */
    size_t content_length;                   /* 8 字节 */
    size_t body_received;                    /* 8 字节 */
    size_t read_buffer_size;                  /* 8 字节 */
    size_t read_buffer_used;                  /* 8 字节 */
    size_t current_header_field_len;          /* 8 字节 */
    
    /* 其他指针和状态 */
    void* ssl;                               /* 8 字节 */
    char* read_buffer;                        /* 8 字节 */
    int last_error;                           /* 4 字节 */
    
    /* WebSocket相关 */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_connection;                      /* 8 字节 */
    int is_websocket;                         /* 4 字节 */
#endif
    
    /* 缓冲区字段（放在最后） */
    char current_header_field[UVHTTP_MAX_HEADER_NAME_SIZE];  /* 大块内存 */
};

// 连接管理函数
uvhttp_connection_t* uvhttp_connection_new(struct uvhttp_server* server);
void uvhttp_connection_free(uvhttp_connection_t* conn);
int uvhttp_connection_start(uvhttp_connection_t* conn);
void uvhttp_connection_close(uvhttp_connection_t* conn);
int uvhttp_connection_restart_read(uvhttp_connection_t* conn);
int uvhttp_connection_schedule_restart_read(uvhttp_connection_t* conn);

// TLS处理函数
int uvhttp_connection_start_tls_handshake(uvhttp_connection_t* conn);
int uvhttp_connection_tls_read(uvhttp_connection_t* conn);
int uvhttp_connection_tls_write(uvhttp_connection_t* conn, const void* data, size_t len);
int uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn);
void uvhttp_connection_tls_cleanup(uvhttp_connection_t* conn);

// 状态管理
void uvhttp_connection_set_state(uvhttp_connection_t* conn, uvhttp_connection_state_t state);
const char* uvhttp_connection_get_state_string(uvhttp_connection_state_t state);

// 连接池管理（内部使用）
void uvhttp_connection_pool_cleanup(struct uvhttp_server* server);

// WebSocket处理函数（内部）
#if UVHTTP_FEATURE_WEBSOCKET
int uvhttp_connection_handle_websocket_handshake(uvhttp_connection_t* conn, const char* ws_key);
void uvhttp_connection_switch_to_websocket(uvhttp_connection_t* conn);
void uvhttp_connection_websocket_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void uvhttp_connection_websocket_close(uvhttp_connection_t* conn);

/* WebSocket处理器查找函数 - 前向声明 */
uvhttp_ws_handler_t* uvhttp_server_find_ws_handler(struct uvhttp_server* server, const char* path);

/* WebSocket认证相关内部函数 */
typedef struct ws_route_entry ws_route_entry_t;
ws_route_entry_t* uvhttp_server_find_ws_route_entry(struct uvhttp_server* server, const char* path);
uvhttp_ws_auth_result_t uvhttp_server_ws_authenticate(
    struct uvhttp_server* server,
    const char* path,
    const char* client_ip,
    const char* token
);
#endif

#ifdef __cplusplus
}
#endif

#endif