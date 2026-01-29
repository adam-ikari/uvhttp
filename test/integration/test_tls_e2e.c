/*
 * 简化的 TLS/HTTPS 端到端测试
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <signal.h>
#include <string.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

static app_context_t* g_app_context = NULL;

void signal_handler(int sig) {
    (void)sig;
    if (g_app_context && g_app_context->server) {
        uvhttp_server_stop(g_app_context->server);
        uvhttp_server_free(g_app_context->server);
        g_app_context->server = NULL;
    }
    exit(0);
}

/* 简单处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "{\"status\":\"ok\",\"message\":\"TLS test endpoint\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8443;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    g_app_context = ctx;
    
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    uvhttp_router_add_route(ctx->router, "/", simple_handler);
    uvhttp_router_add_route(ctx->router, "/api", simple_handler);
    
    ctx->server->router = ctx->router;
    
    result = uvhttp_server_listen(ctx->server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("TLS/HTTPS E2E Test Server (Simplified)\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("========================================\n\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    uvhttp_free(ctx);
    
    return 0;
}
