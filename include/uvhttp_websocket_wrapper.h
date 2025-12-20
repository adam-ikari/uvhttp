/* WebSocket 包装层 - 隔离 libwebsockets 定义 */

#ifndef UVHTTP_WEBSOCKET_WRAPPER_H
#define UVHTTP_WEBSOCKET_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明，避免包含其他头文件 */
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

/* libwebsockets 类型映射 - 避免直接包含 */
struct lws;
struct lws_context;
enum lws_callback_reasons;

/* WebSocket 错误码 */
typedef enum {
    UVHTTP_WEBSOCKET_ERROR_NONE = 0,
    UVHTTP_WEBSOCKET_ERROR_INVALID_PARAM = -1,
    UVHTTP_WEBSOCKET_ERROR_MEMORY = -2,
    UVHTTP_WEBSOCKET_ERROR_TLS_CONFIG = -3,
    UVHTTP_WEBSOCKET_ERROR_CONNECTION = -4,
    UVHTTP_WEBSOCKET_ERROR_NOT_CONNECTED = -5,
    UVHTTP_WEBSOCKET_ERROR_CERT_VERIFY = -6,
    UVHTTP_WEBSOCKET_ERROR_CERT_EXPIRED = -7,
    UVHTTP_WEBSOCKET_ERROR_CERT_NOT_YET_VALID = -8,
    UVHTTP_WEBSOCKET_ERROR_PROTOCOL = -9
} uvhttp_websocket_error_t;

/* WebSocket 消息类型 - 映射到 libwebsockets 但不直接使用其常量 */
typedef enum {
    UVHTTP_WEBSOCKET_TEXT = 0,     /* 对应 LWS_WRITE_TEXT */
    UVHTTP_WEBSOCKET_BINARY = 1,   /* 对应 LWS_WRITE_BINARY */
    UVHTTP_WEBSOCKET_CONTINUATION = 2, /* 对应 LWS_WRITE_CONTINUATION */
    UVHTTP_WEBSOCKET_PING = 3,     /* 对应 LWS_WRITE_PING */
    UVHTTP_WEBSOCKET_PONG = 4,     /* 对应 LWS_WRITE_PONG */
    UVHTTP_WEBSOCKET_CLOSE = 5     /* 通过 lws_close_reason() 处理 */
} uvhttp_websocket_type_t;

/* WebSocket 消息结构 */
typedef struct {
    uvhttp_websocket_type_t type;
    const char* data;
    size_t length;
} uvhttp_websocket_message_t;

/* WebSocket 连接结构 - 不暴露 libwebsockets 内部 */
typedef struct uvhttp_websocket uvhttp_websocket_t;

/* WebSocket 处理器 */
typedef void (*uvhttp_websocket_handler_t)(uvhttp_websocket_t* ws, 
                                               const uvhttp_websocket_message_t* msg, 
                                               void* user_data);

/* mTLS 配置结构 */
typedef struct {
    const char* server_cert_path;
    const char* server_key_path;
    const char* ca_cert_path;
    const char* client_cert_path;
    const char* client_key_path;
    int require_client_cert;
    int verify_depth;
    const char* cipher_list;
} uvhttp_websocket_mtls_config_t;

/* WebSocket 创建选项 */
typedef struct {
    uvhttp_websocket_mtls_config_t* mtls_config;
    int enable_tls;
    const char* tls_cipher_suites;
    size_t max_frame_size;
    int ping_interval;
    int enable_compression;
} uvhttp_websocket_options_t;

/* WebSocket API 函数 */
uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, 
                                          uvhttp_response_t* response);
void uvhttp_websocket_free(uvhttp_websocket_t* ws);

uvhttp_websocket_error_t uvhttp_websocket_send(uvhttp_websocket_t* ws, 
                                               const char* data, 
                                               size_t length, 
                                               uvhttp_websocket_type_t type);

uvhttp_websocket_error_t uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, 
                                                      uvhttp_websocket_handler_t handler, 
                                                      void* user_data);

uvhttp_websocket_error_t uvhttp_websocket_close(uvhttp_websocket_t* ws, 
                                                int code, 
                                                const char* reason);

/* mTLS 支持 */
uvhttp_websocket_error_t uvhttp_websocket_enable_mtls(uvhttp_websocket_t* ws, 
                                                      const uvhttp_websocket_mtls_config_t* config);
void uvhttp_websocket_get_client_cert_info(uvhttp_websocket_t* ws, 
                                          char* subject, 
                                          size_t subject_len,
                                          char* issuer, 
                                          size_t issuer_len);

/* TLS 连接信息 */
const char* uvhttp_websocket_get_peer_cert(uvhttp_websocket_t* ws);
uvhttp_websocket_error_t uvhttp_websocket_verify_peer_cert(uvhttp_websocket_t* ws);
uvhttp_websocket_error_t uvhttp_websocket_verify_peer_cert_enhanced(uvhttp_websocket_t* ws);

/* 全局清理函数 */
void uvhttp_websocket_cleanup_global(void);

/* 便捷宏 */
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_websocket_send(ws, text, strlen(text), UVHTTP_WEBSOCKET_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_websocket_send(ws, data, len, UVHTTP_WEBSOCKET_BINARY)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_WRAPPER_H */