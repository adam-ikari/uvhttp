/*
 * UVHTTP 静态文件服务性能测试
 * 用于测试真实场景下的静态文件服务性能
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_static_context_t* g_static_ctx = NULL;
static uvhttp_context_t* g_context = NULL;

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
    
    int result = uvhttp_static_handle_request(g_static_ctx, request, response);
    if (result != 0) {
        const char* error_body = "Error processing static file request";
    printf("DEBUG: uvhttp_static_handle_request returned %d\n", result);
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
    /* 成功时发送响应 */
    uvhttp_response_send(response);
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);
    }
    
    return 0;
}

// 主页处理器
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
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
        "            <a href=\"/static/image.png\" class=\"file-item\">图片文件 (50KB)</a>\n"
        "            <a href=\"/static/style.css\" class=\"file-item\">CSS 文件 (5KB)</a>\n"
        "            <a href=\"/static/script.js\" class=\"file-item\">JS 文件 (8KB)</a>\n"
        "            <a href=\"/static/data.json\" class=\"file-item\">JSON 文件 (2KB)</a>\n"
        "            <a href=\"/static/index.html\" class=\"file-item\">HTML 页面 (3KB)</a>\n"
        "        </div>\n"
        "        <h2>性能指标</h2>\n"
        "        <p>此页面用于性能测试，包含多种不同类型和大小的文件。</p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, html_content, strlen(html_content));
    uvhttp_response_send(response);
    
    return 0;
}

// 创建测试文件
void create_test_files() {
    // 创建测试目录
    (void)system("mkdir -p ./public/static");
    
    // 创建小文件（1KB）
    FILE* small_file = fopen("./public/static/small.html", "w");
    if (small_file) {
        for (int i = 0; i < 1024; i++) {
            fputc('A' + (i % 26), small_file);
        }
        fclose(small_file);
    }
    
    // 创建中等文件（10KB）
    FILE* medium_file = fopen("./public/static/medium.html", "w");
    if (medium_file) {
        for (int i = 0; i < 10240; i++) {
            fputc('B' + (i % 26), medium_file);
        }
        fclose(medium_file);
    }
    
    // 创建大文件（100KB）
    FILE* large_file = fopen("./public/static/large.html", "w");
    if (large_file) {
        for (int i = 0; i < 102400; i++) {
            fputc('C' + (i % 26), large_file);
        }
        fclose(large_file);
    }
    
    // 创建图片文件（50KB）
    FILE* image_file = fopen("./public/static/image.png", "wb");
    if (image_file) {
        for (int i = 0; i < 51200; i++) {
            fputc(0x89 + (i % 128), image_file);
        }
        fclose(image_file);
    }
    
    // 创建CSS文件（5KB）
    FILE* css_file = fopen("./public/static/style.css", "w");
    if (css_file) {
        for (int i = 0; i < 5120; i++) {
            fputc('D' + (i % 26), css_file);
        }
        fclose(css_file);
    }
    
    // 创建JS文件（8KB）
    FILE* js_file = fopen("./public/static/script.js", "w");
    if (js_file) {
        for (int i = 0; i < 8192; i++) {
            fputc('E' + (i % 26), js_file);
        }
        fclose(js_file);
    }
    
    // 创建JSON文件（2KB）
    FILE* json_file = fopen("./public/static/data.json", "w");
    if (json_file) {
        fprintf(json_file, "{\"status\":\"ok\",\"data\":\"test\"}");
        fclose(json_file);
    }
    
    // 创建HTML页面（3KB）
    FILE* html_file = fopen("./public/static/index.html", "w");
    if (html_file) {
        fprintf(html_file, "<!DOCTYPE html><html><head><title>Test</title></head><body>Test</body></html>");
        fclose(html_file);
    }
    
    printf("测试文件已创建在 ./public/static/ 目录下\n");
}

int main() {
    printf("=== UVHTTP 静态文件服务性能测试（优化版）===\n");

    // 创建测试文件
    (void)create_test_files();

    // 创建事件循环
    g_loop = uv_default_loop();

    // 创建上下文
    g_context = uvhttp_context_create(g_loop);
    if (!g_context) {
        printf("错误：无法创建上下文\n");
        return 1;
    }

    // 优化配置：增加最大连接数和缓冲区大小
    uvhttp_config_update_max_connections(g_context, 5000);  /* 增加到5000连接 */
    uvhttp_config_update_read_buffer_size(g_context, 16384);     /* 增加缓冲区到16KB */
    
    // 配置静态文件服务（优化小文件性能）
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 100 * 1024 * 1024,  /* 100MB缓存 - 增加缓存大小 */
        .cache_ttl = 7200,                    /* 2小时TTL - 延长缓存时间 */
        .custom_headers = ""
    };
    
    // 创建静态文件服务上下文
    g_static_ctx = uvhttp_static_create(&static_config);
    if (!g_static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        return 1;
    }

    // 创建HTTP服务器
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        printf("错误：无法创建HTTP服务器\n");
        uvhttp_static_free(g_static_ctx);
        return 1;
    }
    
    // 创建路由
    g_router = uvhttp_router_new();
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", home_handler);
    uvhttp_router_add_route(g_router, "/static/*", (uvhttp_request_handler_t)static_file_handler);
    
    // 设置路由
    g_server->router = g_router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        printf("错误：无法启动服务器 (错误码: %d)\n", result);
        uvhttp_static_free(g_static_ctx);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("🚀 静态文件服务启动成功！\n");
    printf("📍 服务地址: http://localhost:8080\n");
    printf("📁 静态文件目录: %s\n", static_config.root_directory);
    printf("\n按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);

    // 清理资源
    if (g_context) {
        uvhttp_context_destroy(g_context);
        g_context = NULL;
    }
    uvhttp_static_free(g_static_ctx);
    uvhttp_server_free(g_server);

    printf("\n服务器已停止\n");
    return 0;
}