#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <uv.h>

// 简单的HTTP服务器用于压力测试

typedef struct {
    uv_tcp_t handle;
    uv_write_t write_req;
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    int is_header_complete;
} client_t;

static uv_tcp_t server;
static uv_loop_t* loop;
static int request_count = 0;

// 分配缓冲区
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    client_t* client = (client_t*)handle->data;
    
    if (client->buffer_pos < client->buffer_size) {
        buf->base = client->buffer + client->buffer_pos;
        buf->len = client->buffer_size - client->buffer_pos;
    } else {
        // 扩展缓冲区
        size_t new_size = client->buffer_size * 2;
        char* new_buffer = realloc(client->buffer, new_size);
        if (new_buffer) {
            client->buffer = new_buffer;
            client->buffer_size = new_size;
            buf->base = client->buffer + client->buffer_pos;
            buf->len = client->buffer_size - client->buffer_pos;
        } else {
            buf->base = NULL;
            buf->len = 0;
        }
    }
}

// 写入回调
static void on_write(uv_write_t* req, int status) {
    client_t* client = (client_t*)req->data;
    uv_close((uv_handle_t*)&client->handle, NULL);
}

// 发送HTTP响应
static void send_response(client_t* client) {
    char response[4096];
    int response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!");
    
    uv_buf_t buf = uv_buf_init(response, response_len);
    client->write_req.data = client;
    
    uv_write(&client->write_req, (uv_stream_t*)&client->handle, &buf, 1, on_write);
}

// 读取客户端数据
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_t* client = (client_t*)stream->data;
    
    if (nread > 0) {
        client->buffer_pos += nread;
        
        // 检查HTTP头是否完整
        if (!client->is_header_complete) {
            char* header_end = strstr(client->buffer, "\r\n\r\n");
            if (header_end) {
                client->is_header_complete = 1;
                request_count++;
                
                // 立即发送响应
                send_response(client);
            }
        }
    } else if (nread < 0) {
        // 错误或连接关闭
        uv_close((uv_handle_t*)&client->handle, NULL);
    }
}

// 新连接回调
static void on_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        fprintf(stderr, "连接错误: %s\n", uv_strerror(status));
        return;
    }
    
    client_t* client = (client_t*)malloc(sizeof(client_t));
    memset(client, 0, sizeof(client_t));
    
    int result = uv_tcp_init(loop, &client->handle);
    if (result < 0) {
        fprintf(stderr, "初始化客户端失败: %s\n", uv_strerror(result));
        free(client);
        return;
    }
    
    client->handle.data = client;
    client->buffer_size = 4096;
    client->buffer = (char*)malloc(client->buffer_size);
    
    if (!client->buffer) {
        uv_close((uv_handle_t*)&client->handle, NULL);
        free(client);
        return;
    }
    
    result = uv_accept(server, (uv_stream_t*)&client->handle);
    if (result < 0) {
        uv_close((uv_handle_t*)&client->handle, NULL);
        free(client->buffer);
        free(client);
        return;
    }
    
    // 开始读取数据
    uv_read_start((uv_stream_t*)&client->handle, alloc_buffer, on_read);
}

// 关闭回调
static void on_close(uv_handle_t* handle) {
    client_t* client = (client_t*)handle->data;
    if (client) {
        if (client->buffer) free(client->buffer);
        free(client);
    }
}

// 定时器用于统计
static void on_stats_timer(uv_timer_t* handle) {
    printf("\r服务器已处理 %d 个请求", request_count);
    fflush(stdout);
}

int main() {
    printf("启动UVHTTP压力测试服务器...\n");
    
    loop = uv_default_loop();
    
    // 初始化TCP服务器
    int result = uv_tcp_init(loop, &server);
    if (result < 0) {
        fprintf(stderr, "初始化TCP服务器失败: %s\n", uv_strerror(result));
        return 1;
    }
    
    // 绑定地址
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 8080, &addr);
    result = uv_tcp_bind(&server, (struct sockaddr*)&addr, 0);
    if (result < 0) {
        fprintf(stderr, "绑定地址失败: %s\n", uv_strerror(result));
        return 1;
    }
    
    // 开始监听
    result = uv_listen((uv_stream_t*)&server, 128, on_connection);
    if (result < 0) {
        fprintf(stderr, "监听失败: %s\n", uv_strerror(result));
        return 1;
    }
    
    printf("服务器启动在 http://0.0.0.0:8080\n");
    printf("按 Ctrl+C 停止服务器\n");
    
    // 设置统计定时器
    uv_timer_t stats_timer;
    uv_timer_init(loop, &stats_timer);
    uv_timer_start(&stats_timer, on_stats_timer, 1000, 1000);
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    printf("\n服务器已停止\n");
    
    // 清理
    uv_close((uv_handle_t*)&server, NULL);
    uv_loop_close(loop);
    
    return 0;
}