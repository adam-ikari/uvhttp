/*
 * UVHTTP 缓存测试服务器
 * 
 * 演示新的LRU缓存功能，包括缓存统计和性能监控
 */

#include "uvhttp.h"
#include "uvhttp_static.h"
#include "uvhttp_lru_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 静态文件服务上下文 */
static uvhttp_static_context_t* g_static_ctx = NULL;

/**
 * 静态文件请求处理器
 */
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 未使用参数
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
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
    }
    
    uvhttp_response_send(response);
    return 0;
}

/**
 * 缓存统计处理器
 */
int cache_stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 未使用参数
    (void)request;  // 未使用参数
    if (!g_static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Service not initialized", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 获取缓存统计信息 */
    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;
    
    /* 获取真实缓存统计信息 */
    uvhttp_static_get_cache_stats(g_static_ctx, &total_memory_usage, &entry_count,
                                  &hit_count, &miss_count, &eviction_count);
    
    double hit_rate = uvhttp_static_get_cache_hit_rate(g_static_ctx);
    
    /* 生成统计信息HTML */
    char stats_html[2048];
    snprintf(stats_html, sizeof(stats_html),
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP 缓存统计</title>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .stats { background: #f5f5f5; padding: 20px; border-radius: 8px; }\n"
        "        .metric { margin: 10px 0; }\n"
        "        .metric strong { color: #333; display: inline-block; width: 200px; }\n"
        "        .hit-rate { font-size: 24px; font-weight: bold; color: #28a745; }\n"
        "        .refresh { margin-top: 20px; }\n"
        "        button { padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>UVHTTP 缓存统计信息</h1>\n"
        "    <div class=\"stats\">\n"
        "        <div class=\"metric hit-rate\">缓存命中率: %.2f%%</div>\n"
        "        <div class=\"metric\"><strong>缓存条目数:</strong> %d</div>\n"
        "        <div class=\"metric\"><strong>内存使用量:</strong> %zu 字节 (%.2f MB)</div>\n"
        "        <div class=\"metric\"><strong>命中次数:</strong> %d</div>\n"
        "        <div class=\"metric\"><strong>未命中次数:</strong> %d</div>\n"
        "        <div class=\"metric\"><strong>驱逐次数:</strong> %d</div>\n"
        "        <div class=\"metric\"><strong>总请求次数:</strong> %d</div>\n"
        "        <div class=\"metric\"><strong>平均条目大小:</strong> %.2f KB</div>\n"
        "    </div>\n"
        "    <div class=\"refresh\">\n"
        "        <button onclick=\"location.reload()\">刷新统计</button>\n"
        "        <button onclick=\"clearExpiredCache()\">清理过期缓存</button>\n"
        "    </div>\n"
        "    <script>\n"
        "        function clearExpiredCache() {\n"
        "            fetch('/clear-cache', {method: 'POST'})\n"
        "                .then(response => response.text())\n"
        "                .then(data => {\n"
        "                    alert(data);\n"
        "                    location.reload();\n"
        "                });\n"
        "        }\n"
        "        // 自动刷新\n"
        "        setTimeout(() => location.reload(), 5000);\n"
        "    </script>\n"
        "</body>\n"
        "</html>",
        hit_rate * 100.0,
        entry_count,
        total_memory_usage,
        total_memory_usage / (1024.0 * 1024.0),
        hit_count,
        miss_count,
        eviction_count,
        hit_count + miss_count,
        entry_count > 0 ? (double)total_memory_usage / entry_count / 1024.0 : 0.0
    );
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, stats_html, strlen(stats_html));
    uvhttp_response_send(response);
    return 0;
}

/**
 * 清理过期缓存处理器
 */
int clear_cache_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 未使用参数
    if (!g_static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Service not initialized", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    /* 清理过期缓存 */
    int cleaned_count = uvhttp_static_cleanup_expired_cache(g_static_ctx);
    
    char result[256];
    snprintf(result, sizeof(result), "清理了 %d 个过期缓存条目", cleaned_count);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, result, strlen(result));
    uvhttp_response_send(response);
    return 0;
}

/**
 * 主页处理器
 */
