/*
 * 性能基准测试
 * 用于验证性能优化效果
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#define TEST_PORT 9999
#define TEST_THREADS 4
#define TEST_CONNECTIONS 100
#define TEST_REQUESTS_PER_CONNECTION 100

// 性能统计
typedef struct {
    double total_time;
    double avg_time_per_request;
    double requests_per_second;
    int total_requests;
    int successful_requests;
    int failed_requests;
} performance_stats_t;

// 简单的HTTP客户端
static int send_http_request(const char* host, int port, const char* path) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }

    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: keep-alive\r\n"
             "\r\n",
             path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -1;
    }

    char response[4096];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        close(sock);
        return 0; // 成功
    }

    close(sock);
    return -1;
}

// 运行性能测试
static void run_performance_test(const char* host, int port, performance_stats_t* stats) {
    clock_t start_time = clock();
    
    stats->total_requests = TEST_CONNECTIONS * TEST_REQUESTS_PER_CONNECTION;
    stats->successful_requests = 0;
    stats->failed_requests = 0;

    printf("开始性能测试...\n");
    printf("  连接数: %d\n", TEST_CONNECTIONS);
    printf("  每连接请求数: %d\n", TEST_REQUESTS_PER_CONNECTION);
    printf("  总请求数: %d\n", stats->total_requests);
    printf("\n");

    for (int conn = 0; conn < TEST_CONNECTIONS; conn++) {
        for (int req = 0; req < TEST_REQUESTS_PER_CONNECTION; req++) {
            if (send_http_request(host, port, "/") == 0) {
                stats->successful_requests++;
            } else {
                stats->failed_requests++;
            }

            // 每1000个请求显示进度
            if ((conn * TEST_REQUESTS_PER_CONNECTION + req + 1) % 1000 == 0) {
                printf("已处理: %d/%d 请求\n",
                       conn * TEST_REQUESTS_PER_CONNECTION + req + 1,
                       stats->total_requests);
            }
        }
    }

    clock_t end_time = clock();
    stats->total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    stats->avg_time_per_request = stats->total_time / stats->total_requests;
    stats->requests_per_second = stats->total_requests / stats->total_time;
}

// 打印性能统计
static void print_performance_stats(const performance_stats_t* stats) {
    printf("\n========== 性能测试结果 ==========\n");
    printf("总请求数: %d\n", stats->total_requests);
    printf("成功请求: %d\n", stats->successful_requests);
    printf("失败请求: %d\n", stats->failed_requests);
    printf("成功率: %.2f%%\n",
           (double)stats->successful_requests / stats->total_requests * 100);
    printf("\n");
    printf("总耗时: %.3f 秒\n", stats->total_time);
    printf("平均每请求耗时: %.6f 秒\n", stats->avg_time_per_request);
    printf("吞吐量: %.2f 请求/秒\n", stats->requests_per_second);
    printf("==================================\n");
}

// 简单的HTTP服务器
static uvhttp_server_t* g_server = NULL;
static int g_server_port = TEST_PORT;

int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    
    return 0;
}

void* server_thread(void* arg) {
    (void)arg;
    
    uvhttp_server_config_t config;
    uvhttp_server_config_init(&config);
    config.port = g_server_port;
    config.host = "0.0.0.0";
    config.worker_threads = 2;
    
    g_server = uvhttp_server_create(&config);
    if (!g_server) {
        printf("服务器创建失败\n");
        return NULL;
    }
    
    uvhttp_server_register_handler(g_server, "/", simple_handler);
    
    printf("服务器启动在端口 %d\n", g_server_port);
    uvhttp_server_run(g_server);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("========== UVHTTP 性能基准测试 ==========\n\n");
    
    // 启动服务器（在后台）
    pthread_t server_thread_id;
    pthread_create(&server_thread_id, NULL, server_thread, NULL);
    
    // 等待服务器启动
    sleep(2);
    
    // 运行性能测试
    performance_stats_t stats;
    run_performance_test("127.0.0.1", g_server_port, &stats);
    
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
