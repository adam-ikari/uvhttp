/**
 * @file performance_static_server_refactored.c
 * @brief UVHTTP 静态文件服务性能测试（使用 libuv data 指针模式）
 *
 * 用于测试真实场景下的静态文件服务性能
 * 使用 libuv data 指针模式避免全局变量
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief 应用上下文结构
 *
 * 使用 libuv data 指针模式避免全局变量
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_static_context_t* static_ctx;
    uvhttp_config_t* config;
    uvhttp_context_t* uvhttp_ctx;
    int request_count;
    time_t start_time;
} app_context_t;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

// 静态文件请求处理器
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 从请求中获取服务器，然后获取应用上下文
    uvhttp_connection_t* conn = (uvhttp_connection_t*)request->client->data;
    if (!conn || !conn->server) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Server not found", 17);
        uvhttp_response_send(response);
        return -1;
    }
    
    app_context_t* ctx = (app_context_t*)conn->server->user_data;
    if (!ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Application context not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    if (!ctx->static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static file service not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 处理静态文件请求 */
    int result = uvhttp_static_handle_request(ctx->static_ctx, request, response);
    
    if (result == 0) {
        ctx->request_count++;
    }
    
    return result;
}

// 主页请求处理器
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 从请求中获取服务器，然后获取应用上下文
    uvhttp_connection_t* conn = (uvhttp_connection_t*)request->client->data;
    if (!conn || !conn->server) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Server not found", 17);
        uvhttp_response_send(response);
        return -1;
    }
    
    app_context_t* ctx = (app_context_t*)conn->server->user_data;
    if (!ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Application context not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    char html_body[2048];
    time_t uptime = time(NULL) - ctx->start_time;
    /* 使用字符串连接避免格式化警告 */
    snprintf(html_body, sizeof(html_body), "%s", "<html><head><title>UVHTTP Performance Test</title></head><body>");
    char stats[256];
    snprintf(stats, sizeof(stats), "<h1>UVHTTP Performance Test</h1><p>Requests: %lu</p><p>Uptime: %ld seconds</p>",
             (unsigned long)ctx->request_count, (long)uptime);
    strncat(html_body, stats, sizeof(html_body) - strlen(html_body) - 1);
    strncat(html_body, "</body></html>", sizeof(html_body) - strlen(html_body) - 1);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html_body, strlen(html_body));
    uvhttp_response_send(response);
    
    ctx->request_count++;
    
    return 0;
}

void print_usage(const char* program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("\n选项:\n");
    printf("  -d <目录>      静态文件根目录 (默认: ./public)\n");
    printf("  -p <端口>      监听端口 (默认: 8080)\n");
    printf("  -h             显示帮助信息\n");
    printf("\n示例:\n");
    printf("  %s -d /path/to/static -p 8080\n", program_name);
}

