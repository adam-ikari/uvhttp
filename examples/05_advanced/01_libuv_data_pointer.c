/**
 * @file libuv_data_pointer.c
 * @brief 演示如何使用 libuv 循环的 data 指针避免全局变量
 * 
 * 本示例展示：
 * 1. 创建应用上下文结构
 * 2. 将上下文设置到事件循环的 data 指针
 * 3. 在回调函数中访问上下文
 * 4. 完整的生命周期管理
 */

#include "../../include/uvhttp.h"
#include "../../include/uvhttp_allocator.h"
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
    int request_count;
    time_t start_time;
    char server_name[64];
} app_context_t;

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
    ctx->server = uvhttp_server_new(loop);
    if (!ctx->server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        uvhttp_free(ctx);
        return NULL;
    }
    
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
    
    // 将上下文设置到事件循环的 data 指针
    loop->data = ctx;
    
    printf("✓ 应用上下文创建成功\n");
    printf("  服务器名称: %s\n", ctx->server_name);
    printf("  启动时间: %s", ctime(&ctx->start_time));
    
    return ctx;
}

/**
 * @brief 销毁应用上下文
 */
void app_context_destroy(app_context_t* ctx, uv_loop_t* loop) {
    if (!ctx) return;
    
    printf("\n清理应用上下文...\n");
    printf("  总请求数: %d\n", ctx->request_count);
    printf("  运行时间: %ld 秒\n", time(NULL) - ctx->start_time);
    
    // 清理服务器
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    
    // 重置 data 指针
    loop->data = NULL;
    
    free(ctx);
    printf("✓ 应用上下文已销毁\n");
}

/**
 * @brief 从事件循环获取应用上下文的宏
 */
#define GET_CTX(loop) ((app_context_t*)((loop)->data))

/**
 * @brief 主页处理器
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop);
    
    // 检查上下文是否存在
    if (!ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP Data Pointer 示例</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        "h1 { color: #007bff; }"
        ".info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>🚀 UVHTTP Data Pointer 示例</h1>"
        "<p>本示例演示如何使用 libuv 循环的 data 指针避免全局变量。</p>"
        "<div class='info'>"
        "<h3>服务器信息</h3>"
        "<ul>"
        "<li>服务器名称: %s</li>"
        "<li>总请求数: %d</li>"
        "<li>运行时间: %ld 秒</li>"
        "</ul>"
        "</div>"
        "<h3>可用的 API</h3>"
        "<ul>"
        "<li><a href='/stats'>/stats</a> - 查看详细统计</li>"
        "<li><a href='/info'>/info</a> - 服务器信息</li>"
        "</ul>"
        "</div>"
        "</body>"
        "</html>";
    
    char response[1024];
    snprintf(response, sizeof(response), html,
        ctx->server_name,
        ctx->request_count,
        time(NULL) - ctx->start_time);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    ctx->request_count++;
    
    return uvhttp_response_send(res);
}

/**
 * @brief 统计处理器
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop);
    
    if (!ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    long uptime = time(NULL) - ctx->start_time;
    double rps = uptime > 0 ? (double)ctx->request_count / uptime : 0.0;
    
    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"server_name\": \"%s\",\n"
        "  \"request_count\": %d,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"requests_per_second\": %.2f,\n"
        "  \"active_connections\": %zu\n"
        "}",
        ctx->server_name,
        ctx->request_count,
        uptime,
        rps,
        ctx->server ? ctx->server->active_connections : 0);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    ctx->request_count++;
    
    return uvhttp_response_send(res);
}

/**
 * @brief 信息处理器
 */
int info_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop);
    
    if (!ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"server_name\": \"%s\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"description\": \"UVHTTP libuv data pointer 演示\",\n"
        "  \"start_time\": %ld,\n"
        "  \"current_time\": %ld\n"
        "}",
        ctx->server_name,
        ctx->start_time,
        time(NULL));
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    ctx->request_count++;
    
    return uvhttp_response_send(res);
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
    printf("  UVHTTP libuv Data Pointer 示例\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 获取事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = app_context_create(loop, "UVHTTP-Demo-Server");
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
    
    // 启动服务器
    int result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
        app_context_destroy(ctx, loop);
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
    app_context_destroy(ctx, loop);
    
    printf("\n服务器已停止\n");
    return 0;
}
