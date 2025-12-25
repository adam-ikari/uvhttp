/* WebSocket模块 */

#ifndef UVHTTP_WEBSOCKET_H
#define UVHTTP_WEBSOCKET_H

/* 直接使用包装层 */
#include "uvhttp_websocket_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 便捷宏 */
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_websocket_send(ws, text, strlen(text), UVHTTP_WEBSOCKET_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_websocket_send(ws, data, len, UVHTTP_WEBSOCKET_BINARY)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_H */