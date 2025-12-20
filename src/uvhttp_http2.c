#include "uvhttp_http2.h"
#include "uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>

/* HTTP/2流结构 */
struct uvhttp_http2_stream {
    uint32_t stream_id;
    uvhttp_http2_stream_state_t state;
    uvhttp_http2_server_t* server;
    void* user_data;
};

/* HTTP/2服务器结构 */
struct uvhttp_http2_server {
    uv_tcp_t tcp_handle;
    uv_loop_t* loop;
    uvhttp_http2_stream_handler_t handler;
    void* user_data;
    uint32_t next_stream_id;
    uvhttp_http2_stream_t* streams[1024]; /* 简化：固定大小流表 */
    int stream_count;
};

/* 创建HTTP/2服务器 */
uvhttp_http2_server_t* uvhttp_http2_server_new(uv_loop_t* loop) {
    if (!loop) return NULL;
    
    uvhttp_http2_server_t* server = uvhttp_alloc(sizeof(uvhttp_http2_server_t));
    if (!server) return NULL;
    
    memset(server, 0, sizeof(uvhttp_http2_server_t));
    
    server->loop = loop;
    server->next_stream_id = 1;
    
    if (uv_tcp_init(loop, &server->tcp_handle) != 0) {
        uvhttp_dealloc(server);
        return NULL;
    }
    
    server->tcp_handle.data = server;
    
    return server;
}

void uvhttp_http2_server_free(uvhttp_http2_server_t* server) {
    if (!server) return;
    
    /* 清理所有流 */
    for (int i = 0; i < server->stream_count; i++) {
        if (server->streams[i]) {
            uvhttp_dealloc(server->streams[i]);
        }
    }
    
    uvhttp_dealloc(server);
}

/* 监听端口 */
int uvhttp_http2_server_listen(uvhttp_http2_server_t* server, 
                             const char* host, 
                             int port) {
    if (!server) return -1;
    
    struct sockaddr_in addr;
    uv_ip4_addr(host, port, &addr);
    
    int ret = uv_tcp_bind(&server->tcp_handle, (const struct sockaddr*)&addr, 0);
    if (ret != 0) return ret;
    
    ret = uv_listen((uv_stream_t*)&server->tcp_handle, 128, NULL);
    if (ret != 0) return ret;
    
    return 0;
}

/* 设置流处理器 */
void uvhttp_http2_server_set_handler(uvhttp_http2_server_t* server,
                                     uvhttp_http2_stream_handler_t handler,
                                     void* user_data) {
    if (!server) return;
    
    server->handler = handler;
    server->user_data = user_data;
}

/* 创建新流 */
static uvhttp_http2_stream_t* create_stream(uvhttp_http2_server_t* server) {
    if (!server || server->stream_count >= 1024) return NULL;
    
    uvhttp_http2_stream_t* stream = uvhttp_alloc(sizeof(uvhttp_http2_stream_t));
    if (!stream) return NULL;
    
    memset(stream, 0, sizeof(uvhttp_http2_stream_t));
    
    stream->stream_id = server->next_stream_id;
    stream->server = server;
    stream->state = UVHTTP_HTTP2_STREAM_IDLE;
    
    server->streams[server->stream_count++] = stream;
    server->next_stream_id += 2; /* HTTP/2客户端流ID为奇数，服务器为偶数 */
    
    return stream;
}

/* 发送HTTP/2头部 */
int uvhttp_http2_stream_send_headers(uvhttp_http2_stream_t* stream,
                                      const char* name,
                                      const char* value) {
    if (!stream || !name || !value) return -1;
    
    /* 简化实现：实际应该构建HPACK帧 */
    /* 这里只是标记头部已发送 */
    
    return 0;
}

/* 发送HTTP/2数据 */
int uvhttp_http2_stream_send_data(uvhttp_http2_stream_t* stream,
                                   const char* data,
                                   size_t length) {
    if (!stream || !data || length == 0) return -1;
    
    /* 简化实现：实际应该构建DATA帧 */
    /* 这里只是标记数据已发送 */
    
    return 0;
}

/* 关闭流 */
void uvhttp_http2_stream_close(uvhttp_http2_stream_t* stream, int error_code) {
    if (!stream) return;
    
    stream->state = UVHTTP_HTTP2_STREAM_CLOSED;
    
    /* 从服务器流表中移除 */
    uvhttp_http2_server_t* server = stream->server;
    if (server) {
        for (int i = 0; i < server->stream_count; i++) {
            if (server->streams[i] == stream) {
                server->streams[i] = server->streams[--server->stream_count];
                break;
            }
        }
    }
    
    uvhttp_dealloc(stream);
}