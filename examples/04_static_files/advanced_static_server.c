/* UVHTTP 静态文件服务示例 - 演示集成文件读取功能 */

#include "uvhttp.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* 应用上下文 - 使用 server->user_data 传递 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_static_context_t* static_ctx;
    volatile sig_atomic_t keep_running;
} app_context_t;

/* 全局应用上下文 - 仅在 main 函数中设置和使用 */


/* 信号处理函数 */
void signal_handler(int signal) {
    app_context_t* ctx = (app_context_t*)uv_default_loop()->data;
    if (!ctx) return;


    printf("\n收到信号 %d，正在关闭服务器...\n", signal);
    if (ctx) {
        ctx->keep_running = 0;
        if (ctx->server) {
            /* 服务器关闭需要手动实现 */
        }
    }
}

/* API 请求处理器 - 显示统计信息 */
int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* ctx = (app_context_t*)request->client->loop->data;
    if (!ctx || !ctx->static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Static context not initialized\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return -1;
    }

    /* 使用 cJSON 创建 JSON 响应 */
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return -1;
    }

    cJSON_AddStringToObject(json_obj, "status", "ok");
    cJSON_AddStringToObject(json_obj, "message", "UVHTTP static file server is running");
    cJSON_AddStringToObject(json_obj, "version", "v2");

    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return -1;
    }

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, json_string, strlen(json_string));

    int result = uvhttp_response_send(response);
    uvhttp_free(json_string);

    return result;
}

/* 主页处理器 */
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 未使用参数
    const char* html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP 静态文件服务演示</title>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .stats { background: #f5f5f5; padding: 20px; border-radius: 5px; margin: 20px 0; }\n"
        "        .file-list { background: #fff; border: 1px solid #ddd; padding: 20px; border-radius: 5px; }\n"
        "        a { color: #007bff; text-decoration: none; }\n"
        "        a:hover { text-decoration: underline; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1> UVHTTP 静态文件服务演示</h1>\n"
        "        <p>这个演示展示了 UVHTTP 的集成文件读取功能，包括：</p>\n"
        "        <ul>\n"
        "            <li>🔄 智能同步/异步文件读取</li>\n"
        "            <li> 性能统计和监控</li>\n"
        "            <li>💾 内存缓存机制</li>\n"
        "            <li>🌊 流式文件传输</li>\n"
        "            <li> 安全的路径解析</li>\n"
        "        </ul>\n"
        "        \n"
        "        <div class=\"stats\">\n"
        "            <h2> 服务统计</h2>\n"
        "            <p><a href=\"/api/stats\">查看详细统计信息</a></p>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"file-list\">\n"
        "            <h2>📁 测试文件</h2>\n"
        "            <ul>\n"
        "                <li><a href=\"/test.txt\">test.txt</a> - 小文件测试</li>\n"
        "                <li><a href=\"/sample.html\">sample.html</a> - HTML 文件测试</li>\n"
        "                <li><a href=\"/images/\">images/</a> - 图片目录</li>\n"
        "            </ul>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"features\">\n"
        "            <h2>✨ 功能特性</h2>\n"
        "            <ul>\n"
        "                <li><strong>智能读取策略：</strong>根据文件大小和系统负载自动选择最优读取方式</li>\n"
        "                <li><strong>异步处理：</strong>大文件使用异步读取，避免阻塞事件循环</li>\n"
        "                <li><strong>流式传输：</strong>超大文件使用分块传输，降低内存使用</li>\n"
        "                <li><strong>缓存优化：</strong>智能缓存机制，提高重复请求性能</li>\n"
        "                <li><strong>安全防护：</strong>路径遍历攻击防护，确保文件访问安全</li>\n"
        "            </ul>\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=UTF-8");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    return 0;
}

/* 静态文件请求处理器 */
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* ctx = (app_context_t*)request->client->loop->data;
    if (!ctx || !ctx->static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static context not initialized", 32);
        uvhttp_response_send(response);
        return -1;
    }
    
    int result = uvhttp_static_handle_request(ctx->static_ctx, request, response);
    if (result != 0) {
        const char* error_body = "Error processing static file request";
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
    }
    
    uvhttp_response_send(response);
    return 0;
}

