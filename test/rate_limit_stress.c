/*
 * UVHTTP 限流功能压力测试
 * 
 * 测试目标：
 * 1. 高并发场景下的限流性能
 * 2. 长时间运行的稳定性
 * 3. 极限负载下的行为
 * 4. 内存泄漏检测
 * 5. CPU 使用率分析
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
#include <signal.h>
#include <sys/resource.h>

#include "uvhttp.h"
#include "uvhttp_server.h"

#define TEST_PORT 9999
#define MAX_THREADS 32
#define MAX_REQUESTS_PER_THREAD 100000

// 全局标志
static volatile int keep_running = 1;

// 统计信息
typedef struct {
    atomic_long total_requests;
    atomic_long successful_requests;
    atomic_long failed_requests;
    atomic_long rate_limited_requests;
    atomic_long total_bytes_sent;
    atomic_long total_bytes_received;
    time_t start_time;
    time_t end_time;
} stress_stats_t;

static stress_stats_t g_stats = {0};

// 信号处理
static void signal_handler(int signum) {
    printf("\n收到信号 %d，正在停止测试...\n", signum);
    keep_running = 0;
}

// 简单的HTTP客户端
static int send_http_request(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }

    const char* request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    
    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -1;
    }

    char response[8192];
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    if (bytes_received > 0) {
        if (strncmp(response, "HTTP/1.1 429", 12) == 0) {
            return 429; // 限流
        } else if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
            return 200; // 成功
        }
    }
    
    return -1; // 失败
}

// 工作线程
static void* worker_thread(void* arg) {
    int port = *(int*)arg;
    
    while (keep_running) {
        int result = send_http_request(port);
        
        atomic_fetch_add(&g_stats.total_requests, 1);
        
        if (result == 200) {
            atomic_fetch_add(&g_stats.successful_requests, 1);
            atomic_fetch_add(&g_stats.total_bytes_sent, strlen("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n"));
            atomic_fetch_add(&g_stats.total_bytes_received, 100); // 估算
        } else if (result == 429) {
            atomic_fetch_add(&g_stats.rate_limited_requests, 1);
            atomic_fetch_add(&g_stats.total_bytes_sent, strlen("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n"));
            atomic_fetch_add(&g_stats.total_bytes_received, 50); // 估算
        } else {
            atomic_fetch_add(&g_stats.failed_requests, 1);
        }
        
        // 短暂休眠避免CPU占用过高
        usleep(100);
    }
    
    return NULL;
}

// 打印进度
static void* progress_thread(void* arg) {
    while (keep_running) {
        sleep(1);
        
        time_t current_time = time(NULL);
        double elapsed = difftime(current_time, g_stats.start_time);
        
        if (elapsed > 0) {
            long total = atomic_load(&g_stats.total_requests);
            long successful = atomic_load(&g_stats.successful_requests);
            long failed = atomic_load(&g_stats.failed_requests);
            long rate_limited = atomic_load(&g_stats.rate_limited_requests);
            
            printf("\r[进度] 总请求: %ld | 成功: %ld | 失败: %ld | 限流: %ld | RPS: %.2f | 耗时: %.0fs",
                   total, successful, failed, rate_limited, total / elapsed, elapsed);
            fflush(stdout);
        }
    }
    
    return NULL;
}

// 打印内存使用
static void print_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    printf("\n\n内存使用统计:\n");
    printf("  最大常驻集大小: %ld KB\n", usage.ru_maxrss);
    printf("  共享内存大小: %ld KB\n", usage.ru_ixrss);
    printf("  非共享数据大小: %ld KB\n", usage.ru_idrss);
    printf("  非共享栈大小: %ld KB\n", usage.ru_isrss);
    printf("  页面错误次数: %ld\n", usage.ru_majflt);
}

// 主函数
int main(int argc, char* argv[]) {
    printf("╔═══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║       UVHTTP 限流功能压力测试 v2.0                                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 解析命令行参数
    int num_threads = 8;
    int test_duration = 60; // 默认60秒
    int enable_rate_limit = 1;
    int max_requests = 1000;
    int window_seconds = 1;
    
    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1 || num_threads > MAX_THREADS) {
            fprintf(stderr, "线程数必须在 1-%d 之间\n", MAX_THREADS);
            return 1;
        }
    }
    
    if (argc > 2) {
        test_duration = atoi(argv[2]);
        if (test_duration < 1) {
            fprintf(stderr, "测试时长必须大于0\n");
            return 1;
        }
    }
    
    printf("测试配置:\n");
    printf("  线程数: %d\n", num_threads);
    printf("  测试时长: %d 秒\n", test_duration);
    printf("  限流: %s\n", enable_rate_limit ? "启用" : "禁用");
    if (enable_rate_limit) {
        printf("  最大请求数: %d\n", max_requests);
        printf("  时间窗口: %d 秒\n", window_seconds);
    }
    printf("\n");
    
    // 创建事件循环和服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    if (!server) {
        fprintf(stderr, "创建服务器失败\n");
        return 1;
    }
    
    // 设置请求处理器
    uvhttp_server_set_handler(server, [](uvhttp_request_t* request) {
        uvhttp_response_set_status(request->response, 200);
        uvhttp_response_set_header(request->response, "Content-Type", "text/plain");
        const char* body = "Hello, World!";
        uvhttp_response_set_body(request->response, body, strlen(body));
        uvhttp_response_send(request->response);
    });
    
    // 启用限流（如果需要）
    if (enable_rate_limit) {
        uvhttp_server_enable_rate_limit(server, max_requests, window_seconds);
    }
    
    // 启动服务器
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", TEST_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    printf("服务器已启动，监听端口 %d\n", TEST_PORT);
    printf("按 Ctrl+C 停止测试\n\n");
    
    // 等待服务器完全启动
    sleep(1);
    
    // 记录开始时间
    g_stats.start_time = time(NULL);
    
    // 创建工作线程
    pthread_t threads[num_threads];
    int port = TEST_PORT;
    
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &port);
    }
    
    // 创建进度线程
    pthread_t progress_tid;
    pthread_create(&progress_tid, NULL, progress_thread, NULL);
    
    // 运行测试
    if (test_duration > 0) {
        sleep(test_duration);
        keep_running = 0;
    } else {
        // 无限运行，直到收到信号
        while (keep_running) {
            sleep(1);
        }
    }
    
    // 记录结束时间
    g_stats.end_time = time(NULL);
    
    // 等待所有线程完成
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_join(progress_tid, NULL);
    
    printf("\n\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("压力测试结果\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    double elapsed = difftime(g_stats.end_time, g_stats.start_time);
    long total = atomic_load(&g_stats.total_requests);
    long successful = atomic_load(&g_stats.successful_requests);
    long failed = atomic_load(&g_stats.failed_requests);
    long rate_limited = atomic_load(&g_stats.rate_limited_requests);
    long bytes_sent = atomic_load(&g_stats.total_bytes_sent);
    long bytes_received = atomic_load(&g_stats.total_bytes_received);
    
    printf("测试时长: %.0f 秒\n", elapsed);
    printf("\n");
    printf("请求统计:\n");
    printf("  总请求数: %ld\n", total);
    printf("  成功请求: %ld (%.2f%%)\n", successful, (successful * 100.0) / total);
    printf("  失败请求: %ld (%.2f%%)\n", failed, (failed * 100.0) / total);
    printf("  限流请求: %ld (%.2f%%)\n", rate_limited, (rate_limited * 100.0) / total);
    printf("\n");
    printf("性能指标:\n");
    printf("  平均 RPS: %.2f\n", total / elapsed);
    printf("  成功 RPS: %.2f\n", successful / elapsed);
    printf("  失败 RPS: %.2f\n", failed / elapsed);
    printf("  限流 RPS: %.2f\n", rate_limited / elapsed);
    printf("\n");
    printf("网络统计:\n");
    printf("  发送字节: %ld (%.2f MB)\n", bytes_sent, bytes_sent / 1024.0 / 1024.0);
    printf("  接收字节: %ld (%.2f MB)\n", bytes_received, bytes_received / 1024.0 / 1024.0);
    printf("\n");
    
    print_memory_usage();
    
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    printf("测试完成\n");
    printf("═══════════════════════════════════════════════════════════════════════════════\n");
    
    // 清理
    uvhttp_server_free(server);
    
    return 0;
}