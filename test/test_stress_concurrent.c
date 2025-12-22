#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "include/uvhttp_logging.h"
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_request_simple.h"

// 压力测试配置
typedef struct {
    int concurrent_connections;
    int requests_per_connection;
    int test_duration_seconds;
    int warmup_seconds;
    char* server_host;
    int server_port;
} stress_test_config_t;

// 测试统计信息
typedef struct {
    int total_requests;
    int successful_requests;
    int failed_requests;
    double total_response_time;
    double min_response_time;
    double max_response_time;
    double total_bytes_sent;
    size_t peak_memory_usage;
    int peak_connections;
    time_t start_time;
    time_t end_time;
} stress_test_stats_t;

// 线程数据
typedef struct {
    int thread_id;
    stress_test_config_t* config;
    stress_test_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    volatile int* should_stop;
} thread_data_t;

// 全局测试控制
static volatile int g_should_stop = 0;
static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// 获取当前时间戳（毫秒）
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 模拟HTTP请求处理
static int simulate_request(thread_data_t* data) {
    uvhttp_perf_timer_t request_timer;
    uvhttp_perf_start(&request_timer, "http_request");
    
    // 创建模拟请求和响应
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    if (uvhttp_request_init(&request, (void*)0x1) != 0) {
        return -1;
    }
    
    if (uvhttp_response_init(&response, (void*)0x1) != 0) {
        uvhttp_request_cleanup(&request);
        return -1;
    }
    
    // 模拟请求处理
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    
    // 生成JSON响应
    const char* json_body = "{"
        "\"status\": \"ok\","
        "\"message\": \"Request processed successfully\","
        "\"timestamp\": \"$(date)\","
        "\"thread_id\": $(thread_id),"
        "\"request_id\": $(request_id)"
        "}";
    
    if (uvhttp_response_set_body(&response, json_body, strlen(json_body)) != 0) {
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    // 模拟网络延迟（1-5ms）
    usleep(1000 + (rand() % 4000));
    
    // 记录响应时间
    uvhttp_perf_end(&request_timer);
    double response_time = uvhttp_perf_get_duration(&request_timer);
    
    // 更新统计信息
    pthread_mutex_lock(data->stats_mutex);
    data->stats->total_requests++;
    data->stats->successful_requests++;
    data->stats->total_response_time += response_time;
    
    if (response_time < data->stats->min_response_time || data->stats->min_response_time == 0) {
        data->stats->min_response_time = response_time;
    }
    if (response_time > data->stats->max_response_time) {
        data->stats->max_response_time = response_time;
    }
    
    data->stats->total_bytes_sent += strlen(json_body);
    
    pthread_mutex_unlock(data->stats_mutex);
    
    // 清理资源
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    
    return 0;
}

// 工作线程函数
static void* worker_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    stress_test_config_t* config = data->config;
    
    UVHTTP_LOG_INFO("Worker thread %d started", data->thread_id);
    
    // 预热阶段
    sleep(config->warmup_seconds);
    
    int requests_sent = 0;
    while (!(*data->should_stop && requests_sent < config->requests_per_connection)) {
        if (simulate_request(data) == 0) {
            requests_sent++;
        } else {
            pthread_mutex_lock(data->stats_mutex);
            data->stats->failed_requests++;
            pthread_mutex_unlock(data->stats_mutex);
        }
        
        // 短暂休息，模拟真实场景
        usleep(1000); // 1ms
    }
    
    UVHTTP_LOG_INFO("Worker thread %d completed %d requests", data->thread_id, requests_sent);
    return NULL;
}

// 打印测试结果
static void print_test_results(stress_test_stats_t* stats, stress_test_config_t* config) {
    double duration = difftime(stats->end_time, stats->start_time);
    double avg_response_time = stats->successful_requests > 0 ? 
                           stats->total_response_time / stats->successful_requests : 0;
    double success_rate = stats->total_requests > 0 ? 
                        (double)stats->successful_requests / stats->total_requests * 100 : 0;
    double rps = duration > 0 ? stats->total_requests / duration : 0;
    double throughput_mbps = duration > 0 ? 
                           (stats->total_bytes_sent / 1024.0 / 1024.0) / duration : 0;
    
    printf("\n" "============================================================\n");
    printf("                    压力测试结果报告\n");
    printf("============================================================\n");
    
    printf("测试配置:\n");
    printf("  并发连接数: %d\n", config->concurrent_connections);
    printf("  每连接请求数: %d\n", config->requests_per_connection);
    printf("  测试持续时间: %d 秒\n", config->test_duration_seconds);
    printf("  预热时间: %d 秒\n", config->warmup_seconds);
    
    printf("\n性能指标:\n");
    printf("  总请求数: %d\n", stats->total_requests);
    printf("  成功请求数: %d\n", stats->successful_requests);
    printf("  失败请求数: %d\n", stats->failed_requests);
    printf("  成功率: %.2f%%\n", success_rate);
    printf("  平均响应时间: %.2f ms\n", avg_response_time);
    printf("  最小响应时间: %.2f ms\n", stats->min_response_time);
    printf("  最大响应时间: %.2f ms\n", stats->max_response_time);
    printf("  请求速率: %.2f RPS\n", rps);
    printf("  吞吐量: %.2f MB/s\n", throughput_mbps);
    
    printf("\n资源使用:\n");
    printf("  总数据传输: %.2f MB\n", stats->total_bytes_sent / 1024.0 / 1024.0);
    printf("  峰值内存使用: %zu bytes\n", stats->peak_memory_usage);
    printf("  峰值连接数: %d\n", stats->peak_connections);
    
    printf("\n测试时间:\n");
    printf("  开始时间: %s", ctime(&stats->start_time));
    printf("  结束时间: %s", ctime(&stats->end_time));
    printf("  实际持续时间: %.2f 秒\n", duration);
    
    // 性能评级
    printf("\n性能评级:\n");
    if (rps >= 10000) {
        printf("  🚀 优秀 (>10K RPS)\n");
    } else if (rps >= 5000) {
        printf("  ✅ 良好 (5K-10K RPS)\n");
    } else if (rps >= 1000) {
        printf("  ⚠️  一般 (1K-5K RPS)\n");
    } else {
        printf("  ❌ 需要优化 (<1K RPS)\n");
    }
    
    if (success_rate >= 99.0) {
        printf("  🛡️ 可靠性优秀 (>99%%)\n");
    } else if (success_rate >= 95.0) {
        printf("  ✅ 可靠性良好 (95-99%%)\n");
    } else {
        printf("  ⚠️  可靠性需要改进 (<95%%)\n");
    }
    
    printf("============================================================\n");
}

