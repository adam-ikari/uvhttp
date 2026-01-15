#include "uvhttp.h"
#include <signal.h>

// 应用上下文结构 - 使用循环注入模式
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

void signal_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    uv_loop_t* loop = uv_default_loop();
    if (loop && loop->data) {
        app_context_t* ctx = (app_context_t*)loop->data;
        if (ctx && ctx->server) {
            uvhttp_server_stop(ctx->server);
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
    }
    exit(0);
}

int test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_url(request);
    (void)path;  // Suppress unused variable warning
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "OK", 2);
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    
    // 注入到循环
    loop->data = ctx;
    
    // 创建服务器
    ctx->server = uvhttp_server_new(loop);
    if (!ctx->server) {
        fprintf(stderr, "Failed to create server\n");
        free(ctx);
        return 1;
    }
    
    // 创建路由器
    ctx->router = uvhttp_router_new();
    if (!ctx->router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/*", test_handler);
    ctx->server->router = ctx->router;
    
    // 启动服务器
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
        free(ctx);
        loop->data = NULL;
    }
    
    return 0;
}