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
    struct uvhttp_server* server;
    uvhttp_request_t* request;
    uvhttp_response_t* response;
    
    // 网络连接
    uv_tcp_t tcp_handle;
    
    // 用于keep-alive连接重用的idle句柄
    uv_idle_t idle_handle;
    
    // TLS相关 - 简化版本
    void* ssl;
    int tls_enabled;
    
    // WebSocket相关
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_connection;  // uvhttp_ws_connection_t*
    int is_websocket;
#endif

    // 连接状态
    uvhttp_connection_state_t state;
    
    // 读写缓冲区
    char* read_buffer;
    size_t read_buffer_size;
    size_t read_buffer_used;
    
    // HTTP解析器在request中管理
    
    // HTTP/1.1优化字段
    int current_header_is_important;    // 当前头部是否为关键字段
    int keep_alive;                     // 是否保持连接
    int chunked_encoding;               // 是否使用分块传输
    size_t content_length;              // 内容长度
    size_t body_received;               // 已接收的body长度
    int parsing_complete;               // 解析是否完成
    
    // HTTP解析状态（替代全局变量）
    char current_header_field[UVHTTP_MAX_HEADER_NAME_SIZE];  // 当前解析的头部字段名
    size_t current_header_field_len;     // 当前头部字段名长度
    int parsing_header_field;            // 是否正在解析头部字段名
    
    // 用于keep-alive连接重用
    int need_restart_read;              // 标记是否需要重启读取
    
    // 内存池（用于请求/响应的临时内存分配）
    uvhttp_mempool_t* mempool;
    
    // 错误处理
    int last_error;
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