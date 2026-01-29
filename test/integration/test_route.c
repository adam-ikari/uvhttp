#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <signal.h>
#include <string.h>

// 应用上下文结构 - 使用 server->user_data
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

// 全局应用上下文 - 仅在 main 函数中设置和使用
static app_context_t* g_app_context = NULL;

void signal_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    if (g_app_context && g_app_context->server) {
        uvhttp_server_stop(g_app_context->server);
        uvhttp_server_free(g_app_context->server);
        g_app_context->server = NULL;
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
    
    // 创建应用上下文 - 使用 uvhttp_alloc
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    
    // 设置全局应用上下文
    g_app_context = ctx;
    
    // 创建服务器
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 设置服务器用户数据
    ctx->server->user_data = ctx;
    
    // 创建路由器
    uvhttp_error_t router_result = uvhttp_router_new(&ctx->router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        g_app_context = NULL;
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
        uvhttp_free(ctx);
        g_app_context = NULL;
    }
    
    return 0;
}