/*
 * 静态文件服务端到端测试
 * 测试静态文件服务的各种场景
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#if UVHTTP_FEATURE_STATIC_FILES
#include "uvhttp_static.h"
#endif
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#if UVHTTP_FEATURE_STATIC_FILES
/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_static_context_t* static_ctx;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
} app_context_t;

/* 信号处理器 */
static void on_sigint(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

static void on_sigterm(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

/* 全局应用上下文 */
static app_context_t* g_app_context = NULL;

/* 静态文件处理器 */
static int static_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    printf("Static file request: %s\n", path);
    
    /* 从全局上下文获取静态文件上下文 */
    if (!g_app_context || !g_app_context->static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static context not initialized", 28);
        uvhttp_response_send(response);
        return 0;
    }
    
    /* 处理静态文件请求 */
    uvhttp_static_handle_request(g_app_context->static_ctx, request, response);
    
    return 0;
}

/* 主页处理器 - 测试说明 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Static Files E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>📁 Static Files End-to-End Test Server</h1>"
        "<p>测试静态文件服务的各种场景</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /static/* - 静态文件服务"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /static/index.html - HTML 文件"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /static/test.txt - 文本文件"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /static/image.png - 图片文件"
        "</div>"
        ""
        "<h2>测试命令示例：</h2>"
        "<pre>"
        "# 测试 HTML 文件\n"
        "curl http://localhost:8083/static/index.html\n"
        ""
        "# 测试文本文件\n"
        "curl http://localhost:8083/static/test.txt\n"
        ""
        "# 测试不存在的文件\n"
        "curl http://localhost:8083/static/notfound.html\n"
        ""
        "# 测试目录访问\n"
        "curl http://localhost:8083/static/\n"
        ""
        "# 测试 Range 请求\n"
        "curl -H \"Range: bytes=0-10\" http://localhost:8083/static/test.txt\n"
        ""
        "# 测试 HEAD 请求\n"
        "curl -I http://localhost:8083/static/test.txt\n"
        "</pre>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    printf("Index page accessed\n");
    return 0;
}

/* 创建测试文件 */
static int create_test_files(const char* base_dir) {
    char path[1024];
    FILE* fp;
    
    /* 创建 test.txt */
    snprintf(path, sizeof(path), "%s/test.txt", base_dir);
    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "This is a test file for static file serving.\n");
        fprintf(fp, "It contains multiple lines of text.\n");
        fprintf(fp, "Line 3\n");
        fprintf(fp, "Line 4\n");
        fprintf(fp, "Line 5\n");
        fclose(fp);
        printf("Created: %s\n", path);
    }
    
    /* 创建 index.html */
    snprintf(path, sizeof(path), "%s/index.html", base_dir);
    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "<!DOCTYPE html>\n");
        fprintf(fp, "<html>\n");
        fprintf(fp, "<head>\n");
        fprintf(fp, "<title>Test Page</title>\n");
        fprintf(fp, "</head>\n");
        fprintf(fp, "<body>\n");
        fprintf(fp, "<h1>Static File Test</h1>\n");
        fprintf(fp, "<p>This is a test HTML file served by uvhttp.</p>\n");
        fprintf(fp, "</body>\n");
        fprintf(fp, "</html>\n");
        fclose(fp);
        printf("Created: %s\n", path);
    }
    
    /* 创建 data.json */
    snprintf(path, sizeof(path), "%s/data.json", base_dir);
    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "{\n");
        fprintf(fp, "  \"name\": \"uvhttp\",\n");
        fprintf(fp, "  \"version\": \"2.2.0\",\n");
        fprintf(fp, "  \"description\": \"High-performance HTTP server\"\n");
        fprintf(fp, "}\n");
        fclose(fp);
        printf("Created: %s\n", path);
    }
    
    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8083;
    const char* static_dir = "./public/static";
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    if (argc > 2) {
        static_dir = argv[2];
    }
    
    /* 创建测试文件目录 */
    struct stat st = {0};
    if (stat(static_dir, &st) == -1) {
        mkdir(static_dir, 0755);
        printf("Created directory: %s\n", static_dir);
    }
    
    /* 创建测试文件 */
    create_test_files(static_dir);
    
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t static_config = {0};
    strcpy(static_config.root_directory, static_dir);
    strcpy(static_config.index_file, "index.html");
    static_config.enable_directory_listing = 1;
    static_config.max_cache_size = 10 * 1024 * 1024; /* 10MB 缓存 */
    static_config.cache_ttl = 3600; /* 1小时缓存 */
    
    /* 创建静态文件上下文 */
    uvhttp_error_t result = uvhttp_static_create(&static_config, &ctx->static_ctx);
    if (result != UVHTTP_OK || !ctx->static_ctx) {
        fprintf(stderr, "Failed to create static context: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 预热缓存 */
    uvhttp_static_prewarm_cache(ctx->static_ctx, "/static/index.html");
    printf("Cache prewarmed\n");
    
    /* 创建服务器 */
    result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 设置全局应用上下文 */
    g_app_context = ctx;
    
    /* 设置服务器用户数据 */
    ctx->server->user_data = ctx;
    
    /* 创建路由器 */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 - 主页 */
    uvhttp_router_add_route(ctx->router, "/", index_handler);
    
    /* 添加路由 - 静态文件 */
    uvhttp_router_add_route(ctx->router, "/static/*", static_handler);
    
    /* 设置路由器到服务器 */
    ctx->server->router = ctx->router;
    
    /* 初始化信号处理器 */
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    /* 启动服务器 */
    result = uvhttp_server_listen(ctx->server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        uvhttp_static_free(ctx->static_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("Static Files E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("Static Dir: %s\n", static_dir);
    printf("========================================\n");
    printf("\n测试功能:\n");
    printf("  - 静态文件服务\n");
    printf("  - 文件类型检测\n");
    printf("  - 缓存预热\n");
    printf("  - Range 请求支持\n");
    printf("  - 404 错误处理\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /static/* (静态文件)\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx) {
        if (ctx->static_ctx) {
            uvhttp_static_free(ctx->static_ctx);
        }
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
        }
        uvhttp_free(ctx);
    }

    printf("\n========================================\n");
    printf("服务器已停止\n");
    printf("========================================\n");

    return 0;
}
#else /* UVHTTP_FEATURE_STATIC_FILES */

/* 当静态文件功能禁用时，提供一个简单的 main 函数 */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("静态文件功能已禁用，跳过测试\n");
    return 0;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */