/* HTTP/2模块 */

#ifndef UVHTTP_HTTP2_H
#define UVHTTP_HTTP2_H

#include "uvhttp.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HTTP/2流状态 */
typedef enum {
    UVHTTP_HTTP2_STREAM_IDLE,
    UVHTTP_HTTP2_STREAM_OPEN,
    UVHTTP_HTTP2_STREAM_RESERVED_LOCAL,
    UVHTTP_HTTP2_STREAM_RESERVED_REMOTE,
    UVHTTP_HTTP2_STREAM_CLOSED
} uvhttp_http2_stream_state_t;

/* HTTP/2流 */
typedef struct uvhttp_http2_stream uvhttp_http2_stream_t;

/* HTTP/2服务器 */
typedef struct uvhttp_http2_server uvhttp_http2_server_t;

/* HTTP/2流处理器 */
typedef void (*uvhttp_http2_stream_handler_t)(uvhttp_http2_stream_t* stream,
                                               uvhttp_request_t* request,
                                               uvhttp_response_t* response,
                                               void* user_data);

/* 创建HTTP/2服务器 */
uvhttp_http2_server_t* uvhttp_http2_server_new(uv_loop_t* loop);
void uvhttp_http2_server_free(uvhttp_http2_server_t* server);

/* 监听端口 */
int uvhttp_http2_server_listen(uvhttp_http2_server_t* server, 
                             const char* host, 
                             int port);

/* 设置流处理器 */
void uvhttp_http2_server_set_handler(uvhttp_http2_server_t* server,
                                     uvhttp_http2_stream_handler_t handler,
                                     void* user_data);

/* 流操作 */
int uvhttp_http2_stream_send_headers(uvhttp_http2_stream_t* stream,
                                      const char* name,
                                      const char* value);
int uvhttp_http2_stream_send_data(uvhttp_http2_stream_t* stream,
                                   const char* data,
                                   size_t length);
void uvhttp_http2_stream_close(uvhttp_http2_stream_t* stream, int error_code);

/* 便捷宏 */
#define uvhttp_http2_stream_send_json(stream, json) \
    uvhttp_http2_stream_send_headers(stream, "content-type", "application/json"); \
    uvhttp_http2_stream_send_data(stream, json, strlen(json))

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_HTTP2_H */