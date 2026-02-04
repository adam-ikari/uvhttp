/**
 * @file benchmark_database_simulation.c
 * @brief 数据库查询模拟性能基准测试
 *
 * 这个程序模拟真实的数据库查询场景，测试服务器在处理不同延迟查询时的性能。
 * 模拟场景包括：
 * - 快速查询（1-5ms）
 * - 中等查询（10-50ms）
 * - 慢速查询（100-500ms）
 * - 混合查询（真实生产环境场景）
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <inttypes.h>

#define PORT 18085

/* 查询延迟配置（微秒） */
#define QUERY_DELAY_FAST_MIN      1000   /* 1ms */
#define QUERY_DELAY_FAST_MAX      5000   /* 5ms */
#define QUERY_DELAY_MEDIUM_MIN    10000  /* 10ms */
#define QUERY_DELAY_MEDIUM_MAX    50000  /* 50ms */
#define QUERY_DELAY_SLOW_MIN      100000 /* 100ms */
#define QUERY_DELAY_SLOW_MAX      500000 /* 500ms */

/* 查询类型 */
typedef enum {
    QUERY_TYPE_FAST,      /* 快速查询 */
    QUERY_TYPE_MEDIUM,    /* 中等查询 */
    QUERY_TYPE_SLOW,      /* 慢速查询 */
    QUERY_TYPE_MIXED      /* 混合查询 */
} query_type_t;

/* 查询统计 */
typedef struct {
    int total_queries;
    int successful_queries;
    int failed_queries;

    /* 延迟统计 */
    uint64_t total_latency_us;
    uint64_t min_latency_us;
    uint64_t max_latency_us;

    /* 按类型统计 */
    int fast_queries;
    int medium_queries;
    int slow_queries;
} query_stats_t;

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    query_stats_t stats;
} benchmark_context_t;

/* 静态上下文指针 - 仅用于 benchmark 测试程序 */
static benchmark_context_t* g_benchmark_ctx = NULL;

/* 获取当前时间（微秒） */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 模拟数据库查询延迟 */
static void simulate_database_delay(query_type_t type) {
    uint64_t delay_us;
    uint64_t now = get_timestamp_us();

    switch (type) {
        case QUERY_TYPE_FAST:
            /* 快速查询：1-5ms */
            delay_us = QUERY_DELAY_FAST_MIN +
                      (rand() % (QUERY_DELAY_FAST_MAX - QUERY_DELAY_FAST_MIN));
            break;

        case QUERY_TYPE_MEDIUM:
            /* 中等查询：10-50ms */
            delay_us = QUERY_DELAY_MEDIUM_MIN +
                      (rand() % (QUERY_DELAY_MEDIUM_MAX - QUERY_DELAY_MEDIUM_MIN));
            break;

        case QUERY_TYPE_SLOW:
            /* 慢速查询：100-500ms */
            delay_us = QUERY_DELAY_SLOW_MIN +
                      (rand() % (QUERY_DELAY_SLOW_MAX - QUERY_DELAY_SLOW_MIN));
            break;

        case QUERY_TYPE_MIXED:
            /* 混合查询：70% 快速，20% 中等，10% 慢速 */
            {
                int r = rand() % 100;
                if (r < 70) {
                    delay_us = QUERY_DELAY_FAST_MIN +
                              (rand() % (QUERY_DELAY_FAST_MAX - QUERY_DELAY_FAST_MIN));
                } else if (r < 90) {
                    delay_us = QUERY_DELAY_MEDIUM_MIN +
                              (rand() % (QUERY_DELAY_MEDIUM_MAX - QUERY_DELAY_MEDIUM_MIN));
                } else {
                    delay_us = QUERY_DELAY_SLOW_MIN +
                              (rand() % (QUERY_DELAY_SLOW_MAX - QUERY_DELAY_SLOW_MIN));
                }
            }
            break;

        default:
            delay_us = 1000;
            break;
    }

    /* 模拟延迟 */
    usleep(delay_us);
}

