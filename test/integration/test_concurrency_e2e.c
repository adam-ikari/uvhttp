/*
 * 并发测试端到端测试
 * 测试服务器在高并发场景下的稳定性和正确性
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

/* 并发测试处理器 - 快速响应 */
static int concurrent_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    const char* client_ip = uvhttp_request_get_client_ip(request);
    const char* method = uvhttp_request_get_method(request);
    
    char body[256];
    snprintf(body, sizeof(body),
             "{\"status\":\"ok\",\"client\":\"%s\",\"method\":\"%s\",\"request_id\":%lu}",
             client_ip, method, ctx->request_count);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "X-Request-Count", "1");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 并发 POST 处理器 */
static int concurrent_post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    size_t body_len = uvhttp_request_get_body_length(request);
    const char* body = uvhttp_request_get_body(request);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "{\"status\":\"ok\",\"received_bytes\":%zu,\"request_id\":%lu}",
             body_len, ctx->request_count);
    
    uvhttp_response_set_status(response, 201);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 并发 PUT 处理器 */
static int concurrent_put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Resource updated\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 并发 DELETE 处理器 */
static int concurrent_delete_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Resource deleted\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 并发 HEAD 处理器 */
static int concurrent_head_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "50");
    /* HEAD 请求不发送 body */
    uvhttp_response_send(response);
    
    return 0;
}

/* 并发 OPTIONS 处理器 */
static int concurrent_options_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Allow", "GET, POST, PUT, DELETE, HEAD, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    uvhttp_response_send(response);
    
    return 0;
}

/* 统计处理器 */
static int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    app_context_t* ctx = (app_context_t*)g_app_context;
    time_t now = time(NULL);
    double elapsed = difftime(now, ctx->start_time);
    double rps = elapsed > 0 ? ctx->request_count / elapsed : 0;
    
    char stats[512];
    snprintf(stats, sizeof(stats),
             "{\n"
             "  \"total_requests\": %lu,\n"
             "  \"uptime_seconds\": %.0f,\n"
             "  \"requests_per_second\": %.2f,\n"
             "  \"test_mode\": \"concurrency\"\n"
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
    app_context_t* ctx = (app_context_t*)g_app_context;
    
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
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Concurrency E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "pre { background: #f5f5f5; padding: 15px; border-radius: 5px; overflow-x: auto; }"
        ".scenario { margin: 15px 0; padding: 15px; background: #e7f3ff; border-radius: 5px; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🔄 Concurrency End-to-End Test Server</h1>"
        "<p>测试服务器在高并发场景下的稳定性和正确性</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /concurrent - 并发 GET 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">POST</span> /concurrent - 并发 POST 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">PUT</span> /concurrent - 并发 PUT 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">DELETE</span> /concurrent - 并发 DELETE 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">HEAD</span> /concurrent - 并发 HEAD 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">OPTIONS</span> /concurrent - 并发 OPTIONS 请求"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /stats - 并发统计"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /reset - 重置统计"
        "</div>"
        ""
        "<h2>并发测试场景：</h2>"
        ""
        "<div class=\"scenario\">"
        "<strong>场景 1: 低并发测试（10 connections）</strong><br>"
        "ab -n 1000 -c 10 http://localhost:8088/concurrent"
        "</div>"
        ""
        "<div class=\"scenario\">"
        "<strong>场景 2: 中等并发测试（100 connections）</strong><br>"
        "ab -n 10000 -c 100 http://localhost:8088/concurrent"
        "</div>"
        ""
        "<div class=\"scenario\">"
        "<strong>场景 3: 高并发测试（1000 connections）</strong><br>"
        "ab -n 50000 -c 1000 http://localhost:8088/concurrent"
        "</div>"
        ""
        "<div class=\"scenario\">"
        "<strong>场景 4: 混合方法并发测试</strong><br>"
        "<pre>"
        "# 使用 wrk 进行混合方法测试\n"
        "wrk -t10 -c100 -d30s -s post.lua http://localhost:8088/concurrent\n"
        "</pre>"
        "</div>"
        ""
        "<div class=\"scenario\">"
        "<strong>场景 5: 长时间并发测试（5 minutes）</strong><br>"
        "wrk -t10 -c100 -d300s http://localhost:8088/concurrent"
        "</div>"
        ""
        "<h2>测试命令示例：</h2>"
        "<pre>"
        "# 低并发测试\n"
        "ab -n 1000 -c 10 http://localhost:8088/concurrent\n"
        ""
        "# 中等并发测试\n"
        "ab -n 10000 -c 100 http://localhost:8088/concurrent\n"
        ""
        "# 高并发测试\n"
        "ab -n 50000 -c 1000 http://localhost:8088/concurrent\n"
        ""
        "# 使用 wrk 进行长时间测试\n"
        "wrk -t10 -c100 -d300s http://localhost:8088/concurrent\n"
        ""
        "# 查看实时统计\n"
        "curl http://localhost:8088/stats\n"
        ""
        "# 重置统计\n"
        "curl http://localhost:8088/reset\n"
        "</pre>"
        ""
        "<h2>测试目标：</h2>"
        "<ul>"
        "<li>✓ 无内存泄漏</li>"
        "<li>✓ 无连接泄漏</li>"
        "<li>✓ 响应正确性</li>"
        "<li>✓ 请求计数准确</li>"
        "<li>✓ 高并发稳定性</li>"
        "<li>✓ 错误处理正确</li>"
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
    int port = 8088;
    
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
    
    /* 添加路由 - 并发测试端点 */
    uvhttp_router_add_route(ctx->router, "/concurrent", concurrent_handler);
    
    /* 添加路由 - 统计端点 */
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    uvhttp_router_add_route(ctx->router, "/reset", reset_stats_handler);
    
    /* 设置路由器到服务器 */
    ctx->server->router = ctx->router;
    
    /* 初始化信号处理器 */
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    g_app_context = ctx;
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
    printf("Concurrency E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("========================================\n");
    printf("\n测试功能:\n");
    printf("  - 高并发请求处理\n");
    printf("  - 混合 HTTP 方法测试\n");
    printf("  - 并发稳定性测试\n");
    printf("  - 内存泄漏检测\n");
    printf("  - 连接泄漏检测\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /concurrent (并发测试)\n");
    printf("  - /stats (并发统计)\n");
    printf("  - /reset (重置统计)\n");
    printf("\n并发测试工具:\n");
    printf("  - ab: ab -n 10000 -c 100 http://localhost:%d/concurrent\n", port);
    printf("  - wrk: wrk -t10 -c100 -d300s http://localhost:%d/concurrent\n", port);
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