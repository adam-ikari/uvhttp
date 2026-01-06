/*
 * uvhttp WebSocket Native Implementation
 * 完全自主实现的 WebSocket 协议支持，基于 RFC 6455
 */

#if UVHTTP_FEATURE_WEBSOCKET

#ifndef UVHTTP_WEBSOCKET_NATIVE_H
#define UVHTTP_WEBSOCKET_NATIVE_H

#include <stddef.h>
#include <stdint.h>
#include "uvhttp_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket 操作码 (RFC 6455) */
typedef enum {
    UVHTTP_WS_OPCODE_CONTINUATION = 0x0,
    UVHTTP_WS_OPCODE_TEXT = 0x1,
    UVHTTP_WS_OPCODE_BINARY = 0x2,
    UVHTTP_WS_OPCODE_CLOSE = 0x8,
    UVHTTP_WS_OPCODE_PING = 0x9,
    UVHTTP_WS_OPCODE_PONG = 0xA
} uvhttp_ws_opcode_t;

/* WebSocket 状态 */
typedef enum {
    UVHTTP_WS_STATE_CONNECTING = 0,
    UVHTTP_WS_STATE_OPEN = 1,
    UVHTTP_WS_STATE_CLOSING = 2,
    UVHTTP_WS_STATE_CLOSED = 3
} uvhttp_ws_state_t;

/* WebSocket 帧头 (RFC 6455) */
typedef struct {
    uint8_t fin : 1;
    uint8_t rsv1 : 1;
    uint8_t rsv2 : 1;
    uint8_t rsv3 : 1;
    uint8_t opcode : 4;
    uint8_t mask : 1;
    uint8_t payload_len : 7;
} uvhttp_ws_frame_header_t;

/* WebSocket 帧结构 */
typedef struct {
    uvhttp_ws_frame_header_t header;
    uint64_t payload_length;
    uint8_t masking_key[4];
    uint8_t* payload;
    size_t payload_size;
} uvhttp_ws_frame_t;

/* WebSocket 配置 */
typedef struct {
    int max_frame_size;
    int max_message_size;
    int ping_interval;
    int ping_timeout;
    int enable_compression;
} uvhttp_ws_config_t;

/* 前向声明 */
struct uvhttp_ws_connection;

/* 回调函数类型 */
typedef void (*uvhttp_ws_on_message_callback)(struct uvhttp_ws_connection* conn, 
                                               const uint8_t* data, 
                                               size_t len, 
                                               uvhttp_ws_opcode_t opcode);
typedef void (*uvhttp_ws_on_close_callback)(struct uvhttp_ws_connection* conn, 
                                              int code, 
                                              const char* reason);
typedef void (*uvhttp_ws_on_error_callback)(struct uvhttp_ws_connection* conn, 
                                              int error_code, 
                                              const char* error_msg);

/* WebSocket 连接 */
typedef struct uvhttp_ws_connection {
    int fd;
    uvhttp_ws_state_t state;
    uvhttp_ws_config_t config;
    mbedtls_ssl_context* ssl;
    int is_server;
    
    /* 客户端握手时保存的原始 key（用于验证 accept） */
    char client_key[64];
    
    /* 接收缓冲区 */
    uint8_t* recv_buffer;
    size_t recv_buffer_size;
    size_t recv_buffer_pos;
    
    /* 发送缓冲区 */
    uint8_t* send_buffer;
    size_t send_buffer_size;
    
    /* 分片重组 */
    uint8_t* fragmented_message;
    size_t fragmented_size;
    size_t fragmented_capacity;
    uvhttp_ws_opcode_t fragmented_opcode;
    
    /* 回调函数 */
    uvhttp_ws_on_message_callback on_message;
    uvhttp_ws_on_close_callback on_close;
    uvhttp_ws_on_error_callback on_error;
    void* user_data;
    
    /* 统计信息 */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t frames_sent;
    uint64_t frames_received;
} uvhttp_ws_connection_t;

