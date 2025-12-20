/* WebSocket模块 */

#ifndef UVHTTP_WEBSOCKET_H
#define UVHTTP_WEBSOCKET_H

#include "uvhttp.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket消息类型 */
typedef enum {
    UVHTTP_WEBSOCKET_TEXT,
    UVHTTP_WEBSOCKET_BINARY,
    UVHTTP_WEBSOCKET_PING,
    UVHTTP_WEBSOCKET_PONG,
    UVHTTP_WEBSOCKET_CLOSE
} uvhttp_websocket_type_t;

/* WebSocket消息 */
typedef struct {
    uvhttp_websocket_type_t type;
    const char* data;
    size_t length;
} uvhttp_websocket_message_t;

/* WebSocket连接 */
typedef struct uvhttp_websocket uvhttp_websocket_t;

/* WebSocket处理器 */
typedef void (*uvhttp_websocket_handler_t)(uvhttp_websocket_t* ws, 
                                               const uvhttp_websocket_message_t* msg, 
                                               void* user_data);

/* WebSocket操作 */
uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, 
                                          uvhttp_response_t* response);
void uvhttp_websocket_free(uvhttp_websocket_t* ws);

/* 消息发送 */
int uvhttp_websocket_send(uvhttp_websocket_t* ws, 
                          const char* data, 
                          size_t length, 
                          uvhttp_websocket_type_t type);

/* 连接管理 */
void uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, 
                                 uvhttp_websocket_handler_t handler, 
                                 void* user_data);
void uvhttp_websocket_close(uvhttp_websocket_t* ws, int code, const char* reason);

/* 便捷宏 */
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_websocket_send(ws, text, strlen(text), UVHTTP_WEBSOCKET_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_websocket_send(ws, data, len, UVHTTP_WEBSOCKET_BINARY)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_H */