/* 更新查询统计 */
static void update_query_stats(query_stats_t* stats, query_type_t type, uint64_t latency_us) {
    stats->total_queries++;
    stats->successful_queries++;
    stats->total_latency_us += latency_us;

    if (stats->total_queries == 1) {
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

    /* 更新类型统计 */
    switch (type) {
        case QUERY_TYPE_FAST:
            stats->fast_queries++;
            break;
        case QUERY_TYPE_MEDIUM:
            stats->medium_queries++;
            break;
        case QUERY_TYPE_SLOW:
            stats->slow_queries++;
            break;
        default:
            break;
    }
}

/* 打印查询统计 */
static void print_query_stats(query_stats_t* stats) {
    printf("========================================\n");
    printf("  数据库查询统计\n");
    printf("========================================\n\n");

    printf("=== 查询统计 ===\n");
    printf("总查询数: %d\n", stats->total_queries);
    printf("成功查询数: %d\n", stats->successful_queries);
    printf("失败查询数: %d\n", stats->failed_queries);

    if (stats->total_queries > 0) {
        double success_rate = (double)stats->successful_queries / stats->total_queries * 100.0;
        printf("成功率: %.2f%%\n", success_rate);

        double avg_latency_ms = (double)stats->total_latency_us / stats->total_queries / 1000.0;
        printf("平均延迟: %.3f ms\n", avg_latency_ms);
        printf("最小延迟: %.3f ms\n", stats->min_latency_us / 1000.0);
        printf("最大延迟: %.3f ms\n", stats->max_latency_us / 1000.0);
    }
    printf("\n");

    printf("=== 查询类型分布 ===\n");
    if (stats->total_queries > 0) {
        printf("快速查询: %d (%.1f%%)\n",
               stats->fast_queries,
               (double)stats->fast_queries / stats->total_queries * 100.0);
        printf("中等查询: %d (%.1f%%)\n",
               stats->medium_queries,
               (double)stats->medium_queries / stats->total_queries * 100.0);
        printf("慢速查询: %d (%.1f%%)\n",
               stats->slow_queries,
               (double)stats->slow_queries / stats->total_queries * 100.0);
    }
    printf("\n");

    printf("=== 性能评估 ===\n");
    if (stats->total_queries > 0) {
        double avg_latency_ms = (double)stats->total_latency_us / stats->total_queries / 1000.0;

        /* 平均延迟目标: < 50ms */
        if (avg_latency_ms < 50.0) {
            printf("✅ 平均延迟: 优秀 (%.3f ms < 50 ms)\n", avg_latency_ms);
        } else if (avg_latency_ms < 100.0) {
            printf("⚠️  平均延迟: 良好 (%.3f ms < 100 ms)\n", avg_latency_ms);
        } else {
            printf("❌ 平均延迟: 需要改进 (%.3f ms >= 100 ms)\n", avg_latency_ms);
        }

        /* 错误率目标: < 0.1% */
        if (stats->total_queries > 0) {
            double error_rate = (double)stats->failed_queries / stats->total_queries * 100.0;
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

/* 快速查询处理器 */
static int fast_query_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response) {
        g_benchmark_ctx->stats.failed_queries++;
        return -1;
    }

    uint64_t start = get_timestamp_us();

    /* 模拟快速数据库查询 */
    simulate_database_delay(QUERY_TYPE_FAST);

    const char* body = "{\"status\":\"ok\",\"query_type\":\"fast\",\"data\":[]}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    update_query_stats(&g_benchmark_ctx->stats, QUERY_TYPE_FAST, end - start);

    return 0;
}

/* 中等查询处理器 */
static int medium_query_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response) {
        g_benchmark_ctx->stats.failed_queries++;
        return -1;
    }

    uint64_t start = get_timestamp_us();

    /* 模拟中等数据库查询 */
    simulate_database_delay(QUERY_TYPE_MEDIUM);

    const char* body = "{\"status\":\"ok\",\"query_type\":\"medium\",\"data\":[{\"id\":1},{\"id\":2}]}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    update_query_stats(&g_benchmark_ctx->stats, QUERY_TYPE_MEDIUM, end - start);

    return 0;
}

