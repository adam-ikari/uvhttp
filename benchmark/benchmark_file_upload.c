/**
 * @file benchmark_file_upload.c
 * @brief 文件上传性能基准测试
 *
 * 这个程序测试服务器处理文件上传请求的性能，包括不同大小的文件上传。
 * 测试场景包括：
 * - 小文件上传（1KB-10KB）
 * - 中等文件上传（100KB-1MB）
 * - 大文件上传（10MB-100MB）
 * - 并发文件上传
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>

#define PORT 18086
#define MAX_FILE_SIZE (100 * 1024 * 1024)  /* 100MB */

/* 文件上传统计 */
typedef struct {
    int total_uploads;
    int successful_uploads;
    int failed_uploads;

    /* 字节统计 */
    uint64_t total_bytes_uploaded;
    uint64_t total_bytes_processed;

    /* 延迟统计 */
    uint64_t total_latency_us;
    uint64_t min_latency_us;
    uint64_t max_latency_us;

    /* 按大小分类统计 */
    int small_uploads;    /* < 10KB */
    int medium_uploads;   /* 10KB - 1MB */
    int large_uploads;    /* > 1MB */
} upload_stats_t;

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    upload_stats_t stats;
} benchmark_context_t;

/* 静态上下文指针 - 仅用于 benchmark 测试程序 */
static benchmark_context_t* g_benchmark_ctx = NULL;

/* 获取当前时间（微秒） */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 更新上传统计 */
static void update_upload_stats(upload_stats_t* stats, uint64_t bytes_uploaded, uint64_t latency_us) {
    stats->total_uploads++;
    stats->successful_uploads++;
    stats->total_bytes_uploaded += bytes_uploaded;
    stats->total_bytes_processed += bytes_uploaded;
    stats->total_latency_us += latency_us;

    if (stats->total_uploads == 1) {
        stats->min_latency_us = latency_us;
        stats->max_latency_us = latency_us;
    } else {
        if (latency_us < stats->min_latency_us) {
            stats->min_latency_us = latency_us;
        }
        if (latency_us > stats->max_latency_us) {
            stats->max_latency_us = latency_us;
        }
    }

    /* 按大小分类 */
    if (bytes_uploaded < 10 * 1024) {
        stats->small_uploads++;
    } else if (bytes_uploaded < 1024 * 1024) {
        stats->medium_uploads++;
    } else {
        stats->large_uploads++;
    }
}

/* 打印上传统计 */
static void print_upload_stats(upload_stats_t* stats) {
    printf("========================================\n");
    printf("  文件上传统计\n");
    printf("========================================\n\n");

    printf("=== 上传统计 ===\n");
    printf("总上传数: %d\n", stats->total_uploads);
    printf("成功上传数: %d\n", stats->successful_uploads);
    printf("失败上传数: %d\n", stats->failed_uploads);

    if (stats->total_uploads > 0) {
        double success_rate = (double)stats->successful_uploads / stats->total_uploads * 100.0;
        printf("成功率: %.2f%%\n", success_rate);

        double avg_latency_ms = (double)stats->total_latency_us / stats->total_uploads / 1000.0;
        printf("平均延迟: %.3f ms\n", avg_latency_ms);
        printf("最小延迟: %.3f ms\n", stats->min_latency_us / 1000.0);
        printf("最大延迟: %.3f ms\n", stats->max_latency_us / 1000.0);
    }
    printf("\n");

    printf("=== 字节统计 ===\n");
    printf("总上传字节: %" PRIu64 " bytes (%.2f MB)\n",
           stats->total_bytes_uploaded,
           stats->total_bytes_uploaded / (1024.0 * 1024.0));
    printf("总处理字节: %" PRIu64 " bytes (%.2f MB)\n",
           stats->total_bytes_processed,
           stats->total_bytes_processed / (1024.0 * 1024.0));

    if (stats->total_uploads > 0) {
        double avg_size_kb = (double)stats->total_bytes_uploaded / stats->total_uploads / 1024.0;
        printf("平均文件大小: %.2f KB\n", avg_size_kb);

        double avg_throughput_mb_s = (double)stats->total_bytes_uploaded /
                                     (stats->total_latency_us / 1000000.0) /
                                     (1024.0 * 1024.0);
        printf("平均吞吐量: %.2f MB/s\n", avg_throughput_mb_s);
    }
    printf("\n");

    printf("=== 文件大小分布 ===\n");
    if (stats->total_uploads > 0) {
        printf("小文件 (< 10KB): %d (%.1f%%)\n",
               stats->small_uploads,
               (double)stats->small_uploads / stats->total_uploads * 100.0);
        printf("中等文件 (10KB - 1MB): %d (%.1f%%)\n",
               stats->medium_uploads,
               (double)stats->medium_uploads / stats->total_uploads * 100.0);
        printf("大文件 (> 1MB): %d (%.1f%%)\n",
               stats->large_uploads,
               (double)stats->large_uploads / stats->total_uploads * 100.0);
    }
    printf("\n");

    printf("=== 性能评估 ===\n");
    if (stats->total_uploads > 0) {
        double avg_latency_ms = (double)stats->total_latency_us / stats->total_uploads / 1000.0;

        /* 平均延迟目标: < 100ms */
        if (avg_latency_ms < 100.0) {
            printf(" 平均延迟: 优秀 (%.3f ms < 100 ms)\n", avg_latency_ms);
        } else if (avg_latency_ms < 500.0) {
            printf("  平均延迟: 良好 (%.3f ms < 500 ms)\n", avg_latency_ms);
        } else {
            printf(" 平均延迟: 需要改进 (%.3f ms >= 500 ms)\n", avg_latency_ms);
        }

        /* 吞吐量目标: > 10 MB/s */
        double avg_throughput_mb_s = (double)stats->total_bytes_uploaded /
                                     (stats->total_latency_us / 1000000.0) /
                                     (1024.0 * 1024.0);
        if (avg_throughput_mb_s > 10.0) {
            printf(" 吞吐量: 优秀 (%.2f MB/s > 10 MB/s)\n", avg_throughput_mb_s);
        } else if (avg_throughput_mb_s > 5.0) {
            printf("  吞吐量: 良好 (%.2f MB/s > 5 MB/s)\n", avg_throughput_mb_s);
        } else {
            printf(" 吞吐量: 需要改进 (%.2f MB/s <= 5 MB/s)\n", avg_throughput_mb_s);
        }

        /* 错误率目标: < 0.1% */
        double error_rate = (double)stats->failed_uploads / stats->total_uploads * 100.0;
        if (error_rate < 0.1) {
            printf(" 错误率: 优秀 (%.2f%% < 0.1%%)\n", error_rate);
        } else if (error_rate < 1.0) {
            printf("  错误率: 良好 (%.2f%% < 1%%)\n", error_rate);
        } else {
            printf(" 错误率: 需要改进 (%.2f%% >= 1%%)\n", error_rate);
        }
    }
    printf("\n");
}

