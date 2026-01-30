#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uvhttp_server_t* g_server = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_static_context_t* g_static_ctx = NULL;
static uvhttp_context_t* g_context = NULL;

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
    
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        return 1;
    }

    uvhttp_error_t result_g_context = uvhttp_context_create(g_loop, &g_context);
    if (result_g_context != UVHTTP_OK) {
        printf("错误：无法创建上下文\n");
        uvhttp_config_free(config);
        return 1;
    }

    uvhttp_config_set_current(g_context, config);

    uvhttp_config_set_current(g_context, config);
    
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
    
    uvhttp_error_t static_result = uvhttp_static_create(&static_config, &g_static_ctx);
    if (static_result != UVHTTP_OK || !g_static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        return 1;
    }
    
    uvhttp_error_t server_result = uvhttp_server_new(g_loop, &g_server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!g_server) {
        printf("错误：无法创建HTTP服务器\n");
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        return 1;
    }
    
    uvhttp_router_t* router = NULL;
    uvhttp_error_t tmp_result = uvhttp_router_new(&router);
    if (tmp_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(tmp_result));
        return 1;
    }
    if (!router) {
        printf("错误：无法创建路由器\n");
        uvhttp_static_free(g_static_ctx);
        uvhttp_config_free(config);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    g_server->router = router;
    
    int listen_result = uvhttp_server_listen(g_server, "0.0.0.0", port);
    if (listen_result != 0) {
        printf("错误：无法启动服务器 (错误码: %d)\n", listen_result);
        uvhttp_router_free(router);
        uvhttp_static_free(g_static_ctx);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("服务器启动成功：http://localhost:%d\n", port);
    printf("静态文件目录：%s\n", root_directory);
    
    uv_run(g_loop, UV_RUN_DEFAULT);

    if (g_context) {
        uvhttp_context_destroy(g_context);
    }

    uvhttp_router_free(router);
    uvhttp_static_free(g_static_ctx);
    uvhttp_server_free(g_server);

    return 0;
}