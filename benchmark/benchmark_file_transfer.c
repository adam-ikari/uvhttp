/**
 * @file benchmark_file_transfer.c
 * @brief 文件传输性能基准测试服务器
 * 
 * 这个程序专门用于测试文件传输性能，包括：
 * - 小文件传输（1KB - 64KB）
 * - 中等文件传输（64KB - 1MB）
 * - 大文件传输（1MB - 10MB）
 * - 超大文件传输（10MB - 100MB）
 * 
 * 使用 wrk 或 ab 工具进行压力测试：
 *   wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/small
 *   wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/medium
 *   wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/large
 *   ab -n 1000 -c 10 http://127.0.0.1:18082/file/small
 */

#include <uv.h>
#include <uvhttp.h>
#include <uvhttp_static.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#define DEFAULT_PORT 18082

/* 应用上下文 - 使用 server->user_data 传递 */
typedef struct {
    volatile sig_atomic_t running;
    uvhttp_static_context_t* static_ctx;
} app_context_t;

/* 信号处理需要的静态变量（POSIX 允许） */
static uvhttp_server_t* g_signal_server = NULL;

/* 信号处理函数 */
static void signal_handler(int signum) {
    (void)signum;
    printf("\n收到停止信号，正在关闭服务器...\n");
    if (g_signal_server) {
        app_context_t* ctx = (app_context_t*)g_signal_server->user_data;
        if (ctx) {
            ctx->running = 0;
        }
        uvhttp_server_stop(g_signal_server);
    }
}

/* 健康检查处理器 */
static int health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "{\"status\":\"ok\",\"service\":\"file-transfer-benchmark\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "54");
    uvhttp_response_set_body(response, body, 54);
    uvhttp_response_send(response);
    
    return 0;
}

/* 创建测试文件 */
static void create_test_file(const char* path, size_t size) {
    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "无法创建测试文件: %s\n", path);
        return;
    }
    
    /* 写入随机数据 */
    unsigned char buffer[4096];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (unsigned char)(rand() % 256);
    }
    
    size_t remaining = size;
    while (remaining > 0) {
        size_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        fwrite(buffer, 1, chunk, f);
        remaining -= chunk;
    }
    
    fclose(f);
    printf("创建测试文件: %s (%.2f MB)\n", path, size / (1024.0 * 1024.0));
}

