/*
 * UVHTTP 静态文件服务器示例
 * 
 * 演示如何使用UVHTTP的静态文件服务功能
 * 
 * 最佳实践：
 * 1. 静态文件路由应由应用层实现，使用 uvhttp_router_add_route() 添加路由
 * 2. 创建包装函数调用 uvhttp_static_handle_request()
 * 3. 通过 server->context 或 loop->data 传递应用上下文
 * 4. 使用通配符路由处理多个静态文件路径
 */

#include "uvhttp.h"
#include "uvhttp_static.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 应用上下文结构 */
typedef struct {
    uvhttp_static_context_t* static_ctx;
} app_context_t;

/**
 * 静态文件请求处理器
 * 
 * 这是一个应用层实现的包装函数，用于将路由器与静态文件服务连接起来。
 * 这种方式符合 UVHTTP "专注核心" 的设计原则，将路由逻辑留给应用层控制。
 */
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* 从 server->context 获取应用上下文 */
    uv_loop_t* loop = request->client ? request->client->loop : uv_default_loop();
    if (!loop || !loop->data) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Server context not initialized", 32);
        uvhttp_response_send(response);
        return -1;
    }
    
    app_context_t* app_ctx = (app_context_t*)loop->data;
    
    if (!app_ctx->static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static file service not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 处理静态文件请求 */
    int result = uvhttp_static_handle_request(app_ctx->static_ctx, request, response);
    if (result != 0) {
        /* 设置错误响应 - 使用默认错误消息 */
        const char* error_body = "Error processing static file request";
        const char* content_type = "text/plain";
        
        if (error_body) {
            uvhttp_response_set_header(response, "Content-Type", content_type);
            uvhttp_response_set_body(response, error_body, strlen(error_body));
        }
    }
    
    uvhttp_response_send(response);
    return 0;
}

/**
 * 主页处理器
 */
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    
    const char* html_content = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP 静态文件服务器</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .header { text-align: center; margin-bottom: 30px; }\n"
        "        .links { display: flex; flex-wrap: wrap; gap: 10px; }\n"
        "        .link { padding: 10px; background: #f0f0f0; border-radius: 5px; }\n"
        "        .link a { text-decoration: none; color: #333; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"header\">\n"
        "            <h1> UVHTTP 静态文件服务器</h1>\n"
        "            <p>高性能、安全的静态文件服务</p>\n"
        "        </div>\n"
        "        <div class=\"links\">\n"
        "            <div class=\"link\"><a href=\"/test.html\">测试页面</a></div>\n"
        "            <div class=\"link\"><a href=\"/style.css\">样式文件</a></div>\n"
        "            <div class=\"link\"><a href=\"/script.js\">脚本文件</a></div>\n"
        "            <div class=\"link\"><a href=\"/images/\">图片目录</a></div>\n"
        "            <div class=\"link\"><a href=\"/docs/\">文档目录</a></div>\n"
        "        </div>\n"
        "        <div style=\"margin-top: 30px; padding: 20px; background: #f9f9f9; border-radius: 5px;\">\n"
        "            <h3>功能特性：</h3>\n"
        "            <ul>\n"
        "                <li> 自动MIME类型检测</li>\n"
        "                <li> 文件缓存机制</li>\n"
        "                <li> 条件请求支持 (ETag, Last-Modified)</li>\n"
        "                <li> 路径安全验证</li>\n"
        "                <li> 目录列表功能</li>\n"
        "                <li> 压缩支持 (预留)</li>\n"
        "            </ul>\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, html_content, strlen(html_content));
    uvhttp_response_send(response);
    return 0;
}

/**
 * 创建测试文件
 */