/* 慢速查询处理器 */
static int slow_query_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response) {
        g_benchmark_ctx->stats.failed_queries++;
        return -1;
    }

    uint64_t start = get_timestamp_us();

    /* 模拟慢速数据库查询 */
    simulate_database_delay(QUERY_TYPE_SLOW);

    const char* body = "{\"status\":\"ok\",\"query_type\":\"slow\",\"data\":[{\"id\":1,\"name\":\"Item 1\"},{\"id\":2,\"name\":\"Item 2\"},{\"id\":3,\"name\":\"Item 3\"}]}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    update_query_stats(&g_benchmark_ctx->stats, QUERY_TYPE_SLOW, end - start);

    return 0;
}

/* 混合查询处理器 */
static int mixed_query_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response) {
        g_benchmark_ctx->stats.failed_queries++;
        return -1;
    }

    uint64_t start = get_timestamp_us();

    /* 模拟混合数据库查询 */
    simulate_database_delay(QUERY_TYPE_MIXED);

    const char* body = "{\"status\":\"ok\",\"query_type\":\"mixed\",\"data\":[{\"id\":1}]}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    uint64_t end = get_timestamp_us();
    update_query_stats(&g_benchmark_ctx->stats, QUERY_TYPE_MIXED, end - start);

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

/* 运行数据库模拟基准测试 */
static void run_database_benchmark(void) {
    printf("========================================\n");
    printf("  数据库查询模拟性能基准测试\n");
    printf("========================================\n\n");

    printf("测试配置:\n");
    printf("  快速查询延迟: %d-%d μs (%.1f-%.1f ms)\n",
           QUERY_DELAY_FAST_MIN, QUERY_DELAY_FAST_MAX,
           QUERY_DELAY_FAST_MIN / 1000.0, QUERY_DELAY_FAST_MAX / 1000.0);
    printf("  中等查询延迟: %d-%d μs (%.1f-%.1f ms)\n",
           QUERY_DELAY_MEDIUM_MIN, QUERY_DELAY_MEDIUM_MAX,
           QUERY_DELAY_MEDIUM_MIN / 1000.0, QUERY_DELAY_MEDIUM_MAX / 1000.0);
    printf("  慢速查询延迟: %d-%d μs (%.1f-%.1f ms)\n",
           QUERY_DELAY_SLOW_MIN, QUERY_DELAY_SLOW_MAX,
           QUERY_DELAY_SLOW_MIN / 1000.0, QUERY_DELAY_SLOW_MAX / 1000.0);
    printf("  端口: %d\n", PORT);
    printf("\n");

    /* 初始化随机数种子 */
    srand((unsigned int)get_timestamp_us());

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
    memset(&ctx->stats, 0, sizeof(query_stats_t));

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
    uvhttp_router_add_route(router, "/api/fast", fast_query_handler);
    uvhttp_router_add_route(router, "/api/medium", medium_query_handler);
    uvhttp_router_add_route(router, "/api/slow", slow_query_handler);
    uvhttp_router_add_route(router, "/api/mixed", mixed_query_handler);
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
    printf("  /api/fast   - 快速查询 (1-5ms)\n");
    printf("  /api/medium - 中等查询 (10-50ms)\n");
    printf("  /api/slow   - 慢速查询 (100-500ms)\n");
    printf("  /api/mixed  - 混合查询 (70% 快速, 20% 中等, 10% 慢速)\n");
    printf("\n");
    printf("请使用 wrk 或 ab 进行性能测试:\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/api/fast\n", PORT);
    printf("  wrk -t4 -c50 -d30s http://127.0.0.1:%d/api/medium\n", PORT);
    printf("  wrk -t4 -c20 -d30s http://127.0.0.1:%d/api/slow\n", PORT);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/api/mixed\n", PORT);
    printf("\n");
    printf("按 Ctrl+C 停止服务器并查看统计...\n");
    printf("\n");

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 打印查询统计 */
    print_query_stats(&ctx->stats);

    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    uvhttp_server_free(ctx->server);
    g_benchmark_ctx = NULL;
    free(ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 数据库查询模拟基准测试\n");
    printf("========================================\n\n");

    /* 运行数据库模拟基准测试 */
    run_database_benchmark();

    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");

    return 0;
}