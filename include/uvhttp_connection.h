#ifndef UVHTTP_CONNECTION_H
#define UVHTTP_CONNECTION_H

#include "uvhttp_common.h"
#include "uvhttp_platform.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#include "llhttp.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明（避免循环引用）
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;

typedef enum {
    UVHTTP_CONN_STATE_NEW,
    UVHTTP_CONN_STATE_TLS_HANDSHAKE,
    UVHTTP_CONN_STATE_HTTP_READING,
    UVHTTP_CONN_STATE_HTTP_PROCESSING,
    UVHTTP_CONN_STATE_HTTP_WRITING,
    UVHTTP_CONN_STATE_CLOSING
} uvhttp_connection_state_t;

struct uvhttp_connection {
    /* ========== 缓存行1（0-63字节）：热路径字段 - 最频繁访问 ========== */
    /* 在 on_read、on_write、连接管理中频繁访问 */
    uvhttp_connection_state_t state; /* 4 字节 - 连接状态 */
    int parsing_complete;            /* 4 字节 - 解析是否完成 */
    int keepalive;                   /* 4 字节 - 是否保持连接 */
    int chunked_encoding;            /* 4 字节 - 是否使用分块传输 */
    int close_pending;               /* 4 字节 - 待关闭的 handle 计数 */
    int need_restart_read;           /* 4 字节 - 是否需要重启读取 */
    int tls_enabled;                 /* 4 字节 - TLS 是否启用 */
    int _padding1;                   /* 4 字节 - 填充到32字节 */
    size_t body_received;            /* 8 字节 - 已接收的 body 长度 */
    size_t content_length;           /* 8 字节 - Content-Length */
    size_t read_buffer_used;         /* 8 字节 - 读缓冲区已使用 */
    size_t read_buffer_size;         /* 8 字节 - 读缓冲区大小 */
    /* 缓存行1总计：56字节（剩余8字节填充） */

    /* ========== 缓存行2（64-127字节）：指针字段 - 次频繁访问 ========== */
    /* 在连接创建、销毁、请求处理中频繁访问 */
    struct uvhttp_server* server; /* 8 字节 - 所属服务器 */
    uvhttp_request_t* request;    /* 8 字节 - 请求对象 */
    uvhttp_response_t* response;  /* 8 字节 - 响应对象 */
    void* ssl;                    /* 8 字节 - SSL/TLS 上下文 */
    char* read_buffer;            /* 8 字节 - 读缓冲区指针 */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_connection; /* 8 字节 - WebSocket 连接 */
#endif
    int current_header_is_important; /* 4 字节 - 当前头部是否重要 */
    int parsing_header_field; /* 4 字节 - 是否正在解析头部字段 */
    int last_error;           /* 4 字节 - 最后的错误码 */
#if UVHTTP_FEATURE_WEBSOCKET
    int is_websocket; /* 4 字节 - 是否为 WebSocket 连接 */
#endif
    int _padding2; /* 4 字节 - 填充到64字节 */
    /* 缓存行2总计：约64字节（取决于 WebSocket 是否启用） */

    /* ========== 缓存行3（128-191字节）：libuv 句柄 ========== */
    /* libuv 内部结构体，大小固定 */
    uv_tcp_t tcp_handle;      /* 约40-48字节 */
    uv_idle_t idle_handle;    /* 约24-32字节 */
    uv_timer_t timeout_timer; /* 约24-32字节 */
    /* 缓存行3总计：约88-112字节 */

    /* ========== 缓存行4（192-255字节）：HTTP 解析状态 ========== */
    /* 在 HTTP 解析过程中频繁访问 */
    size_t current_header_field_len; /* 8 字节 - 当前头部字段长度 */
    int _padding3[14];               /* 56字节 - 填充到64字节 */
    /* 缓存行4总计：64字节 */

    /* ========== 缓存行5+（256+字节）：大块缓冲区 ========== */
    /* 放在最后，避免影响热路径字段的缓存局部性 */
    char current_header_field[UVHTTP_MAX_HEADER_NAME_SIZE]; /* 大块内存 */
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, server, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, request, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, response, UVHTTP_POINTER_ALIGNMENT);

/* 验证size_t对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, content_length,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, body_received,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, read_buffer_size,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* 验证大型缓冲区在结构体末尾 */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_connection_t, current_header_field) >= 64,
                     "current_header_field should be after first 64 bytes");

// 连接管理函数
/**
 * @brief 创建新的连接对象
 * @param server 服务器对象
 * @param conn 输出参数，用于接收连接指针
 * @return UVHTTP_OK 成功，其他值表示失败
 * @note 成功时，*conn 被设置为有效的连接对象，必须使用 uvhttp_connection_free
 * 释放
 * @note 失败时，*conn 被设置为 NULL
 */
uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                     uvhttp_connection_t** conn);
void uvhttp_connection_free(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn);
void uvhttp_connection_close(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_restart_read(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_schedule_restart_read(
    uvhttp_connection_t* conn);

// TLS处理函数
uvhttp_error_t uvhttp_connection_start_tls_handshake(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_tls_read(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_tls_write(uvhttp_connection_t* conn,
                                           const void* data, size_t len);
uvhttp_error_t uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn);
void uvhttp_connection_tls_cleanup(uvhttp_connection_t* conn);

// 状态管理
void uvhttp_connection_set_state(uvhttp_connection_t* conn,
                                 uvhttp_connection_state_t state);
const char* uvhttp_connection_get_state_string(uvhttp_connection_state_t state);

// WebSocket处理函数（内部）
#if UVHTTP_FEATURE_WEBSOCKET
uvhttp_error_t uvhttp_connection_handle_websocket_handshake(
    uvhttp_connection_t* conn, const char* ws_key);
void uvhttp_connection_switch_to_websocket(uvhttp_connection_t* conn);
void uvhttp_connection_websocket_read(uv_stream_t* stream, ssize_t nread,
                                      const uv_buf_t* buf);
void uvhttp_connection_websocket_close(uvhttp_connection_t* conn);

/**
 * @brief 启动连接超时定时器
 *
 * 为连接启动超时定时器，使用配置中的默认超时时间。
 * 如果连接在超时时间内没有活动，将自动关闭连接。
 *
 * @param conn 连接对象
 * @return int 成功返回 UVHTTP_OK，失败返回负数错误码
 *
 * @note 超时时间从 conn->server->config->connection_timeout 读取，
 *       如果 config 为 NULL，则使用 UVHTTP_CONNECTION_TIMEOUT_DEFAULT
 * @note 此函数会停止并重启现有的定时器（如果有）
 */
uvhttp_error_t uvhttp_connection_start_timeout(uvhttp_connection_t* conn);

/**
 * @brief 启动连接超时定时器（自定义超时时间）
 *
 * 为连接启动超时定时器，使用指定的超时时间。
 * 如果连接在超时时间内没有活动，将自动关闭连接。
 *
 * @param conn 连接对象
 * @param timeout_seconds 超时时间（秒），范围：5-300
 * @return uvhttp_error_t 成功返回 UVHTTP_OK，失败返回负数错误码
 *
 * @note 超时时间必须在 UVHTTP_CONNECTION_TIMEOUT_MIN 和
 *       UVHTTP_CONNECTION_TIMEOUT_MAX 之间
 * @note 此函数会停止并重启现有的定时器（如果有）
 * @note 如果超时时间过大导致整数溢出，返回 UVHTTP_ERROR_INVALID_PARAM
 */
uvhttp_error_t uvhttp_connection_start_timeout_custom(uvhttp_connection_t* conn,
                                                      int timeout_seconds);

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_CONNECTION_H */

#endif