void create_test_files() {
    /* 创建测试HTML文件 */
    FILE* html_file = fopen("./public/test.html", "w");
    if (html_file) {
        fprintf(html_file, 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <title>测试页面</title>\n"
            "    <link rel=\"stylesheet\" href=\"/style.css\">\n"
            "</head>\n"
            "<body>\n"
            "    <h1>测试页面</h1>\n"
            "    <p>这是一个测试页面，用于验证静态文件服务功能。</p>\n"
            "    <button onclick=\"testJavaScript()\">测试JavaScript</button>\n"
            "    <script src=\"/script.js\"></script>\n"
            "</body>\n"
            "</html>");
        fclose(html_file);
    }
    
    /* 创建测试CSS文件 */
    FILE* css_file = fopen("./public/style.css", "w");
    if (css_file) {
        fprintf(css_file,
            "body {\n"
            "    font-family: Arial, sans-serif;\n"
            "    margin: 0;\n"
            "    padding: 20px;\n"
            "    background-color: #f5f5f5;\n"
            "}\n"
            "h1 {\n"
            "    color: #333;\n"
            "    text-align: center;\n"
            "}\n"
            "button {\n"
            "    background-color: #007bff;\n"
            "    color: white;\n"
            "    border: none;\n"
            "    padding: 10px 20px;\n"
            "    border-radius: 5px;\n"
            "    cursor: pointer;\n"
            "}\n"
            "button:hover {\n"
            "    background-color: #0056b3;\n"
            "}");
        fclose(css_file);
    }
    
    /* 创建测试JavaScript文件 */
    FILE* js_file = fopen("./public/script.js", "w");
    if (js_file) {
        fprintf(js_file,
            "function testJavaScript() {\n"
            "    alert('JavaScript文件加载成功！');\n"
            "    console.log('UVHTTP静态文件服务正常工作');\n"
            "}\n"
            "\n"
            "// 页面加载完成后的初始化\n"
            "document.addEventListener('DOMContentLoaded', function() {\n"
            "    console.log('页面加载完成');\n"
            "});");
        fclose(js_file);
    }
    
    /* 创建图片目录 */
    (void)system("mkdir -p ./public/images");
    (void)system("mkdir -p ./public/docs");
    
    printf("测试文件已创建在 ./public/ 目录下\n");
}

int main() {
    printf("=== UVHTTP 静态文件服务器示例 ===\n");
    
    /* 创建测试文件 */
    create_test_files();
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .max_cache_size = 10 * 1024 * 1024,  /* 10MB缓存 */
        .cache_ttl = 3600,                   /* 1小时TTL */
        .custom_headers = ""
    };
    
    /* 创建应用上下文 */
    app_context_t* app_ctx = uvhttp_alloc(sizeof(app_context_t));
    if (!app_ctx) {
        printf("错误：无法分配应用上下文\n");
        return 1;
    }
    memset(app_ctx, 0, sizeof(app_context_t));
    
    /* 创建静态文件服务上下文 */
    uvhttp_error_t result = uvhttp_static_create(&static_config, &app_ctx->static_ctx);
    if (result != UVHTTP_OK || !app_ctx->static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        uvhttp_free(app_ctx);
        return 1;
    }
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建HTTP服务器 */
    uvhttp_server_t* server = NULL;
    uvhttp_error_t server_result = uvhttp_server_new(loop, &server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        uvhttp_static_free(app_ctx->static_ctx);
        uvhttp_free(app_ctx);
        return 1;
    }
    if (!server) {
        printf("错误：无法创建HTTP服务器\n");
        uvhttp_static_free(app_ctx->static_ctx);
        uvhttp_free(app_ctx);
        return 1;
    }
    
    /* 设置服务器上下文 - 避免使用全局变量 */
    loop->data = app_ctx;
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    uvhttp_error_t router_result = uvhttp_router_new(&router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(router_result));
        uvhttp_static_free(app_ctx->static_ctx);
        uvhttp_free(app_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 添加路由 - 应用层实现静态文件路由 */
    uvhttp_router_add_route(router, "/", home_handler);
    
    /* 静态文件路由模式 1: 使用前缀路径 */
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    
    /* 静态文件路由模式 2: 使用通配符处理所有未匹配的请求 */
    /* 注意: 通配符路由应该放在最后，否则会阻止其他路由的匹配 */
    uvhttp_router_add_route(router, "/*", static_file_handler);
    
    /* 设置路由 */
    server->router = router;
    
    /* 启动服务器 */
    uvhttp_error_t listen_result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (listen_result != UVHTTP_OK) {
        printf("错误：无法启动服务器 (错误码: %d)\n", listen_result);
        uvhttp_static_free(app_ctx->static_ctx);
        uvhttp_free(app_ctx);
        uvhttp_server_free(server);
        return 1;
    }
    
    printf(" 静态文件服务器启动成功！\n");
    printf("📍 服务地址: http://localhost:8080\n");
    printf("📁 静态文件目录: %s\n", static_config.root_directory);
    printf("📄 测试页面: http://localhost:8080/test.html\n");
    printf("🎨 样式文件: http://localhost:8080/style.css\n");
    printf("📜 脚本文件: http://localhost:8080/script.js\n");
    printf("📁 目录列表: http://localhost:8080/images/\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    if (app_ctx && app_ctx->static_ctx) {
        uvhttp_static_free(app_ctx->static_ctx);
    }
    if (app_ctx) {
        uvhttp_free(app_ctx);
    }
    uvhttp_server_free(server);
    
    printf("\n服务器已停止\n");
    return 0;
}