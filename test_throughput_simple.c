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

// 吞吐量测试配置
typedef struct {
    int target_rps;
    int test_duration_seconds;
    int payload_size;
    int concurrent_workers;
    char* test_name;
} throughput_config_t;

// 吞吐量统计
typedef struct {
    int total_requests;
    int successful_requests;
    int failed_requests;
    double total_bytes_processed;
    double test_duration;
    double actual_rps;
    double throughput_mbps;
    double avg_response_time;
    double min_response_time;
    double max_response_time;
} throughput_stats_t;

// 工作线程数据
typedef struct {
    int worker_id;
    throughput_config_t* config;
    throughput_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    volatile int* should_stop;
    int requests_processed;
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

// 获取内存使用量（KB）
static size_t get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// 模拟HTTP请求处理
static int simulate_http_request(int payload_size) {
    // 模拟请求解析
    usleep(100); // 0.1ms 解析时间
    
    // 模拟业务逻辑处理
    if (payload_size > 0) {
        // 分配内存处理payload
        char* payload = malloc(payload_size);
        if (payload) {
            // 模拟数据处理
            memset(payload, 'A', payload_size - 1);
            payload[payload_size - 1] = '\0';
            
            // 模拟处理时间（基于payload大小）
            usleep(payload_size / 100); // 每100字节需要0.01ms
            
            free(payload);
        } else {
            return -1; // 内存分配失败
        }
    }
    
    // 模拟响应生成
    usleep(200); // 0.2ms 响应时间
    
    return 0; // 成功
}

// 工作线程函数
static void* worker_thread(void* arg) {
    worker_data_t* data = (worker_data_t*)arg;
    throughput_stats_t local_stats = {0};
    local_stats.min_response_time = 1e9;
    
    printf("工作线程 %d 开始运行\n", data->worker_id);
    
    double start_time = get_time_ms();
    double last_request_time = start_time;
    
    while (!(*data->should_stop)) {
        double current_time = get_time_ms();
        
        // 控制请求速率
        double expected_interval = 1000.0 / (data->config->target_rps / data->config->concurrent_workers);
        if (current_time - last_request_time < expected_interval) {
            usleep((expected_interval - (current_time - last_request_time)) * 1000 / 2);
            continue;
        }
        
        // 处理请求
        double request_start = get_time_ms();
        int result = simulate_http_request(data->config->payload_size);
        double request_end = get_time_ms();
        
        double response_time = request_end - request_start;
        last_request_time = request_start;
        
        local_stats.total_requests++;
        
        if (result == 0) {
            local_stats.successful_requests++;
            local_stats.total_bytes_processed += data->config->payload_size * 2; // 请求 + 响应
            
            // 更新响应时间统计
            if (response_time < local_stats.min_response_time) {
                local_stats.min_response_time = response_time;
            }
            if (response_time > local_stats.max_response_time) {
                local_stats.max_response_time = response_time;
            }
        } else {
            local_stats.failed_requests++;
        }
        
        data->requests_processed++;
        
        // 检查测试是否应该结束
        if ((current_time - start_time) / 1000.0 >= data->config->test_duration_seconds) {
            break;
        }
    }
    
    // 更新全局统计
    pthread_mutex_lock(data->stats_mutex);
    
    data->stats->total_requests += local_stats.total_requests;
    data->stats->successful_requests += local_stats.successful_requests;
    data->stats->failed_requests += local_stats.failed_requests;
    data->stats->total_bytes_processed += local_stats.total_bytes_processed;
    
    if (local_stats.min_response_time < data->stats->min_response_time || 
        data->stats->min_response_time == 0) {
        data->stats->min_response_time = local_stats.min_response_time;
    }
    
    if (local_stats.max_response_time > data->stats->max_response_time) {
        data->stats->max_response_time = local_stats.max_response_time;
    }
    
    pthread_mutex_unlock(data->stats_mutex);
    
    printf("工作线程 %d 完成，处理了 %d 个请求\n", data->worker_id, data->requests_processed);
    return NULL;
}

// 运行吞吐量测试
static void run_throughput_test(throughput_config_t* config) {
    printf("\n=== %s ===\n", config->test_name);
    printf("目标RPS: %d\n", config->target_rps);
    printf("测试持续时间: %d 秒\n", config->test_duration_seconds);
    printf("负载大小: %d 字节\n", config->payload_size);
    printf("并发工作线程: %d\n", config->concurrent_workers);
    
    throughput_stats_t stats = {0};
    stats.min_response_time = 1e9;
    
    size_t initial_memory = get_memory_usage();
    double test_start = get_time_ms();
    
    // 创建工作线程
    pthread_t* threads = malloc(config->concurrent_workers * sizeof(pthread_t));
    worker_data_t* worker_data = malloc(config->concurrent_workers * sizeof(worker_data_t));
    pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    volatile int should_stop = 0;
    
    // 启动工作线程
    for (int i = 0; i < config->concurrent_workers; i++) {
        worker_data[i].worker_id = i;
        worker_data[i].config = config;
        worker_data[i].stats = &stats;
        worker_data[i].stats_mutex = &stats_mutex;
        worker_data[i].should_stop = &should_stop;
        worker_data[i].requests_processed = 0;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &worker_data[i]) != 0) {
            fprintf(stderr, "创建工作线程 %d 失败\n", i);
            should_stop = 1;
            break;
        }
    }
    
    // 等待测试完成
    sleep(config->test_duration_seconds);
    should_stop = 1;
    
    // 等待所有线程结束
    for (int i = 0; i < config->concurrent_workers; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double test_end = get_time_ms();
    size_t final_memory = get_memory_usage();
    
    // 计算最终统计
    stats.test_duration = (test_end - test_start) / 1000.0;
    stats.actual_rps = stats.total_requests / stats.test_duration;
    
    if (stats.successful_requests > 0) {
        stats.avg_response_time = (stats.min_response_time + stats.max_response_time) / 2.0;
    }
    
    // 计算吞吐量 (Mbps)
    stats.throughput_mbps = (stats.total_bytes_processed * 8) / (stats.test_duration * 1024 * 1024);
    
    // 输出结果
    printf("\n--- %s 结果 ---\n", config->test_name);
    printf("总请求数: %d\n", stats.total_requests);
    printf("成功请求: %d (%.1f%%)\n", stats.successful_requests,
           stats.total_requests > 0 ? (double)stats.successful_requests / stats.total_requests * 100.0 : 0.0);
    printf("失败请求: %d (%.1f%%)\n", stats.failed_requests,
           stats.total_requests > 0 ? (double)stats.failed_requests / stats.total_requests * 100.0 : 0.0);
    printf("目标RPS: %d\n", config->target_rps);
    printf("实际RPS: %.1f\n", stats.actual_rps);
    printf("RPS达成率: %.1f%%\n", (double)stats.actual_rps / config->target_rps * 100.0);
    printf("平均响应时间: %.3f ms\n", stats.avg_response_time);
    printf("最小响应时间: %.3f ms\n", stats.min_response_time);
    printf("最大响应时间: %.3f ms\n", stats.max_response_time);
    printf("吞吐量: %.2f Mbps\n", stats.throughput_mbps);
    printf("测试持续时间: %.2f 秒\n", stats.test_duration);
    printf("内存使用变化: %ld KB\n", final_memory - initial_memory);
    
    free(threads);
    free(worker_data);
}

