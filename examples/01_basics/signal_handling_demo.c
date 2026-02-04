/**
 * @file signal_handling_demo.c
 * @brief UVHTTP 信号处理演示 - 使用 libuv 的 uv_signal_t
 * 
 * 本示例演示如何使用 libuv 的 uv_signal_t 来处理信号
 * 这是比标准 signal() 更好的方式
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// 应用上下文结构
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
} app_context_t;

// SIGINT 信号处理器
void on_sigint(uv_signal_t* handle, int signum) {
    printf("\n收到信号 %d (SIGINT)，正在优雅关闭服务器...\n", signum);
    
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        printf("停止服务器...\n");
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    
    // 停止事件循环
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

// SIGTERM 信号处理器
void on_sigterm(uv_signal_t* handle, int signum) {
    printf("\n收到信号 %d (SIGTERM)，正在优雅关闭服务器...\n", signum);
    
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        printf("停止服务器...\n");
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    
    // 停止事件循环
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

// 简单的请求处理器
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用参数
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP 信号处理演示</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }"
        "h1 { color: #333; }"
        ".info { background: #e7f3ff; padding: 15px; margin: 20px 0; border-radius: 5px; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1> UVHTTP 信号处理演示</h1>"
        "<p>本示例演示如何使用 libuv 的 uv_signal_t 来处理信号。</p>"
        "<div class='info'>"
        "<h3> 信号处理方式</h3>"
        "<ul>"
        "<li><strong>SIGINT (Ctrl+C)</strong>: 优雅关闭服务器</li>"
        "<li><strong>SIGTERM</strong>: 优雅关闭服务器</li>"
        "</ul>"
        "</div>"
        "<p>按 Ctrl+C 或发送 SIGTERM 信号来测试。</p>"
        "</div>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

int main() {
    printf(" UVHTTP 信号处理演示 (使用 libuv uv_signal_t)\n\n");
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, " 无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, " 服务器创建失败: %s\n", uvhttp_error_string(server_result));
        free(ctx);
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, " 服务器创建失败\n");
        free(ctx);
        return 1;
    }
    
    // 创建路由
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, " 路由器创建失败: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    uvhttp_server_set_router(ctx->server, router);
    
    // 添加路由
    uvhttp_router_add_route(router, "/", hello_handler);
    
    // 初始化 SIGINT 信号处理器
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    printf(" SIGINT 信号处理器已注册\n");
    
    // 初始化 SIGTERM 信号处理器
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    printf(" SIGTERM 信号处理器已注册\n");
    
    // 启动服务器
    printf("\n🌐 启动服务器...\n");
    int listen_result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8082);
    if (listen_result != 0) {
        fprintf(stderr, " 服务器启动失败: %d\n", listen_result);
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    
    printf(" 服务器启动成功！\n");
    printf("📍 访问地址: http://localhost:8082\n");
    printf("⏹️  按 Ctrl+C 或发送 SIGTERM 信号停止服务器\n\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    printf("\n🧹 清理资源...\n");
    
    // 停止信号处理器
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    printf(" 信号处理器已停止\n");
    
    // 清理服务器
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    printf(" 服务器已释放\n");
    
    // 清理上下文
    free(ctx);
    printf(" 上下文已释放\n");
    
    printf("\n👋 服务器已停止\n");
    return 0;
}