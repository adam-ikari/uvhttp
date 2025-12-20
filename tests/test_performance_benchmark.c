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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

// 性能基准测试框架
typedef struct {
    const char* name;
    double (*benchmark_func)(void);
    const char* unit;
    double baseline; // 基准值
} benchmark_t;

// 性能统计
typedef struct {
    double min;
    double max;
    double mean;
    double p50;
    double p95;
    double p99;
    double std_dev;
    size_t sample_count;
} performance_stats_t;

// 延迟测试数据
typedef struct {
    double latency_ms;
    struct timeval timestamp;
} latency_sample_t;

// 全局变量
static volatile int running = 1;
static latency_sample_t* latency_samples = NULL;
static size_t sample_count = 0;
static size_t sample_capacity = 0;

// 信号处理
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        printf("\n收到停止信号，正在停止基准测试...\n");
    }
}

// 时间工具函数
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

// 内存使用统计
static size_t get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss * 1024; // KB to bytes
}

// CPU使用率统计
static double get_cpu_usage(void) {
    static struct rusage prev_usage = {0};
    static struct timeval prev_time = {0};
    
    struct rusage curr_usage;
    struct timeval curr_time;
    
    getrusage(RUSAGE_SELF, &curr_usage);
    gettimeofday(&curr_time, NULL);
    
    if (prev_time.tv_sec == 0) {
        prev_usage = curr_usage;
        prev_time = curr_time;
        return 0.0;
    }
    
    double user_time = (curr_usage.ru_utime.tv_sec - prev_usage.ru_utime.tv_sec) * 1000.0 +
                      (curr_usage.ru_utime.tv_usec - prev_usage.ru_utime.tv_usec) / 1000.0;
    double sys_time = (curr_usage.ru_stime.tv_sec - prev_usage.ru_stime.tv_sec) * 1000.0 +
                     (curr_usage.ru_stime.tv_usec - prev_usage.ru_stime.tv_usec) / 1000.0;
    
    double time_diff = (curr_time.tv_sec - prev_time.tv_sec) * 1000.0 +
                      (curr_time.tv_usec - prev_time.tv_usec) / 1000.0;
    
    prev_usage = curr_usage;
    prev_time = curr_time;
    
    return time_diff > 0 ? ((user_time + sys_time) / time_diff) * 100.0 : 0.0;
}

// 添加延迟样本
static void add_latency_sample(double latency_ms) {
    if (sample_count >= sample_capacity) {
        sample_capacity = sample_capacity == 0 ? 10000 : sample_capacity * 2;
        latency_samples = realloc(latency_samples, sample_capacity * sizeof(latency_sample_t));
        assert(latency_samples != NULL);
    }
    
    latency_samples[sample_count].latency_ms = latency_ms;
    gettimeofday(&latency_samples[sample_count].timestamp, NULL);
    sample_count++;
}

// 计算性能统计
static performance_stats_t calculate_stats(double* values, size_t count) {
    performance_stats_t stats = {0};
    
    if (count == 0) return stats;
    
    // 计算最小值、最大值和总和
    stats.min = values[0];
    stats.max = values[0];
    double sum = 0.0;
    
    for (size_t i = 0; i < count; i++) {
        if (values[i] < stats.min) stats.min = values[i];
        if (values[i] > stats.max) stats.max = values[i];
        sum += values[i];
    }
    
    stats.mean = sum / count;
    stats.sample_count = count;
    
    // 计算标准差
    double variance = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = values[i] - stats.mean;
        variance += diff * diff;
    }
    stats.std_dev = sqrt(variance / count);
    
    // 计算百分位数（需要排序）
    double* sorted = malloc(count * sizeof(double));
    memcpy(sorted, values, count * sizeof(double));
    
    // 简单排序
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (sorted[i] > sorted[j]) {
                double temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }
    
    stats.p50 = sorted[(size_t)(count * 0.5)];
    stats.p95 = sorted[(size_t)(count * 0.95)];
    stats.p99 = sorted[(size_t)(count * 0.99)];
    
    free(sorted);
    return stats;
}

