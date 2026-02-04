/**
 * @file benchmark_comprehensive.c
 * @brief 综合性能基准测试
 * 
 * 这个程序运行全面的性能测试，包括 RPS、延迟、连接管理和内存使用。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <inttypes.h>

#define TEST_DURATION 10  /* 测试时长（秒） */
#define PORT 18084
#define MAX_SAMPLES 100000

/* 综合统计 */
typedef struct {
    /* RPS 统计 */
    int total_requests;
    int successful_requests;
    int failed_requests;

    /* 延迟统计 */
    uint64_t latency_samples[MAX_SAMPLES];
    int latency_count;
    uint64_t min_latency;
    uint64_t max_latency;
    double latency_sum;

    /* 内存统计 */
    size_t peak_memory;
    size_t current_memory;

    /* CPU 统计 */
    double cpu_usage_percent;
    uint64_t cpu_time_total;
    uint64_t cpu_time_user;
    uint64_t cpu_time_system;

    /* 时间统计 */
    uint64_t start_time;
    uint64_t end_time;
} comprehensive_stats_t;

/* 应用上下文结构 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    comprehensive_stats_t stats;
} benchmark_context_t;

/* 静态上下文指针 - 仅用于 benchmark 测试程序 */
static benchmark_context_t* g_benchmark_ctx = NULL;

/* 获取当前时间（微秒） */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 获取当前进程内存使用量（KB） */
static size_t get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

/* 获取当前 CPU 使用率（百分比） */
static double get_cpu_usage_percent(void) {
    static uint64_t last_cpu_time_total = 0;
    static struct timeval last_time = {0, 0};

    struct rusage usage;
    struct timeval current_time;
    uint64_t cpu_time_user, cpu_time_system, cpu_time_total;
    uint64_t time_delta_us;
    double cpu_usage = 0.0;

    /* 获取当前 CPU 时间 */
    getrusage(RUSAGE_SELF, &usage);
    cpu_time_user = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
    cpu_time_system = usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
    cpu_time_total = cpu_time_user + cpu_time_system;

    /* 获取当前时间 */
    gettimeofday(&current_time, NULL);

    /* 第一次调用，初始化 */
    if (last_time.tv_sec == 0) {
        last_cpu_time_total = cpu_time_total;
        last_time = current_time;
        return 0.0;
    }

    /* 计算时间差 */
    time_delta_us = (current_time.tv_sec - last_time.tv_sec) * 1000000 +
                    (current_time.tv_usec - last_time.tv_usec);

    /* 计算使用率 */
    if (time_delta_us > 0) {
        uint64_t cpu_time_delta = cpu_time_total - last_cpu_time_total;
        cpu_usage = (double)cpu_time_delta * 100.0 / time_delta_us;
    }

    /* 更新最后值 */
    last_cpu_time_total = cpu_time_total;
    last_time = current_time;

    return cpu_usage;
}

/* 添加延迟样本 */
static void add_latency_sample(benchmark_context_t* ctx, uint64_t latency_us) {
    if (ctx->stats.latency_count < MAX_SAMPLES) {
        ctx->stats.latency_samples[ctx->stats.latency_count++] = latency_us;
        ctx->stats.latency_sum += latency_us;
        
        if (ctx->stats.latency_count == 1) {
            ctx->stats.min_latency = latency_us;
            ctx->stats.max_latency = latency_us;
        } else {
            if (latency_us < ctx->stats.min_latency) ctx->stats.min_latency = latency_us;
            if (latency_us > ctx->stats.max_latency) ctx->stats.max_latency = latency_us;
        }
    }
}

/* 比较函数用于 qsort */
static int compare_uint64(const void* a, const void* b) {
    uint64_t val_a = *(const uint64_t*)a;
    uint64_t val_b = *(const uint64_t*)b;
    return (val_a > val_b) ? 1 : ((val_a < val_b) ? -1 : 0);
}

/* 计算百分位数 */
static uint64_t calculate_percentile(benchmark_context_t* ctx, double percentile) {
    if (ctx->stats.latency_count == 0) return 0;
    
    /* 使用 qsort 进行高效排序 */
    qsort(ctx->stats.latency_samples, ctx->stats.latency_count, sizeof(uint64_t), compare_uint64);
    
    int index = (int)(percentile * ctx->stats.latency_count);
    if (index >= ctx->stats.latency_count) index = ctx->stats.latency_count - 1;
    
    return ctx->stats.latency_samples[index];
}

