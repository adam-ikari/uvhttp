/* WebSocket模块 - 使用包装层隔离 libwebsockets */

#ifndef UVHTTP_WEBSOCKET_H
#define UVHTTP_WEBSOCKET_H

/* 包含包装层头文件，避免直接包含 libwebsockets */
#include "uvhttp_websocket_wrapper.h"

/* 重新导出所有包装层的类型和函数，保持 API 兼容性 */

#ifdef __cplusplus
extern "C" {
#endif

/* 包装层已经定义了所有必要的类型和函数，这里不需要重复定义 */

/* 便捷宏 - 从包装层重新导出 */
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_websocket_send(ws, text, strlen(text), UVHTTP_WEBSOCKET_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_websocket_send(ws, data, len, UVHTTP_WEBSOCKET_BINARY)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_H */