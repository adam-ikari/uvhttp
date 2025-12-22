/*
 * UVHTTP Hello World 示例 - 独立版本
 * 这个示例展示了如何使用UVHTTP创建一个简单的HTTP服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

/* 简单的HTTP响应 */
static const char* HTTP_RESPONSE = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Hello, World!";

/* 分配缓冲区 */
static void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

/* 读取客户端数据 */
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*)stream, NULL);
        free(buf->base);
        return;
    }
    
    /* 发送HTTP响应 */
    uv_write_t* write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t resbuf = uv_buf_init((char*)HTTP_RESPONSE, strlen(HTTP_RESPONSE));
    uv_write(write_req, stream, &resbuf, 1, NULL);
    
    free(buf->base);
}

/* 新连接回调 */
static void on_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        fprintf(stderr, "Connection error %s\n", uv_strerror(status));
        return;
    }
    
    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), client);
    
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

int main() {
    printf("Hello World HTTP Server starting...\n");
    
    uv_loop_t* loop = uv_default_loop();
    
    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 8080, &addr);
    
    if (uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0) != 0) {
        fprintf(stderr, "Failed to bind to port 8080\n");
        return 1;
    }
    
    if (uv_listen((uv_stream_t*)&server, 128, on_connection) != 0) {
        fprintf(stderr, "Failed to listen on port 8080\n");
        return 1;
    }
    
    printf("Server running on http://localhost:8080\n");
    printf("Press Ctrl+C to stop\n");
    
    return uv_run(loop, UV_RUN_DEFAULT);
}