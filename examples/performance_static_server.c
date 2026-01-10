/**
 * UVHTTP 静态文件服务性能测试
 * 用于测试真实场景下的静态文件服务性能
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_static_context_t* g_static_ctx = NULL;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }
    
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

// 静态文件请求处理器
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!g_static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static file service not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 处理静态文件请求 */
    int result = uvhttp_static_handle_request(g_static_ctx, request, response);
    
    if (result != 0) {
        /* 设置错误响应 */
        const char* error_body = "Error processing static file request";
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
    }
    
    /* 发送响应 */
    uvhttp_response_send(response);
    return 0;
}

// 主页处理器
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    const char* html_content = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP 静态文件服务性能测试</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .file-list { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 10px; }\n"
        "        .file-item { padding: 15px; background: #f0f0f0; border-radius: 5px; text-decoration: none; color: #333; }\n"
        "        .file-item:hover { background: #e0e0e0; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>🚀 UVHTTP 静态文件服务性能测试</h1>\n"
        "        <h2>测试文件列表</h2>\n"
        "        <div class=\"file-list\">\n"
        "            <a href=\"/static/small.html\" class=\"file-item\">小文件 (1KB)</a>\n"
        "            <a href=\"/static/medium.html\" class=\"file-item\">中等文件 (10KB)</a>\n"
        "            <a href=\"/static/large.html\" class=\"file-item\">大文件 (100KB)</a>\n"
        "            <a href=\"/static/image.png\" class=\"file-item\">图片 (50KB)</a>\n"
        "            <a href=\"/static/script.js\" class=\"file-item\">脚本 (8KB)</a>\n"
        "            <a href=\"/static/style.css\" class=\"file-item\">样式 (5KB)</a>\n"
        "            <a href=\"/static/data.json\" class=\"file-item\">JSON (29B)</a>\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html_content, strlen(html_content));
    uvhttp_response_send(response);
    
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
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建事件循环
    g_loop = uv_default_loop();
    
    // 应用 Nginx 优化配置
    uvhttp_config_t* config = uvhttp_config_new();
    if (!config) {
        printf("错误：无法创建配置\n");
        return 1;
    }
    
    uvhttp_config_set_current(config);
    uvhttp_config_update_max_connections(5000);  /* 增加到5000连接 */
    uvhttp_config_update_buffer_size(16384);     /* 增加缓冲区到16KB */
    
    // 配置静态文件服务（优化小文件性能）
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
    
    // 创建静态文件服务上下文
    g_static_ctx = uvhttp_static_create(&static_config);
    if (!g_static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建HTTP服务器
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        printf("错误：无法创建HTTP服务器\n");
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建路由
    g_router = uvhttp_router_new();
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", home_handler);
    
    /* 设置静态文件路由 */
    uvhttp_router_add_static_route(g_router, "/static/", g_static_ctx);
    
    /* 设置回退路由（处理所有其他请求） */
    uvhttp_router_add_fallback_route(g_router, g_static_ctx);
    
    // 设置路由
    g_server->router = g_router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", port);
    if (result != 0) {
        printf("错误：无法启动服务器 (错误码: %d)\n", result);
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("🚀 静态文件服务启动成功！\n");
    printf("📍 服务地址: http://localhost:%d\n", port);
    printf("📁 静态文件目录: %s\n", static_config.root_directory);
    printf("\n按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    // 清理资源
    uvhttp_static_free(g_static_ctx);
    uvhttp_config_free(config);
    uvhttp_server_free(g_server);
    
    printf("\n服务器已停止\n");
    return 0;
}