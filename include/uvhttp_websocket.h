/* WebSocket模块 */

#ifndef UVHTTP_WEBSOCKET_H
#define UVHTTP_WEBSOCKET_H

/* 使用原生 WebSocket 实现 */
#include "uvhttp_websocket_native.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 便捷宏 */
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_ws_send_frame(ws, (const uint8_t*)(text), strlen(text), UVHTTP_WS_OPCODE_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_ws_send_frame(ws, (const uint8_t*)(data), (len), UVHTTP_WS_OPCODE_BINARY)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_H */