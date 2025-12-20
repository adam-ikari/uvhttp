#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include "include/uvhttp_logging.h"
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_request_simple.h"

// 内存泄漏测试配置
typedef struct {
    int test_duration_hours;
    int allocation_cycles_per_second;
    int max_allocation_size;
    int concurrent_allocators;
    int enable_stress_gc;
    char* test_name;
} memory_leak_config_t;

// 内存泄漏统计
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t deallocation_count;
    size_t memory_leaks;
    time_t start_time;
    time_t end_time;
    double test_duration;
} memory_leak_stats_t;

// 分配器线程数据
typedef struct {
    int allocator_id;
    memory_leak_config_t* config;
    memory_leak_stats_t* stats;
    pthread_mutex_t* stats_mutex;
    volatile int* should_stop;
    size_t allocations_made;
    size_t deallocations_made;
} allocator_data_t;

// 全局控制
static volatile int g_should_stop = 0;
static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_signal_received = 0;

// 信号处理函数
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n收到信号 %d，正在停止测试...\n", sig);
        g_should_stop = 1;
        g_signal_received = 1;
    }
}

// 内存分配模式
typedef enum {
    ALLOC_PATTERN_RANDOM,    // 随机大小分配
    ALLOC_PATTERN_SMALL,      // 小块分配 (<1KB)
    ALLOC_PATTERN_MEDIUM,     // 中等分配 (1KB-10KB)
    ALLOC_PATTERN_LARGE,      // 大块分配 (>10KB)
    ALLOC_PATTERN_MEGA        // 超大分配 (>1MB)
} allocation_pattern_t;

// 生成随机大小
static size_t get_random_size(size_t max_size) {
    return (rand() % max_size) + 1;
}

// 根据模式分配内存
static void* allocate_memory(allocator_data_t* data, allocation_pattern_t pattern) {
    size_t size = 0;
    
    switch (pattern) {
        case ALLOC_PATTERN_RANDOM:
            size = get_random_size(data->config->max_allocation_size);
            break;
        case ALLOC_PATTERN_SMALL:
            size = get_random_size(1024);
            break;
        case ALLOC_PATTERN_MEDIUM:
            size = 1024 + get_random_size(9216); // 1KB-10KB
            break;
        case ALLOC_PATTERN_LARGE:
            size = 10240 + get_random_size(90112); // 10KB-100KB
            break;
        case ALLOC_PATTERN_MEGA:
            size = 1048576 + get_random_size(4194304); // 1MB-5MB
            break;
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        // 填充内存模式，确保实际分配
        memset(ptr, 0xAA, size);
        
        // 更新统计
        pthread_mutex_lock(data->stats_mutex);
        data->stats->total_allocated += size;
        data->stats->current_usage += size;
        data->stats->allocation_count++;
        data->allocations_made++;
        
        if (data->stats->current_usage > data->stats->peak_usage) {
            data->stats->peak_usage = data->stats->current_usage;
        }
        
        pthread_mutex_unlock(data->stats_mutex);
        
        // 随机写入数据，模拟真实使用
        if (size > 100) {
            char* data_ptr = (char*)ptr;
            for (size_t i = 0; i < size - 100; i += 100) {
                data_ptr[i] = (char)(rand() % 256);
            }
        }
    }
    
    return ptr;
}

