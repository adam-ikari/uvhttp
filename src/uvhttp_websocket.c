#include "uvhttp_websocket.h"
#include "uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* WebSocket连接结构 */
struct uvhttp_websocket {
    uv_tcp_t* tcp_handle;
    uvhttp_websocket_handler_t handler;
    void* user_data;
    int is_connected;
    char* write_buffer;
    size_t write_buffer_size;
};

/* WebSocket魔术字符串 */
static const char WS_MAGIC_STRING[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/* 生成WebSocket接受密钥 */
static void generate_accept_key(const char* client_key, char* accept_key) {
    /* 简化实现，实际应该使用SHA1 */
    snprintf(accept_key, 29, "%s%s", client_key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
}

/* WebSocket握手 */
static int websocket_handshake(uvhttp_websocket_t* ws, uvhttp_request_t* request, 
                            uvhttp_response_t* response) {
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    const char* connection = uvhttp_request_get_header(request, "Connection");
    const char* ws_key = uvhttp_request_get_header(request, "Sec-WebSocket-Key");
    
    if (!upgrade || !connection || !ws_key) {
        return -1;
    }
    
    if (strcmp(upgrade, "websocket") != 0 || strstr(connection, "Upgrade") == NULL) {
        return -1;
    }
    
    char accept_key[64];
    generate_accept_key(ws_key, accept_key);
    
    uvhttp_response_set_status(response, 101);
    uvhttp_response_set_header(response, "Upgrade", "websocket");
    uvhttp_response_set_header(response, "Connection", "Upgrade");
    uvhttp_response_set_header(response, "Sec-WebSocket-Accept", accept_key);
    
    return 0;
}

/* 创建WebSocket连接 */
uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, 
                                          uvhttp_response_t* response) {
    if (!request || !response) return NULL;
    
    if (websocket_handshake(NULL, request, response) != 0) {
        return NULL;
    }
    
    uvhttp_websocket_t* ws = uvhttp_alloc(sizeof(uvhttp_websocket_t));
    if (!ws) return NULL;
    
    memset(ws, 0, sizeof(uvhttp_websocket_t));
    
    /* 分配写缓冲区 */
    ws->write_buffer = uvhttp_alloc(4096);
    if (!ws->write_buffer) {
        uvhttp_dealloc(ws);
        return NULL;
    }
    ws->write_buffer_size = 4096;
    
    return ws;
}

void uvhttp_websocket_free(uvhttp_websocket_t* ws) {
    if (!ws) return;
    
    if (ws->write_buffer) {
        uvhttp_dealloc(ws->write_buffer);
    }
    
    uvhttp_dealloc(ws);
}

/* 发送WebSocket消息 */
int uvhttp_websocket_send(uvhttp_websocket_t* ws, 
                          const char* data, 
                          size_t length, 
                          uvhttp_websocket_type_t type) {
    if (!ws || !data || !ws->is_connected) return -1;
    
    char frame[10];
    size_t frame_len = 2;
    
    /* 构建WebSocket帧头 */
    frame[0] = 0x80 | type; /* FIN + opcode */
    
    if (length < 126) {
        frame[1] = length;
    } else if (length < 65536) {
        frame[1] = 126;
        frame[2] = (length >> 8) & 0xFF;
        frame[3] = length & 0xFF;
        frame_len = 4;
    } else {
        /* 简化处理，不支持超大消息 */
        return -1;
    }
    
    /* 简化实现：直接发送 */
    /* 实际应该分片发送 */
    
    return 0;
}

/* 设置处理器 */
void uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, 
                                 uvhttp_websocket_handler_t handler, 
                                 void* user_data) {
    if (!ws) return;
    
    ws->handler = handler;
    ws->user_data = user_data;
    ws->is_connected = 1;
}

/* 关闭WebSocket连接 */
void uvhttp_websocket_close(uvhttp_websocket_t* ws, int code, const char* reason) {
    if (!ws) return;
    
    ws->is_connected = 0;
    
    /* 发送关闭帧 */
    char close_frame[8];
    close_frame[0] = 0x88; /* FIN + CLOSE */
    close_frame[1] = 2; /* 长度 */
    close_frame[2] = (code >> 8) & 0xFF;
    close_frame[3] = code & 0xFF;
    
    /* 简化实现 */
    ws->is_connected = 0;
}