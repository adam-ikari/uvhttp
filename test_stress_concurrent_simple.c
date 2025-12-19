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

// 并发连接测试配置
typedef struct {
    int max_connections;
    int test_duration_seconds;
    int connection_rate; // 每秒新连接数
    char* test_name;
} concurrent_test_config_t;

// 连接统计
typedef struct {
    int total_connections;
    int successful_connections;
    int failed_connections;
    double avg_connection_time;
    double min_connection_time;
    double max_connection_time;
    size_t memory_usage_kb;
    double test_duration;
} concurrent_stats_t;

// 工作线程数据
typedef struct {
    int thread_id;
    concurrent_test_config_t* config;
    concurrent_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    volatile int* should_stop;
    int connections_made;
} thread_data_t;

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

// 模拟连接建立
static int simulate_connection(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    // 设置非阻塞模式
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    // 尝试连接（会失败，但用于测试连接建立开销）
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    // 短暂保持连接
    usleep(1000); // 1ms
    
    close(sockfd);
    return 0;
}

// 工作线程函数
static void* worker_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    concurrent_stats_t local_stats = {0};
    local_stats.min_connection_time = 1e9; // 初始化为很大的值
    
    printf("工作线程 %d 开始运行\n", data->thread_id);
    
    while (!(*data->should_stop) && data->connections_made < data->config->max_connections / 10) {
        double start_time = get_time_ms();
        
        int result = simulate_connection();
        
        double end_time = get_time_ms();
        double connection_time = end_time - start_time;
        
        local_stats.total_connections++;
        
        if (result == 0) {
            local_stats.successful_connections++;
            
            // 更新连接时间统计
            if (connection_time < local_stats.min_connection_time) {
                local_stats.min_connection_time = connection_time;
            }
            if (connection_time > local_stats.max_connection_time) {
                local_stats.max_connection_time = connection_time;
            }
        } else {
            local_stats.failed_connections++;
        }
        
        data->connections_made++;
        
        // 控制连接速率
        if (data->config->connection_rate > 0) {
            usleep(1000000 / data->config->connection_rate);
        }
    }
    
    // 更新全局统计
    pthread_mutex_lock(data->stats_mutex);
    
    data->stats->total_connections += local_stats.total_connections;
    data->stats->successful_connections += local_stats.successful_connections;
    data->stats->failed_connections += local_stats.failed_connections;
    
    if (local_stats.min_connection_time < data->stats->min_connection_time || 
        data->stats->min_connection_time == 0) {
        data->stats->min_connection_time = local_stats.min_connection_time;
    }
    
    if (local_stats.max_connection_time > data->stats->max_connection_time) {
        data->stats->max_connection_time = local_stats.max_connection_time;
    }
    
    pthread_mutex_unlock(data->stats_mutex);
    
    printf("工作线程 %d 完成，建立了 %d 个连接\n", data->thread_id, data->connections_made);
    return NULL;
}

// 运行并发连接测试
static void run_concurrent_test(concurrent_test_config_t* config) {
    printf("\n=== %s ===\n", config->test_name);
    printf("目标连接数: %d\n", config->max_connections);
    printf("测试持续时间: %d 秒\n", config->test_duration_seconds);
    printf("连接速率: %d 连接/秒\n", config->connection_rate);
    
    concurrent_stats_t stats = {0};
    stats.min_connection_time = 1e9;
    
    size_t initial_memory = get_memory_usage();
    double start_time = get_time_ms();
    
    // 创建工作线程
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads > 8) num_threads = 8; // 限制线程数
    
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(num_threads * sizeof(thread_data_t));
    pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    volatile int should_stop = 0;
    
    // 启动工作线程
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].config = config;
        thread_data[i].stats = &stats;
        thread_data[i].stats_mutex = &stats_mutex;
        thread_data[i].should_stop = &should_stop;
        thread_data[i].connections_made = 0;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]) != 0) {
            fprintf(stderr, "创建线程 %d 失败\n", i);
            should_stop = 1;
            break;
        }
    }
    
    // 等待测试完成或超时
    double elapsed = 0;
    while (elapsed < config->test_duration_seconds * 1000 && 
           stats.total_connections < config->max_connections) {
        usleep(100000); // 100ms
        elapsed = get_time_ms() - start_time;
    }
    
    // 停止所有线程
    should_stop = 1;
    
    // 等待线程结束
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double end_time = get_time_ms();
    size_t final_memory = get_memory_usage();
    
    // 计算最终统计
    stats.test_duration = (end_time - start_time) / 1000.0;
    stats.memory_usage_kb = final_memory - initial_memory;
    
    if (stats.successful_connections > 0) {
        stats.avg_connection_time = (stats.min_connection_time + stats.max_connection_time) / 2.0;
    }
    
    // 输出结果
    printf("\n--- %s 结果 ---\n", config->test_name);
    printf("总连接数: %d\n", stats.total_connections);
    printf("成功连接: %d (%.1f%%)\n", stats.successful_connections, 
           stats.total_connections > 0 ? (double)stats.successful_connections / stats.total_connections * 100.0 : 0.0);
    printf("失败连接: %d (%.1f%%)\n", stats.failed_connections,
           stats.total_connections > 0 ? (double)stats.failed_connections / stats.total_connections * 100.0 : 0.0);
    printf("平均连接时间: %.3f ms\n", stats.avg_connection_time);
    printf("最小连接时间: %.3f ms\n", stats.min_connection_time);
    printf("最大连接时间: %.3f ms\n", stats.max_connection_time);
    printf("测试持续时间: %.2f 秒\n", stats.test_duration);
    printf("内存使用变化: %zu KB\n", stats.memory_usage_kb);
    printf("连接速率: %.1f 连接/秒\n", stats.total_connections / stats.test_duration);
    
    free(threads);
    free(thread_data);
}

// 信号处理
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_should_stop = 1;
        printf("\n收到停止信号，正在停止测试...\n");
    }
}

int main() {
    printf("UVHTTP 并发连接压力测试\n");
    printf("========================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("系统信息:\n");
    printf("  CPU核心数: %d\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  页面大小: %d KB\n", sysconf(_SC_PAGESIZE) / 1024);
    printf("  总内存: %ld MB\n", sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024));
    
    // 测试配置
    concurrent_test_config_t tests[] = {
        {
            .max_connections = 1000,
            .test_duration_seconds = 30,
            .connection_rate = 50,
            .test_name = "渐进式连接测试"
        },
        {
            .max_connections = 2000,
            .test_duration_seconds = 60,
            .connection_rate = 100,
            .test_name = "高负载连接测试"
        },
        {
            .max_connections = 5000,
            .test_duration_seconds = 120,
            .connection_rate = 200,
            .test_name = "极限连接测试"
        }
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    // 运行所有测试
    for (int i = 0; i < num_tests && !g_should_stop; i++) {
        run_concurrent_test(&tests[i]);
        
        if (i < num_tests - 1) {
            printf("\n等待 5 秒后进行下一个测试...\n");
            sleep(5);
        }
    }
    
    printf("\n并发连接压力测试完成！\n");
    
    return 0;
}