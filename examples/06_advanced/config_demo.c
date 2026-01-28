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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uv_timer_t* g_config_timer = NULL;
static int g_request_count = 0;
static uvhttp_context_t* g_context = NULL;

// 信号处理器
void signal_handler(int sig) {
    printf("\n收到信号 %d，正在优雅关闭服务器...\n", sig);
    
if (g_config_timer) {
        uvhttp_free(g_config_timer);
        g_config_timer = NULL;
    }
    
    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    
    printf("清理完成，退出。\n");
    exit(0);
}

// 简单的请求处理器
int demo_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    g_request_count++;

    // 获取当前配置
    const uvhttp_config_t* config = uvhttp_config_get_current(g_context);
    
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
        g_server ? g_server->active_connections : 0,
        g_request_count,
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
        
        char response_body[512];
        if (result == UVHTTP_OK) {
            snprintf(response_body, sizeof(response_body),
                "{\"status\":\"success\",\"message\":\"最大连接数已更新为 %d\",\"new_value\":%d}",
                new_max_conn, new_max_conn);
        } else {
            snprintf(response_body, sizeof(response_body),
                "{\"status\":\"error\",\"message\":\"更新失败，错误码: %d\",\"error_code\":%d}",
                result, result);
        }
        
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, response_body, strlen(response_body));
        uvhttp_response_send(response);
        
        return 0;
    }
    
    // 返回当前配置信息
    char config_json[1024];
    snprintf(config_json, sizeof(config_json),
        "{"
        "\"max_connections\":%d,"
        "\"max_requests_per_connection\":%d,"
        "\"max_body_size\":%zu,"
        "\"max_header_size\":%zu,"
        "\"read_buffer_size\":%d,"
        "\"backlog\":%d,"
        "\"enable_compression\":%d,"
        "\"enable_tls\":%d,"
        "\"current_active_connections\":%zu,"
        "\"total_requests_handled\":%d"
        "}",
        config->max_connections,
        config->max_requests_per_connection,
        config->max_body_size,
        config->max_header_size,
        config->read_buffer_size,
        config->backlog,
        config->enable_compression,
        config->enable_tls,
        g_server ? g_server->active_connections : 0,
        g_request_count
    );
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, config_json, strlen(config_json));
    uvhttp_response_send(response);
    
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
    
    // 2. 尝试从配置文件加载
    printf("2️⃣ 尝试从配置文件加载...\n");
    if (uvhttp_config_load_file(config, "uvhttp_demo.conf") == UVHTTP_OK) {
        printf("   ✅ 配置文件加载成功\n");
        printf("   文件配置最大连接数: %d\n", config->max_connections);
    } else {
        printf("   ⚠️  配置文件加载失败，将创建示例配置文件\n");
        
        // 创建示例配置文件
        FILE* conf_file = fopen("uvhttp_demo.conf", "w");
        if (conf_file) {
            fprintf(conf_file, "# UVHTTP 配置演示文件\n");
            fprintf(conf_file, "# 服务器配置\n");
            fprintf(conf_file, "max_connections=2500\n");
            fprintf(conf_file, "max_requests_per_connection=150\n");
            fprintf(conf_file, "backlog=1024\n\n");
            fprintf(conf_file, "# 性能配置\n");
            fprintf(conf_file, "max_body_size=2097152\n");
            fprintf(conf_file, "max_header_size=16384\n");
            fprintf(conf_file, "read_buffer_size=16384\n\n");
            fprintf(conf_file, "# 安全配置\n");
            fprintf(conf_file, "rate_limit_window=60\n");
            fprintf(conf_file, "enable_compression=1\n");
            fprintf(conf_file, "enable_tls=0\n");
            fclose(conf_file);
            printf("   📝 已创建示例配置文件: uvhttp_demo.conf\n");
            
            // 重新加载配置文件
            uvhttp_config_load_file(config, "uvhttp_demo.conf");
        }
    }
    
    // 3. 从环境变量加载（会覆盖文件配置）
    printf("3️⃣ 从环境变量加载配置...\n");
    if (uvhttp_config_load_env(config) == UVHTTP_OK) {
        printf("   ✅ 环境变量加载成功\n");
        printf("   环境变量配置最大连接数: %d\n", config->max_connections);
    } else {
        printf("   ℹ️  未设置相关环境变量\n");
    }
    
    // 4. 代码中直接覆盖某些配置
    printf("4️⃣ 代码中自定义配置...\n");
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
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 获取事件循环
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "❌ 获取事件循环失败\n");
        return 1;
    }
    
    // 演示配置加载
    uvhttp_config_t* config = load_config_demo();
    if (!config) {
        fprintf(stderr, "❌ 配置加载失败\n");
        return 1;
    }
    
    // 打印最终配置信息
    printf("\n📋 最终配置信息:\n");
    print_config_info(config);
    
    // 创建服务器
    printf("\n🌐 创建HTTP服务器...\n");
    uvhttp_error_t server_result = uvhttp_server_new(g_loop, &g_server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!g_server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        uvhttp_config_free(config);
        return 1;
    }
    
    // 应用配置
    g_server->config = config;

    // 创建上下文
    uvhttp_error_t result_g_context = uvhttp_context_create(g_loop, &g_context);
    if (result_g_context != UVHTTP_OK) {
        fprintf(stderr, "❌ 上下文创建失败\n");
        uvhttp_server_free(g_server);
        return 1;
    }

    // 设置全局配置（重要：这会消除"Global configuration not initialized"警告）
    uvhttp_config_set_current(g_context, config);

    printf("✅ 服务器创建成功\n");
    
    // 创建路由器
    printf("\n🛣️  设置路由...\n");
    uvhttp_error_t router_result = uvhttp_router_new(&g_router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "❌ 路由器创建失败: %s\n", uvhttp_error_string(router_result));
        uvhttp_server_free(g_server);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", demo_handler);
    uvhttp_router_add_route(g_router, "/config", config_api_handler);
    g_server->router = g_router;
    printf("✅ 路由设置完成\n");
    
    // 启用配置变化监控
    printf("\n👂 启用配置变化监控...\n");
    uvhttp_config_monitor_changes(g_context, on_config_change);
    printf("✅ 配置监控已启用\n");
    
    // 启动配置动态调整定时器
    printf("\n⏰ 启动动态配置调整定时器...\n");
    g_config_timer = (uv_timer_t*)uvhttp_alloc(sizeof(uv_timer_t));
    uv_timer_init(g_loop, g_config_timer);
    uv_timer_start(g_config_timer, config_adjustment_timer, 10000, 10000); // 10秒后开始，每10秒执行一次
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
    
    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "❌ 服务器启动失败，错误码: %d\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ 服务器启动成功！\n");
    printf("🌍 服务器地址: http://localhost:%d\n", port);
    printf("📊 配置API: http://localhost:%d/config\n", port);
    printf("🔧 动态更新示例: curl 'http://localhost:%d/config?action=update&max_connections=3500'\n", port);
    printf("\n按 Ctrl+C 停止服务器\n\n");
    
    // 启动事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    // 清理资源（正常退出时）
    if (g_config_timer) {
        uv_timer_stop(g_config_timer);
        uvhttp_free(g_config_timer);
    }

    if (g_context) {
        uvhttp_context_destroy(g_context);
    }

    printf("👋 服务器已停止\n");
    return 0;
}