/**
 * @file memory_performance_test.c
 * @brief 栈内存 vs 系统分配器性能对比测试
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* ==================== 测试配置 ==================== */

#define TEST_ITERATIONS 100000
#define SMALL_SIZE 64
#define MEDIUM_SIZE 1024
#define LARGE_SIZE 8192
#define HUGE_SIZE 65536

// 高精度时间获取
static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

// 内存使用统计
typedef struct {
    size_t total_allocated;
    size_t peak_usage;
    size_t allocation_count;
    double total_time;
} memory_stats_t;

/* ==================== 栈内存测试 ==================== */

// 栈内存分配测试
void test_stack_memory(memory_stats_t* stats, size_t size, int iterations) {
    printf("测试栈内存分配 (大小: %zu, 迭代: %d)\n", size, iterations);
    
    double start_time = get_time();
    size_t current_usage = 0;
    size_t peak_usage = 0;
    
    for (int i = 0; i < iterations; i++) {
        // 栈分配
        char stack_buffer[size];
        
        // 填充数据
        memset(stack_buffer, 'A' + (i % 26), size - 1);
        stack_buffer[size - 1] = '\0';
        
        // 模拟使用
        volatile size_t len = strlen(stack_buffer);
        current_usage = len;
        
        if (current_usage > peak_usage) {
            peak_usage = current_usage;
        }
        
        // 栈内存自动释放
    }
    
    double end_time = get_time();
    
    stats->total_allocated += size * iterations;
    stats->peak_usage += peak_usage;
    stats->allocation_count += iterations;
    stats->total_time += (end_time - start_time);
    
    printf("  时间: %.6f 秒, 平均: %.9f 秒/次\n", 
           end_time - start_time, (end_time - start_time) / iterations);
}

/* ==================== 系统分配器测试 ==================== */

// 系统分配器测试
void test_system_memory(memory_stats_t* stats, size_t size, int iterations) {
    printf("测试系统分配器 (大小: %zu, 迭代: %d)\n", size, iterations);
    
    double start_time = get_time();
    size_t current_usage = 0;
    size_t peak_usage = 0;
    
    void** pointers = malloc(sizeof(void*) * iterations);
    if (!pointers) {
        printf("  错误: 无法分配指针数组\n");
        return;
    }
    
    // 分配阶段
    for (int i = 0; i < iterations; i++) {
        pointers[i] = malloc(size);
        if (pointers[i]) {
            memset(pointers[i], 'A' + (i % 26), size - 1);
            ((char*)pointers[i])[size - 1] = '\0';
            current_usage += size;
            
            if (current_usage > peak_usage) {
                peak_usage = current_usage;
            }
        }
    }
    
    // 使用阶段
    for (int i = 0; i < iterations; i++) {
        if (pointers[i]) {
            volatile size_t len = strlen((char*)pointers[i]);
            (void)len; // 避免编译器优化
        }
    }
    
    // 释放阶段
    for (int i = 0; i < iterations; i++) {
        if (pointers[i]) {
            free(pointers[i]);
            current_usage -= size;
        }
    }
    
    double end_time = get_time();
    free(pointers);
    
    stats->total_allocated += size * iterations;
    stats->peak_usage += peak_usage;
    stats->allocation_count += iterations;
    stats->total_time += (end_time - start_time);
    
    printf("  时间: %.6f 秒, 平均: %.9f 秒/次\n", 
           end_time - start_time, (end_time - start_time) / iterations);
}

/* ==================== 主测试程序 ==================== */

int main(int argc, char* argv[]) {
    printf("🚀 内存分配器性能对比测试\n");
    printf("=====================================\n");
    
    // 测试配置
    size_t test_sizes[] = {SMALL_SIZE, MEDIUM_SIZE, LARGE_SIZE, HUGE_SIZE};
    const char* size_names[] = {"小", "中", "大", "巨大"};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    memory_stats_t stack_stats = {0};
    memory_stats_t system_stats = {0};
    
    // 单线程性能测试
    printf("\n=== 单线程性能测试 ===\n");
    
    for (int i = 0; i < num_sizes; i++) {
        printf("\n--- %s内存块 (%zu 字节) ---\n", size_names[i], test_sizes[i]);
        
        // 栈内存测试
        test_stack_memory(&stack_stats, test_sizes[i], TEST_ITERATIONS);
        
        // 系统分配器测试
        test_system_memory(&system_stats, test_sizes[i], TEST_ITERATIONS);
        
        printf("\n");
    }
    
    // 结果汇总
    printf("\n=== 测试结果汇总 ===\n");
    printf("栈内存:\n");
    printf("  总分配: %zu 字节\n", stack_stats.total_allocated);
    printf("  峰值使用: %zu 字节\n", stack_stats.peak_usage);
    printf("  分配次数: %zu\n", stack_stats.allocation_count);
    printf("  总时间: %.6f 秒\n", stack_stats.total_time);
    printf("  平均时间: %.9f 秒/次\n", stack_stats.total_time / stack_stats.allocation_count);
    
    printf("\n系统分配器:\n");
    printf("  总分配: %zu 字节\n", system_stats.total_allocated);
    printf("  峰值使用: %zu 字节\n", system_stats.peak_usage);
    printf("  分配次数: %zu\n", system_stats.allocation_count);
    printf("  总时间: %.6f 秒\n", system_stats.total_time);
    printf("  平均时间: %.9f 秒/次\n", system_stats.total_time / system_stats.allocation_count);
    
    // 性能对比
    printf("\n=== 性能对比 ===\n");
    if (system_stats.total_time > 0 && stack_stats.total_time > 0) {
        printf("系统分配器 vs 栈内存: %.2fx 慢\n", system_stats.total_time / stack_stats.total_time);
    }
    
    // 内存使用对比
    printf("\n=== 内存使用对比 ===\n");
    printf("栈内存峰值: %zu 字节 (自动管理)\n", stack_stats.peak_usage);
    printf("系统分配器峰值: %zu 字节 (手动管理)\n", system_stats.peak_usage);
    
    // 结论
    printf("\n=== 结论 ===\n");
    printf("1. 栈内存优势:\n");
    printf("   ✅ 最快的分配/释放速度\n");
    printf("   ✅ 自动内存管理，无泄漏风险\n");
    printf("   ✅ 缓存友好，局部性好\n");
    printf("   ❌ 大小受限，可能导致栈溢出\n");
    printf("   ❌ 生命周期受限于函数作用域\n");
    
    printf("\n2. 系统分配器优势:\n");
    printf("   ✅ 支持任意大小的内存块\n");
    printf("   ✅ 灵活的生命周期管理\n");
    printf("   ❌ 分配/释放开销较大\n");
    printf("   ❌ 需要手动管理，有泄漏风险\n");
    
    printf("\n3. 推荐使用场景:\n");
    printf("   - 小对象、临时数据: 使用栈内存\n");
    printf("   - 大对象、长生命周期: 使用系统分配器\n");
    printf("   - 高频分配释放: 优先栈内存\n");
    printf("   \n");
    printf("注意: UVHTTP 是单线程框架，不支持多线程操作\n");
    
    return 0;
}