// 信号处理
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_should_stop = 1;
        printf("\n收到停止信号，正在停止测试...\n");
    }
}

int main() {
    printf("UVHTTP 吞吐量压力测试\n");
    printf("====================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("系统信息:\n");
    printf("  CPU核心数: %d\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  总内存: %ld MB\n", sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024));
    
    // 测试配置
    throughput_config_t tests[] = {
        {
            .target_rps = 1000,
            .test_duration_seconds = 30,
            .payload_size = 1024,      // 1KB
            .concurrent_workers = 4,
            .test_name = "低负载吞吐量测试"
        },
        {
            .target_rps = 5000,
            .test_duration_seconds = 30,
            .payload_size = 1024,      // 1KB
            .concurrent_workers = 8,
            .test_name = "中负载吞吐量测试"
        },
        {
            .target_rps = 10000,
            .test_duration_seconds = 30,
            .payload_size = 1024,      // 1KB
            .concurrent_workers = 12,
            .test_name = "高负载吞吐量测试"
        },
        {
            .target_rps = 5000,
            .test_duration_seconds = 30,
            .payload_size = 10240,     // 10KB
            .concurrent_workers = 8,
            .test_name = "大负载吞吐量测试"
        },
        {
            .target_rps = 1000,
            .test_duration_seconds = 30,
            .payload_size = 102400,    // 100KB
            .concurrent_workers = 4,
            .test_name = "超大负载吞吐量测试"
        }
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    // 运行所有测试
    for (int i = 0; i < num_tests && !g_should_stop; i++) {
        run_throughput_test(&tests[i]);
        
        if (i < num_tests - 1) {
            printf("\n等待 3 秒后进行下一个测试...\n");
            sleep(3);
        }
    }
    
    printf("\n吞吐量压力测试完成！\n");
    
    return 0;
}