/* 创建测试文件 */
int create_test_files() {
    /* 创建测试目录 */
    (void)system("mkdir -p ./public/images");
    
    /* 创建测试文本文件 */
    FILE* f = fopen("./public/test.txt", "w");
    if (f) {
        fprintf(f, "这是一个测试文件，用于演示 UVHTTP 的静态文件服务功能。\n");
        fprintf(f, "文件读取时间: %ld\n", time(NULL));
        fprintf(f, "文件大小适中，应该使用缓存机制。\n");
        fclose(f);
    }
    
    /* 创建 HTML 测试文件 */
    f = fopen("./public/sample.html", "w");
    if (f) {
        fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n<title>示例 HTML</title>\n</head>\n");
        fprintf(f, "<body>\n<h1>示例 HTML 文件</h1>\n<p>这是一个用于测试的 HTML 文件。</p>\n");
        fprintf(f, "<p>创建时间: %ld</p>\n</body>\n</html>\n", time(NULL));
        fclose(f);
    }
    
    /* 创建较大的测试文件（用于演示流式传输） */
    f = fopen("./public/large_file.txt", "w");
    if (f) {
        for (int i = 0; i < 10000; i++) {
            fprintf(f, "这是第 %d 行，用于测试大文件的流式传输功能。\n", i + 1);
        }
        fclose(f);
    }
    
    printf(" 测试文件已创建在 ./public/ 目录下\n");
    return 0;
}

int main(int argc, char* argv[]) {
    const char* root_dir = "./public";
    int port = 8080;
    
    /* 解析命令行参数 */
    if (argc > 1) {
        root_dir = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    
    printf(" UVHTTP 静态文件服务器\n");
    printf("📁 根目录: %s\n", root_dir);
    printf("🔌 端口: %d\n\n", port);
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, " 无法创建事件循环\n");
        return 1;
    }
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, " 无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->keep_running = 1;
    
    /* 设置全局应用上下文 */
    loop->data = ctx;
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建测试文件 */
    create_test_files();
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t static_config = {0};
    strcpy(static_config.root_directory, root_dir);
    strcpy(static_config.index_file, "index.html");
    static_config.enable_directory_listing = 1;
    static_config.max_cache_size = 10 * 1024 * 1024; /* 10MB 缓存 */
    static_config.cache_ttl = 3600; /* 1小时缓存 */
    
    /* 创建静态文件上下文 */
    ctx->static_ctx = NULL;
    /* 创建HTTP服务器 */
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        uvhttp_free(ctx);
        return 1;
    }
    

    uvhttp_error_t result = uvhttp_static_create(&static_config, &ctx->static_ctx);
    if (result != UVHTTP_OK || !ctx->static_ctx) {
        fprintf(stderr, " 无法创建静态文件服务上下文\n");
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 初始化异步文件读取 */
    if (0) { /* 异步初始化暂未实现 */
        fprintf(stderr, "  异步文件读取初始化失败，将使用同步读取\n");
    }
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    uvhttp_error_t router_result = uvhttp_router_new(&router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(router_result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api/stats", stats_handler);
    
    /* 设置静态文件路由 - 应用层实现 */
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    uvhttp_router_add_route(router, "/*", static_file_handler);
    
    /* 配置服务器 */
    ctx->server->router = router;
    ctx->server->user_data = ctx;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(ctx->server, "0.0.0.0", port) != 0) {
        fprintf(stderr, " 无法启动服务器\n");
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        
        return 1;
    }
    
    printf(" 服务器已启动！\n");
    printf("🌐 访问地址：\n");
    printf("   http://localhost:%d/ - 主页\n", port);
    printf("   http://localhost:%d/api/stats - 统计信息\n", port);
    printf("   http://localhost:%d/test.txt - 测试文件\n", port);
    printf("   http://localhost:%d/sample.html - HTML 测试\n", port);
    printf("   http://localhost:%d/large_file.txt - 大文件测试\n", port);
    printf("\n按 Ctrl+C 停止服务器\n\n");
    
    /* 运行事件循环 */
    while (ctx->keep_running) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(10000); /* 10ms 延迟，避免 CPU 占用过高 */
    }
    
    /* 清理资源 */
    printf("\n🧹 正在清理资源...\n");
    
    if (ctx->static_ctx) {
        printf("\n 静态文件服务已停止\n");
        uvhttp_static_free(ctx->static_ctx);
    }
    
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    
    uvhttp_free(ctx);
    
    
    printf(" 资源清理完成\n");
    return 0;
}