/* 打印综合统计 */
static void print_comprehensive_stats(benchmark_context_t* ctx) {
    printf("========================================\n");
    printf("  综合性能统计报告\n");
    printf("========================================\n\n");
    
    /* RPS 统计 */
    printf("=== RPS 统计 ===\n");
    printf("总请求数: %d\n", ctx->stats.total_requests);
    printf("成功请求数: %d\n", ctx->stats.successful_requests);
    printf("失败请求数: %d\n", ctx->stats.failed_requests);
    
    if (ctx->stats.total_requests > 0) {
        double success_rate = (double)ctx->stats.successful_requests / ctx->stats.total_requests * 100.0;
        printf("成功率: %.2f%%\n", success_rate);
    }
    
    uint64_t duration_sec = (ctx->stats.end_time - ctx->stats.start_time) / 1000000;
    if (duration_sec > 0) {
        double rps = (double)ctx->stats.successful_requests / duration_sec;
        printf("RPS: %.2f\n", rps);
    }
    printf("\n");
    
    /* 延迟统计 */
    printf("=== 延迟统计 ===\n");
    printf("样本数量: %d\n", ctx->stats.latency_count);
    
    if (ctx->stats.latency_count > 0) {
        double avg_latency = ctx->stats.latency_sum / ctx->stats.latency_count;
        uint64_t p50 = calculate_percentile(ctx, 0.50);
        uint64_t p95 = calculate_percentile(ctx, 0.95);
        uint64_t p99 = calculate_percentile(ctx, 0.99);
        
        printf("平均延迟: %.2f μs (%.3f ms)\n", avg_latency, avg_latency / 1000.0);
        printf("最小延迟: %" PRIu64 " μs (%.3f ms)\n", ctx->stats.min_latency, ctx->stats.min_latency / 1000.0);
        printf("最大延迟: %" PRIu64 " μs (%.3f ms)\n", ctx->stats.max_latency, ctx->stats.max_latency / 1000.0);
        printf("P50 延迟: %" PRIu64 " μs (%.3f ms)\n", p50, p50 / 1000.0);
        printf("P95 延迟: %" PRIu64 " μs (%.3f ms)\n", p95, p95 / 1000.0);
        printf("P99 延迟: %" PRIu64 " μs (%.3f ms)\n", p99, p99 / 1000.0);
    }
    printf("\n");
    
    /* 内存统计 */
    printf("=== 内存统计 ===\n");
    printf("峰值内存使用: %zu KB (%.2f MB)\n",
           ctx->stats.peak_memory, ctx->stats.peak_memory / 1024.0);
    printf("当前内存使用: %zu KB (%.2f MB)\n",
           ctx->stats.current_memory, ctx->stats.current_memory / 1024.0);
    
    if (ctx->stats.successful_requests > 0) {
        double memory_per_request = (double)ctx->stats.peak_memory / ctx->stats.successful_requests;
        printf("每请求平均内存: %.2f KB\n", memory_per_request);
    }
    printf("\n");
    
    /* CPU 统计 */
    printf("=== CPU 统计 ===\n");
    printf("CPU 使用率: %.2f%%\n", ctx->stats.cpu_usage_percent);
    printf("用户态 CPU 时间: %" PRIu64 " μs (%.3f 秒)\n",
           ctx->stats.cpu_time_user, ctx->stats.cpu_time_user / 1000000.0);
    printf("内核态 CPU 时间: %" PRIu64 " μs (%.3f 秒)\n",
           ctx->stats.cpu_time_system, ctx->stats.cpu_time_system / 1000000.0);
    printf("总 CPU 时间: %" PRIu64 " μs (%.3f 秒)\n",
           ctx->stats.cpu_time_total, ctx->stats.cpu_time_total / 1000000.0);
    
    if (ctx->stats.successful_requests > 0) {
        double cpu_time_per_request = (double)ctx->stats.cpu_time_total / ctx->stats.successful_requests;
        printf("每请求平均 CPU 时间: %.2f μs\n", cpu_time_per_request);
    }
    printf("\n");
    
    /* 性能评估 */
    printf("=== 性能评估 ===\n");
    if (duration_sec > 0) {
        double rps = (double)ctx->stats.successful_requests / duration_sec;
        
        /* 低并发目标: 17,000+ RPS */
        if (rps >= 17000) {
            printf("✅ RPS 性能: 优秀 (%.2f RPS >= 17,000)\n", rps);
        } else if (rps >= 15000) {
            printf("⚠️  RPS 性能: 良好 (%.2f RPS >= 15,000)\n", rps);
        } else {
            printf("❌ RPS 性能: 需要改进 (%.2f RPS < 15,000)\n", rps);
        }
        
        /* 延迟目标: < 15ms */
        if (ctx->stats.latency_count > 0) {
            double avg_latency_ms = ctx->stats.latency_sum / ctx->stats.latency_count / 1000.0;
            if (avg_latency_ms < 15.0) {
                printf("✅ 延迟性能: 优秀 (%.3f ms < 15 ms)\n", avg_latency_ms);
            } else if (avg_latency_ms < 30.0) {
                printf("⚠️  延迟性能: 良好 (%.3f ms < 30 ms)\n", avg_latency_ms);
            } else {
                printf("❌ 延迟性能: 需要改进 (%.3f ms >= 30 ms)\n", avg_latency_ms);
            }
        }
        
        /* 错误率目标: < 0.1% */
        if (ctx->stats.total_requests > 0) {
            double error_rate = (double)ctx->stats.failed_requests / ctx->stats.total_requests * 100.0;
            if (error_rate < 0.1) {
                printf("✅ 错误率: 优秀 (%.2f%% < 0.1%%)\n", error_rate);
            } else if (error_rate < 1.0) {
                printf("⚠️  错误率: 良好 (%.2f%% < 1%%)\n", error_rate);
            } else {
                printf("❌ 错误率: 需要改进 (%.2f%% >= 1%%)\n", error_rate);
            }
        }
    }
    printf("\n");
}

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */

    uint64_t start = get_timestamp_us();

    /* 更新内存统计 */
    size_t current_mem = get_memory_usage_kb();
    if (current_mem > g_benchmark_ctx->stats.peak_memory) {
        g_benchmark_ctx->stats.peak_memory = current_mem;
    }
    g_benchmark_ctx->stats.current_memory = current_mem;

    /* 更新 CPU 统计 */
    double cpu_usage = get_cpu_usage_percent();
    g_benchmark_ctx->stats.cpu_usage_percent = cpu_usage;

    g_benchmark_ctx->stats.total_requests++;

    if (!response) {
        g_benchmark_ctx->stats.failed_requests++;
        return -1;
    }

    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_header(response, "Connection", "keep-alive");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    add_latency_sample(g_benchmark_ctx, end - start);

    g_benchmark_ctx->stats.successful_requests++;

    return 0;
}

