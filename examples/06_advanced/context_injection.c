/**
 * @file context_injection.c
 * @brief 演示如何使用上下文注入模式，避免全局变量和独占 loop->data
 *
 * 本示例展示：
 * 1. 使用 server->context 存储 UVHTTP 上下文
 * 2. 使用 server->user_data 存储应用上下文
 * 3. 避免独占 loop->data，允许多个应用共享同一个 loop
 * 4. 从请求中获取应用上下文，不使用全局变量
 */

#include "../../include/uvhttp.h"
#include "../../include/uvhttp_allocator.h"
#include "../../include/uvhttp_context.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

/**
 * @brief 应用上下文结构
 *
 * 封装所有应用相关的数据，避免使用全局变量
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_context_t* uvhttp_ctx;  // 保存 uvhttp_context 引用以便释放
    int request_count;
    time_t start_time;
    char server_name[64];
} app_context_t;

/**
 * @brief 其他应用上下文结构（模拟其他应用使用 loop->data）
 * 
 * 这个结构模拟其他应用也在使用 loop->data 的场景
 */
typedef struct {
    char app_name[64];
    int app_id;
    void* custom_data;
} other_app_context_t;

/**
 * @brief 创建应用上下文
 */
app_context_t* app_context_create(uv_loop_t* loop, const char* name) {
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "错误: 无法分配内存\n");
        return NULL;
    }

    // 初始化上下文
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    strncpy(ctx->server_name, name, sizeof(ctx->server_name) - 1);
    ctx->server_name[sizeof(ctx->server_name) - 1] = '\0';

    // 创建服务器
    ctx->uvhttp_error_t server_result = uvhttp_server_new(loop, &server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        uvhttp_free(ctx);
        return NULL;
    }

    // 将 app_context_t 设置到 server->user_data
    ctx->server->user_data = ctx;

    // 创建路由器
    ctx->router = uvhttp_router_new();
    if (!ctx->router) {
        fprintf(stderr, "错误: 无法创建路由器\n");
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return NULL;
    }

    // 设置路由器
    uvhttp_server_set_router(ctx->server, ctx->router);

    // 创建 uvhttp_context 并设置到服务器
    // 这是避免独占 loop->data 的关键！
    uvhttp_context_t* uvhttp_error_t result_uvhttp_ctx = uvhttp_context_create(loop, &uvhttp_ctx);
    if (!uvhttp_ctx) {
        fprintf(stderr, "错误: 无法创建 uvhttp_context\n");
        uvhttp_router_free(ctx->router);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return NULL;
    }

    // 保存 uvhttp_ctx 引用以便后续释放
    ctx->uvhttp_ctx = uvhttp_ctx;

    // 将 uvhttp_context 设置到服务器
    uvhttp_server_set_context(ctx->server, uvhttp_ctx);

    printf("✓ 应用上下文创建成功\n");
    printf("  服务器名称: %s\n", ctx->server_name);
    printf("  启动时间: %s", ctime(&ctx->start_time));
    printf("  使用 server->context 而非 loop->data\n");
    printf("  使用 server->user_data 存储应用上下文\n");

    return ctx;
}

/**
 * @brief 销毁应用上下文
 */
void app_context_destroy(app_context_t* ctx, uv_loop_t* loop) {
    (void)loop;  // 未使用的参数
    if (!ctx) return;

    printf("\n清理应用上下文...\n");
    printf("  总请求数: %d\n", ctx->request_count);
    printf("  运行时间: %ld 秒\n", time(NULL) - ctx->start_time);

    // 清理服务器
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }

    // 清理 uvhttp_context
    if (ctx->uvhttp_ctx) {
        uvhttp_context_destroy(ctx->uvhttp_ctx);
        ctx->uvhttp_ctx = NULL;
    }

    free(ctx);
    printf("✓ 应用上下文已销毁\n");
}

/**
 * @brief 创建其他应用上下文（模拟其他应用使用 loop->data）
 */
other_app_context_t* other_app_context_create(const char* name, int app_id) {
    other_app_context_t* ctx = (other_app_context_t*)malloc(sizeof(other_app_context_t));
    if (!ctx) {
        fprintf(stderr, "错误: 无法分配内存\n");
        return NULL;
    }
    
    strncpy(ctx->app_name, name, sizeof(ctx->app_name) - 1);
    ctx->app_name[sizeof(ctx->app_name) - 1] = '\0';
    ctx->app_id = app_id;
    ctx->custom_data = NULL;
    
    return ctx;
}

/**
 * @brief 销毁其他应用上下文
 */
void other_app_context_destroy(other_app_context_t* ctx) {
    if (!ctx) return;

    printf("  清理其他应用上下文: %s (ID: %d)\n", ctx->app_name, ctx->app_id);
    free(ctx);
}

/**
 * @brief 从请求获取应用上下文的辅助函数
 */
app_context_t* get_app_context_from_request(uvhttp_request_t* req) {
    if (!req || !req->client || !req->client->data) {
        return NULL;
    }

    uvhttp_connection_t* conn = (uvhttp_connection_t*)req->client->data;
    if (!conn || !conn->server) {
        return NULL;
    }

    return (app_context_t*)conn->server->user_data;
}