// 内存分配器线程
static void* memory_allocator(void* arg) {
    allocator_data_t* data = (allocator_data_t*)arg;
    
    UVHTTP_LOG_INFO("Memory allocator %d started", data->allocator_id);
    
    // 设置随机种子
    srand(time(NULL) + data->allocator_id);
    
    while (!(*data->should_stop)) {
        int cycles_per_batch = data->config->allocation_cycles_per_second / 10; // 每100ms的周期数
        
        // 分配阶段
        for (int i = 0; i < cycles_per_batch && !(*data->should_stop); i++) {
            allocation_pattern_t pattern = rand() % 5; // 随机选择分配模式
            
            void* ptr = allocate_memory(data, pattern);
            if (ptr) {
                // 短暂使用内存
                usleep(1000); // 1ms
                
                // 随机决定是否释放（模拟内存泄漏）
                if (rand() % 100 < 70) { // 70%概率释放
                    free(ptr);
                    
                    pthread_mutex_lock(data->stats_mutex);
                    data->stats->total_freed += data->stats->current_usage;
                    data->stats->current_usage = 0;
                    data->stats->deallocation_count++;
                    data->deallocations_made++;
                    pthread_mutex_unlock(data->stats_mutex);
                }
            }
        }
        
        // 短暂休息
        usleep(100000); // 100ms
    }
    
    UVHTTP_LOG_INFO("Memory allocator %d completed: %d allocations, %d deallocations", 
                     data->allocator_id, data->allocations_made, data->deallocations_made);
    return NULL;
}

// 垃圾回收线程
static void* garbage_collector(void* arg) {
    memory_leak_config_t* config = (memory_leak_config_t*)arg;
    
    UVHTTP_LOG_INFO("Garbage collector started");
    
    while (!g_should_stop) {
        sleep(5); // 每5秒执行一次GC
        
        if (config->enable_stress_gc) {
            // 压力GC：强制释放一些内存
            uvhttp_mem_stats_t current_stats;
            uvhttp_mem_get_stats(&current_stats);
            
            if (current_stats.current_usage > 0) {
                // 模拟释放一些内存（实际应用中需要跟踪指针）
                UVHTTP_LOG_DEBUG("GC: Simulating cleanup of %zu bytes", 
                                 current_stats.current_usage / 2);
            }
        }
    }
    
    UVHTTP_LOG_INFO("Garbage collector stopped");
    return NULL;
}

// 系统资源监控
static void monitor_system_resources(void) {
    struct rusage usage;
    
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        UVHTTP_LOG_DEBUG("Memory usage: %lu KB, CPU time: %.2f sec", 
                         usage.ru_maxrss / 1024, 
                         usage.ru_utime.tv_sec + usage.ru_stime.tv_sec + 
                         usage.ru_utime.tv_usec / 1000000.0);
    }
}

// 运行内存泄漏测试
static int run_memory_leak_test(memory_leak_config_t* config, memory_leak_stats_t* stats) {
    printf("开始内存泄漏测试: %s\n", config->test_name);
    printf("  测试时长: %d 小时\n", config->test_duration_hours);
    printf  ("  每秒分配周期: %d\n", config->allocation_cycles_per_second);
    printf("  最大分配大小: %d bytes\n", config->max_allocation_size);
    printf("  并发分配器: %d\n", config->concurrent_allocators);
    printf("  启用压力GC: %s\n", config->enable_stress_gc ? "是" : "否");
    
    // 重置统计
    memset(stats, 0, sizeof(memory_leak_stats_t));
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建分配器线程
    pthread_t* allocator_threads = malloc(config->concurrent_allocators * sizeof(pthread_t));
    allocator_data_t* allocator_data = malloc(config->concurrent_allocators * sizeof(allocator_data_t));
    
    if (!allocator_threads || !allocator_data) {
        UVHTTP_LOG_ERROR("Failed to allocate memory for allocator threads");
        return -1;
    }
    
    // 启动分配器线程
    for (int i = 0; i < config->concurrent_allocators; i++) {
        allocator_data[i].allocator_id = i;
        allocator_data[i].config = config;
        allocator_data[i].stats = stats;
        allocator_data[i].stats_mutex = &g_stats_mutex;
        allocator_data[i].should_stop = &g_should_stop;
        allocator_data[i].allocations_made = 0;
        allocator_data[i].deallocations_made = 0;
        
        if (pthread_create(&allocator_threads[i], NULL, memory_allocator, &allocator_data[i]) != 0) {
            UVHTTP_LOG_ERROR("Failed to create allocator thread %d", i);
            g_should_stop = 1;
            break;
        }
    }
    
    // 启动垃圾回收线程
    pthread_t gc_thread;
    if (config->enable_stress_gc) {
        pthread_create(&gc_thread, NULL, garbage_collector, config);
    }
    
    // 设置测试开始时间
    stats->start_time = time(NULL);
    
    // 监控系统资源
    time_t last_monitor_time = time(NULL);
    
    // 运行测试
    int test_duration_seconds = config->test_duration_hours * 3600;
    while (!g_should_stop && time(NULL) - stats->start_time < test_duration_seconds) {
        sleep(10); // 每10秒检查一次
        
        // 监控系统资源
        monitor_system_resources();
        
        // 每分钟输出一次进度
        if (time(NULL) - last_monitor_time >= 60) {
            double elapsed = difftime(time(NULL), stats->start_time);
            double progress = (elapsed / test_duration_seconds) * 100;
            
            printf("\r测试进度: %.1f%% (%.0f/%.0f 秒)", 
                   progress, elapsed, test_duration_seconds);
            fflush(stdout);
            
            last_monitor_time = time(NULL);
        }
    }
    
    printf("\n"); // 换行
    
    // 等待所有线程完成
    for (int i = 0; i < config->concurrent_allocators; i++) {
        pthread_join(allocator_threads[i], NULL);
    }
    
    if (config->enable_stress_gc) {
        pthread_join(gc_thread, NULL);
    }
    
    // 设置测试结束时间
    stats->end_time = time(NULL);
    stats->test_duration = difftime(stats->end_time, stats->start_time);
    
    // 计算内存泄漏
    stats->memory_leaks = stats->total_allocated - stats->total_freed;
    
    // 清理资源
    free(allocator_threads);
    free(allocator_data);
    
    return 0;
}

