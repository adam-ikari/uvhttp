#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <sys/resource.h>
#include <uv.h>
#include "include/uvhttp.h"
#include "include/uvhttp_logging.h"

// 压力测试配置
typedef struct {
    int max_connections;
    int test_duration_seconds;
    int target_rps;
    int payload_size;
    char* test_name;
} stress_test_config_t;

// 连接统计
typedef struct {
    int total_connections;
    int successful_connections;
    int failed_connections;
    int total_requests;
    int successful_requests;
    int failed_requests;
    double total_response_time;
    double min_response_time;
    double max_response_time;
    size_t total_bytes_sent;
    size_t total_bytes_received;
    double test_duration;
    double actual_rps;
} stress_stats_t;

// 客户端连接数据
typedef struct {
    uv_tcp_t tcp_handle;
    uv_connect_t connect_req;
    uv_write_t write_req;
    uv_timer_t timer_req;
    
    stress_test_config_t* config;
    stress_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    
    char* request_buffer;
    char* response_buffer;
    size_t response_pos;
    size_t response_capacity;
    
    int request_sent;
    int response_received;
    double connect_start_time;
    double request_start_time;
    int is_connected;
    int is_error;
    
    struct sockaddr_in server_addr;
} client_connection_t;

// 测试管理器
typedef struct {
    uv_loop_t* loop;
    stress_test_config_t* config;
    stress_stats_t* stats;
    pthread_mutex_t stats_mutex;
    
    client_connection_t* connections;
    int active_connections;
    int max_active_connections;
    
    double test_start_time;
    volatile int should_stop;
    int requests_sent_this_second;
    int target_requests_per_second;
    
    uv_timer_t stats_timer;
    uv_timer_t rate_limit_timer;
} test_manager_t;

// 全局测试管理器
static test_manager_t g_test_manager = {0};

// 获取当前时间戳（毫秒）
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 获取内存使用量（KB）
static size_t get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// 更新统计信息
static void update_stats(stress_stats_t* stats, pthread_mutex_t* mutex, 
                        int success, double response_time, size_t bytes_sent, size_t bytes_received) {
    pthread_mutex_lock(mutex);
    
    stats->total_requests++;
    if (success) {
        stats->successful_requests++;
        stats->total_response_time += response_time;
        
        if (response_time < stats->min_response_time || stats->min_response_time == 0) {
            stats->min_response_time = response_time;
        }
        if (response_time > stats->max_response_time) {
            stats->max_response_time = response_time;
        }
    } else {
        stats->failed_requests++;
    }
    
    stats->total_bytes_sent += bytes_sent;
    stats->total_bytes_received += bytes_received;
    
    pthread_mutex_unlock(mutex);
}

// 分配缓冲区
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    client_connection_t* conn = (client_connection_t*)handle->data;
    
    if (conn->response_pos < conn->response_capacity) {
        buf->base = conn->response_buffer + conn->response_pos;
        buf->len = conn->response_capacity - conn->response_pos;
    } else {
        // 扩展缓冲区
        size_t new_capacity = conn->response_capacity * 2;
        char* new_buffer = realloc(conn->response_buffer, new_capacity);
        if (new_buffer) {
            conn->response_buffer = new_buffer;
            conn->response_capacity = new_capacity;
            buf->base = conn->response_buffer + conn->response_pos;
            buf->len = conn->response_capacity - conn->response_pos;
        } else {
            buf->base = NULL;
            buf->len = 0;
        }
    }
}

// HTTP响应读取回调
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_connection_t* conn = (client_connection_t*)stream->data;
    
    if (nread > 0) {
        conn->response_pos += nread;
        
        // 简单检查是否收到完整HTTP响应（检查\r\n\r\n）
        if (conn->response_pos >= 4) {
            char* end = strstr(conn->response_buffer, "\r\n\r\n");
            if (end) {
                // 响应完整
                double response_time = get_time_ms() - conn->request_start_time;
                update_stats(conn->stats, conn->stats_mutex, 1, response_time, 
                            strlen(conn->request_buffer), conn->response_pos);
                conn->response_received++;
                
                // 关闭连接
                uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
                conn->is_connected = 0;
                g_test_manager.active_connections--;
            }
        }
    } else if (nread < 0) {
        // 错误或连接关闭
        if (nread != UV_EOF) {
            update_stats(conn->stats, conn->stats_mutex, 0, 0, 0, 0);
        }
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
        conn->is_connected = 0;
        g_test_manager.active_connections--;
    }
}

