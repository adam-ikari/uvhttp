/*
 * 简化的性能测试 - 测试不限流的最大性能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#define TEST_PORT 9999
#define TEST_CONNECTIONS 50
#define TEST_REQUESTS_PER_CONNECTION 100

// 性能统计
typedef struct {
    double total_time;
    double requests_per_second;
    int total_requests;
    int successful_requests;
    int failed_requests;
} performance_stats_t;

// 简单的HTTP客户端
static int send_http_request(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }

    char request[] = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -1;
    }

    char response[4096];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);
    
    return bytes_received > 0 ? 0 : -1;
}

// 运行性能测试
static void run_performance_test(const char* host, int port, performance_stats_t* stats) {
    clock_t start_time = clock();
    
    stats->total_requests = TEST_CONNECTIONS * TEST_REQUESTS_PER_CONNECTION;
    stats->successful_requests = 0;
    stats->failed_requests = 0;

    printf("开始性能测试（无限流）...\n");
    printf("  连接数: %d\n", TEST_CONNECTIONS);
    printf("  每连接请求数: %d\n", TEST_REQUESTS_PER_CONNECTION);
    printf("  总请求数: %d\n\n", stats->total_requests);

    for (int conn = 0; conn < TEST_CONNECTIONS; conn++) {
        for (int req = 0; req < TEST_REQUESTS_PER_CONNECTION; req++) {
            if (send_http_request(host, port) == 0) {
                stats->successful_requests++;
            } else {
                stats->failed_requests++;
            }

            // 每100个请求显示进度
            if ((conn * TEST_REQUESTS_PER_CONNECTION + req + 1) % 100 == 0) {
                printf("已处理: %d/%d 请求\n",
                       conn * TEST_REQUESTS_PER_CONNECTION + req + 1,
                       stats->total_requests);
            }
        }
    }

    clock_t end_time = clock();
    stats->total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    stats->requests_per_second = stats->total_requests / stats->total_time;
}

// 打印性能统计
static void print_performance_stats(const performance_stats_t* stats) {
    printf("\n========== 性能测试结果（无限流） ==========\n");
    printf("总请求数: %d\n", stats->total_requests);
    printf("成功请求: %d\n", stats->successful_requests);
    printf("失败请求: %d\n", stats->failed_requests);
    printf("成功率: %.2f%%\n",
           (double)stats->successful_requests / stats->total_requests * 100);
    printf("\n");
    printf("总耗时: %.3f 秒\n", stats->total_time);
    printf("吞吐量: %.2f 请求/秒\n", stats->requests_per_second);
    printf("平均每请求耗时: %.6f 秒\n", stats->total_time / stats->total_requests);
    printf("=============================================\n");
}

// 服务器
static uvhttp_server_t* g_server = NULL;

int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "OK", 2);
    uvhttp_response_send(response);
    
    return 0;
}

void* server_thread(void* arg) {
    (void)arg;
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        printf("服务器创建失败\n");
        return NULL;
    }
    
    g_server = server;
    
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    uvhttp_router_add_route(router, "/", simple_handler);
    
    printf("服务器启动在端口 %d\n", TEST_PORT);
    uvhttp_server_listen(server, "0.0.0.0", TEST_PORT);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return NULL;
}

int main() {
    printf("========== UVHTTP 性能测试（无限流） ==========\n\n");
    
    // 启动服务器
    pthread_t server_thread_id;
    pthread_create(&server_thread_id, NULL, server_thread, NULL);
    
    // 等待服务器启动
    sleep(2);
    
    // 运行性能测试
    performance_stats_t stats;
    run_performance_test("127.0.0.1", TEST_PORT, &stats);
    
    // 打印结果
    print_performance_stats(&stats);
    
    // 停止服务器
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
    }
    
    pthread_join(server_thread_id, NULL);
    
    printf("\n性能测试完成！\n");
    
    return 0;
}
