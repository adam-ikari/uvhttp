#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>

// 应用上下文结构 - 使用 uv_signal_t
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
} app_context_t;

// SIGINT 信号处理器
void on_sigint(uv_signal_t* handle, int signum) {
    (void)signum;  // Suppress unused parameter warning
    
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

// SIGTERM 信号处理器
void on_sigterm(uv_signal_t* handle, int signum) {
    (void)signum;  // Suppress unused parameter warning
    
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

int test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    (void)path;  // Suppress unused variable warning
    
    const char* url = uvhttp_request_get_url(request);
    (void)url;  // Suppress unused variable warning
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Test", 4);
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文 - 使用 uvhttp_alloc
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    // 创建服务器
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    // 创建路由器
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/*", test_handler);
    ctx->server->router = ctx->router;
    
    // 初始化 SIGINT 信号处理器
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    // 初始化 SIGTERM 信号处理器
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    // 启动服务器
    result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8081);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
        uvhttp_free(ctx);
    }
    
    return 0;
}