// 打印内存泄漏测试结果
static void print_memory_leak_results(memory_leak_stats_t* stats, memory_leak_config_t* config) {
    double leak_rate = stats->total_allocated > 0 ? 
                      (double)stats->memory_leaks / stats->total_allocated * 100 : 0;
    
    printf("\n" "============================================================\n");
    printf("                内存泄漏测试结果: %s\n", config->test_name);
    printf("============================================================\n");
    
    printf("测试配置:\n");
    printf("  测试时长: %.2f 小时\n", stats->test_duration / 3600.0);
    printf("  总分配次数: %zu\n", stats->allocation_count);
    printf("  总释放次数: %zu\n", stats->deallocation_count);
    printf("  最大分配大小: %d bytes\n", config->max_allocation_size);
    printf("  并发分配器: %d\n", config->concurrent_allocators);
    
    printf("\n内存统计:\n");
    printf("  总分配量: %.2f MB\n", stats->total_allocated / 1024.0 / 1024.0);
    printf("  总释放量: %.2f MB\n", stats->total_freed / 1024.0 / 1024.0);
    printf("  当前使用: %.2f MB\n", stats->current_usage / 1024.0 / 1024.0);
    printf("  峰值使用: %.2f MB\n", stats->peak_usage / 1024.0 / 1024.0);
    printf("  内存泄漏: %.2f MB (%.2f%%)\n", 
           stats->memory_leaks / 1024.0 / 1024.0, leak_rate);
    
    printf("\n泄漏分析:\n");
    if (leak_rate < 1.0) {
        printf("  🟢 优秀 (泄漏率 < 1%%)\n");
    } else if (leak_rate < 5.0) {
        printf("  ✅ 良好 (泄漏率 1-5%%)\n");
    } else if (leak_rate < 10.0) {
        printf("  ⚠️  一般 (泄漏率 5-10%%)\n");
    } else {
        printf("  ❌ 需要修复 (泄漏率 > 10%%)\n");
    }
    
    if (stats->allocation_count > 0) {
        double deallocation_ratio = (double)stats->deallocation_count / stats->allocation_count * 100;
        if (deallocation_ratio >= 95.0) {
            printf("  🛡️ 释放率优秀 (≥95%%)\n");
        } else if (deallocation_ratio >= 80.0) {
            printf("  ✅ 释放率良好 (80-95%%)\n");
        } else {
            printf("  ⚠️ 释放率需要改进 (<80%%)\n");
        }
    }
    
    printf("  分配速率: %.2f 次/秒\n", stats->allocation_count / stats->test_duration);
    printf("  释放速率: %.2f 次/秒\n", stats->deallocation_count / stats->test_duration);
    
    printf("============================================================\n");
}