/* 文件上传处理器 */
static int upload_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response) {
        g_benchmark_ctx->stats.failed_uploads++;
        return -1;
    }

    uint64_t start = get_timestamp_us();

    /* 获取请求体大小 */
    const char* content_length = uvhttp_request_get_header(request, "Content-Length");
    uint64_t body_size = 0;
    if (content_length) {
        body_size = (uint64_t)atoll(content_length);
    }

    /* 限制最大文件大小 */
    if (body_size > MAX_FILE_SIZE) {
        uvhttp_response_set_status(response, 413);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error_body = "{\"error\":\"File too large\"}";
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);

        g_benchmark_ctx->stats.failed_uploads++;
        return 0;
    }

    /* 获取请求体 */
    const char* body = uvhttp_request_get_body(request);
    size_t body_len = uvhttp_request_get_body_length(request);

    /* 模拟文件处理 */
    if (body && body_len > 0) {
        /* 在实际应用中，这里会保存文件到磁盘 */
        /* 这里只是模拟处理 */
        usleep(100);  /* 模拟 100μs 处理时间 */
    }

    /* 返回成功响应 */
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "{\"status\":\"ok\",\"size\":%zu,\"filename\":\"uploaded_%lu\"}",
             body_len, start);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    update_upload_stats(&g_benchmark_ctx->stats, body_len, end - start);

    return 0;
}

/* 健康检查处理器 */
static int health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;

    if (!response) {
        return -1;
    }

    const char* body = "{\"status\":\"ok\",\"service\":\"file-upload\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

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

/* 运行文件上传基准测试 */
static void run_upload_benchmark(void) {
    printf("========================================\n");
    printf("  文件上传性能基准测试\n");
    printf("========================================\n\n");

    printf("测试配置:\n");
    printf("  最大文件大小: %d bytes (%.2f MB)\n",
           MAX_FILE_SIZE, MAX_FILE_SIZE / (1024.0 * 1024.0));
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
    memset(&ctx->stats, 0, sizeof(upload_stats_t));

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
    uvhttp_router_add_route(router, "/health", health_handler);
    uvhttp_router_add_route(router, "/upload", upload_handler);
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
    printf("\n");
    printf("测试端点:\n");
    printf("  /health - 健康检查\n");
    printf("  /upload - 文件上传\n");
    printf("\n");
    printf("请使用 curl 或 wrk 进行性能测试:\n");
    printf("  # 小文件上传\n");
    printf("  curl -X POST -F 'file=@/path/to/small.txt' http://127.0.0.1:%d/upload\n", PORT);
    printf("\n");
    printf("  # 中等文件上传\n");
    printf("  curl -X POST -F 'file=@/path/to/medium.txt' http://127.0.0.1:%d/upload\n", PORT);
    printf("\n");
    printf("  # 大文件上传\n");
    printf("  curl -X POST -F 'file=@/path/to/large.dat' http://127.0.0.1:%d/upload\n", PORT);
    printf("\n");
    printf("  # 使用 wrk 进行并发测试\n");
    printf("  # 注意: wrk 不支持文件上传，需要使用其他工具\n");
    printf("\n");
    printf("按 Ctrl+C 停止服务器并查看统计...\n");
    printf("\n");

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 打印上传统计 */
    print_upload_stats(&ctx->stats);

    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    uvhttp_server_free(ctx->server);
    g_benchmark_ctx = NULL;
    free(ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 文件上传性能基准测试\n");
    printf("========================================\n\n");

    /* 运行文件上传基准测试 */
    run_upload_benchmark();

    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");

    return 0;
}