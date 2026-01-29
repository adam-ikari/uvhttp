/*
 * 性能测试端到端测试
 * 测试服务器在高并发场景下的性能表现
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    unsigned long request_count;
    time_t start_time;
} app_context_t;

/* 全局应用上下文 */
static app_context_t* g_app_context = NULL;

/* 信号处理器 */
static void on_sigint(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

static void on_sigterm(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

/* 简单响应处理器 - 最小延迟 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    if (g_app_context) {
        g_app_context->request_count++;
    }
    
    const char* body = "OK";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, 2);
    uvhttp_response_send(response);
    
    return 0;
}

/* JSON 响应处理器 */
static int json_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    if (g_app_context) {
        g_app_context->request_count++;
    }
    
    const char* json_body = "{\"status\":\"ok\",\"message\":\"Performance test\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 大响应处理器 - 1KB */
static int large_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    if (g_app_context) {
        g_app_context->request_count++;
    }
    
    const char* large_body = "This is a larger response body designed to test performance under load. "
                            "It contains approximately 1024 bytes of data to simulate a more realistic "
                            "response size. Performance testing is crucial for ensuring that the server "
                            "can handle high traffic loads without degradation in response times or "
                            "throughput. This helps identify bottlenecks and optimize the system for "
                            "production environments with varying levels of concurrent access.";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, large_body, strlen(large_body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 延迟处理器 - 模拟慢速响应 */
static int delayed_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    if (g_app_context) {
        g_app_context->request_count++;
    }
    
    /* 模拟 10ms 延迟 */
    struct timespec ts = {0, 10000000}; /* 10ms */
    nanosleep(&ts, NULL);
    
    const char* body = "Delayed response";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 统计处理器 */
static int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = g_app_context;
    time_t now = time(NULL);
    double elapsed = difftime(now, ctx->start_time);
    double rps = elapsed > 0 ? ctx->request_count / elapsed : 0;
    
    char stats[512];
    snprintf(stats, sizeof(stats),
             "{\n"
             "  \"total_requests\": %lu,\n"
             "  \"uptime_seconds\": %.0f,\n"
             "  \"requests_per_second\": %.2f,\n"
             "  \"server_info\": {\n"
             "    \"name\": \"uvhttp\",\n"
             "    \"version\": \"2.2.0\",\n"
             "    \"test_mode\": \"performance\"\n"
             "  }\n"
             "}",
             ctx->request_count, elapsed, rps);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, stats, strlen(stats));
    uvhttp_response_send(response);
    
    printf("Stats: %lu requests, %.2f RPS\n", ctx->request_count, rps);
    return 0;
}

/* 重置统计处理器 */
static int reset_stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = g_app_context;
    
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Statistics reset\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("Statistics reset\n");
    return 0;
}

/* 主页处理器 - 测试说明 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Performance E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "pre { background: #f5f5f5; padding: 15px; border-radius: 5px; overflow-x: auto; }"
        ".stat { margin: 10px 0; padding: 15px; background: #e7f3ff; border-radius: 5px; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>⚡ Performance End-to-End Test Server</h1>"
        "<p>测试服务器在高并发场景下的性能表现</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /simple - 简单响应（最小延迟）"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /json - JSON 响应"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /large - 大响应（1KB）"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /delayed - 延迟响应（10ms）"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /stats - 性能统计"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /reset - 重置统计"
        "</div>"
        ""
        "<h2>性能测试命令：</h2>"
        "<pre>"
        "# 使用 wrk 进行性能测试\n"
        "wrk -t4 -c100 -d30s http://localhost:8086/simple\n"
        ""
        "# 测试 JSON 响应\n"
        "wrk -t4 -c100 -d30s http://localhost:8086/json\n"
        ""
        "# 测试大响应\n"
        "wrk -t4 -c100 -d30s http://localhost:8086/large\n"
        ""
        "# 测试延迟响应\n"
        "wrk -t4 -c100 -d30s http://localhost:8086/delayed\n"
        ""
        "# 使用 ab 进行性能测试\n"
        "ab -n 10000 -c 100 http://localhost:8086/simple\n"
        ""
        "# 查看实时统计\n"
        "curl http://localhost:8086/stats\n"
        ""
        "# 重置统计\n"
        "curl http://localhost:8086/reset\n"
        "</pre>"
        ""
        "<h2>性能指标：</h2>"
        "<div class=\"stat\">"
        "<strong>目标指标：</strong><br>"
        "- 峰值吞吐量: 20,000+ RPS<br>"
        "- 平均延迟: < 5ms<br>"
        "- P99 延迟: < 50ms<br>"
        "- 错误率: < 0.1%"
        "</div>"
        ""
        "<h2>测试场景：</h2>"
        "<ul>"
        "<li>✓ 低并发测试（10 connections）</li>"
        "<li>✓ 中等并发测试（100 connections）</li>"
        "<li>✓ 高并发测试（1000 connections）</li>"
        "<li>✓ 长时间运行测试（5+ minutes）</li>"
        "<li>✓ 不同响应大小测试</li>"
        "<li>✓ 延迟测试</li>"
        "</ul>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    printf("Index page accessed\n");
    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8086;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    
    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 设置服务器用户数据 */
    ctx->server->user_data = ctx;
    
    /* 设置全局应用上下文 */
    g_app_context = ctx;
    
    /* 创建路由器 */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 - 主页 */
    uvhttp_router_add_route(ctx->router, "/", index_handler);
    
    /* 添加路由 - 性能测试端点 */
    uvhttp_router_add_route(ctx->router, "/simple", simple_handler);
    uvhttp_router_add_route(ctx->router, "/json", json_handler);
    uvhttp_router_add_route(ctx->router, "/large", large_handler);
    uvhttp_router_add_route(ctx->router, "/delayed", delayed_handler);
    
    /* 添加路由 - 统计端点 */
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    uvhttp_router_add_route(ctx->router, "/reset", reset_stats_handler);
    
    /* 设置路由器到服务器 */
    ctx->server->router = ctx->router;
    
    /* 初始化信号处理器 */
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    /* 启动服务器 */
    result = uvhttp_server_listen(ctx->server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("Performance E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("========================================\n");
    printf("\n测试功能:\n");
    printf("  - 高并发请求处理\n");
    printf("  - 性能指标统计\n");
    printf("  - 延迟测试\n");
    printf("  - 吞吐量测试\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /simple (简单响应)\n");
    printf("  - /json (JSON 响应)\n");
    printf("  - /large (大响应)\n");
    printf("  - /delayed (延迟响应)\n");
    printf("  - /stats (性能统计)\n");
    printf("  - /reset (重置统计)\n");
    printf("\n性能测试工具:\n");
    printf("  - wrk: wrk -t4 -c100 -d30s http://localhost:%d/simple\n", port);
    printf("  - ab: ab -n 10000 -c 100 http://localhost:%d/simple\n", port);
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
        }
        uvhttp_free(ctx);
    }
    
    printf("\n========================================\n");
    printf("服务器已停止\n");
    printf("========================================\n");
    
    return 0;
}