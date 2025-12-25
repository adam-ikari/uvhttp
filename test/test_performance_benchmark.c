/**
 * @file test_performance_benchmark.c
 * @brief 性能基准测试程序
 */

#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define BENCHMARK_ROUNDS 10000
#define SMALL_PAYLOAD_SIZE 100
#define MEDIUM_PAYLOAD_SIZE 1024
#define LARGE_PAYLOAD_SIZE 10240

typedef struct {
    const char* name;
    int rounds;
    int payload_size;
    double total_time;
    double min_time;
    double max_time;
} benchmark_result_t;

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void benchmark_request_creation() {
    printf("=== 请求创建基准测试 ===\n");
    
    double start_time = get_time_ms();
    double min_time = 1e9, max_time = 0;
    
    for (int i = 0; i < BENCHMARK_ROUNDS; i++) {
        double round_start = get_time_ms();
        
        /* 模拟请求创建 */
        uvhttp_request_t* request = malloc(sizeof(uvhttp_request_t));
        if (request) {
            memset(request, 0, sizeof(uvhttp_request_t));
            free(request);
        }
        
        double round_end = get_time_ms();
        double elapsed = round_end - round_start;
        
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
    }
    
    double total_time = get_time_ms() - start_time;
    double avg_time = total_time / BENCHMARK_ROUNDS;
    
    printf("轮次: %d\n", BENCHMARK_ROUNDS);
    printf("总时间: %.2f ms\n", total_time);
    printf("平均时间: %.6f ms\n", avg_time);
    printf("最小时间: %.6f ms\n", min_time);
    printf("最大时间: %.6f ms\n", max_time);
    printf("每秒操作数: %.0f ops/s\n", BENCHMARK_ROUNDS / (total_time / 1000.0));
    printf("\n");
}

void benchmark_response_creation() {
    printf("=== 响应创建基准测试 ===\n");
    
    double start_time = get_time_ms();
    double min_time = 1e9, max_time = 0;
    
    for (int i = 0; i < BENCHMARK_ROUNDS; i++) {
        double round_start = get_time_ms();
        
        /* 模拟响应创建 */
        uvhttp_response_t* response = malloc(sizeof(uvhttp_response_t));
        if (response) {
            memset(response, 0, sizeof(uvhttp_response_t));
            /* 注意：status字段通过函数设置，这里仅模拟内存分配 */
            free(response);
        }
        
        double round_end = get_time_ms();
        double elapsed = round_end - round_start;
        
        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
    }
    
    double total_time = get_time_ms() - start_time;
    double avg_time = total_time / BENCHMARK_ROUNDS;
    
    printf("轮次: %d\n", BENCHMARK_ROUNDS);
    printf("总时间: %.2f ms\n", total_time);
    printf("平均时间: %.6f ms\n", avg_time);
    printf("最小时间: %.6f ms\n", min_time);
    printf("最大时间: %.6f ms\n", max_time);
    printf("每秒操作数: %.0f ops/s\n", BENCHMARK_ROUNDS / (total_time / 1000.0));
    printf("\n");
}

void benchmark_memory_allocation() {
    printf("=== 内存分配基准测试 ===\n");
    
    const char* test_names[] = {"小分配(100B)", "中分配(1KB)", "大分配(10KB)"};
    const int test_sizes[] = {SMALL_PAYLOAD_SIZE, MEDIUM_PAYLOAD_SIZE, LARGE_PAYLOAD_SIZE};
    const int test_rounds[] = {BENCHMARK_ROUNDS * 10, BENCHMARK_ROUNDS, BENCHMARK_ROUNDS / 10};
    
    for (int test = 0; test < 3; test++) {
        printf("--- %s ---\n", test_names[test]);
        
        double start_time = get_time_ms();
        void* ptrs[test_rounds[test]];
        
        /* 分配内存 */
        for (int i = 0; i < test_rounds[test]; i++) {
            ptrs[i] = malloc(test_sizes[test]);
            if (ptrs[i]) {
                memset(ptrs[i], 0xAA, test_sizes[test]);
            }
        }
        
        double alloc_time = get_time_ms() - start_time;
        
        /* 释放内存 */
        start_time = get_time_ms();
        for (int i = 0; i < test_rounds[test]; i++) {
            if (ptrs[i]) {
                free(ptrs[i]);
            }
        }
        
        double free_time = get_time_ms() - start_time;
        double total_time = alloc_time + free_time;
        
        printf("分配轮次: %d\n", test_rounds[test]);
        printf("分配时间: %.2f ms\n", alloc_time);
        printf("释放时间: %.2f ms\n", free_time);
        printf("总时间: %.2f ms\n", total_time);
        printf("分配速度: %.0f allocs/s\n", test_rounds[test] / (alloc_time / 1000.0));
        printf("释放速度: %.0f frees/s\n", test_rounds[test] / (free_time / 1000.0));
        printf("吞吐量: %.2f MB/s\n", 
               (test_rounds[test] * test_sizes[test] / 1024.0 / 1024.0) / (total_time / 1000.0));
        printf("\n");
    }
}

void benchmark_string_operations() {
    printf("=== 字符串操作基准测试 ===\n");
    
    const char* test_string = "This is a test string for benchmarking string operations";
    char dest[256];
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < BENCHMARK_ROUNDS * 100; i++) {
        /* 测试字符串复制 */
        uvhttp_safe_strncpy(dest, test_string, sizeof(dest));
        
        /* 测试字符串长度 */
        size_t len = strlen(dest);
        
        /* 测试字符串比较 */
        if (strcmp(dest, test_string) == 0) {
            /* 字符串相等 */
        }
    }
    
    double total_time = get_time_ms() - start_time;
    double ops_per_sec = (BENCHMARK_ROUNDS * 100 * 3) / (total_time / 1000.0);
    
    printf("操作总数: %d\n", BENCHMARK_ROUNDS * 100 * 3);
    printf("总时间: %.2f ms\n", total_time);
    printf("每秒操作数: %.0f ops/s\n", ops_per_sec);
    printf("平均操作时间: %.6f ms\n", total_time / (BENCHMARK_ROUNDS * 100 * 3));
    printf("\n");
}

int main(int argc, char* argv[]) {
    printf("=== UVHTTP 性能基准测试 ===\n");
    printf("测试时间: %s", ctime(&(time_t){time(NULL)}));
    printf("系统信息: ");
    fflush(stdout);
    system("uname -a");
    printf("\n");
    
    /* 运行各项基准测试 */
    benchmark_request_creation();
    benchmark_response_creation();
    benchmark_memory_allocation();
    benchmark_string_operations();
    
    printf("=== 性能基准测试完成 ===\n");
    
    return 0;
}