/**
 * @brief 主页处理器
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 使用辅助函数获取应用上下文，避免全局变量
    app_context_t* ctx = get_app_context_from_request(req);
    if (!ctx) {
        const char* error = "{\"error\":\"服务器未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // 从 request 获取 server
    if (!req->client || !req->client->data) {
        const char* error = "{\"error\":\"连接未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_connection_t* conn = (uvhttp_connection_t*)req->client->data;
    uvhttp_server_t* server = conn->server;
    uvhttp_context_t* uvhttp_ctx = server->context;

    if (!uvhttp_ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // 从服务器的 config 获取配置
    const uvhttp_config_t* config = uvhttp_config_get_current(uvhttp_ctx);
    int max_connections = config ? config->max_connections : 10000;

    char response[2048];
    snprintf(response, sizeof(response),
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP Shared Loop Data 示例</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        "h1 { color: #28a745; }"
        ".info { background: #d4edda; padding: 15px; border-radius: 5px; margin: 20px 0; }"
        ".warning { background: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1> UVHTTP Shared Loop Data 示例</h1>"
        "<p>本示例演示如何避免独占 loop->data，允许多个应用共享同一个 libuv 循环。</p>"
        "<div class='info'>"
        "<h3>✓ 优势</h3>"
        "<ul>"
        "<li>不独占 loop->data</li>"
        "<li>允许多个应用共享同一个 loop</li>"
        "<li>使用 server->context 而非 loop->data</li>"
        "</ul>"
        "</div>"
        "<div class='warning'>"
        "<h3> 场景</h3>"
        "<p>当其他应用也在使用 loop->data 时，本方案可以避免冲突。</p>"
        "</div>"
        "<h3>服务器信息</h3>"
        "<ul>"
        "<li>最大连接数: %d</li>"
        "<li>活跃连接数: %zu</li>"
        "</ul>"
        "<h3>可用的 API</h3>"
        "<ul>"
        "<li><a href='/stats'>/stats</a> - 查看详细统计</li>"
        "<li><a href='/info'>/info</a> - 服务器信息</li>"
        "</ul>"
        "</div>"
        "</body>"
        "</html>",
        max_connections,
        server->active_connections);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

/**
 * @brief 统计处理器
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 使用辅助函数获取应用上下文，避免全局变量
    app_context_t* ctx = get_app_context_from_request(req);
    if (!ctx) {
        const char* error = "{\"error\":\"服务器未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // 从 request 获取 server
    if (!req->client || !req->client->data) {
        const char* error = "{\"error\":\"连接未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_connection_t* conn = (uvhttp_connection_t*)req->client->data;
    uvhttp_server_t* server = conn->server;
    uvhttp_context_t* uvhttp_ctx = server->context;

    if (!uvhttp_ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    // 使用 cJSON 创建 JSON 响应
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "description", "使用 server->context 而非 loop->data");
    cJSON_AddNumberToObject(json_obj, "active_connections", server->active_connections);
    cJSON_AddNumberToObject(json_obj, "is_listening", server->is_listening);
    cJSON_AddNumberToObject(json_obj, "owns_loop", server->owns_loop);

    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief 信息处理器
 */
int info_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用的参数

    // 使用 cJSON 创建 JSON 响应
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "example", "shared_loop_data");
    cJSON_AddStringToObject(json_obj, "description", "演示如何避免独占 loop->data");
    cJSON_AddStringToObject(json_obj, "solution", "使用 uvhttp_server_set_context() 设置服务器上下文");
    cJSON_AddStringToObject(json_obj, "scenario", "当其他应用也在使用 loop->data 时");

    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief 信号处理
 */
void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    exit(0);
}

int main() {
    printf("========================================\n");
    printf("  UVHTTP Shared Loop Data 示例\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 获取事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = app_context_create(loop, "UVHTTP-Shared-Data-Server");
    if (!ctx) {
        fprintf(stderr, "错误: 无法创建应用上下文\n");
        return 1;
    }

    printf("\n");
    
    // 添加路由
    printf("添加路由...\n");
    uvhttp_router_add_route(ctx->router, "/", home_handler);
    printf("  ✓ / - 主页\n");
    
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    printf("  ✓ /stats - 统计信息\n");
    
    uvhttp_router_add_route(ctx->router, "/info", info_handler);
    printf("  ✓ /info - 服务器信息\n");
    
    printf("\n");
    
    // 模拟其他应用使用 loop->data
    printf("模拟其他应用使用 loop->data...\n");
    other_app_context_t* other_ctx = other_app_context_create("Other-App", 12345);
    loop->data = other_ctx;
    printf("  ✓ 其他应用已设置 loop->data\n");
    printf("    应用名称: %s\n", other_ctx->app_name);
    printf("    应用 ID: %d\n", other_ctx->app_id);
    printf("\n");
    
    printf("注意：即使 loop->data 被其他应用占用，\n");
    printf("UVHTTP 仍然可以正常工作，因为使用的是 server->context！\n\n");
    
    // 启动服务器
    int result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
        app_context_destroy(ctx, loop);
        other_app_context_destroy(other_ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("  服务器运行在 http://localhost:8080\n");
    printf("========================================\n\n");
    
    printf("测试命令：\n");
    printf("  curl http://localhost:8080/       # 主页\n");
    printf("  curl http://localhost:8080/stats  # 统计信息\n");
    printf("  curl http://localhost:8080/info   # 服务器信息\n\n");
    
    printf("按 Ctrl+C 停止服务器\n\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    other_app_context_destroy(other_ctx);
    app_context_destroy(ctx, loop);
    
    printf("\n服务器已停止\n");
    return 0;
}