// HTTP请求发送回调
static void on_write(uv_write_t* req, int status) {
    client_connection_t* conn = (client_connection_t*)req->data;
    
    if (status < 0) {
        update_stats(conn->stats, conn->stats_mutex, 0, 0, 0, 0);
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
        conn->is_connected = 0;
        g_test_manager.active_connections--;
        return;
    }
    
    conn->request_sent++;
    conn->request_start_time = get_time_ms();
    
    // 开始读取响应
    uv_read_start((uv_stream_t*)&conn->tcp_handle, alloc_buffer, on_read);
}

// 连接建立回调
static void on_connect(uv_connect_t* req, int status) {
    client_connection_t* conn = (client_connection_t*)req->data;
    
    double connect_time = get_time_ms() - conn->connect_start_time;
    
    if (status < 0) {
        update_stats(conn->stats, conn->stats_mutex, 0, 0, 0, 0);
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
        conn->is_error = 1;
        return;
    }
    
    conn->is_connected = 1;
    
    // 发送HTTP请求
    uv_buf_t buf = uv_buf_init(conn->request_buffer, strlen(conn->request_buffer));
    conn->write_req.data = conn;
    
    int write_result = uv_write(&conn->write_req, (uv_stream_t*)&conn->tcp_handle, 
                               &buf, 1, on_write);
    if (write_result < 0) {
        update_stats(conn->stats, conn->stats_mutex, 0, 0, 0, 0);
        uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
        conn->is_connected = 0;
        g_test_manager.active_connections--;
    }
}

// 创建新的客户端连接
static int create_connection(test_manager_t* manager) {
    if (manager->active_connections >= manager->max_active_connections) {
        return -1;
    }
    
    client_connection_t* conn = &manager->connections[manager->active_connections];
    memset(conn, 0, sizeof(client_connection_t));
    
    // 初始化TCP句柄
    int result = uv_tcp_init(manager->loop, &conn->tcp_handle);
    if (result < 0) {
        return -1;
    }
    
    conn->tcp_handle.data = conn;
    conn->config = manager->config;
    conn->stats = manager->stats;
    conn->stats_mutex = &manager->stats_mutex;
    conn->server_addr = *(struct sockaddr_in*)&manager->connections[0].server_addr;
    
    // 分配请求和响应缓冲区
    conn->request_buffer = malloc(4096);
    conn->response_buffer = malloc(4096);
    conn->response_capacity = 4096;
    
    if (!conn->request_buffer || !conn->response_buffer) {
        if (conn->request_buffer) free(conn->request_buffer);
        if (conn->response_buffer) free(conn->response_buffer);
        return -1;
    }
    
    // 构造HTTP请求
    snprintf(conn->request_buffer, 4096,
             "GET /stress_test HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "User-Agent: uvhttp-stress-test/1.0\r\n"
             "Accept: */*\r\n"
             "Connection: close\r\n"
             "Content-Length: %d\r\n"
             "\r\n",
             conn->config->payload_size);
    
    // 如果有payload，添加到请求中
    if (conn->config->payload_size > 0) {
        size_t header_len = strlen(conn->request_buffer);
        if (header_len + conn->config->payload_size < 4096) {
            memset(conn->request_buffer + header_len, 'A', conn->config->payload_size);
            conn->request_buffer[header_len + conn->config->payload_size] = '\0';
        }
    }
    
    // 开始连接
    conn->connect_req.data = conn;
    conn->connect_start_time = get_time_ms();
    
    result = uv_tcp_connect(&conn->connect_req, &conn->tcp_handle, 
                           (struct sockaddr*)&conn->server_addr, on_connect);
    if (result < 0) {
        free(conn->request_buffer);
        free(conn->response_buffer);
        return -1;
    }
    
    manager->active_connections++;
    return 0;
}