/* SIGINT 信号处理器 */
void on_sigint(uv_signal_t* handle, int signum) {
    (void)signum;
    benchmark_context_t* ctx = (benchmark_context_t*)handle->data;
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

/* SIGTERM 信号处理器 */
void on_sigterm(uv_signal_t* handle, int signum) {
    (void)signum;
    benchmark_context_t* ctx = (benchmark_context_t*)handle->data;
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

/* 运行综合基准测试 */
static void run_comprehensive_benchmark(const char* test_name) {
    printf("========================================\n");
    printf("  %s\n", test_name);
    printf("========================================\n\n");

    printf("测试配置:\n");
    printf("  测试时长: %d 秒\n", TEST_DURATION);
    printf("  端口: %d\n", PORT);
    printf("\n");

    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "无法创建事件循环\n");
        return;
    }

    /* 创建应用上下文 */
    benchmark_context_t* ctx = (benchmark_context_t*)malloc(sizeof(benchmark_context_t));
    if (!ctx) {
        fprintf(stderr, "无法分配上下文\n");
        return;
    }
    memset(ctx, 0, sizeof(benchmark_context_t));
    ctx->loop = loop;
    
    /* 设置全局上下文指针 */
    g_benchmark_ctx = ctx;

    /* 重置统计 */
    ctx->stats.peak_memory = get_memory_usage_kb();
    ctx->stats.start_time = get_timestamp_us();

    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK || !ctx->server) {
        fprintf(stderr, "无法创建服务器\n");
        free(ctx);
        return;
    }
    
    /* 设置服务器用户数据 */
    ctx->server->user_data = ctx;
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法创建路由\n");
        uvhttp_server_free(ctx->server);
        free(ctx);
        return;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", simple_handler);
    ctx->server->router = router;
    
    /* 初始化 SIGINT 信号处理器 */
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    /* 初始化 SIGTERM 信号处理器 */
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    /* 启动服务器 */
    result = uvhttp_server_listen(ctx->server, "127.0.0.1", PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法启动服务器\n");
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        free(ctx);
        return;
    }
    
    printf("服务器已启动在 http://127.0.0.1:%d\n", PORT);
    printf("请使用 wrk 或 ab 进行性能测试:\n");
    printf("  wrk -t4 -c100 -d%ds http://127.0.0.1:%d/\n", TEST_DURATION, PORT);
    printf("  ab -n 100000 -c 100 -k http://127.0.0.1:%d/\n", PORT);
    printf("\n");
    printf("等待 %d 秒...\n", TEST_DURATION);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    ctx->stats.end_time = get_timestamp_us();

    /* 收集最终 CPU 统计 */
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    ctx->stats.cpu_time_user = usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec;
    ctx->stats.cpu_time_system = usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
    ctx->stats.cpu_time_total = ctx->stats.cpu_time_user + ctx->stats.cpu_time_system;

    /* 打印综合统计 */
    print_comprehensive_stats(ctx);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    uvhttp_server_free(ctx->server);
    g_benchmark_ctx = NULL;
    free(ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 综合性能基准测试\n");
    printf("========================================\n\n");
    
    /* 综合测试 */
    run_comprehensive_benchmark("综合性能基准测试");
    
    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");
    
    return 0;
}