int main(int argc, char* argv[]) {
    printf("程序启动...\n");
    fflush(stdout);
    
    const char* root_directory = "./public";
    int port = 8080;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            root_directory = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
            /* 验证端口范围 */
            if (port < 1 || port > 65535) {
                fprintf(stderr, "错误: 端口必须在 1-65535 范围内\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("错误: 未知参数 '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* 验证目录是否存在且可访问 */
    struct stat st;
    if (stat(root_directory, &st) != 0) {
        fprintf(stderr, "错误: 目录 '%s' 不存在或无法访问\n", root_directory);
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: '%s' 不是目录\n", root_directory);
        return 1;
    }
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "错误: 无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->start_time = time(NULL);
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 应用 Nginx 优化配置
    printf("创建配置...\n");
    fflush(stdout);
    uvhttp_config_t* config = uvhttp_config_new();
    if (!config) {
        printf("错误：无法创建配置\n");
        fflush(stdout);
        uvhttp_free(ctx);
        return 1;
    }
    ctx->config = config;
    printf("配置创建成功\n");
    fflush(stdout);

    // 创建 uvhttp 上下文
    ctx->uvhttp_ctx = uvhttp_context_create(loop);
    if (!ctx->uvhttp_ctx) {
        printf("错误：无法创建 uvhttp 上下文\n");
        fflush(stdout);
        uvhttp_config_free(config);
        uvhttp_free(ctx);
        return 1;
    }

    uvhttp_config_set_current(ctx->uvhttp_ctx, config);
    uvhttp_config_update_max_connections(ctx->uvhttp_ctx, 5000);  /* 增加到5000连接 */
    uvhttp_config_update_read_buffer_size(ctx->uvhttp_ctx, 16384);     /* 增加缓冲区到16KB */
    printf("配置更新成功\n");
    fflush(stdout);
    
    // 配置静态文件服务（优化小文件性能）
    printf("配置静态文件服务...\n");
    fflush(stdout);
    uvhttp_static_config_t static_config;
    memset(&static_config, 0, sizeof(static_config));
    strncpy(static_config.root_directory, root_directory, sizeof(static_config.root_directory) - 1);
    static_config.root_directory[sizeof(static_config.root_directory) - 1] = '\0';
    
    strncpy(static_config.index_file, "index.html", sizeof(static_config.index_file) - 1);
    static_config.enable_directory_listing = 1;
    static_config.enable_etag = 1;
    static_config.enable_last_modified = 1;
    static_config.max_cache_size = 100 * 1024 * 1024;  /* 100MB缓存 - 增加缓存大小 */
    static_config.cache_ttl = 7200;                    /* 2小时TTL - 延长缓存时间 */
    printf("静态文件配置完成\n");
    fflush(stdout);
    
    // 创建静态文件服务上下文
    printf("创建静态文件服务上下文...\n");
    fflush(stdout);
    ctx->static_ctx = uvhttp_static_create(&static_config);
    if (!ctx->static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        fflush(stdout);
        uvhttp_config_free(config);
        uvhttp_free(ctx);
        return 1;
    }
    printf("静态文件服务上下文创建成功\n");
    fflush(stdout);
    
    // 创建HTTP服务器
    printf("创建HTTP服务器...\n");
    fflush(stdout);
    ctx->server = uvhttp_server_new(loop);
    if (!ctx->server) {
        printf("错误：无法创建HTTP服务器\n");
        fflush(stdout);
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_config_free(config);
        uvhttp_free(ctx);
        return 1;
    }
    printf("HTTP服务器创建成功\n");
    fflush(stdout);
    
    // 创建路由
    ctx->router = uvhttp_router_new();
    if (!ctx->router) {
        printf("错误：无法创建路由器\n");
        fflush(stdout);
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_server_free(ctx->server);
        uvhttp_config_free(config);
        uvhttp_free(ctx);
        return 1;
    }
    printf("路由器创建成功\n");
    fflush(stdout);
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/", home_handler);
    printf("主页路由添加成功\n");
    fflush(stdout);
    
    /* 设置静态文件路由 */
    uvhttp_router_add_static_route(ctx->router, "/static/", ctx->static_ctx);
    printf("静态文件路由添加成功\n");
    fflush(stdout);
    
    /* 设置回退路由（处理所有其他请求） */
    uvhttp_router_add_fallback_route(ctx->router, ctx->static_ctx);
    printf("回退路由添加成功\n");
    fflush(stdout);
    
    // 设置路由
    ctx->server->router = ctx->router;
    printf("路由器设置成功\n");
    fflush(stdout);
    
    // 设置应用上下文到服务器的 user_data
    ctx->server->user_data = ctx;
    
    // 将 uvhttp 上下文设置到服务器
    uvhttp_server_set_context(ctx->server, ctx->uvhttp_ctx);
    
    // 启动服务器
    int result = uvhttp_server_listen(ctx->server, "0.0.0.0", port);
    if (result != 0) {
        printf("错误：无法启动服务器 (错误码: %d)\n", result);
        fflush(stdout);
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_config_free(config);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("🚀 静态文件服务启动成功！\n");
    fflush(stdout);
    printf("📍 服务地址: http://localhost:%d\n", port);
    fflush(stdout);
    printf("📁 静态文件目录: %s\n", static_config.root_directory);
    fflush(stdout);
    printf("\n按 Ctrl+C 停止服务器\n");
    fflush(stdout);
    
    // 缓存预热：预加载常用小文件
    printf("🔥 缓存预热中...\n");
    fflush(stdout);
    uvhttp_static_prewarm_cache(ctx->static_ctx, "/static/index.html");
    uvhttp_static_prewarm_cache(ctx->static_ctx, "/static/style.css");
    uvhttp_static_prewarm_cache(ctx->static_ctx, "/static/script.js");
    printf("✅ 缓存预热完成\n");
    fflush(stdout);
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理（正常退出）
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    if (ctx->static_ctx) {
        uvhttp_static_free(ctx->static_ctx);
    }
    if (ctx->config) {
        uvhttp_config_free(ctx->config);
    }
    if (ctx->uvhttp_ctx) {
        uvhttp_context_destroy(ctx->uvhttp_ctx);
    }
    uvhttp_free(ctx);

    return 0;
}