int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 未使用参数
    const char* html_content = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP 缓存测试服务器</title>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .header { text-align: center; margin-bottom: 30px; }\n"
        "        .links { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 20px; }\n"
        "        .link { padding: 10px; background: #f0f0f0; border-radius: 5px; text-decoration: none; }\n"
        "        .link:hover { background: #e0e0e0; }\n"
        "        .info { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"header\">\n"
        "            <h1> UVHTTP 缓存测试服务器</h1>\n"
        "            <p>高性能LRU缓存演示</p>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"info\">\n"
        "            <h2> 缓存特性</h2>\n"
        "            <ul>\n"
        "                <li><strong>LRU算法</strong> - 最近最少使用缓存替换策略</li>\n"
        "                <li><strong>uthash支持</strong> - 高性能哈希表实现</li>\n"
        "                <li><strong>内存控制</strong> - 可配置最大内存使用量</li>\n"
        "                <li><strong>TTL支持</strong> - 可配置缓存过期时间</li>\n"
        "                <li><strong>统计监控</strong> - 实时缓存命中率统计</li>\n"
        "            </ul>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"info\">\n"
        "            <h2>🔗 测试链接</h2>\n"
        "            <div class=\"links\">\n"
        "                <a href=\"/cache-stats\" class=\"link\">缓存统计</a>\n"
        "                <a href=\"/test.html\" class=\"link\">测试文件</a>\n"
        "                <a href=\"/test.css\" class=\"link\">样式文件</a>\n"
        "                <a href=\"/test.js\" class=\"link\">脚本文件</a>\n"
        "                <a href=\"/large-file.dat\" class=\"link\">大文件</a>\n"
        "            </div>\n"
        "        </div>\n"
        "        \n"
        "        <div class=\"info\">\n"
        "            <h2> 测试说明</h2>\n"
        "            <ol>\n"
        "                <li>多次访问同一文件，观察缓存命中</li>\n"
        "                <li>访问缓存统计页面查看实时数据</li>\n"
        "                <li>等待TTL过期后再次访问</li>\n"
        "                <li>观察内存使用量的变化</li>\n"
        "            </ol>\n"
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

/**
 * 创建测试文件
 */
void create_test_files() {
    /* 创建测试HTML文件 */
    FILE* html_file = fopen("public/test.html", "w");
    if (html_file) {
        fprintf(html_file, 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head><title>测试页面</title></head>\n"
            "<body><h1>这是一个测试HTML文件</h1><p>缓存测试内容</p></body>\n"
            "</html>\n");
        fclose(html_file);
    }
    
    /* 创建测试CSS文件 */
    FILE* css_file = fopen("public/test.css", "w");
    if (css_file) {
        fprintf(css_file, 
            "body { font-family: Arial, sans-serif; background: #f0f0f0; }\n"
            "h1 { color: #333; text-align: center; }\n"
            "p { color: #666; line-height: 1.6; }\n");
        fclose(css_file);
    }
    
    /* 创建测试JS文件 */
    FILE* js_file = fopen("public/test.js", "w");
    if (js_file) {
        fprintf(js_file, 
            "console.log('这是一个测试JavaScript文件');\n"
            "function test() {\n"
            "    return '缓存测试成功';\n"
            "}\n"
            "document.addEventListener('DOMContentLoaded', function() {\n"
            "    console.log(test());\n"
            "});\n");
        fclose(js_file);
    }
    
    /* 创建大文件 */
    FILE* large_file = fopen("public/large-file.dat", "w");
    if (large_file) {
        for (int i = 0; i < 10000; i++) {
            fprintf(large_file, "这是大文件测试内容行 %d - 用于测试大文件缓存性能\n", i);
        }
        fclose(large_file);
    }
}

int main() {
    printf("启动UVHTTP缓存测试服务器...\n");
    
    /* 创建测试文件 */
    create_test_files();
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .max_cache_size = 10 * 1024 * 1024,  /* 10MB缓存 */
        .cache_ttl = 300,                      /* 5分钟TTL */
        .max_cache_entries = 1000,             /* 最大1000个条目 */
        .custom_headers = ""
    };

    /* 创建静态文件服务上下文 */
    uvhttp_error_t result = uvhttp_static_create(&config, &g_static_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create static file context: %s\n", uvhttp_error_string(result));
        return 1;
    }
    if (!g_static_ctx) {
        fprintf(stderr, "Failed to create static file context\n");
        return 1;
    }
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_error_t server_result = uvhttp_server_new(loop, &server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }

    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    server_result = uvhttp_router_new(&router);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/cache-stats", cache_stats_handler);
    uvhttp_router_add_route(router, "/clear-cache", clear_cache_handler);
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    uvhttp_router_add_route(router, "/*", static_file_handler);  /* 处理所有其他请求 */
    
    server->router = router;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(server, "0.0.0.0", 8080) != 0) {
        fprintf(stderr, "Failed to start server\n");
        uvhttp_static_free(g_static_ctx);
        return 1;
    }
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("访问 http://localhost:8080/cache-stats 查看缓存统计\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    uvhttp_static_free(g_static_ctx);
    
    return 0;
}