// 基准测试1：系统调用延迟
static double benchmark_syscall_latency(void) {
    const int iterations = 10000;
    double* latencies = malloc(iterations * sizeof(double));
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        getpid(); // 简单系统调用
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0; // 转换为毫秒
    }
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    
    printf("  系统调用延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试2：内存分配延迟
static double benchmark_memory_allocation(void) {
    const int iterations = 10000;
    const size_t alloc_size = 1024; // 1KB
    double* latencies = malloc(iterations * sizeof(double));
    void** ptrs = malloc(iterations * sizeof(void*));
    
    if (!latencies || !ptrs) {
        if (latencies) free(latencies);
        if (ptrs) free(ptrs);
        return 0.0;
    }
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        ptrs[i] = malloc(alloc_size);
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0;
        
        // 检查分配是否成功
        if (!ptrs[i]) {
            latencies[i] = -1.0; // 标记失败
        }
    }
    
    // 清理内存
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            free(ptrs[i]);
        }
    }
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    free(ptrs);
    
    printf("  内存分配延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试3：字符串处理性能
static double benchmark_string_processing(void) {
    const int iterations = 10000;
    const char* test_string = "Hello, World! This is a test string for benchmarking.";
    double* latencies = malloc(iterations * sizeof(double));
    
    if (!latencies) {
        return 0.0;
    }
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        
        // 字符串操作
        size_t len = strlen(test_string);
        char* copy = malloc(len + 50); // 额外空间用于追加
        if (copy) {
            strcpy(copy, test_string);
            strcat(copy, " [APPENDED]");
            int cmp = strcmp(copy, test_string);
            free(copy);
        }
        
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0;
    }
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    
    printf("  字符串处理延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试4：并发线程创建性能
static double benchmark_thread_creation(void) {
    const int iterations = 100;
    double* latencies = malloc(iterations * sizeof(double));
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        
        pthread_t thread;
        pthread_create(&thread, NULL, (void*(*)(void*))pthread_exit, NULL);
        pthread_join(thread, NULL);
        
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0;
    }
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    
    printf("  线程创建延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试5：文件I/O性能
static double benchmark_file_io(void) {
    const int iterations = 1000;
    const char* filename = "/tmp/uvhttp_benchmark.tmp";
    const char* test_data = "This is test data for file I/O benchmarking.\n";
    double* latencies = malloc(iterations * sizeof(double));
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        
        FILE* file = fopen(filename, "w");
        if (file) {
            fwrite(test_data, 1, strlen(test_data), file);
            fclose(file);
        }
        
        file = fopen(filename, "r");
        if (file) {
            char buffer[256];
            fread(buffer, 1, sizeof(buffer), file);
            fclose(file);
        }
        
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0;
    }
    
    unlink(filename); // 清理测试文件
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    
    printf("  文件I/O延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试6：网络连接模拟
static double benchmark_network_simulation(void) {
    const int iterations = 1000;
    double* latencies = malloc(iterations * sizeof(double));
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_us();
        
        // 模拟网络操作
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd >= 0) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(80);
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            
            // 非阻塞连接尝试
            int flags = fcntl(sockfd, F_GETFL, 0);
            fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
            
            connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
            
            close(sockfd);
        }
        
        double end = get_time_us();
        latencies[i] = (end - start) / 1000.0;
    }
    
    performance_stats_t stats = calculate_stats(latencies, iterations);
    free(latencies);
    
    printf("  网络连接模拟延迟统计:\n");
    printf("    平均值: %.3f ms\n", stats.mean);
    printf("    P50: %.3f ms, P95: %.3f ms, P99: %.3f ms\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.3f ms\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试7：内存带宽测试
static double benchmark_memory_bandwidth(void) {
    const size_t buffer_size = 64 * 1024 * 1024; // 64MB
    char* buffer = malloc(buffer_size);
    const int iterations = 100;
    double* bandwidths = malloc(iterations * sizeof(double));
    
    if (!buffer || !bandwidths) {
        if (buffer) free(buffer);
        if (bandwidths) free(bandwidths);
        return 0.0;
    }
    
    // 初始化缓冲区
    memset(buffer, 0xAA, buffer_size);
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_ms();
        
        // 安全的内存复制测试 - 确保不会越界
        size_t copy_size = buffer_size / 4; // 复制1/4的大小
        memcpy(buffer, buffer + copy_size, copy_size);
        
        double end = get_time_ms();
        double time_sec = (end - start) / 1000.0;
        if (time_sec > 0) {
            double bytes_per_sec = copy_size / time_sec;
            bandwidths[i] = bytes_per_sec / (1024.0 * 1024.0); // MB/s
        } else {
            bandwidths[i] = 0.0;
        }
    }
    
    free(buffer);
    
    performance_stats_t stats = calculate_stats(bandwidths, iterations);
    free(bandwidths);
    
    printf("  内存带宽统计:\n");
    printf("    平均值: %.2f MB/s\n", stats.mean);
    printf("    P50: %.2f MB/s, P95: %.2f MB/s, P99: %.2f MB/s\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.2f MB/s\n", stats.std_dev);
    
    return stats.mean;
}

// 基准测试8：CPU计算密集型测试
static double benchmark_cpu_intensive(void) {
    const int iterations = 100;
    double* throughputs = malloc(iterations * sizeof(double));
    
    for (int i = 0; i < iterations; i++) {
        double start = get_time_ms();
        
        // CPU密集型计算：质数计算
        int count = 0;
        for (int n = 2; n <= 100000; n++) {
            int is_prime = 1;
            for (int d = 2; d * d <= n; d++) {
                if (n % d == 0) {
                    is_prime = 0;
                    break;
                }
            }
            if (is_prime) count++;
        }
        
        double end = get_time_ms();
        double time_sec = (end - start) / 1000.0;
        throughputs[i] = count / time_sec; // 质数/秒
    }
    
    performance_stats_t stats = calculate_stats(throughputs, iterations);
    free(throughputs);
    
    printf("  CPU计算性能统计:\n");
    printf("    平均值: %.0f 质数/秒\n", stats.mean);
    printf("    P50: %.0f, P95: %.0f, P99: %.0f\n", stats.p50, stats.p95, stats.p99);
    printf("    标准差: %.0f\n", stats.std_dev);
    
    return stats.mean;
}

// 运行单个基准测试
static void run_benchmark(const benchmark_t* benchmark) {
    printf("\n=== %s ===\n", benchmark->name);
    
    size_t initial_memory = get_memory_usage();
    double initial_cpu = get_cpu_usage();
    
    double result = benchmark->benchmark_func();
    
    double final_cpu = get_cpu_usage();
    size_t final_memory = get_memory_usage();
    
    printf("  结果: %.3f %s\n", result, benchmark->unit);
    printf("  内存使用: %zu KB -> %zu KB (增加: %zu KB)\n", 
           initial_memory / 1024, final_memory / 1024, 
           (final_memory - initial_memory) / 1024);
    printf("  CPU使用率: %.1f%% -> %.1f%%\n", initial_cpu, final_cpu);
    
    if (benchmark->baseline > 0) {
        double ratio = result / benchmark->baseline;
        printf("  与基准比较: %.2fx %s\n", 
               ratio, ratio > 1.0 ? "(更快)" : "(更慢)");
    }
}

// 基准测试定义
static const benchmark_t benchmarks[] = {
    {"系统调用延迟", benchmark_syscall_latency, "ms", 0.01},
    {"内存分配延迟", benchmark_memory_allocation, "ms", 0.05},
    {"字符串处理性能", benchmark_string_processing, "ms", 0.1},
    {"并发线程创建", benchmark_thread_creation, "ms", 1.0},
    {"文件I/O性能", benchmark_file_io, "ms", 0.5},
    {"网络连接模拟", benchmark_network_simulation, "ms", 2.0},
    {"内存带宽测试", benchmark_memory_bandwidth, "MB/s", 1000.0},
    {"CPU计算密集型测试", benchmark_cpu_intensive, "质数/秒", 1000.0}
};

// 主测试函数
int main(void) {
    printf("UVHTTP 性能基准测试\n");
    printf("====================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("系统信息:\n");
    printf("  CPU核心数: %d\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  页面大小: %d KB\n", sysconf(_SC_PAGESIZE) / 1024);
    printf("  总内存: %ld MB\n", sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024));
    
    size_t initial_memory = get_memory_usage();
    printf("  初始内存使用: %zu KB\n", initial_memory / 1024);
    
    printf("\n开始性能基准测试...\n");
    
    double total_start = get_time_ms();
    
    // 运行所有基准测试
    for (size_t i = 0; i < sizeof(benchmarks) / sizeof(benchmarks[0]) && running; i++) {
        run_benchmark(&benchmarks[i]);
    }
    
    double total_end = get_time_ms();
    
    printf("\n=== 基准测试总结 ===\n");
    printf("总测试时间: %.2f 秒\n", (total_end - total_start) / 1000.0);
    printf("完成的测试: %zu/%zu\n", 
           sizeof(benchmarks) / sizeof(benchmarks[0]), 
           sizeof(benchmarks) / sizeof(benchmarks[0]));
    
    size_t final_memory = get_memory_usage();
    printf("最终内存使用: %zu KB\n", final_memory / 1024);
    printf("内存泄漏: %zu KB\n", (final_memory - initial_memory) / 1024);
    
    // 清理延迟样本
    if (latency_samples) {
        free(latency_samples);
        latency_samples = NULL;
    }
    
    printf("\n性能基准测试完成！\n");
    
    return 0;
}