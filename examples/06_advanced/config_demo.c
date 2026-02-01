/**
 * @file config_demo.c
 * @brief UVHTTP 配置管理演示程序
 *
 * 本示例演示了如何使用 UVHTTP 的配置管理系统来设置并发连接数限制
 * 和其他服务器参数。包括代码配置、文件配置、环境变量配置和动态调整。
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include "../include/uvhttp_allocator.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// 应用上下文
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uv_timer_t* config_timer;
    int request_count;
    uvhttp_context_t* context;
} app_context_t;

// 信号处理器
void signal_handler(int sig) {
    printf("\n收到信号 %d，正在优雅关闭服务器...\n", sig);
    
    uv_loop_t* loop = uv_default_loop();
    if (loop && loop->data) {
        app_context_t* ctx = (app_context_t*)loop->data;
        if (ctx) {
            if (ctx->config_timer) {
                uvhttp_free(ctx->config_timer);
                ctx->config_timer = NULL;
            }
            
            if (ctx->server) {
                uvhttp_server_free(ctx->server);
                ctx->server = NULL;
            }
        }
    }
    
    printf("清理完成，退出。\n");
    exit(0);
}

// 简单的请求处理器
int demo_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    uv_loop_t* loop = uvhttp_request_get_loop(request);
    app_context_t* ctx = loop ? (app_context_t*)loop->data : NULL;
    
    if (!ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal error", 14);
        uvhttp_response_send(response);
        return -1;
    }
    
    ctx->request_count++;

    // 获取当前配置
    const uvhttp_config_t* config = uvhttp_config_get_current(ctx->context);
    
    // 创建响应内容
    char response_body[1024];
    snprintf(response_body, sizeof(response_body),
        "<html><body>"
        "<h1>UVHTTP 配置演示服务器</h1>"
        "<h2>当前配置信息</h2>"
        "<ul>"
        "<li>最大连接数: %d</li>"
        "<li>每连接最大请求数: %d</li>"
        "<li>当前活动连接数: %zu</li>"
        "<li>已处理请求数: %d</li>"
        "<li>最大请求体大小: %zuMB</li>"
        "<li>读取缓冲区大小: %zuKB</li>"
        "</ul>"
        "<p>请求时间: %s</p>"
        "</body></html>",
        config->max_connections,
        config->max_requests_per_connection,
        ctx->server ? ctx->server->active_connections : 0,
        ctx->request_count,
        config->max_body_size / (1024 * 1024),
        config->read_buffer_size / 1024,
        ctime(&(time_t){time(NULL)})
    );
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("处理请求 #%d: %s %s\n", g_request_count, 
           uvhttp_method_to_string(request->method), request->url);
    
    return 0;
}

// 配置管理API演示处理器
int config_api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const uvhttp_config_t* config = uvhttp_config_get_current(g_context);
    
    // 解析查询参数
    char query_param[256] = {0};
    if (request->url && strchr(request->url, '?')) {
        char* query = strchr(request->url, '?') + 1;
        strncpy(query_param, query, sizeof(query_param) - 1);
    }
    
    // 处理配置更新请求
    if (strstr(query_param, "action=update") && strstr(query_param, "max_connections=")) {
        char* max_conn_str = strstr(query_param, "max_connections=") + strlen("max_connections=");
        int new_max_conn = atoi(max_conn_str);

        int result = uvhttp_config_update_max_connections(g_context, new_max_conn);

        // 使用 cJSON 创建 JSON 响应
        cJSON* json_obj = cJSON_CreateObject();
        if (!json_obj) {
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "application/json");
            const char* error = "{\"error\":\"Failed to create JSON\"}";
            uvhttp_response_set_body(response, error, strlen(error));
            uvhttp_response_send(response);
            return 0;
        }

        if (result == UVHTTP_OK) {
            cJSON_AddStringToObject(json_obj, "status", "success");
            char message[256];
            snprintf(message, sizeof(message), "最大连接数已更新为 %d", new_max_conn);
            cJSON_AddStringToObject(json_obj, "message", message);
            cJSON_AddNumberToObject(json_obj, "new_value", new_max_conn);
        } else {
            cJSON_AddStringToObject(json_obj, "status", "error");
            char message[256];
            snprintf(message, sizeof(message), "更新失败，错误码: %d", result);
            cJSON_AddStringToObject(json_obj, "message", message);
            cJSON_AddNumberToObject(json_obj, "error_code", result);
        }

        char* json_string = cJSON_PrintUnformatted(json_obj);
        cJSON_Delete(json_obj);

        if (!json_string) {
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "application/json");
            const char* error = "{\"error\":\"Failed to generate JSON\"}";
            uvhttp_response_set_body(response, error, strlen(error));
            uvhttp_response_send(response);
            return 0;
        }

        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, json_string, strlen(json_string));
        uvhttp_response_send(response);
        free(json_string);

        return 0;
    }
    
    // 返回当前配置信息
    
        // 使用 cJSON 创建 JSON 响应
    
        cJSON* json_obj = cJSON_CreateObject();
    
        if (!json_obj) {
    
            uvhttp_response_set_status(response, 500);
    
            uvhttp_response_set_header(response, "Content-Type", "application/json");
    
            const char* error = "{\"error\":\"Failed to create JSON\"}";
    
            uvhttp_response_set_body(response, error, strlen(error));
    
            uvhttp_response_send(response);
    
            return 0;
    
        }
    
    
    
        cJSON_AddNumberToObject(json_obj, "max_connections", config->max_connections);
    
        cJSON_AddNumberToObject(json_obj, "max_requests_per_connection", config->max_requests_per_connection);
    
        cJSON_AddNumberToObject(json_obj, "max_body_size", config->max_body_size);
    
        cJSON_AddNumberToObject(json_obj, "max_header_size", config->max_header_size);
    
        cJSON_AddNumberToObject(json_obj, "read_buffer_size", config->read_buffer_size);
    
        cJSON_AddNumberToObject(json_obj, "backlog", config->backlog);
    
        cJSON_AddNumberToObject(json_obj, "enable_compression", config->enable_compression);
    
        cJSON_AddNumberToObject(json_obj, "enable_tls", config->enable_tls);
    
        cJSON_AddNumberToObject(json_obj, "current_active_connections", g_server ? g_server->active_connections : 0);
    
        cJSON_AddNumberToObject(json_obj, "total_requests_handled", g_request_count);
    
    
    
        char* json_string = cJSON_PrintUnformatted(json_obj);
    
        cJSON_Delete(json_obj);
    
    
    
        if (!json_string) {
    
            uvhttp_response_set_status(response, 500);
    
            uvhttp_response_set_header(response, "Content-Type", "application/json");
    
            const char* error = "{\"error\":\"Failed to generate JSON\"}";
    
            uvhttp_response_set_body(response, error, strlen(error));
    
            uvhttp_response_send(response);
    
            return 0;
    
        }
    
    
    
        uvhttp_response_set_status(response, 200);
    
        uvhttp_response_set_header(response, "Content-Type", "application/json");
    
        uvhttp_response_set_body(response, json_string, strlen(json_string));
    
        uvhttp_response_send(response);
    
        free(json_string);
    
    
    
        return 0;
}

// 配置变化监控回调
void on_config_change(const char* key, const void* old_value, const void* new_value) {
    printf("🔧 配置变化通知: %s\n", key);
    
    if (strcmp(key, "max_connections") == 0) {
        int old_conn = *(const int*)old_value;
        int new_conn = *(const int*)new_value;
        printf("   最大连接数: %d -> %d\n", old_conn, new_conn);
        
        // 记录配置变化到日志
        FILE* log_file = fopen("config_changes.log", "a");
        if (log_file) {
            time_t now = time(NULL);
            fprintf(log_file, "[%s] max_connections: %d -> %d\n", 
                   ctime(&now), old_conn, new_conn);
            fclose(log_file);
        }
    }
}

// 动态配置调整定时器
void config_adjustment_timer(uv_timer_t* handle) {
    static int adjustment_count = 0;
    adjustment_count++;

    // 模拟基于时间的配置调整（实际应用中应基于系统负载）
    const uvhttp_config_t* current = uvhttp_config_get_current(g_context);
    int current_max = current->max_connections;

    // 每5次调整进行一次变化
    if (adjustment_count % 5 == 0) {
        // 在 2000-4000 之间循环调整
        int new_max = 2000 + (adjustment_count / 5 % 3) * 1000;

        if (new_max != current_max) {
            printf("⏰ 定时调整: 最大连接数 %d -> %d\n", current_max, new_max);
            uvhttp_config_update_max_connections(g_context, new_max);
        }
    }
    
    // 每10次调整打印一次状态
    if (adjustment_count % 10 == 0) {
        printf("📊 服务器状态: 活动连接=%zu, 总请求=%d, 最大连接=%d\n",
               g_server ? g_server->active_connections : 0,
               g_request_count,
               current->max_connections);
    }
}

// 打印配置信息
void print_config_info(const uvhttp_config_t* config) {
    printf("=== 服务器配置信息 ===\n");
    printf("🔗 最大连接数: %d\n", config->max_connections);
    printf("📝 每连接最大请求数: %d\n", config->max_requests_per_connection);
    printf("💾 最大请求体大小: %zuMB\n", config->max_body_size / (1024 * 1024));
    printf("📄 最大请求头大小: %zuKB\n", config->max_header_size / 1024);
    printf("📖 读取缓冲区大小: %zuKB\n", config->read_buffer_size / 1024);
    printf("📋 监听队列大小: %d\n", config->backlog);
    printf("🗜️  启用压缩: %s\n", config->enable_compression ? "是" : "否");
    printf("🔒 启用TLS: %s\n", config->enable_tls ? "是" : "否");
    printf("========================\n");
}

// 演示不同的配置加载方式
uvhttp_config_t* load_config_demo() {
    printf("🔧 配置加载演示\n");
    
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        return NULL;
    }
    
    // 1. 设置默认值
    printf("1️⃣ 设置默认配置...\n");
    uvhttp_config_set_defaults(config);
    printf("   默认最大连接数: %d\n", config->max_connections);

    // 2. 代码中直接覆盖某些配置
    printf("2️⃣ 代码中自定义配置...\n");
    config->max_connections = 3000;  // 演示用，实际中应该基于需求设置
    config->max_requests_per_connection = 200;
    printf("   代码设置最大连接数: %d\n", config->max_connections);
    
    // 5. 验证配置
    printf("5️⃣ 验证配置参数...\n");
    if (uvhttp_config_validate(config) == UVHTTP_OK) {
        printf("   ✅ 配置验证通过\n");
    } else {
        printf("   ❌ 配置验证失败\n");
        uvhttp_config_free(config);
        return NULL;
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    printf("🚀 UVHTTP 配置管理演示服务器启动中...\n\n");
    
    // 获取事件循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "❌ 获取事件循环失败\n");
        return 1;
    }
    
    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "❌ 无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    // 设置循环数据指针
    loop->data = ctx;
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 演示配置加载
    uvhttp_config_t* config = load_config_demo();
    if (!config) {
        fprintf(stderr, "❌ 配置加载失败\n");
        free(ctx);
        return 1;
    }
    
    // 打印最终配置信息
    printf("\n📋 最终配置信息:\n");
    print_config_info(config);
    
    // 创建服务器
    printf("\n🌐 创建HTTP服务器...\n");
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        uvhttp_config_free(config);
        free(ctx);
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        uvhttp_config_free(config);
        free(ctx);
        return 1;
    }
    
    // 应用配置
    ctx->server->config = config;

    // 创建上下文
    uvhttp_error_t result_context = uvhttp_context_create(loop, &ctx->context);
    if (result_context != UVHTTP_OK) {
        fprintf(stderr, "❌ 上下文创建失败\n");
        uvhttp_server_free(ctx->server);
        uvhttp_config_free(config);
        free(ctx);
        return 1;
    }

    // 设置全局配置（重要：这会消除"Global configuration not initialized"警告）
    uvhttp_config_set_current(ctx->context, config);

    printf("✅ 服务器创建成功\n");
    
    // 创建路由器
    printf("\n🛣️  设置路由...\n");
    uvhttp_error_t router_result = uvhttp_router_new(&ctx->router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "❌ 路由器创建失败: %s\n", uvhttp_error_string(router_result));
        uvhttp_server_free(ctx->server);
        uvhttp_context_destroy(ctx->context);
        uvhttp_config_free(config);
        free(ctx);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/", demo_handler);
    uvhttp_router_add_route(ctx->router, "/config", config_api_handler);
    ctx->server->router = ctx->router;
    printf("✅ 路由设置完成\n");
    
    // 启用配置变化监控
    printf("\n👂 启用配置变化监控...\n");
    uvhttp_config_monitor_changes(ctx->context, on_config_change);
    printf("✅ 配置监控已启用\n");
    
    // 启动配置动态调整定时器
    printf("\n⏰ 启动动态配置调整定时器...\n");
    ctx->config_timer = (uv_timer_t*)uvhttp_alloc(sizeof(uv_timer_t));
    uv_timer_init(loop, ctx->config_timer);
    uv_timer_start(ctx->config_timer, config_adjustment_timer, 10000, 10000); // 10秒后开始，每10秒执行一次
    printf("✅ 定时器已启动（每10秒检查一次）\n");
    
    // 启动服务器监听
    printf("\n🎯 启动服务器监听...\n");
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            port = 8080;
        }
    }
    
    uvhttp_error_t result = uvhttp_server_listen(ctx->server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "❌ 服务器启动失败，错误码: %d\n", result);
        uvhttp_server_free(ctx->server);
        uvhttp_context_destroy(ctx->context);
        uvhttp_config_free(config);
        free(ctx);
        return 1;
    }
    
    printf("✅ 服务器启动成功！\n");
    printf("🌍 服务器地址: http://localhost:%d\n", port);
    printf("📊 配置API: http://localhost:%d/config\n", port);
    printf("🔧 动态更新示例: curl 'http://localhost:%d/config?action=update&max_connections=3500'\n", port);
    printf("\n按 Ctrl+C 停止服务器\n\n");
    
    // 启动事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源（正常退出时）
    if (ctx->config_timer) {
        uv_timer_stop(ctx->config_timer);
        uvhttp_free(ctx->config_timer);
    }

    if (ctx->context) {
        uvhttp_context_destroy(ctx->context);
    }
    
    free(ctx);
    loop->data = NULL;

    printf("👋 服务器已停止\n");
    return 0;
}