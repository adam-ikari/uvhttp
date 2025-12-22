#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <sys/resource.h>
#include "include/uvhttp_logging.h"
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_request_simple.h"

// 吞吐量测试配置
typedef struct {
    int target_rps;
    int test_duration_seconds;
    int payload_size_bytes;
    int concurrent_workers;
    char* test_name;
} throughput_test_config_t;

// 吞吐量统计
typedef struct {
    int total_requests;
    int successful_requests;
    int failed_requests;
    double total_bytes_sent;
    double test_duration;
    double actual_rps;
    double throughput_mbps;
    double avg_response_time;
    double min_response_time;
    double max_response_time;
    time_t start_time;
    time_t end_time;
} throughput_stats_t;

// 工作线程数据
typedef struct {
    int worker_id;
    throughput_test_config_t* config;
    throughput_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    volatile int* should_stop;
    double requests_per_second;
    int requests_sent;
} worker_data_t;

// 全局控制
static volatile int g_should_stop = 0;
static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// 获取当前时间戳（毫秒）
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 生成测试负载
static void generate_payload(char* buffer, size_t size) {
    const char* pattern = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t pattern_len = strlen(pattern);
    
    for (size_t i = 0; i < size; i++) {
        buffer[i] = pattern[i % pattern_len];
    }
    buffer[size - 1] = '\0';
}

