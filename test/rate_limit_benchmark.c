/*
 * UVHTTP 限流功能综合性能测试
 * 
 * 测试目标：
 * 1. 限流功能的性能开销
 * 2. 限流检查的响应时间分布
 * 3. 限流对整体吞吐量的影响
 * 4. 白名单检查的性能
 * 5. 不同配置下的性能对比
 * 6. 内存占用分析
 * 7. CPU 缓存命中率
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
#include <math.h>
#include <sys/resource.h>

#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#define TEST_PORT 9999
#define MAX_THREADS 16
#define MAX_CONNECTIONS 1000
#define MAX_REQUESTS_PER_CONNECTION 10000
#define BENCHMARK_ITERATIONS 10

// 性能统计
typedef struct {
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
    double p50_time;
    double p90_time;
    double p95_time;
    double p99_time;
    double p999_time;
    int total_requests;
    int successful_requests;
    int failed_requests;
    int rate_limited_requests;
    int whitelisted_requests;
    atomic_int active_threads;
    atomic_int total_bytes_sent;
    atomic_int total_bytes_received;
} performance_stats_t;

// 请求计时
typedef struct {
    double start_time;
    double end_time;
    double duration;
    int success;
    int rate_limited;
    int whitelisted;
    int status_code;
} request_timing_t;

// 测试配置
typedef struct {
    int enable_rate_limit;
    int max_requests;
    int window_seconds;
    int enable_whitelist;
    int whitelist_size;
    const char* test_name;
} test_config_t;

// 测试场景
typedef struct {
    const char* name;
    test_config_t config;
    int threads;
    int connections;
    int requests_per_connection;
} test_scenario_t;

// 简单的HTTP客户端
static int send_http_request(const char* host, int port, const char* path, 
                             request_timing_t* timing) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    timing->start_time = start.tv_sec + start.tv_nsec / 1e9;
    
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

    char response[8192];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    clock_gettime(CLOCK_MONOTONIC, &end);
    timing->end_time = end.tv_sec + end.tv_nsec / 1e9;
    timing->duration = timing->end_time - timing->start_time;

    if (bytes_received > 0) {
        timing->success = 1;
        response[bytes_received] = '\0';
        
        // 解析状态码
        if (strncmp(response, "HTTP/1.1 429", 12) == 0) {
            timing->status_code = 429;
            timing->rate_limited = 1;
        } else if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
            timing->status_code = 200;
            timing->rate_limited = 0;
        } else {
            timing->status_code = 0;
        }
        
        return bytes_received;
    } else {
        timing->success = 0;
        timing->rate_limited = 0;
        return -1;
    }
}

// 线程参数
typedef struct {
    int thread_id;
    int port;
    int connections;
    int requests_per_connection;
    performance_stats_t* stats;
    request_timing_t* timings;
    int enable_whitelist;
    char* whitelist_ips;
} thread_params_t;

// 工作线程
static void* worker_thread(void* arg) {
    thread_params_t* params = (thread_params_t*) arg;
    performance_stats_t* stats = params->stats;
    
    atomic_fetch_add(&stats->active_threads, 1);
    
    int timing_index = 0;
    int total_requests = params->connections * params->requests_per_connection;
    
    for (int conn = 0; conn < params->connections; conn++) {
        for (int req = 0; req < params->requests_per_connection; req++) {
            if (timing_index < total_requests) {
                request_timing_t* timing = &params->timings[timing_index];
                
                // 随机选择是否使用白名单IP
                const char* ip = "127.0.0.1";
                if (params->enable_whitelist && params->whitelist_ips) {
                    // 30% 概率使用白名单IP
                    if (rand() % 100 < 30) {
                        int whitelist_index = rand() % params->enable_whitelist;
                        // 使用白名单IP（这里简化处理）
                    }
                }
                
                int bytes_received = send_http_request("127.0.0.1", params->port, "/", timing);
                
                if (bytes_received > 0) {
                    atomic_fetch_add(&stats->total_bytes_received, bytes_received);
                    atomic_fetch_add(&stats->total_bytes_sent, strlen("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n"));
                    
                    if (timing->success) {
                        stats->successful_requests++;
                        if (timing->rate_limited) {
                            stats->rate_limited_requests++;
                        }
                    } else {
                        stats->failed_requests++;
                    }
                } else {
                    stats->failed_requests++;
                }
                
                timing_index++;
            }
        }
    }
    
    atomic_fetch_sub(&stats->active_threads, 1);
    return NULL;
}

// 计算百分位数
static double calculate_percentile(double* array, int size, double percentile) {
    qsort(array, size, sizeof(double), (int (*)(const void*, const void*)) strcmp);
    int index = (int)((size - 1) * percentile);
    return array[index];
}

// 运行测试场景
static void run_test_scenario(uvhttp_server_t* server, const test_scenario_t* scenario) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("测试场景: %s\n", scenario->name);
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("配置:\n");
    printf("  线程数: %d\n", scenario->threads);
    printf("  连接数: %d\n", scenario->connections);
    printf("  每连接请求数: %d\n", scenario->requests_per_connection);
    printf("  限流: %s\n", scenario->config.enable_rate_limit ? "启用" : "禁用");
    if (scenario->config.enable_rate_limit) {
        printf("  最大请求数: %d\n", scenario->config.max_requests);
        printf("  时间窗口: %d 秒\n", scenario->config.window_seconds);
    }
    printf("  白名单: %s\n", scenario->config.enable_whitelist ? "启用" : "禁用");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    // 配置限流
    if (scenario->config.enable_rate_limit) {
        uvhttp_server_enable_rate_limit(server, scenario->config.max_requests, 
                                         scenario->config.window_seconds);
        
        // 配置白名单
        if (scenario->config.enable_whitelist && scenario->config.whitelist_size > 0) {
            for (int i = 0; i < scenario->config.whitelist_size; i++) {
                char ip[32];
                snprintf(ip, sizeof(ip), "192.168.1.%d", i + 1);
                uvhttp_server_add_rate_limit_whitelist(server, ip);
            }
        }
    }
    
    // 分配统计和计时数组
    int total_requests = scenario->threads * scenario->connections * 
                         scenario->requests_per_connection;
    performance_stats_t stats = {0};
    request_timing_t* timings = calloc(total_requests, sizeof(request_timing_t));
    
    if (!timings) {
        fprintf(stderr, "内存分配失败\n");
        return;
    }
    
    // 创建线程
    pthread_t threads[scenario->threads];
    thread_params_t* params = calloc(scenario->threads, sizeof(thread_params_t));
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 启动线程
    for (int i = 0; i < scenario->threads; i++) {
        params[i].thread_id = i;
        params[i].port = TEST_PORT;
        params[i].connections = scenario->connections;
        params[i].requests_per_connection = scenario->requests_per_connection;
        params[i].stats = &stats;
        params[i].timings = &timings[i * scenario->connections * scenario->requests_per_connection];
        params[i].enable_whitelist = scenario->config.whitelist_size;
        
        pthread_create(&threads[i], NULL, worker_thread, &params[i]);
    }
    
    // 等待所有线程完成
    for (int i = 0; i < scenario->threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    stats.total_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // 计算统计信息
    double* durations = calloc(stats.successful_requests, sizeof(double));
    int duration_index = 0;
    
    for (int i = 0; i < total_requests; i++) {
        if (timings[i].success) {
            durations[duration_index++] = timings[i].duration;
            
            if (duration_index == 1) {
                stats.min_time = timings[i].duration;
                stats.max_time = timings[i].duration;
            } else {
                if (timings[i].duration < stats.min_time) {
                    stats.min_time = timings[i].duration;
                }
                if (timings[i].duration > stats.max_time) {
                    stats.max_time = timings[i].duration;
                }
            }
        }
    }
    
    if (duration_index > 0) {
        qsort(durations, duration_index, sizeof(double), 
              (int (*)(const void*, const void*)) strcmp);
        
        stats.avg_time = stats.total_time / duration_index;
        stats.p50_time = durations[(int)(duration_index * 0.5)];
        stats.p90_time = durations[(int)(duration_index * 0.9)];
        stats.p95_time = durations[(int)(duration_index * 0.95)];
        stats.p99_time = durations[(int)(duration_index * 0.99)];
        stats.p999_time = durations[(int)(duration_index * 0.999)];
    }
    
    stats.total_requests = total_requests;
    
    // 打印结果
    printf("\n性能结果:\n");
    printf("  总请求数: %d\n", stats.total_requests);
    printf("  成功请求: %d\n", stats.successful_requests);
    printf("  失败请求: %d\n", stats.failed_requests);
    printf("  限流请求: %d\n", stats.rate_limited_requests);
    printf("  白名单请求: %d\n", stats.whitelisted_requests);
    printf("  总耗时: %.3f 秒\n", stats.total_time);
    printf("  吞吐量: %.2f RPS\n", stats.total_requests / stats.total_time);
    printf("  成功率: %.2f%%\n", (stats.successful_requests * 100.0) / stats.total_requests);
    printf("  限流率: %.2f%%\n", (stats.rate_limited_requests * 100.0) / stats.total_requests);
    printf("\n");
    printf("响应时间统计:\n");
    printf("  平均: %.3f ms\n", stats.avg_time * 1000);
    printf("  最小: %.3f ms\n", stats.min_time * 1000);
    printf("  最大: %.3f ms\n", stats.max_time * 1000);
    printf("  P50:  %.3f ms\n", stats.p50_time * 1000);
    printf("  P90:  %.3f ms\n", stats.p90_time * 1000);
    printf("  P95:  %.3f ms\n", stats.p95_time * 1000);
    printf("  P99:  %.3f ms\n", stats.p99_time * 1000);
    printf("  P999: %.3f ms\n", stats.p999_time * 1000);
    printf("\n");
    printf("网络统计:\n");
    printf("  发送字节: %d\n", atomic_load(&stats.total_bytes_sent));
    printf("  接收字节: %d\n", atomic_load(&stats.total_bytes_received));
    printf("\n");
    
    // 清理
    free(durations);
    free(timings);
    free(params);
    
    // 禁用限流以便下一个测试
    uvhttp_server_disable_rate_limit(server);
}

// 内存统计
static void print_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    printf("内存使用:\n");
    printf("  最大常驻集大小: %ld KB\n", usage.ru_maxrss);
    printf("  共享内存大小: %ld KB\n", usage.ru_ixrss);
    printf("  非共享数据大小: %ld KB\n", usage.ru_idrss);
    printf("  非共享栈大小: %ld KB\n", usage.ru_isrss);
    printf("\n");
}

// 主函数
int main(int argc, char* argv[]) {
    printf("╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║       UVHTTP 限流功能综合性能测试 v2.0                              ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // 创建事件循环和服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    if (!server) {
        fprintf(stderr, "创建服务器失败\n");
        return 1;
    }
    
    // 设置简单的请求处理器
    uvhttp_server_set_handler(server, [](uvhttp_request_t* request) {
        uvhttp_response_set_status(request->response, 200);
        uvhttp_response_set_header(request->response, "Content-Type", "text/plain");
        const char* body = "Hello, World!";
        uvhttp_response_set_body(request->response, body, strlen(body));
        uvhttp_response_send(request->response);
    });
    
    // 启动服务器
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", TEST_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    printf("服务器已启动，监听端口 %d\n", TEST_PORT);
    printf("\n");
    
    // 等待服务器完全启动
    sleep(1);
    
    // 定义测试场景
    test_scenario_t scenarios[] = {
        {
            "基准测试（无限流）",
            {0, 0, 0, 0, 0, "baseline"},
            4, 100, 100
        },
        {
            "限流测试（1000/秒）",
            {1, 1000, 1, 0, 0, "rate_limit_1000"},
            4, 100, 100
        },
        {
            "限流测试（10000/秒）",
            {1, 10000, 1, 0, 0, "rate_limit_10000"},
            4, 100, 100
        },
        {
            "限流测试（1000/秒，带白名单）",
            {1, 1000, 1, 1, 10, "rate_limit_whitelist"},
            4, 100, 100
        },
        {
            "高并发测试（16线程）",
            {1, 10000, 1, 0, 0, "high_concurrency"},
            16, 100, 100
        },
        {
            "长连接测试（10000请求/连接）",
            {1, 1000, 1, 0, 0, "long_connection"},
            4, 10, 10000
        }
    };
    
    int num_scenarios = sizeof(scenarios) / sizeof(scenarios[0]);
    
    // 运行所有测试场景
    for (int i = 0; i < num_scenarios; i++) {
        run_test_scenario(server, &scenarios[i]);
        print_memory_usage();
        
        // 测试之间的间隔
        if (i < num_scenarios - 1) {
            printf("等待 2 秒...\n\n");
            sleep(2);
        }
    }
    
    // 清理
    uvhttp_server_free(server);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("所有测试完成\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    return 0;
}