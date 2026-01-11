#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uvhttp_server_t* g_server = NULL;
static uvhttp_loop_t* g_loop = NULL;
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
    g_loop = NULL;
    exit(0);
}

int main(int argc, char* argv[]) {
    const char* root_directory = argc > 1 ? argv[1] : "./public";
    int port = argc > 2 ? atoi(argv[2]) : 8888;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    g_loop = uv_default_loop();
    
    uvhttp_config_t* config = uvhttp_config_new();
    if (!config) {
        printf("错误：无法创建配置\n");
        return 1;
    }
    uvhttp_config_set_current(config);
    
    uvhttp_static_config_t static_config;
    memset(&static_config, 0, sizeof(static_config));
    strncpy(static_config.root_directory, root_directory, sizeof(static_config.root_directory) - 1);
    static_config.root_directory[sizeof(static_config.root_directory) - 1] = '\0';
    strncpy(static_config.index_file, "index.html", sizeof(static_config.index_file) - 1);
    static_config.enable_directory_listing = 1;
    static_config.enable_etag = 1;
    static_config.enable_last_modified = 1;
    static_config.max_cache_size = 100 * 1024 * 1024;
    static_config.cache_ttl = 7200;
    
    g_static_ctx = uvhttp_static_create(&static_config);
    if (!g_static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        uvhttp_config_free(config);
        return 1;
    }
    
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        printf("错误：无法创建HTTP服务器\n");
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        return 1;
    }
    
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        printf("错误：无法创建路由器\n");
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    g_server->router = router;
    
    int result = uvhttp_server_listen(g_server, "0.0.0.0", port);
    if (result != 0) {
        printf("错误：无法启动服务器 (错误码: %d)\n", result);
        uvhttp_router_free(router);
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("服务器启动成功：http://localhost:%d\n", port);
    printf("静态文件目录：%s\n", root_directory);
    
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    uvhttp_router_free(router);
    uvhttp_static_free(g_static_ctx);
    uvhttp_config_free(config);
    uvhttp_server_free(g_server);
    
    return 0;
}