// 模拟HTTP请求处理
static int process_request(worker_data_t* data, char* payload, size_t payload_size) {
    uvhttp_perf_timer_t request_timer;
    uvhttp_perf_start(&request_timer, "request_processing");
    
    // 创建请求和响应对象
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    if (uvhttp_request_init(&request, (void*)0x1) != 0) {
        return -1;
    }
    
    if (uvhttp_response_init(&response, (void*)0x1) != 0) {
        uvhttp_request_cleanup(&request);
        return -1;
    }
    
    // 设置响应
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    uvhttp_response_set_header(&response, "Connection", "keep-alive");
    
    // 构建JSON响应
    char json_response[1024];
    snprintf(json_response, sizeof(json_response),
        "{"
        "\"status\": \"ok\","
        "\"worker_id\": %d,"
        "\"payload_size\": %zu,"
        "\"timestamp\": %ld,"
        "\"request_id\": %ld"
        "}",
        data->worker_id,
        payload_size,
        time(NULL),
        random());
    
    if (uvhttp_response_set_body(&response, json_response, strlen(json_response)) != 0) {
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    // 模拟处理延迟（根据负载大小调整）
    int processing_delay = payload_size / 100; // 每100字节1ms
    if (processing_delay > 0 && processing_delay < 100) {
        usleep(processing_delay * 1000);
    }
    
    uvhttp_perf_end(&request_timer);
    double response_time = uvhttp_perf_get_duration(&request_timer);
    
    // 更新统计信息
    pthread_mutex_lock(data->stats_mutex);
    data->stats->total_requests++;
    data->stats->successful_requests++;
    data->stats->total_bytes_sent += strlen(json_response);
    
    // 更新响应时间统计
    if (data->stats->min_response_time == 0 || response_time < data->stats->min_response_time) {
        data->stats->min_response_time = response_time;
    }
    if (response_time > data->stats->max_response_time) {
        data->stats->max_response_time = response_time;
    }
    
    pthread_mutex_unlock(data->stats_mutex);
    
    // 清理资源
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    
    return 0;
}

// 吞吐量工作线程
static void* throughput_worker(void* arg) {
    worker_data_t* data = (worker_data_t*)arg;
    
    UVHTTP_LOG_DEBUG("Worker %d started, target RPS: %.2f", 
                     data->worker_id, data->requests_per_second);
    
    // 分配负载缓冲区
    char* payload = malloc(data->config->payload_size_bytes);
    if (!payload) {
        UVHTTP_LOG_ERROR("Failed to allocate payload buffer");
        return NULL;
    }
    
    generate_payload(payload, data->config->payload_size_bytes);
    
    // 计算请求间隔
    double request_interval = 1000.0 / data->requests_per_second; // 毫秒
    
    while (!(*data->should_stop)) {
        double start_time = get_time_ms();
        
        // 处理请求
        if (process_request(data, payload, data->config->payload_size_bytes) == 0) {
            data->requests_sent++;
        } else {
            pthread_mutex_lock(data->stats_mutex);
            data->stats->failed_requests++;
            pthread_mutex_unlock(data->stats_mutex);
        }
        
        // 控制请求速率
        double elapsed = get_time_ms() - start_time;
        if (elapsed < request_interval) {
            usleep((request_interval - elapsed) * 1000); // 转换为微秒
        }
    }
    
    free(payload);
    UVHTTP_LOG_DEBUG("Worker %d completed %d requests", 
                     data->worker_id, data->requests_sent);
    return NULL;
}

// 运行吞吐量测试
static int run_throughput_test(throughput_test_config_t* config, throughput_stats_t* stats) {
    printf("开始吞吐量测试: %s\n", config->test_name);
    printf("  目标RPS: %d\n", config->target_rps);
    printf("  负载大小: %d bytes\n", config->payload_size_bytes);
    printf("  并发工作线程: %d\n", config->concurrent_workers);
    printf("  测试时长: %d 秒\n", config->test_duration_seconds);
    
    // 初始化统计
    memset(stats, 0, sizeof(throughput_stats_t));
    stats->min_response_time = 0;
    
    // 创建工作线程
    pthread_t* threads = malloc(config->concurrent_workers * sizeof(pthread_t));
    worker_data_t* worker_data = malloc(config->concurrent_workers * sizeof(worker_data_t));
    
    if (!threads || !worker_data) {
        UVHTTP_LOG_ERROR("Failed to allocate memory for workers");
        return -1;
    }
    
    // 计算每个工作线程的RPS
    double rps_per_worker = (double)config->target_rps / config->concurrent_workers;
    
    // 启动工作线程
    for (int i = 0; i < config->concurrent_workers; i++) {
        worker_data[i].worker_id = i;
        worker_data[i].config = config;
        worker_data[i].stats = stats;
        worker_data[i].stats_mutex = &g_stats_mutex;
        worker_data[i].should_stop = &g_should_stop;
        worker_data[i].requests_per_second = rps_per_worker;
        worker_data[i].requests_sent = 0;
        
        if (pthread_create(&threads[i], NULL, throughput_worker, &worker_data[i]) != 0) {
            UVHTTP_LOG_ERROR("Failed to create worker thread %d", i);
            g_should_stop = 1;
            break;
        }
    }
    
    // 设置测试开始时间
    stats->start_time = time(NULL);
    
    // 运行测试
    sleep(config->test_duration_seconds);
    g_should_stop = 1;
    
    // 等待所有线程完成
    for (int i = 0; i < config->concurrent_workers; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 设置测试结束时间
    stats->end_time = time(NULL);
    stats->test_duration = difftime(stats->end_time, stats->start_time);
    
    // 计算实际RPS和吞吐量
    if (stats->test_duration > 0) {
        stats->actual_rps = stats->total_requests / stats->test_duration;
        stats->throughput_mbps = (stats->total_bytes_sent / 1024.0 / 1024.0) / stats->test_duration;
        stats->avg_response_time = stats->successful_requests > 0 ? 
                               (stats->min_response_time + stats->max_response_time) / 2 : 0;
    }
    
    // 清理资源
    free(threads);
    free(worker_data);
    
    return 0;
}

// 打印吞吐量测试结果
static void print_throughput_results(throughput_stats_t* stats, throughput_test_config_t* config) {
    double success_rate = stats->total_requests > 0 ? 
                        (double)stats->successful_requests / stats->total_requests * 100 : 0;
    double rps_efficiency = config->target_rps > 0 ? 
                          (stats->actual_rps / config->target_rps) * 100 : 0;
    
    printf("\n============================================================\n");
    printf("                    吞吐量测试结果: %s\n", config->test_name);
    printf("============================================================\n");
    
    printf("测试配置:\n");
    printf("  目标RPS: %d\n", config->target_rps);
    printf("  实际RPS: %.2f\n", stats->actual_rps);
    printf("  RPS效率: %.1f%%\n", rps_efficiency);
    printf("  负载大小: %d bytes\n", config->payload_size_bytes);
    printf("  并发线程: %d\n", config->concurrent_workers);
    printf("  测试时长: %.2f 秒\n", stats->test_duration);
    
    printf("\n性能指标:\n");
    printf("  总请求数: %d\n", stats->total_requests);
    printf("  成功请求数: %d\n", stats->successful_requests);
    printf("  失败请求数: %d\n", stats->failed_requests);
    printf("  成功率: %.2f%%\n", success_rate);
    printf("  平均响应时间: %.2f ms\n", stats->avg_response_time);
    printf("  最小响应时间: %.2f ms\n", stats->min_response_time);
    printf("  最大响应时间: %.2f ms\n", stats->max_response_time);
    printf("  吞吐量: %.2f MB/s\n", stats->throughput_mbps);
    printf("  总数据传输: %.2f MB\n", stats->total_bytes_sent / 1024.0 / 1024.0);
    
    // 性能评级
    printf("\n性能评级:\n");
    if (rps_efficiency >= 95.0) {
        printf("  🚀 优秀 (≥95%% 效率)\n");
    } else if (rps_efficiency >= 80.0) {
        printf("  ✅ 良好 (80-95%% 效率)\n");
    } else if (rps_efficiency >= 60.0) {
        printf("  ⚠️  一般 (60-80%% 效率)\n");
    } else {
        printf("  ❌ 需要优化 (<60%% 效率)\n");
    }
    
    if (stats->max_response_time <= 10.0) {
        printf("  ⚡ 低延迟 (≤10ms)\n");
    } else if (stats->max_response_time <= 50.0) {
        printf("  ✅ 中等延迟 (10-50ms)\n");
    } else {
        printf("  ⚠️ 高延迟 (>50ms)\n");
    }
    
    printf("============================================================\n");
}

// RPS基准测试
int test_rps_benchmark(void) {
    printf("开始RPS基准测试...\n");
    
    int rps_targets[] = {1000, 5000, 10000, 20000, 50000};
    int num_tests = sizeof(rps_targets) / sizeof(rps_targets[0]);
    
    for (int i = 0; i < num_tests; i++) {
        throughput_test_config_t config = {
            .target_rps = rps_targets[i],
            .test_duration_seconds = 30,
            .payload_size_bytes = 1024,
            .concurrent_workers = 10,
            .test_name = "RPS基准测试"
        };
        
        throughput_stats_t stats;
        
        if (run_throughput_test(&config, &stats) != 0) {
            UVHTTP_LOG_ERROR("RPS benchmark test %d failed", i + 1);
            return -1;
        }
        
        // 短暂休息
        sleep(2);
    }
    
    return 0;
}

// 负载大小测试
int test_payload_size_scaling(void) {
    printf("开始负载大小扩展测试...\n");
    
    int payload_sizes[] = {64, 256, 1024, 4096, 16384, 65536};
    int num_tests = sizeof(payload_sizes) / sizeof(payload_sizes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        char test_name[64];
        snprintf(test_name, sizeof(test_name), "负载测试_%dKB", payload_sizes[i] / 1024);
        
        throughput_test_config_t config = {
            .target_rps = 5000,
            .test_duration_seconds = 20,
            .payload_size_bytes = payload_sizes[i],
            .concurrent_workers = 5,
            .test_name = test_name
        };
        
        throughput_stats_t stats;
        
        if (run_throughput_test(&config, &stats) != 0) {
            UVHTTP_LOG_ERROR("Payload size test %d failed", i + 1);
            return -1;
        }
        
        // 短暂休息
        sleep(1);
    }
    
    return 0;
}

// 并发度扩展测试
int test_concurrency_scaling(void) {
    printf("开始并发度扩展测试...\n");
    
    int worker_counts[] = {1, 2, 5, 10, 20, 50};
    int num_tests = sizeof(worker_counts) / sizeof(worker_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        char test_name[64];
        snprintf(test_name, sizeof(test_name), "并发度测试_%d线程", worker_counts[i]);
        
        throughput_test_config_t config = {
            .target_rps = 10000,
            .test_duration_seconds = 25,
            .payload_size_bytes = 2048,
            .concurrent_workers = worker_counts[i],
            .test_name = test_name
        };
        
        throughput_stats_t stats;
        
        if (run_throughput_test(&config, &stats) != 0) {
            UVHTTP_LOG_ERROR("Concurrency scaling test %d failed", i + 1);
            return -1;
        }
        
        // 短暂休息
        sleep(1);
    }
    
    return 0;
}

// 极限吞吐量测试
int test_max_throughput(void) {
    printf("开始极限吞吐量测试...\n");
    
    throughput_test_config_t max_config = {
        .target_rps = 100000,
        .test_duration_seconds = 60,
        .payload_size_bytes = 512,
        .concurrent_workers = 50,
        .test_name = "极限吞吐量测试"
    };
    
    throughput_stats_t stats;
    return run_throughput_test(&max_config, &stats);
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    uvhttp_log_init(UVHTTP_LOG_INFO);
    uvhttp_mem_reset_stats();
    
    printf("=== UVHTTP 吞吐量测试套件 ===\n");
    printf("测试开始时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 重置停止标志
    g_should_stop = 0;
    
    // 执行所有吞吐量测试
    if (test_rps_benchmark() != 0) {
        UVHTTP_LOG_ERROR("RPS benchmark failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (test_payload_size_scaling() != 0) {
        UVHTTP_LOG_ERROR("Payload size scaling test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (test_concurrency_scaling() != 0) {
        UVHTTP_LOG_ERROR("Concurrency scaling test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (test_max_throughput() != 0) {
        UVHTTP_LOG_ERROR("Max throughput test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    printf("\n=== 吞吐量测试完成 ===\n");
    printf("测试结束时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 打印最终内存统计
    uvhttp_mem_stats_t mem_stats;
    uvhttp_mem_get_stats(&mem_stats);
    printf("\n最终内存统计:\n");
    printf("  总分配: %zu bytes\n", mem_stats.total_allocated);
    printf("  当前使用: %zu bytes\n", mem_stats.current_usage);
    printf("  峰值使用: %zu bytes\n", mem_stats.peak_usage);
    printf("  分配次数: %zu\n", mem_stats.allocation_count);
    
    uvhttp_log_cleanup();
    return 0;
}