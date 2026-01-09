/*
 * 限流功能性能基准测试
 * 
 * 测试目标：
 * 1. 限流功能的性能开销
 * 2. 限流检查的响应时间
 * 3. 限流对整体吞吐量的影响
 * 4. 白名单检查的性能
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
#include <stdatomic.h>

#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#define TEST_PORT 9999
#define TEST_THREADS 4
#define TEST_CONNECTIONS 100
#define TEST_REQUESTS_PER_CONNECTION 1000
#define BENCHMARK_ITERATIONS 5

// 性能统计
typedef struct {
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
    double p50_time;
    double p95_time;
    double p99_time;
    int total_requests;
    int successful_requests;
    int failed_requests;
    int rate_limited_requests;
    atomic_int active_threads;
} performance_stats_t;

// 请求计时
typedef struct {
    double start_time;
    double end_time;
    double duration;
    int success;
    int rate_limited;
} request_timing_t;

// 测试配置
typedef struct {
    int enable_rate_limit;
    int max_requests;
    int window_seconds;
    const char* test_name;
} test_config_t;

// 简单的HTTP客户端
static int send_http_request(const char* host, int port, const char* path, 
                             request_timing_t* timing) {
    timing->start_time = (double)clock() / CLOCKS_PER_SEC;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        timing->success = 0;
        timing->rate_limited = 0;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        timing->success = 0;
        timing->rate_limited = 0;
        return -1;
    }

    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        timing->success = 0;
        timing->rate_limited = 0;
        return -1;
    }

    char response[4096];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    
    timing->end_time = (double)clock() / CLOCKS_PER_SEC;
    timing->duration = timing->end_time - timing->start_time;
    
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        
        // 检查是否被限流
        if (strstr(response, "429") != NULL) {
            timing->success = 0;
            timing->rate_limited = 1;
        } else {
            timing->success = 1;
            timing->rate_limited = 0;
        }
        
        close(sock);
        return 0;
    }

    close(sock);
    timing->success = 0;
    timing->rate_limited = 0;
    return -1;
}

// 百分位计算
static double calculate_percentile(request_timing_t* timings, int count, double percentile) {
    if (count == 0) return 0.0;
    
    // 简单排序
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (timings[j].duration > timings[j + 1].duration) {
                request_timing_t temp = timings[j];
                timings[j] = timings[j + 1];
                timings[j + 1] = temp;
            }
        }
    }
    
    int index = (int)(count * percentile / 100.0);
    if (index >= count) index = count - 1;
    
    return timings[index].duration;
}

// 运行性能测试
static void run_performance_test(const char* host, int port, 
                                 performance_stats_t* stats,
                                 const test_config_t* config) {
    clock_t start_time = clock();
    
    int total_requests = TEST_CONNECTIONS * TEST_REQUESTS_PER_CONNECTION;
    request_timing_t* timings = malloc(sizeof(request_timing_t) * total_requests);
    if (!timings) {
        printf("内存分配失败\n");
        return;
    }
    
    stats->total_requests = total_requests;
    stats->successful_requests = 0;
    stats->failed_requests = 0;
    stats->rate_limited_requests = 0;
    stats->min_time = 1e9;
    stats->max_time = 0.0;
    
    printf("开始性能测试: %s\n", config->test_name);
    printf("  连接数: %d\n", TEST_CONNECTIONS);
    printf("  每连接请求数: %d\n", TEST_REQUESTS_PER_CONNECTION);
    printf("  总请求数: %d\n", total_requests);
    if (config->enable_rate_limit) {
        printf("  限流配置: %d 请求 / %d 秒\n", 
               config->max_requests, config->window_seconds);
    }
    printf("\n");

    for (int conn = 0; conn < TEST_CONNECTIONS; conn++) {
        for (int req = 0; req < TEST_REQUESTS_PER_CONNECTION; req++) {
            int idx = conn * TEST_REQUESTS_PER_CONNECTION + req;
            send_http_request(host, port, "/", &timings[idx]);
            
            if (timings[idx].success) {
                stats->successful_requests++;
            } else if (timings[idx].rate_limited) {
                stats->rate_limited_requests++;
            } else {
                stats->failed_requests++;
            }
            
            // 更新最小/最大时间
            if (timings[idx].duration < stats->min_time) {
                stats->min_time = timings[idx].duration;
            }
            if (timings[idx].duration > stats->max_time) {
                stats->max_time = timings[idx].duration;
            }
            
            // 每500个请求显示进度
            if ((idx + 1) % 500 == 0) {
                printf("已处理: %d/%d 请求\n", idx + 1, total_requests);
            }
        }
    }

    clock_t end_time = clock();
    stats->total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    stats->avg_time = stats->total_time / total_requests;
    
    // 计算百分位
    stats->p50_time = calculate_percentile(timings, total_requests, 50.0);
    stats->p95_time = calculate_percentile(timings, total_requests, 95.0);
    stats->p99_time = calculate_percentile(timings, total_requests, 99.0);
    
    free(timings);
}

// 打印性能统计
static void print_performance_stats(const performance_stats_t* stats, 
                                   const test_config_t* config) {
    printf("\n========== 性能测试结果: %s ==========\n", config->test_name);
    printf("总请求数: %d\n", stats->total_requests);
    printf("成功请求: %d\n", stats->successful_requests);
    printf("限流请求: %d\n", stats->rate_limited_requests);
    printf("失败请求: %d\n", stats->failed_requests);
    printf("成功率: %.2f%%\n",
           (double)stats->successful_requests / stats->total_requests * 100);
    printf("\n");
    printf("总耗时: %.3f 秒\n", stats->total_time);
    printf("吞吐量: %.2f 请求/秒\n", 
           stats->total_requests / stats->total_time);
    printf("\n");
    printf("响应时间统计:\n");
    printf("  平均: %.6f 秒\n", stats->avg_time);
    printf("  最小: %.6f 秒\n", stats->min_time);
    printf("  最大: %.6f 秒\n", stats->max_time);
    printf("  P50: %.6f 秒\n", stats->p50_time);
    printf("  P95: %.6f 秒\n", stats->p95_time);
    printf("  P99: %.6f 秒\n", stats->p99_time);
    printf("========================================\n");
}

// 服务器配置
static uvhttp_server_t* g_server = NULL;
static int g_server_port = TEST_PORT;

int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
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
    
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    uvhttp_router_add_route(router, "/", simple_handler);
    
    printf("服务器启动在端口 %d\n", g_server_port);
    uvhttp_server_listen(server, "0.0.0.0", g_server_port);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return NULL;
}

// 配置服务器
static void configure_server(uvhttp_server_t* server, const test_config_t* config) {
    if (config->enable_rate_limit) {
        uvhttp_server_enable_rate_limit(
            server, 
            config->max_requests, 
            config->window_seconds, 
            UVHTTP_RATE_LIMIT_FIXED_WINDOW
        );
        
        // 添加白名单
        uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("========== UVHTTP 限流功能性能基准测试 ==========\n\n");
    
    // 测试配置
    test_config_t configs[] = {
        {
            .enable_rate_limit = 0,
            .max_requests = 0,
            .window_seconds = 0,
            .test_name = "基准测试（无限流）"
        },
        {
            .enable_rate_limit = 1,
            .max_requests = 1000,
            .window_seconds = 1,
            .test_name = "限流测试（1000 请求/秒）"
        },
        {
            .enable_rate_limit = 1,
            .max_requests = 500,
            .window_seconds = 1,
            .test_name = "限流测试（500 请求/秒）"
        }
    };
    
    int num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (int i = 0; i < num_configs; i++) {
        // 启动服务器
        pthread_t server_thread_id;
        pthread_create(&server_thread_id, NULL, server_thread, NULL);
        
        // 等待服务器启动
        sleep(2);
        
        // 配置服务器
        if (configs[i].enable_rate_limit && g_server) {
            configure_server(g_server, &configs[i]);
            sleep(1); // 等待限流配置生效
        }
        
        // 运行性能测试
        performance_stats_t stats;
        memset(&stats, 0, sizeof(stats));
        run_performance_test("127.0.0.1", g_server_port, &stats, &configs[i]);
        
        // 打印结果
        print_performance_stats(&stats, &configs[i]);
        
        // 停止服务器
        if (g_server) {
            uvhttp_server_stop(g_server);
            uvhttp_server_free(g_server);
            g_server = NULL;
        }
        
        pthread_join(server_thread_id, NULL);
        
        printf("\n等待服务器清理...\n");
        sleep(2);
    }
    
    printf("\n性能测试完成！\n");
    
    return 0;
}
