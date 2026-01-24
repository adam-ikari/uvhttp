/*
 * UVHTTP 静态文件中间件演示
 *
 * 演示如何使用静态文件中间件：
 * 1. 创建静态文件中间件
 * 2. 配置静态文件服务
 * 3. 与其他中间件组合使用
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/uvhttp.h"
#include "../include/uvhttp_middleware.h"
#include "../include/uvhttp_static.h"

/* 日志中间件数据 */
typedef struct {
    int request_count;
} log_middleware_data_t;

/* 日志中间件处理函数 */
static int log_middleware_handler(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    (void)response;
    log_middleware_data_t* data = (log_middleware_data_t*)ctx->data;

    const char* method = uvhttp_request_get_method(request);
    const char* path = uvhttp_request_get_path(request);

    data->request_count++;

    printf("[LOG] Request #%d: %s %s\n", data->request_count, method, path);

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 日志中间件清理函数 */
static void log_middleware_cleanup(void* data) {
    if (data) {
        log_middleware_data_t* log_data = (log_middleware_data_t*)data;
        printf("[LOG] Total requests handled: %d\n", log_data->request_count);
        free(data);
    }
}

/* 根路径处理器 */
static int root_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");

    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>Static File Middleware Demo</title></head>"
        "<body>"
        "<h1>UVHTTP Static File Middleware Demo</h1>"
        "<p>This demo shows the static file middleware in action.</p>"
        "<ul>"
        "<li><a href=\"/static/\">Static Files</a></li>"
        "<li><a href=\"/api/\">API Endpoint</a></li>"
        "</ul>"
        "</body>"
        "</html>";

    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    return 0;
}

/* API 处理器 */
static int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");

    const char* json = "{\"message\": \"API endpoint\", \"status\": \"ok\"}";
    uvhttp_response_set_body(response, json, strlen(json));
    uvhttp_response_send(response);
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("========================================\n");
    printf("UVHTTP Static File Middleware Demo\n");
    printf("========================================\n\n");

    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    /* 创建路由器 */
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    /* ========== 添加中间件 ========== */

    /* 1. 日志中间件（高优先级） */
    log_middleware_data_t* log_data = malloc(sizeof(log_middleware_data_t));
    log_data->request_count = 0;

    uvhttp_http_middleware_t* log_middleware = uvhttp_http_middleware_create(
        NULL,
        log_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    uvhttp_http_middleware_set_context(log_middleware, log_data, log_middleware_cleanup);
    uvhttp_server_add_middleware(server, log_middleware);
    printf("[INIT] Log middleware added\n");

    /* 2. 静态文件中间件（正常优先级） */
    /* 使用当前目录下的 public 文件夹作为静态文件根目录 */
    const char* static_root = "./public";

    uvhttp_http_middleware_t* static_middleware = uvhttp_static_middleware_create(
        "/static",  /* 匹配 /static 路径 */
        static_root,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );

    if (static_middleware) {
        uvhttp_server_add_middleware(server, static_middleware);
        printf("[INIT] Static file middleware added for /static -> %s\n", static_root);
    } else {
        printf("[WARN] Failed to create static file middleware\n");
    }

    /* ========== 添加路由 ========== */

    uvhttp_router_add_route(router, "/", root_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    printf("[INIT] Routes added: /, /api\n");

    /* ========== 启动服务器 ========== */

    const char* host = "0.0.0.0";
    int port = 8080;

    printf("\n[INIT] Starting server on %s:%d\n", host, port);
    printf("[INIT] Middleware chain: Log -> Static -> Router\n");
    printf("[INIT] Try these URLs:\n");
    printf("  - http://localhost:8080/\n");
    printf("  - http://localhost:8080/static/ (requires ./public directory)\n");
    printf("  - http://localhost:8080/api/\n\n");

    uvhttp_error_t result = uvhttp_server_listen(server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("[INIT] Server started successfully\n");
    printf("[INIT] Press Ctrl+C to stop\n\n");

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_free(server);
    uvhttp_router_free(router);

    printf("\n[SHUTDOWN] Server stopped\n");

    return 0;
}