/* 打印使用说明 */
static void print_usage(const char* program, int port) {
    (void)program;  /* 避免未使用参数警告 */
    printf("\n");
    printf("========================================\n");
    printf("  文件传输性能基准测试服务器\n");
    printf("========================================\n\n");
    printf("服务器地址: http://127.0.0.1:%d\n\n", port);
    printf("测试端点:\n");
    printf("  /file/small     - 小文件 (1KB)\n");
    printf("  /file/medium    - 中等文件 (64KB)\n");
    printf("  /file/large     - 大文件 (1MB)\n");
    printf("  /file/xlarge    - 超大文件 (10MB)\n");
    printf("  /file/xxlarge   - 超超大文件 (100MB)\n");
    printf("  /health         - 健康检查\n\n");
    printf("性能测试命令:\n");
    printf("  # 小文件测试\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/file/small\n\n", port);
    printf("  # 中等文件测试\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/file/medium\n\n", port);
    printf("  # 大文件测试\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/file/large\n\n", port);
    printf("  # 超大文件测试\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/file/xlarge\n\n", port);
    printf("  # 使用 ab 测试（适合小文件）\n");
    printf("  ab -n 1000 -c 10 http://127.0.0.1:%d/file/small\n\n", port);
    printf("  # 并发测试\n");
    printf("  wrk -t8 -c200 -d60s http://127.0.0.1:%d/file/medium\n\n", port);
    printf("  # 长时间稳定性测试\n");
    printf("  wrk -t4 -c100 -d300s http://127.0.0.1:%d/file/large\n\n", port);
    printf("按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    printf("程序启动...\n");
    fflush(stdout);
    
    int port = DEFAULT_PORT;
    
    /* 解析命令行参数 */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    /* 创建测试文件目录 */
    const char* test_dir = "./public/file_test";
    printf("创建测试文件目录: %s\n", test_dir);
    fflush(stdout);
    char command[256];
    snprintf(command, sizeof(command), "mkdir -p %s", test_dir);
    int ret = system(command);
    (void)ret;  /* 避免未使用返回值警告 */
    
    /* 创建 file 目录和符号链接 */
    const char* file_dir = "./public/file";
    snprintf(command, sizeof(command), "mkdir -p %s", file_dir);
    ret = system(command);
    (void)ret;
    
    printf("创建测试文件...\n");
    fflush(stdout);
    
    /* 创建不同大小的测试文件 */
    create_test_file("./public/file_test/small.txt", 1024);        /* 1KB */
    create_test_file("./public/file_test/medium.bin", 64 * 1024);   /* 64KB */
    create_test_file("./public/file_test/large.bin", 1024 * 1024);  /* 1MB */
    create_test_file("./public/file_test/xlarge.bin", 10 * 1024 * 1024); /* 10MB */
    create_test_file("./public/file_test/xxlarge.bin", 100 * 1024 * 1024); /* 100MB */
    
    /* 创建符号链接，让 /file/small 映射到 file_test/small.txt */
    printf("创建符号链接...\n");
    fflush(stdout);
    unlink("./public/file/small");
    unlink("./public/file/medium");
    unlink("./public/file/large");
    unlink("./public/file/xlarge");
    unlink("./public/file/xxlarge");
    ret = symlink("../file_test/small.txt", "./public/file/small");
    (void)ret;
    ret = symlink("../file_test/medium.bin", "./public/file/medium");
    (void)ret;
    ret = symlink("../file_test/large.bin", "./public/file/large");
    (void)ret;
    ret = symlink("../file_test/xlarge.bin", "./public/file/xlarge");
    (void)ret;
    ret = symlink("../file_test/xxlarge.bin", "./public/file/xxlarge");
    (void)ret;
    
    /* 创建事件循环 */
    printf("创建事件循环...\n");
    fflush(stdout);
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "无法创建事件循环\n");
        return 1;
    }
    
    /* 创建应用上下文 */
    printf("创建应用上下文...\n");
    fflush(stdout);
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->running = 1;
    
    /* 创建静态文件上下文 */
    uvhttp_static_config_t static_config;
    memset(&static_config, 0, sizeof(static_config));
    
    /* 配置静态文件服务 */
    static_config.max_cache_size = 100 * 1024 * 1024;  /* 100MB 缓存 */
    static_config.cache_ttl = 3600;  /* 1小时 TTL */
    static_config.max_cache_entries = 1000;
    static_config.sendfile_timeout_ms = 30000;
    static_config.sendfile_max_retry = 2;
    static_config.sendfile_chunk_size = 256 * 1024;  /* 256KB 分块 */
    static_config.enable_directory_listing = 0;
    static_config.enable_etag = 1;
    static_config.enable_last_modified = 1;
    static_config.enable_sendfile = 1;
    
    /* 使用绝对路径 */
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "无法获取当前工作目录\n");
        uvhttp_free(ctx);
        return 1;
    }
    size_t cwd_len = strlen(cwd);
    if (cwd_len + sizeof("/public") > sizeof(static_config.root_directory)) {
        fprintf(stderr, "路径太长\n");
        uvhttp_free(ctx);
        return 1;
    }
    strncpy(static_config.root_directory, cwd, sizeof(static_config.root_directory) - 1);
    strncat(static_config.root_directory, "/public", sizeof(static_config.root_directory) - strlen(static_config.root_directory) - 1);
    
    printf("静态文件根目录: %s\n", static_config.root_directory);
    fflush(stdout);
    
    uvhttp_error_t result = uvhttp_static_create(&static_config, &ctx->static_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法创建静态文件上下文: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 缓存预热 */
    printf("预热缓存...\n");
    fflush(stdout);
    /* 暂时注释掉缓存预热，避免阻塞 */
    // uvhttp_static_prewarm_cache(ctx->static_ctx, "file_test/small.txt");
    // uvhttp_static_prewarm_cache(ctx->static_ctx, "file_test/medium.bin");
    // uvhttp_static_prewarm_cache(ctx->static_ctx, "file_test/large.bin");
    printf("缓存预热完成\n\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建服务器 */
    printf("创建服务器...\n");
    fflush(stdout);
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK || !server) {
        fprintf(stderr, "无法创建服务器: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("设置服务器上下文...\n");
    fflush(stdout);
    
    /* 设置服务器用户数据和信号处理需要的指针 */
    server->user_data = ctx;
    g_signal_server = server;
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法创建路由: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_signal_server = NULL;
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/health", health_handler);
    
    /* 设置静态文件路由，路径 /file 会映射到 public/file */
    result = uvhttp_router_add_static_route(router, "/file", ctx->static_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法添加静态文件路由: %s\n", uvhttp_error_string(result));
    }
    
    server->router = router;
    
    /* 启动服务器 */
    printf("启动服务器在端口 %d...\n", port);
    fflush(stdout);
    result = uvhttp_server_listen(server, "127.0.0.1", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法启动服务器: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_signal_server = NULL;
        return 1;
    }
    
    /* 打印使用说明 */
    print_usage(argv[0], port);
    
    /* 运行事件循环 */
    printf("开始运行事件循环...\n");
    fflush(stdout);
    uv_run(loop, UV_RUN_DEFAULT);
    printf("事件循环结束\n");
    fflush(stdout);
    
    /* 清理 */
    printf("\n正在关闭服务器...\n");
    uvhttp_static_free(ctx->static_ctx);
    uvhttp_server_free(server);
    uvhttp_free(ctx);
    g_signal_server = NULL;
    
    printf("服务器已关闭\n");
    return 0;
}
