/**
 * UVHTTP 静态文件服务测试（使用中间件）
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_context_t* g_context = NULL;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建事件循环
    g_loop = uv_default_loop();
    
    // 创建HTTP服务器
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        return 1;
    }

    // 创建上下文
    g_context = uvhttp_context_create(g_loop);
    if (!g_context) {
        uvhttp_server_free(g_server);
        return 1;
    }

    // 初始化配置
    uvhttp_config_t* config = uvhttp_config_new();
    if (config) {
        uvhttp_config_set_current(g_context, config);
    }
    // 配置静态文件服务
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 100 * 1024 * 1024,  /* 100MB缓存 */
        .cache_ttl = 7200,                    /* 2小时TTL */
        .custom_headers = ""
    };
    
    // 创建静态文件中间件（使用 "/static" 路径前缀）
    uvhttp_http_middleware_t* static_middleware = uvhttp_static_middleware_create(
        "/static",
        static_config.root_directory,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    
    if (!static_middleware) {
        uvhttp_server_free(g_server);
        return 1;
    }
    
    // 注册中间件到服务器
    uvhttp_server_add_middleware(g_server, static_middleware);
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        uvhttp_server_free(g_server);
        return 1;
    }
    
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);

    // 清理资源
    if (g_context) {
        uvhttp_context_destroy(g_context);
        g_context = NULL;
    }
    uvhttp_server_free(g_server);

    return 0;
}