/* WebSocket API */

/**
 * 创建 WebSocket 连接
 */
struct uvhttp_ws_connection* uvhttp_ws_connection_create(int fd, 
                                                     mbedtls_ssl_context* ssl, 
                                                     int is_server);

/**
 * 释放 WebSocket 连接
 */
void uvhttp_ws_connection_free(struct uvhttp_ws_connection* conn);

/**
 * 执行 WebSocket 握手 (服务器端)
 */
int uvhttp_ws_handshake_server(struct uvhttp_ws_connection* conn, 
                                const char* request, 
                                size_t request_len,
                                char* response, 
                                size_t* response_len);

/**
 * 执行 WebSocket 握手 (客户端)
 */
int uvhttp_ws_handshake_client(struct uvhttp_ws_connection* conn, 
                                const char* host, 
                                const char* path,
                                char* request, 
                                size_t* request_len);

/**
 * 验证握手响应 (客户端)
 */
int uvhttp_ws_verify_handshake_response(struct uvhttp_ws_connection* conn, 
                                         const char* response, 
                                         size_t response_len);

/**
 * 接收 WebSocket 帧
 */
int uvhttp_ws_recv_frame(struct uvhttp_ws_connection* conn, 
                          uvhttp_ws_frame_t* frame);

/**
 * 发送 WebSocket 帧
 */
int uvhttp_ws_send_frame(struct uvhttp_ws_connection* conn, 
                          const uint8_t* data, 
                          size_t len, 
                          uvhttp_ws_opcode_t opcode);

/**
 * 发送文本消息
 */
int uvhttp_ws_send_text(struct uvhttp_ws_connection* conn, 
                         const char* text, 
                         size_t len);

/**
 * 发送二进制消息
 */
int uvhttp_ws_send_binary(struct uvhttp_ws_connection* conn, 
                           const uint8_t* data, 
                           size_t len);

/**
 * 发送 Ping
 */
int uvhttp_ws_send_ping(struct uvhttp_ws_connection* conn, 
                        const uint8_t* data, 
                        size_t len);

/**
 * 发送 Pong
 */
int uvhttp_ws_send_pong(struct uvhttp_ws_connection* conn, 
                        const uint8_t* data, 
                        size_t len);

/**
 * 关闭连接
 */
int uvhttp_ws_close(struct uvhttp_ws_connection* conn, 
                    int code, 
                    const char* reason);

/**
 * 处理接收到的数据
 */
int uvhttp_ws_process_data(struct uvhttp_ws_connection* conn, 
                            const uint8_t* data, 
                            size_t len);

/**
 * 设置回调函数
 */
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection* conn,
                              uvhttp_ws_on_message_callback on_message,
                              uvhttp_ws_on_close_callback on_close,
                              uvhttp_ws_on_error_callback on_error);

/* 帧处理函数 */

/**
 * 解析帧头
 */
int uvhttp_ws_parse_frame_header(const uint8_t* data, 
                                  size_t len, 
                                  uvhttp_ws_frame_header_t* header,
                                  size_t* header_size);

/**
 * 构建帧
 */
int uvhttp_ws_build_frame(uint8_t* buffer, 
                          size_t buffer_size,
                          const uint8_t* payload, 
                          size_t payload_len,
                          uvhttp_ws_opcode_t opcode,
                          int mask,
                          int fin);

/**
 * 应用掩码
 */
void uvhttp_ws_apply_mask(uint8_t* data, 
                          size_t len, 
                          const uint8_t* masking_key);

/**
 * 生成 Sec-WebSocket-Accept
 */
int uvhttp_ws_generate_accept(const char* key, 
                              char* accept, 
                              size_t accept_len);

/**
 * 验证 Sec-WebSocket-Accept
 */
int uvhttp_ws_verify_accept(const char* key, 
                            const char* accept);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_NATIVE_H */
#endif /* UVHTTP_FEATURE_WEBSOCKET */
