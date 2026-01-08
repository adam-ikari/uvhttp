/*
 * UVHTTP 中间件系统演示
 *
 * 演示如何使用中间件系统实现：
 * 1. 日志记录中间件
 * 2. 认证中间件
 * 3. 请求计时中间件
 * 4. 路径匹配中间件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/uvhttp.h"
#include "../include/uvhttp_middleware.h"

/* ========== 中间件示例 ========== */

/* 日志中间件数据 */
typedef struct {
    FILE* log_file;
    int request_count;
} log_middleware_data_t;

/* 日志中间件处理函数 */
static int log_middleware_handler(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    log_middleware_data_t* data = (log_middleware_data_t*)ctx->data;
    
    const char* method = uvhttp_request_get_method(request);
    const char* path = uvhttp_request_get_path(request);
    const char* client_ip = uvhttp_request_get_client_ip(request);
    
    data->request_count++;
    
    /* 记录请求信息 */
    printf("[LOG] Request #%d: %s %s from %s\n",
           data->request_count, method, path, client_ip);
    
    /* 继续执行下一个中间件 */
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

/* 认证中间件数据 */
typedef struct {
    const char* api_key;
} auth_middleware_data_t;

/* 认证中间件处理函数 */
static int auth_middleware_handler(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    auth_middleware_data_t* data = (auth_middleware_data_t*)ctx->data;
    
    const char* path = uvhttp_request_get_path(request);
    
    /* 只对 /api 路径进行认证 */
    if (strncmp(path, "/api", 4) != 0) {
        return UVHTTP_MIDDLEWARE_CONTINUE;
    }
    
    /* 获取 API Key */
    const char* api_key = uvhttp_request_get_header(request, "X-API-Key");
    
    if (!api_key || strcmp(api_key, data->api_key) != 0) {
        /* 认证失败 */
        printf("[AUTH] Authentication failed for %s\n", path);
        uvhttp_response_set_status(response, 401);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error_body = "{\"error\": \"Unauthorized\"}";
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);
        
        /* 停止执行中间件链 */
        return UVHTTP_MIDDLEWARE_STOP;
    }
    
    printf("[AUTH] Authentication successful for %s\n", path);
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 请求计时中间件数据 */
typedef struct {
    clock_t start_time;
} timing_middleware_data_t;

/* 请求计时中间件处理函数 */
static int timing_middleware_handler(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    timing_middleware_data_t* data = (timing_middleware_data_t*)ctx->data;
    
    const char* path = uvhttp_request_get_path(request);
    
    /* 记录开始时间 */
    data->start_time = clock();
    
    /* 继续执行 */
    int result = UVHTTP_MIDDLEWARE_CONTINUE;
    
    /* 计算处理时间 */
    clock_t end_time = clock();
    double elapsed = (double)(end_time - data->start_time) / CLOCKS_PER_SEC * 1000.0;
    
    printf("[TIMING] %s processed in %.2f ms\n", path, elapsed);
    
    return result;
}

/* ========== 请求处理器 ========== */

/* 根路径处理器 */
static int root_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP Middleware Demo</title></head>"
        "<body>"
        "<h1>UVHTTP Middleware Demo</h1>"
        "<p>This demo shows the middleware system in action.</p>"
        "<ul>"
        "<li><a href=\"/api/data\">API Endpoint (requires authentication)</a></li>"
        "<li><a href=\"/public\">Public Page (no authentication)</a></li>"
        "</ul>"
        "</body>"
        "</html>";

    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    return 0;
}

/* 公开页面处理器 */
static int public_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");

    const char* json = "{\"message\": \"This is a public endpoint\", \"authenticated\": false}";
    uvhttp_response_set_body(response, json, strlen(json));
    uvhttp_response_send(response);
    return 0;
}

/* API 数据处理器 */
static int api_data_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");

    const char* json = "{\"message\": \"This is a protected endpoint\", \"authenticated\": true}";
    uvhttp_response_set_body(response, json, strlen(json));
    uvhttp_response_send(response);
    return 0;
}

/* ========== 主函数 ========== */

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("UVHTTP Middleware Demo\n");
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
    
    /* 1. 日志中间件（高优先级，最先执行） */
    log_middleware_data_t* log_data = malloc(sizeof(log_middleware_data_t));
    log_data->log_file = stdout;
    log_data->request_count = 0;
    
    uvhttp_http_middleware_t* log_middleware = uvhttp_http_middleware_create(
        NULL,  /* 匹配所有路径 */
        log_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_HIGH
    );
    uvhttp_http_middleware_set_context(log_middleware, log_data, log_middleware_cleanup);
    uvhttp_server_add_middleware(server, log_middleware);
    printf("[INIT] Log middleware added\n");
    
    /* 2. 认证中间件（正常优先级） */
    auth_middleware_data_t* auth_data = malloc(sizeof(auth_middleware_data_t));
    auth_data->api_key = "secret-key-12345";
    
    uvhttp_http_middleware_t* auth_middleware = uvhttp_http_middleware_create(
        "/api",  /* 只匹配 /api 路径 */
        auth_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_NORMAL
    );
    uvhttp_http_middleware_set_context(auth_middleware, auth_data, free);
    uvhttp_server_add_middleware(server, auth_middleware);
    printf("[INIT] Auth middleware added for /api\n");
    
    /* 3. 请求计时中间件（低优先级，最后执行） */
    timing_middleware_data_t* timing_data = malloc(sizeof(timing_middleware_data_t));
    timing_data->start_time = 0;
    
    uvhttp_http_middleware_t* timing_middleware = uvhttp_http_middleware_create(
        NULL,  /* 匹配所有路径 */
        timing_middleware_handler,
        UVHTTP_MIDDLEWARE_PRIORITY_LOW
    );
    uvhttp_http_middleware_set_context(timing_middleware, timing_data, free);
    uvhttp_server_add_middleware(server, timing_middleware);
    printf("[INIT] Timing middleware added\n");
    
    /* ========== 添加路由 ========== */
    
    uvhttp_router_add_route(router, "/", root_handler);
    uvhttp_router_add_route(router, "/public", public_handler);
    uvhttp_router_add_route(router, "/api/data", api_data_handler);
    
    /* ========== 启动服务器 ========== */
    
    const char* host = "0.0.0.0";
    int port = 8080;
    
    printf("\n[INIT] Starting server on %s:%d\n", host, port);
    printf("[INIT] Middleware chain: Log -> Auth -> Timing\n");
    printf("[INIT] Try these URLs:\n");
    printf("  - http://localhost:8080/\n");
    printf("  - http://localhost:8080/public\n");
    printf("  - http://localhost:8080/api/data (use X-API-Key: secret-key-12345)\n\n");
    
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