// 并发连接压力测试
int test_concurrent_connections(stress_test_config_t* config) {
    printf("开始并发连接压力测试...\n");
    UVHTTP_LOG_INFO("Starting concurrent connections test with %d connections", config->concurrent_connections);
    
    // 初始化统计信息
    stress_test_stats_t stats = {0};
    stats.min_response_time = 0;
    
    // 创建线程
    pthread_t* threads = malloc(config->concurrent_connections * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(config->concurrent_connections * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        UVHTTP_LOG_ERROR("Failed to allocate memory for threads");
        return -1;
    }
    
    // 设置测试开始时间
    stats.start_time = time(NULL);
    
    // 启动工作线程
    for (int i = 0; i < config->concurrent_connections; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].config = config;
        thread_data[i].stats = &stats;
        thread_data[i].stats_mutex = &g_stats_mutex;
        thread_data[i].should_stop = &g_should_stop;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            UVHTTP_LOG_ERROR("Failed to create thread %d", i);
            g_should_stop = 1;
            break;
        }
    }
    
    // 等待测试完成
    sleep(config->test_duration_seconds + config->warmup_seconds);
    g_should_stop = 1;
    
    // 等待所有线程完成
    for (int i = 0; i < config->concurrent_connections; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 设置测试结束时间
    stats.end_time = time(NULL);
    stats.peak_connections = config->concurrent_connections;
    
    // 获取内存统计
    uvhttp_mem_stats_t mem_stats;
    uvhttp_mem_get_stats(&mem_stats);
    stats.peak_memory_usage = mem_stats.peak_usage;
    
    // 打印结果
    print_test_results(&stats, config);
    
    // 清理资源
    free(threads);
    free(thread_data);
    
    return 0;
}

// 渐进式压力测试
int test_progressive_load(void) {
    printf("\n开始渐进式负载测试...\n");
    
    int connection_counts[] = {100, 500, 1000, 2000};
    int num_tests = sizeof(connection_counts) / sizeof(connection_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        stress_test_config_t config = {
            .concurrent_connections = connection_counts[i],
            .requests_per_connection = 10,
            .test_duration_seconds = 30,
            .warmup_seconds = 5,
            .server_host = "localhost",
            .server_port = 8080
        };
        
        printf("\n测试阶段 %d/%d: %d 并发连接\n", i + 1, num_tests, connection_counts[i]);
        
        if (test_concurrent_connections(&config) != 0) {
            printf("测试阶段 %d 失败\n", i + 1);
            return -1;
        }
        
        // 短暂休息，让系统恢复
        sleep(5);
    }
    
    return 0;
}

// 极限压力测试
int test_extreme_load(void) {
    printf("\n开始极限压力测试...\n");
    
    stress_test_config_t extreme_config = {
        .concurrent_connections = 5000,
        .requests_per_connection = 50,
        .test_duration_seconds = 60,
        .warmup_seconds = 10,
        .server_host = "localhost",
        .server_port = 8080
    };
    
    return test_concurrent_connections(&extreme_config);
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    uvhttp_log_init(UVHTTP_LOG_INFO);
    uvhttp_mem_reset_stats();
    
    printf("=== UVHTTP 压力测试套件 ===\n");
    printf("测试开始时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 重置停止标志
    g_should_stop = 0;
    
    // 执行渐进式测试
    if (test_progressive_load() != 0) {
        UVHTTP_LOG_ERROR("Progressive load test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    // 执行极限测试
    if (test_extreme_load() != 0) {
        UVHTTP_LOG_ERROR("Extreme load test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    printf("\n=== 压力测试完成 ===\n");
    printf("测试结束时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 打印最终内存统计
    uvhttp_mem_stats_t final_mem_stats;
    uvhttp_mem_get_stats(&final_mem_stats);
    printf("\n最终内存统计:\n");
    printf("  总分配: %zu bytes\n", final_mem_stats.total_allocated);
    printf("  当前使用: %zu bytes\n", final_mem_stats.current_usage);
    printf("  峰值使用: %zu bytes\n", final_mem_stats.peak_usage);
    printf("  分配次数: %zu\n", final_mem_stats.allocation_count);
    
    uvhttp_log_cleanup();
    return 0;
}