// 统计定时器回调
static void on_stats_timer(uv_timer_t* handle) {
    test_manager_t* manager = (test_manager_t*)handle->data;
    
    double current_time = get_time_ms();
    double elapsed = (current_time - manager->test_start_time) / 1000.0;
    
    pthread_mutex_lock(&manager->stats_mutex);
    
    manager->stats->test_duration = elapsed;
    if (elapsed > 0) {
        manager->stats->actual_rps = manager->stats->total_requests / elapsed;
    }
    
    printf("\r[%6.2fs] 连接: %d/%d, 请求: %d (成功: %d, 失败: %d), RPS: %.1f, "
           "响应时间: %.3f/%.3f/%.3f ms",
           elapsed,
           manager->active_connections, manager->max_active_connections,
           manager->stats->total_requests,
           manager->stats->successful_requests,
           manager->stats->failed_requests,
           manager->stats->actual_rps,
           manager->stats->min_response_time,
           manager->stats->total_requests > 0 ? manager->stats->total_response_time / manager->stats->successful_requests : 0,
           manager->stats->max_response_time);
    fflush(stdout);
    
    pthread_mutex_unlock(&manager->stats_mutex);
}

// 速率限制定时器回调
static void on_rate_limit_timer(uv_timer_t* handle) {
    test_manager_t* manager = (test_manager_t*)handle->data;
    
    if (manager->should_stop) {
        return;
    }
    
    // 计算需要创建的连接数
    int target_connections_per_second = manager->target_requests_per_second;
    int connections_to_create = target_connections_per_second / 10; // 每100ms创建
    
    for (int i = 0; i < connections_to_create; i++) {
        if (create_connection(manager) < 0) {
            break;
        }
    }
}

// 运行压力测试
static int run_stress_test(stress_test_config_t* config) {
    printf("\n=== %s ===\n", config->test_name);
    printf("最大连接数: %d\n", config->max_connections);
    printf("测试持续时间: %d 秒\n", config->test_duration_seconds);
    printf("目标RPS: %d\n", config->target_rps);
    printf("负载大小: %d 字节\n", config->payload_size);
    
    // 初始化测试管理器
    memset(&g_test_manager, 0, sizeof(test_manager_t));
    g_test_manager.config = config;
    g_test_manager.max_active_connections = config->max_connections;
    g_test_manager.target_requests_per_second = config->target_rps;
    g_test_manager.test_start_time = get_time_ms();
    
    // 分配连接数组
    g_test_manager.connections = calloc(config->max_connections, sizeof(client_connection_t));
    if (!g_test_manager.connections) {
        fprintf(stderr, "内存分配失败\n");
        return -1;
    }
    
    // 初始化统计
    stress_stats_t stats = {0};
    g_test_manager.stats = &stats;
    
    // 初始化互斥锁
    pthread_mutex_init(&g_test_manager.stats_mutex, NULL);
    
    // 初始化libuv循环
    g_test_manager.loop = uv_default_loop();
    
    // 设置服务器地址
    struct sockaddr_in* server_addr = &g_test_manager.connections[0].server_addr;
    uv_ip4_addr("127.0.0.1", 8080, server_addr);
    
    // 初始化统计定时器
    uv_timer_init(g_test_manager.loop, &g_test_manager.stats_timer);
    g_test_manager.stats_timer.data = &g_test_manager;
    uv_timer_start(&g_test_manager.stats_timer, on_stats_timer, 1000, 1000); // 每秒更新
    
    // 初始化速率限制定时器
    uv_timer_init(g_test_manager.loop, &g_test_manager.rate_limit_timer);
    g_test_manager.rate_limit_timer.data = &g_test_manager;
    uv_timer_start(&g_test_manager.rate_limit_timer, on_rate_limit_timer, 100, 100); // 每100ms检查
    
    size_t initial_memory = get_memory_usage();
    
    printf("开始压力测试...\n");
    
    // 运行事件循环
    uv_run(g_test_manager.loop, UV_RUN_DEFAULT);
    
    // 清理资源
    uv_timer_stop(&g_test_manager.stats_timer);
    uv_timer_stop(&g_test_manager.rate_limit_timer);
    
    size_t final_memory = get_memory_usage();
    
    // 输出最终结果
    printf("\n\n--- %s 最终结果 ---\n", config->test_name);
    printf("测试持续时间: %.2f 秒\n", stats.test_duration);
    printf("总请求数: %d\n", stats.total_requests);
    printf("成功请求: %d (%.1f%%)\n", stats.successful_requests,
           stats.total_requests > 0 ? (double)stats.successful_requests / stats.total_requests * 100.0 : 0.0);
    printf("失败请求: %d (%.1f%%)\n", stats.failed_requests,
           stats.total_requests > 0 ? (double)stats.failed_requests / stats.total_requests * 100.0 : 0.0);
    printf("目标RPS: %d\n", config->target_rps);
    printf("实际RPS: %.1f\n", stats.actual_rps);
    printf("RPS达成率: %.1f%%\n", config->target_rps > 0 ? (double)stats.actual_rps / config->target_rps * 100.0 : 0.0);
    printf("平均响应时间: %.3f ms\n", 
           stats.successful_requests > 0 ? stats.total_response_time / stats.successful_requests : 0.0);
    printf("最小响应时间: %.3f ms\n", stats.min_response_time);
    printf("最大响应时间: %.3f ms\n", stats.max_response_time);
    printf("总发送字节: %zu\n", stats.total_bytes_sent);
    printf("总接收字节: %zu\n", stats.total_bytes_received);
    printf("内存使用变化: %ld KB\n", final_memory - initial_memory);
    
    // 清理连接资源
    for (int i = 0; i < config->max_connections; i++) {
        if (g_test_manager.connections[i].request_buffer) {
            free(g_test_manager.connections[i].request_buffer);
        }
        if (g_test_manager.connections[i].response_buffer) {
            free(g_test_manager.connections[i].response_buffer);
        }
    }
    
    free(g_test_manager.connections);
    pthread_mutex_destroy(&g_test_manager.stats_mutex);
    
    return 0;
}

