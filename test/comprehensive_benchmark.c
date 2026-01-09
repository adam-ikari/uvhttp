/*
 * UVHTTP 综合性能测试框架
 * 
 * 测试目标：
 * 1. 不同配置下的性能对比
 * 2. 各种使用场景的性能表现
 * 3. 内存占用分析
 * 4. CPU 使用率分析
 * 5. 网络吞吐量测试
 * 6. 并发性能测试
 * 7. 不同功能模块的性能影响
 * 8. 长时间运行稳定性
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
#include <signal.h>

#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_middleware.h"

#define TEST_PORT 9999
#define MAX_THREADS 32
#define MAX_SCENARIOS 20

// 测试配置
typedef struct {
    int enable_websocket;
    int enable_middleware;
    int enable_rate_limit;
    int enable_cors;
    int enable_router;
    int enable_static_files;
    int use_mimalloc;
    int max_connections;
    const char* name;
} benchmark_config_t;

// 性能指标
typedef struct {
    double total_time;
    double avg_rps;
    double min_latency;
    double max_latency;
    double avg_latency;
    double p50_latency;
    double p95_latency;
    double p99_latency;
    long total_requests;
    long successful_requests;
    long failed_requests;
    long total_bytes_sent;
    long total_bytes_received;
    double cpu_time;
    long max_rss;
    long context_switches;
    int num_threads;
} benchmark_metrics_t;

// 测试场景
typedef struct {
    const char* name;
    benchmark_config_t config;
    int num_threads;
    int requests_per_thread;
    int test_duration;
} test_scenario_t;

// 请求统计
typedef struct {
    double start_time;
    double end_time;
    double duration;
    int success;
    int status_code;
    int bytes_sent;
    int bytes_received;
} request_stat_t;

// 全局统计
typedef struct {
    atomic_long total_requests;
    atomic_long successful_requests;
    atomic_long failed_requests;
    atomic_long total_bytes_sent;
    atomic_long total_bytes_received;
    double* latencies;
    int latency_count;
    time_t start_time;
    time_t end_time;
    volatile int keep_running;
} global_stats_t;

static global_stats_t g_stats;

// 信号处理
static void signal_handler(int signum) {
    printf("\n收到信号 %d，停止测试...\n", signum);
    g_stats.keep_running = 0;
}

// 简单的HTTP客户端
static int send_http_request(int port, request_stat_t* stat) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    stat->start_time = start.tv_sec + start.tv_nsec / 1e9;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        stat->success = 0;
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        stat->success = 0;
        return -1;
    }

    const char* request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    int request_len = strlen(request);
    
    if (send(sock, request, request_len, 0) < 0) {
        close(sock);
        stat->success = 0;
        return -1;
    }

    char response[8192];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    clock_gettime(CLOCK_MONOTONIC, &end);
    stat->end_time = end.tv_sec + end.tv_nsec / 1e9;
    stat->duration = stat->end_time - stat->start_time;
    stat->bytes_sent = request_len;
    stat->bytes_received = bytes_received;

    if (bytes_received > 0) {
        stat->success = 1;
        if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
            stat->status_code = 200;
        } else if (strncmp(response, "HTTP/1.1 404", 12) == 0) {
            stat->status_code = 404;
        } else if (strncmp(response, "HTTP/1.1 429", 12) == 0) {
            stat->status_code = 429;
        } else {
            stat->status_code = 0;
        }
        return bytes_received;
    } else {
        stat->success = 0;
        return -1;
    }
}

// 工作线程
static void* worker_thread(void* arg) {
    int port = *(int*)arg;
    request_stat_t* stats = calloc(10000, sizeof(request_stat_t));
    int stat_count = 0;
    
    while (g_stats.keep_running && stat_count < 10000) {
        request_stat_t* stat = &stats[stat_count];
        int bytes_received = send_http_request(port, stat);
        
        atomic_fetch_add(&g_stats.total_requests, 1);
        
        if (stat->success) {
            atomic_fetch_add(&g_stats.successful_requests, 1);
            atomic_fetch_add(&g_stats.total_bytes_sent, stat->bytes_sent);
            atomic_fetch_add(&g_stats.total_bytes_received, stat->bytes_received);
            
            // 记录延迟
            if (g_stats.latency_count < 100000) {
                g_stats.latencies[g_stats.latency_count++] = stat->duration;
            }
        } else {
            atomic_fetch_add(&g_stats.failed_requests, 1);
        }
        
        stat_count++;
        
        // 短暂休眠
        usleep(10);
    }
    
    free(stats);
    return NULL;
}

// 计算百分位数
static double calculate_percentile(double* array, int size, double percentile) {
    if (size == 0) return 0;
    qsort(array, size, sizeof(double), (int (*)(const void*, const void*)) strcmp);
    int index = (int)((size - 1) * percentile);
    return array[index];
}

// 运行基准测试
static void run_benchmark(const test_scenario_t* scenario, benchmark_metrics_t* metrics) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("测试场景: %s\n", scenario->name);
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("配置:\n");
    printf("  WebSocket: %s\n", scenario->config.enable_websocket ? "启用" : "禁用");
    printf("  中间件: %s\n", scenario->config.enable_middleware ? "启用" : "禁用");
    printf("  限流: %s\n", scenario->config.enable_rate_limit ? "启用" : "禁用");
    printf("  CORS: %s\n", scenario->config.enable_cors ? "启用" : "禁用");
    printf("  路由: %s\n", scenario->config.enable_router ? "启用" : "禁用");
    printf("  静态文件: %s\n", scenario->config.enable_static_files ? "启用" : "禁用");
    printf("  mimalloc: %s\n", scenario->config.use_mimalloc ? "启用" : "禁用");
    printf("  最大连接: %d\n", scenario->config.max_connections);
    printf("  线程数: %d\n", scenario->num_threads);
    printf("  每线程请求数: %d\n", scenario->requests_per_thread);
    printf("  测试时长: %d 秒\n", scenario->test_duration);
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    // 创建服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    if (!server) {
        fprintf(stderr, "创建服务器失败\n");
        return;
    }
    
    // 配置路由
    if (scenario->config.enable_router) {
        uvhttp_router_t* router = uvhttp_router_new();
        uvhttp_router_add_route(router, "/", [](uvhttp_request_t* req) {
            uvhttp_response_set_status(req->response, 200);
            uvhttp_response_set_header(req->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(req->response, "Hello, World!", 13);
            uvhttp_response_send(req->response);
        });
        uvhttp_router_add_route(router, "/api/data", [](uvhttp_request_t* req) {
            uvhttp_response_set_status(req->response, 200);
            uvhttp_response_set_header(req->response, "Content-Type", "application/json");
            uvhttp_response_set_body(req->response, "{\"data\":\"test\"}", 15);
            uvhttp_response_send(req->response);
        });
        uvhttp_router_add_route(router, "/api/status", [](uvhttp_request_t* req) {
            uvhttp_response_set_status(req->response, 200);
            uvhttp_response_set_header(req->response, "Content-Type", "application/json");
            uvhttp_response_set_body(req->response, "{\"status\":\"ok\"}", 15);
            uvhttp_response_send(req->response);
        });
        server->router = router;
    } else {
        uvhttp_server_set_handler(server, [](uvhttp_request_t* req) {
            uvhttp_response_set_status(req->response, 200);
            uvhttp_response_set_header(req->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(req->response, "Hello, World!", 13);
            uvhttp_response_send(req->response);
        });
    }
    
    // 配置限流
    if (scenario->config.enable_rate_limit) {
        uvhttp_server_enable_rate_limit(server, 10000, 1);
    }
    
    // 启动服务器
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", TEST_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return;
    }
    
    printf("服务器已启动，等待 1 秒...\n");
    sleep(1);
    
    // 初始化统计
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.keep_running = 1;
    g_stats.latencies = calloc(100000, sizeof(double));
    g_stats.start_time = time(NULL);
    
    // 记录初始资源使用
    struct rusage usage_before;
    getrusage(RUSAGE_SELF, &usage_before);
    
    // 创建工作线程
    pthread_t threads[scenario->num_threads];
    int port = TEST_PORT;
    
    for (int i = 0; i < scenario->num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &port);
    }
    
    // 运行测试
    if (scenario->test_duration > 0) {
        sleep(scenario->test_duration);
    } else {
        sleep(5);
    }
    
    g_stats.keep_running = 0;
    g_stats.end_time = time(NULL);
    
    // 等待所有线程完成
    for (int i = 0; i < scenario->num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 记录结束资源使用
    struct rusage usage_after;
    getrusage(RUSAGE_SELF, &usage_after);
    
    // 计算指标
    double elapsed = difftime(g_stats.end_time, g_stats.start_time);
    metrics->total_time = elapsed;
    metrics->total_requests = atomic_load(&g_stats.total_requests);
    metrics->successful_requests = atomic_load(&g_stats.successful_requests);
    metrics->failed_requests = atomic_load(&g_stats.failed_requests);
    metrics->total_bytes_sent = atomic_load(&g_stats.total_bytes_sent);
    metrics->total_bytes_received = atomic_load(&g_stats.total_bytes_received);
    metrics->avg_rps = metrics->total_requests / elapsed;
    metrics->cpu_time = (usage_after.ru_utime.tv_sec - usage_before.ru_utime.tv_sec) +
                      (usage_after.ru_utime.tv_usec - usage_before.ru_utime.tv_usec) / 1e6;
    metrics->max_rss = usage_after.ru_maxrss;
    metrics->context_switches = usage_after.ru_nvcsw + usage_after.ru_nivcsw;
    metrics->num_threads = scenario->num_threads;
    
    // 计算延迟统计
    if (g_stats.latency_count > 0) {
        metrics->min_latency = g_stats.latencies[0];
        metrics->max_latency = g_stats.latencies[0];
        double sum = 0;
        
        for (int i = 0; i < g_stats.latency_count; i++) {
            sum += g_stats.latencies[i];
            if (g_stats.latencies[i] < metrics->min_latency) {
                metrics->min_latency = g_stats.latencies[i];
            }
            if (g_stats.latencies[i] > metrics->max_latency) {
                metrics->max_latency = g_stats.latencies[i];
            }
        }
        
        metrics->avg_latency = sum / g_stats.latency_count;
        metrics->p50_latency = calculate_percentile(g_stats.latencies, g_stats.latency_count, 0.5);
        metrics->p95_latency = calculate_percentile(g_stats.latencies, g_stats.latency_count, 0.95);
        metrics->p99_latency = calculate_percentile(g_stats.latencies, g_stats.latency_count, 0.99);
    }
    
    // 打印结果
    printf("\n性能结果:\n");
    printf("  总请求数: %ld\n", metrics->total_requests);
    printf("  成功请求: %ld (%.2f%%)\n", metrics->successful_requests, 
           (metrics->successful_requests * 100.0) / metrics->total_requests);
    printf("  失败请求: %ld (%.2f%%)\n", metrics->failed_requests,
           (metrics->failed_requests * 100.0) / metrics->total_requests);
    printf("  总耗时: %.3f 秒\n", metrics->total_time);
    printf("  平均 RPS: %.2f\n", metrics->avg_rps);
    printf("\n");
    printf("延迟统计:\n");
    printf("  平均: %.3f ms\n", metrics->avg_latency * 1000);
    printf("  最小: %.3f ms\n", metrics->min_latency * 1000);
    printf("  最大: %.3f ms\n", metrics->max_latency * 1000);
    printf("  P50:  %.3f ms\n", metrics->p50_latency * 1000);
    printf("  P95:  %.3f ms\n", metrics->p95_latency * 1000);
    printf("  P99:  %.3f ms\n", metrics->p99_latency * 1000);
    printf("\n");
    printf("资源使用:\n");
    printf("  CPU 时间: %.3f 秒\n", metrics->cpu_time);
    printf("  最大 RSS: %ld KB\n", metrics->max_rss);
    printf("  上下文切换: %ld\n", metrics->context_switches);
    printf("\n");
    printf("网络统计:\n");
    printf("  发送字节: %ld (%.2f MB)\n", metrics->total_bytes_sent, 
           metrics->total_bytes_sent / 1024.0 / 1024.0);
    printf("  接收字节: %ld (%.2f MB)\n", metrics->total_bytes_received,
           metrics->total_bytes_received / 1024.0 / 1024.0);
    printf("  吞吐量: %.2f MB/s\n", 
           (metrics->total_bytes_sent + metrics->total_bytes_received) / 1024.0 / 1024.0 / elapsed);
    printf("\n");
    
    // 清理
    free(g_stats.latencies);
    uvhttp_server_free(server);
}

// 打印对比表格
static void print_comparison_table(benchmark_metrics_t* metrics, int num_scenarios, 
                                   test_scenario_t* scenarios) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("性能对比表格\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("%-30s %10s %10s %10s %10s %10s\n", 
           "场景", "RPS", "P95(ms)", "CPU(s)", "RSS(KB)", "CTXSW");
    printf("────────────────────────────────────────────────────────────────────────────────\n");
    
    for (int i = 0; i < num_scenarios; i++) {
        printf("%-30s %10.2f %10.3f %10.3f %10ld %10ld\n",
               scenarios[i].name,
               metrics[i].avg_rps,
               metrics[i].p95_latency * 1000,
               metrics[i].cpu_time,
               metrics[i].max_rss,
               metrics[i].context_switches);
    }
    
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
}

// 主函数
int main(int argc, char* argv[]) {
    printf("╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║       UVHTTP 综合性能测试框架 v2.0                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 定义测试场景
    test_scenario_t scenarios[] = {
        {
            "1. 基准测试（最小配置）",
            {0, 0, 0, 0, 0, 0, 0, 100, "baseline"},
            4, 1000, 10
        },
        {
            "2. 启用路由",
            {0, 0, 0, 0, 1, 0, 0, 100, "router"},
            4, 1000, 10
        },
        {
            "3. 启用限流",
            {0, 0, 1, 0, 0, 0, 0, 100, "rate_limit"},
            4, 1000, 10
        },
        {
            "4. 路由 + 限流",
            {0, 0, 1, 0, 1, 0, 0, 100, "router_rate_limit"},
            4, 1000, 10
        },
        {
            "5. 高并发（8线程）",
            {0, 0, 0, 0, 0, 0, 0, 100, "high_concurrency"},
            8, 1000, 10
        },
        {
            "6. 极高并发（16线程）",
            {0, 0, 0, 0, 0, 0, 0, 100, "extreme_concurrency"},
            16, 1000, 10
        },
        {
            "7. 长时间运行（30秒）",
            {0, 0, 0, 0, 0, 0, 0, 100, "long_running"},
            4, 1000, 30
        },
        {
            "8. 大量请求（10000/线程）",
            {0, 0, 0, 0, 0, 0, 0, 100, "high_volume"},
            4, 10000, 10
        },
        {
            "9. 全功能启用",
            {0, 0, 1, 0, 1, 0, 0, 100, "full_features"},
            4, 1000, 10
        },
        {
            "10. 高并发 + 全功能",
            {0, 0, 1, 0, 1, 0, 0, 100, "high_concurrency_full"},
            8, 1000, 10
        }
    };
    
    int num_scenarios = sizeof(scenarios) / sizeof(scenarios[0]);
    benchmark_metrics_t metrics[num_scenarios];
    
    // 运行所有测试场景
    for (int i = 0; i < num_scenarios; i++) {
        run_benchmark(&scenarios[i], &metrics[i]);
        
        // 测试之间的间隔
        if (i < num_scenarios - 1) {
            printf("等待 3 秒...\n");
            sleep(3);
        }
    }
    
    // 打印对比表格
    print_comparison_table(metrics, num_scenarios, scenarios);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("所有测试完成\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    return 0;
}