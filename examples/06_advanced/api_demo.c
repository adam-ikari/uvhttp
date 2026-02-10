/**
 * @file api_demo.c
 * @brief UVHTTP 核心API演示 - 使用 libuv 信号处理
 */

#include "../include/uvhttp.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>

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
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    
    // 停止事件循环
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

// 处理器函数 - 使用核心API签名
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_request_get_query_param(req, "name");
    if (!name) name = "World";
    
    char content[1024];
    snprintf(content, sizeof(content), "Hello, %s! 欢迎使用UVHTTP核心API", name);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(res, content, strlen(content));
    
    return uvhttp_response_send(res);
}

int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    const char* body = uvhttp_request_get_body(req);
    
    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 添加字段
    cJSON_AddStringToObject(json_obj, "status", "success");
    cJSON_AddStringToObject(json_obj, "method", method ? method : "unknown");
    cJSON_AddStringToObject(json_obj, "url", url ? url : "unknown");
    cJSON_AddStringToObject(json_obj, "body", body ? body : "");
    cJSON_AddStringToObject(json_obj, "message", "这是使用核心API创建的响应");
    
    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);
    
    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    
    int result = uvhttp_response_send(res);
    free(json_string);
    
    return result;
}

int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req; /* 未使用参数 */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP 核心API演示</title></head>"
        "<body>"
        "<h1> UVHTTP 核心API演示</h1>"
        "<p>这是一个使用核心API创建的HTTP服务器。</p>"
        "<h2>可用的API端点：</h2>"
        "<ul>"
        "<li><a href='/hello?name=UVHTTP'>/hello?name=UVHTTP</a> - 问候API</li>"
        "<li><a href='/api'>/api</a> - JSON API</li>"
        "</ul>"
        "<h2>核心API特性：</h2>"
        "<ul>"
        "<li>轻量级设计，最小依赖</li>"
        "<li>高性能，基于libuv</li>"
        "<li>灵活的路由系统</li>"
        "<li>完整的HTTP/1.1支持</li>"
        "</ul>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

int main() {
    printf(" UVHTTP 核心API演示\n");
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, " 创建事件循环失败\n");
        return 1;
    }
    
    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        free(ctx);
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, " 服务器创建失败\n");
        free(ctx);
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    if (!router) {
        fprintf(stderr, " 路由器创建失败\n");
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    
    // 设置路由器到服务器
    uvhttp_server_set_router(ctx->server, router);
    
    // 添加路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/hello", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    // 初始化 SIGINT 信号处理器
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    // 初始化 SIGTERM 信号处理器
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    printf(" 服务器配置完成!\n");
    printf("🌐 访问 http://localhost:8080 查看演示\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n");
    
    // 启动服务器
    uvhttp_error_t listen_result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (listen_result != UVHTTP_OK) {
        fprintf(stderr, " 服务器启动失败: %s\n", uvhttp_error_string(listen_result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx && ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    if (ctx) {
        free(ctx);
    }
    
    return 0;
}