// 短时间内存测试
int test_short_term_memory(void) {
    printf("开始短时间内存测试...\n");
    
    memory_leak_config_t config = {
        .test_duration_hours = 1,
        .allocation_cycles_per_second = 100,
        .max_allocation_size = 1024 * 1024, // 1MB
        .concurrent_allocators = 5,
        .enable_stress_gc = 0,
        .test_name = "短时间内存测试"
    };
    
    memory_leak_stats_t stats;
    return run_memory_leak_test(&config, &stats);
}

// 长时间内存测试
int test_long_term_memory(void) {
    printf("开始长时间内存测试...\n");
    
    memory_leak_config_t config = {
        .test_duration_hours = 8,
        .allocation_cycles_per_second = 50,
        .max_allocation_size = 10 * 1024 * 1024, // 10MB
        .concurrent_allocators = 10,
        .enable_stress_gc = 1,
        .test_name = "长时间内存测试"
    };
    
    memory_leak_stats_t stats;
    return run_memory_leak_test(&config, &stats);
}

// 压力内存测试
int test_stress_memory(void) {
    printf("开始压力内存测试...\n");
    
    memory_leak_config_t config = {
        .test_duration_hours = 2,
        .allocation_cycles_per_second = 500,
        .max_allocation_size = 50 * 1024 * 1024, // 50MB
        .concurrent_allocators = 20,
        .enable_stress_gc = 1,
        .test_name = "压力内存测试"
    };
    
    memory_leak_stats_t stats;
    return run_memory_leak_test(&config, &stats);
}

// 极限内存测试
int test_extreme_memory(void) {
    printf("开始极限内存测试...\n");
    
    memory_leak_config_t config = {
        .test_duration_hours = 1,
        .allocation_cycles_per_second = 1000,
        .max_allocation_size = 100 * 1024 * 1024, // 100MB
        .concurrent_allocators = 50,
        .enable_stress_gc = 1,
        .test_name = "极限内存测试"
    };
    
    memory_leak_stats_t stats;
    return run_memory_leak_test(&config, &stats);
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    uvhttp_log_init(UVHTTP_LOG_INFO);
    uvhttp_mem_reset_stats();
    
    printf("=== UVHTTP 内存泄漏压力测试套件 ===\n");
    printf("测试开始时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 重置控制标志
    g_should_stop = 0;
    g_signal_received = 0;
    
    // 执行内存泄漏测试
    if (test_short_term_memory() != 0) {
        UVHTTP_LOG_ERROR("Short term memory test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_signal_received && test_long_term_memory() != 0) {
        UVHTTP_LOG_ERROR("Long term memory test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_signal_received && test_stress_memory() != 0) {
        UVHTTP_LOG_ERROR("Stress memory test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_signal_received && test_extreme_memory() != 0) {
        UVHTTP_LOG_ERROR("Extreme memory test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    printf("\n=== 内存泄漏压力测试完成 ===\n");
    printf("测试结束时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 打印最终内存统计
    uvhttp_mem_stats_t final_mem_stats;
    uvhttp_mem_get_stats(&final_mem_stats);
    printf("\n最终内存统计:\n");
    printf("  总分配: %zu bytes\n", final_mem_stats.total_allocated);
    printf("  当前使用: %zu bytes\n", final_mem_stats.current_usage);
    printf("  峰值使用: %zu bytes\n", final_mem_stats.peak_usage);
    printf("  分配次数: %zu\n", final_mem_stats.allocation_count);
    
    if (g_signal_received) {
        printf("\n测试被用户中断\n");
    }
    
    uvhttp_log_cleanup();
    return 0;
}