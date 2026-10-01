/*
 * 限流功能端到端测试
 * 测试服务器内置的限流功能
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    unsigned long request_count;
    unsigned long limited_count;
} app_context_t;

/* 全局应用上下文 */
static app_context_t* g_app_context = NULL;

/* 简单的信号处理器 */
static void simple_signal_handler(int sig) {
    (void)sig;
    exit(0);
}

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

/* 限流端点处理器 */
static int limited_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Request allowed\",\"rate_limited\":true}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("Request #%lu allowed\n", ctx->request_count);
    return 0;
}

/* 无限流端点处理器 */
static int unlimited_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    ctx->request_count++;
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Unlimited endpoint\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 统计处理器 */
static int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    
    char stats[512];
    snprintf(stats, sizeof(stats),
             "{\n"
             "  \"total_requests\": %lu,\n"
             "  \"limited_requests\": %lu,\n"
             "  \"rate_limit_enabled\": %d,\n"
             "  \"rate_limit_max_requests\": %d,\n"
             "  \"rate_limit_window_seconds\": %d\n"
             "}",
             ctx->request_count,
             ctx->limited_count,
             ctx->server->rate_limit_enabled,
             ctx->server->rate_limit_max_requests,
             ctx->server->rate_limit_window_seconds);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, stats, strlen(stats));
    uvhttp_response_send(response);
    
    return 0;
}

/* 重置统计处理器 */
static int reset_stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    
    ctx->request_count = 0;
    ctx->limited_count = 0;
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Statistics reset\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 启用限流处理器 */
static int enable_limit_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    
    /* 启用限流：每分钟 10 个请求 */
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(ctx->server, 10, 60);
    if (result != UVHTTP_OK) {
        const char* error_body = "{\"status\":\"error\",\"message\":\"Failed to enable rate limit\"}";
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);
        return 0;
    }
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Rate limit enabled\",\"max_requests\":10,\"window_seconds\":60}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("Rate limit enabled: 10 requests per 60 seconds\n");
    return 0;
}

/* 禁用限流处理器 */
static int disable_limit_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    app_context_t* ctx = (app_context_t*)g_app_context;
    
    uvhttp_error_t result = uvhttp_server_disable_rate_limit(ctx->server);
    if (result != UVHTTP_OK) {
        const char* error_body = "{\"status\":\"error\",\"message\":\"Failed to disable rate limit\"}";
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);
        return 0;
    }
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Rate limit disabled\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("Rate limit disabled\n");
    return 0;
}

/* 主页处理器 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Rate Limit E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🧪 Rate Limit End-to-End Test Server</h1>"
        "<p>测试服务器内置的限流功能</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /limited - 限流端点（受限流影响）"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /unlimited - 无限流端点（对照组）"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /stats - 统计信息"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">POST</span> /reset - 重置统计"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">POST</span> /enable - 启用限流"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">POST</span> /disable - 禁用限流"
        "</div>"
        ""
        "<h2>测试命令示例：</h2>"
        "<pre>"
        "# 测试限流端点\n"
        "curl http://localhost:8087/limited\n"
        ""
        "# 快速发送多个请求测试限流\n"
        "for i in {1..15}; do curl http://localhost:8087/limited; done\n"
        ""
        "# 查看统计信息\n"
        "curl http://localhost:8087/stats\n"
        ""
        "# 启用限流\n"
        "curl -X POST http://localhost:8087/enable\n"
        ""
        "# 禁用限流\n"
        "curl -X POST http://localhost:8087/disable\n"
        ""
        "# 重置统计\n"
        "curl -X POST http://localhost:8087/reset\n"
        "</pre>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8087;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    signal(SIGINT, simple_signal_handler);
    signal(SIGTERM, simple_signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    g_app_context = ctx;
    
    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 创建路由器 */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(ctx->router, "/", index_handler);
    uvhttp_router_add_route(ctx->router, "/limited", limited_handler);
    uvhttp_router_add_route(ctx->router, "/unlimited", unlimited_handler);
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    uvhttp_router_add_route(ctx->router, "/reset", reset_stats_handler);
    uvhttp_router_add_route(ctx->router, "/enable", enable_limit_handler);
    uvhttp_router_add_route(ctx->router, "/disable", disable_limit_handler);
    
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
    printf("Rate Limit E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("========================================\n");
    printf("\n限流功能测试端点：\n");
    printf("  - /limited - 限流端点\n");
    printf("  - /unlimited - 无限流端点\n");
    printf("  - /stats - 统计信息\n");
    printf("  - /reset - 重置统计\n");
    printf("  - /enable - 启用限流\n");
    printf("  - /disable - 禁用限流\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    uvhttp_free(ctx);
    
    printf("\n========================================\n");
    printf("服务器已停止\n");
    printf("========================================\n");
    
    return 0;
}