// 测试超时处理
static void on_test_timeout(uv_timer_t* handle) {
    test_manager_t* manager = (test_manager_t*)handle->data;
    manager->should_stop = 1;
    uv_stop(manager->loop);
}

// 信号处理
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_test_manager.should_stop = 1;
        uv_stop(g_test_manager.loop);
        printf("\n收到停止信号，正在停止测试...\n");
    }
}

int main(int argc, char* argv[]) {
    printf("UVHTTP libuv 压力测试\n");
    printf("====================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("系统信息:\n");
    printf("  CPU核心数: %d\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  页面大小: %d KB\n", sysconf(_SC_PAGESIZE) / 1024);
    printf("  总内存: %ld MB\n", sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024));
    
    // 测试配置
    stress_test_config_t tests[] = {
        {
            .max_connections = 100,
            .test_duration_seconds = 30,
            .target_rps = 500,
            .payload_size = 1024,
            .test_name = "轻量级负载测试"
        },
        {
            .max_connections = 500,
            .test_duration_seconds = 30,
            .target_rps = 2000,
            .payload_size = 1024,
            .test_name = "中等负载测试"
        },
        {
            .max_connections = 1000,
            .test_duration_seconds = 60,
            .target_rps = 5000,
            .payload_size = 1024,
            .test_name = "高负载测试"
        },
        {
            .max_connections = 500,
            .test_duration_seconds = 30,
            .target_rps = 2000,
            .payload_size = 10240,
            .test_name = "大负载测试"
        }
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    // 运行所有测试
    for (int i = 0; i < num_tests; i++) {
        int result = run_stress_test(&tests[i]);
        
        if (result < 0) {
            fprintf(stderr, "测试 %s 失败\n", tests[i].test_name);
            break;
        }
        
        if (i < num_tests - 1) {
            printf("\n等待 5 秒后进行下一个测试...\n");
            sleep(5);
        }
    }
    
    printf("\nlibuv 压力测试完成！\n");
    
    return 0;
}