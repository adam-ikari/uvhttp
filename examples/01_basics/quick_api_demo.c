/**
 * @file quick_api_demo.c
 * @brief UVHTTP 快速启动演示 - 使用核心API
 */

#include "../include/uvhttp.h"
#include "../../deps/cjson/cJSON.h"
#include <stdio.h>
#include <signal.h>

// 应用上下文结构
typedef struct {
    uvhttp_server_t* server;
} app_context_t;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    
    // 从 loop 中获取上下文
    uv_loop_t* loop = uv_default_loop();
    if (loop && loop->data) {
        app_context_t* ctx = (app_context_t*)loop->data;
        if (ctx && ctx->server) {
            uvhttp_server_stop(ctx->server);
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
        free(ctx);
        loop->data = NULL;
    }
    
    exit(0);
}

// 简单处理器
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_request_get_query_param(req, "name");
    if (!name) name = "World";
    
    char content[256];
    snprintf(content, sizeof(content), "Hello, %s! 这是快速启动演示。", name);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(res, content, strlen(content));
    
    return uvhttp_response_send(res);
}

int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用的参数
    
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
    cJSON_AddStringToObject(json_obj, "message", "这是快速启动演示");
    cJSON_AddStringToObject(json_obj, "status", "success");
    cJSON_AddStringToObject(json_obj, "api", "core");
    
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

int main() {
    printf("🚀 UVHTTP 快速启动演示\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();

    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));

    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        free(ctx);
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        free(ctx);
        return 1;
    }

    // 将上下文设置到 loop->data
    loop->data = ctx;

    // 创建路由器
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        free(ctx);
        return 1;
    }
    uvhttp_server_set_router(ctx->server, router);

    // 添加路由
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    printf("✅ 服务器配置完成!\n");
    printf("🌐 访问 http://localhost:8080 查看演示\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n");

    // 启动服务器
    int listen_result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (listen_result != 0) {
        fprintf(stderr, "❌ 服务器启动失败: %d\n", listen_result);
        return 1;
    }

    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理（正常退出时）
    if (ctx && ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    if (ctx) {
        free(ctx);
    }
    loop->data = NULL;
    
    return 0;
}