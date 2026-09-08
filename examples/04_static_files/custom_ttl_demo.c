/*
 * 自定义静态文件缓存配置示例
 *
 * 演示如何使用 uvhttp_static_create 创建静态文件服务，并通过
 * uvhttp_static_set_cache_config 自定义缓存参数（最大缓存大小、最大条目数、
 * 缓存 TTL），以及 uvhttp_static_get_cache_stats 查看缓存统计。
 *
 * 注意：按文件扩展名分别设置 TTL 的 API（uvhttp_static_set_file_ttl_map）
 * 在当前版本中尚未实现，因此本示例展示的是对整个缓存生效的自定义 TTL
 * 配置。
 */

#include "uvhttp.h"
#include "uvhttp_static.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

static uvhttp_server_t* g_server = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_static_context_t* g_static_ctx = NULL;

void signal_handler(int sig) {
    (void)sig;
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
    }
    exit(0);
}

int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    int result = uvhttp_static_handle_request(g_static_ctx, request, response);
    if (result != UVHTTP_OK) {
        uvhttp_response_set_status(response, 404);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Not Found", 9);
        uvhttp_response_send(response);
    }
    return result;
}

int main(void) {
    printf("=== 自定义静态文件缓存配置示例 ===\n");

    /* 创建测试文件 */
    system("mkdir -p ./public");
    FILE* f;

    f = fopen("./public/script.js", "w");
    if (f) {
        fprintf(f, "console.log('JavaScript file');");
        fclose(f);
    }

    f = fopen("./public/style.css", "w");
    if (f) {
        fprintf(f, "body { color: red; }");
        fclose(f);
    }

    f = fopen("./public/data.json", "w");
    if (f) {
        fprintf(f, "{\"key\": \"value\"}");
        fclose(f);
    }

    f = fopen("./public/index.html", "w");
    if (f) {
        fprintf(f, "<html><body>Custom Cache Config Demo</body></html>");
        fclose(f);
    }

    f = fopen("./public/image.png", "w");
    if (f) {
        fprintf(f, "PNG_DATA");
        fclose(f);
    }

    /* 配置静态文件服务 */
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strncpy(config.root_directory, "./public",
            sizeof(config.root_directory) - 1);
    strncpy(config.index_file, "index.html", sizeof(config.index_file) - 1);
    config.enable_directory_listing = 1;
    config.enable_etag = 1;
    config.enable_last_modified = 1;
    config.max_cache_size = 10 * 1024 * 1024;
    config.cache_ttl = 3600;

    /* 创建静态文件服务上下文 */
    uvhttp_static_context_t* static_ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &static_ctx);
    if (result != UVHTTP_OK || !static_ctx) {
        printf("Error creating static context: %d\n", result);
        return 1;
    }
    g_static_ctx = static_ctx;

    /* 自定义缓存配置：16MB / 2 万条 / TTL 2 小时 */
    result = uvhttp_static_set_cache_config(static_ctx, 16 * 1024 * 1024,
                                            20000, 7200);
    if (result != UVHTTP_OK) {
        printf("Error setting cache config: %d\n", result);
        uvhttp_static_free(static_ctx);
        return 1;
    }

    size_t total_memory = 0;
    int entry_count = 0, hit_count = 0, miss_count = 0, eviction_count = 0;
    uvhttp_static_get_cache_stats(static_ctx, &total_memory, &entry_count,
                                  &hit_count, &miss_count, &eviction_count);

    printf("缓存配置成功：\n");
    printf("  最大缓存: %zu 字节\n", (size_t)16 * 1024 * 1024);
    printf("  最大条目: %d\n", 20000);
    printf("  缓存 TTL: %d 秒 (2 小时)\n", 7200);
    printf("  当前条目: %d, 命中: %d, 未命中: %d, 淘汰: %d\n",
           entry_count, hit_count, miss_count, eviction_count);

    /* 创建服务器 */
    g_loop = uv_default_loop();

    uvhttp_server_t* server = NULL;
    uvhttp_server_new(g_loop, &server);
    g_server = server;

    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    uvhttp_router_add_route(router, "/*", static_file_handler);
    g_server->router = router;

    if (uvhttp_server_listen(server, "0.0.0.0", 8085) != UVHTTP_OK) {
        printf("Failed to listen\n");
        uvhttp_static_free(static_ctx);
        return 1;
    }

    printf("\n 服务器启动成功！\n");
    printf(" 服务地址: http://localhost:8085\n");
    printf("\n测试文件：\n");
    printf("  http://localhost:8085/script.js\n");
    printf("  http://localhost:8085/style.css\n");
    printf("  http://localhost:8085/image.png\n");
    printf("  http://localhost:8085/index.html\n");
    printf("  http://localhost:8085/data.json\n");
    printf("\n使用 curl -I 查看响应头：\n");
    printf("  curl -I http://localhost:8085/script.js\n");
    printf("\n按 Ctrl+C 停止服务器\n");

    uv_run(g_loop, UV_RUN_DEFAULT);

    uvhttp_static_free(static_ctx);
    uvhttp_server_free(server);

    printf("\n服务器已停止\n");
    return 0;
}
