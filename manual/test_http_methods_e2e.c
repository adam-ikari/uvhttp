/*
 * HTTP 方法端到端测试
 * 测试 GET, POST, PUT, DELETE, PATCH 等所有 HTTP 方法
 */

#include "uvhttp.h"
#include "uvhttp_response_internal.h"
#include "uvhttp_allocator.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
} app_context_t;

/* 信号处理器 */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* GET 请求处理器 */
static int get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    const char* body = "GET method response";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Method", "GET");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("GET %s - 200 OK\n", path);
    return 0;
}

/* POST 请求处理器 */
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    size_t body_len = uvhttp_request_get_body_length(request);
    const char* body = uvhttp_request_get_body(request);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body), 
             "POST method received %zu bytes: %.*s", 
             body_len, (int)body_len, body ? body : "(empty)");
    
    uvhttp_response_set_status(response, 201);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Method", "POST");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("POST %s - 201 Created\n", path);
    return 0;
}

/* PUT 请求处理器 */
static int put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    size_t body_len = uvhttp_request_get_body_length(request);
    const char* body = uvhttp_request_get_body(request);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body), 
             "PUT method updated resource: %.*s", 
             (int)body_len, body ? body : "(empty)");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Method", "PUT");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("PUT %s - 200 OK\n", path);
    return 0;
}

/* DELETE 请求处理器 */
static int delete_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    const char* body = "DELETE method - resource deleted";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Method", "DELETE");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("DELETE %s - 200 OK\n", path);
    return 0;
}

/* PATCH 请求处理器 */
static int patch_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    size_t body_len = uvhttp_request_get_body_length(request);
    const char* body = uvhttp_request_get_body(request);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body), 
             "PATCH method applied: %.*s", 
             (int)body_len, body ? body : "(empty)");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Method", "PATCH");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("PATCH %s - 200 OK\n", path);
    return 0;
}

/* HEAD 请求处理器 */
static int head_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "100");
    uvhttp_response_set_header(response, "X-Method", "HEAD");
    /* HEAD 请求不发送 body */
    uvhttp_response_send(response);
    
    printf("HEAD %s - 200 OK (no body)\n", path);
    return 0;
}

/* OPTIONS 请求处理器 */
static int options_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Allow", "GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    uvhttp_response_set_header(response, "X-Method", "OPTIONS");
    uvhttp_response_send(response);
    
    printf("OPTIONS %s - 200 OK\n", path);
    return 0;
}

/* JSON API 处理器 */
static int json_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* method_str = uvhttp_request_get_method(request);
    
    const char* json_body = "{\n"
                           "  \"status\": \"success\",\n"
                           "  \"method\": \"";
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body), 
             "%s%s\",\n"
             "  \"message\": \"JSON response\"\n"
             "}", json_body, method_str);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("JSON API - %s\n", method_str);
    return 0;
}

/* 路径参数处理器 */
static int user_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    char response_body[256];
    snprintf(response_body, sizeof(response_body), 
             "User endpoint: %s", path);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("User endpoint: %s\n", path);
    return 0;
}

/* 查询参数处理器 */
static int query_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* query = uvhttp_request_get_query_string(request);
    
    char response_body[512];
    if (query && strlen(query) > 0) {
        snprintf(response_body, sizeof(response_body), 
                 "Query parameters: %s", query);
    } else {
        snprintf(response_body, sizeof(response_body), 
                 "No query parameters");
    }
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("Query handler: %s\n", query ? query : "(none)");
    return 0;
}

/* 错误处理器 - 404 */
static int not_found_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    const char* body = "404 Not Found";
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("404 Not Found: %s\n", path);
    return 0;
}

/* 错误处理器 - 500 */
static int error_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    const char* body = "500 Internal Server Error";
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("500 Internal Server Error: %s\n", path);
    return 0;
}

/* 重定向处理器 */
static int redirect_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    
    uvhttp_response_set_status(response, 302);
    uvhttp_response_set_header(response, "Location", "/");
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Redirecting to /", strlen("Redirecting to /"));
    uvhttp_response_send(response);
    
    printf("Redirect: %s -> /\n", path);
    return 0;
}

/* 主页处理器 - 测试说明 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>HTTP Methods E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🧪 HTTP Methods End-to-End Test Server</h1>"
        "<p>测试所有 HTTP 方法的端到端测试服务器</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> / - 主页"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /api - JSON API"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">POST</span> /api - 创建资源"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">PUT</span> /api - 更新资源"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">DELETE</span> /api - 删除资源"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">PATCH</span> /api - 部分更新"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">HEAD</span> /api - 获取头信息"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">OPTIONS</span> /api - CORS 预检"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /users/:id - 路径参数"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /search - 查询参数"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /redirect - 重定向"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /error - 500 错误"
        "</div>"
        ""
        "<h2>测试命令示例：</h2>"
        "<pre>"
        "# 测试 GET\n"
        "curl http://localhost:8082/\n"
        "curl http://localhost:8082/api\n"
        ""
        "# 测试 POST\n"
        "curl -X POST -d 'test data' http://localhost:8082/api\n"
        ""
        "# 测试 PUT\n"
        "curl -X PUT -d 'updated data' http://localhost:8082/api\n"
        ""
        "# 测试 DELETE\n"
        "curl -X DELETE http://localhost:8082/api\n"
        ""
        "# 测试 PATCH\n"
        "curl -X PATCH -d 'patch data' http://localhost:8082/api\n"
        ""
        "# 测试 HEAD\n"
        "curl -I http://localhost:8082/api\n"
        ""
        "# 测试 OPTIONS\n"
        "curl -X OPTIONS http://localhost:8082/api\n"
        ""
        "# 测试路径参数\n"
        "curl http://localhost:8082/users/123\n"
        ""
        "# 测试查询参数\n"
        "curl 'http://localhost:8082/search?q=test&page=1'\n"
        ""
        "# 测试重定向\n"
        "curl -L http://localhost:8082/redirect\n"
        ""
        "# 测试错误\n"
        "curl http://localhost:8082/error\n"
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

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8082;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    
    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 创建路由器 */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 - 主页 */
    uvhttp_router_add_route(ctx->router, "/", index_handler);
    uvhttp_router_add_route(ctx->router, "/index.html", index_handler);
    
    /* 添加路由 - API 端点 */
    uvhttp_router_add_route(ctx->router, "/api", json_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_GET, get_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_POST, post_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PUT, put_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_DELETE, delete_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PATCH, patch_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_HEAD, head_handler);
    uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_OPTIONS, options_handler);
    
    /* 添加路由 - 用户端点 */
    uvhttp_router_add_route(ctx->router, "/users/:id", user_handler);
    
    /* 添加路由 - 查询参数 */
    uvhttp_router_add_route(ctx->router, "/search", query_handler);
    
    /* 添加路由 - 重定向 */
    uvhttp_router_add_route(ctx->router, "/redirect", redirect_handler);
    
    /* 添加路由 - 错误 */
    uvhttp_router_add_route(ctx->router, "/error", error_handler);
    
    /* 添加路由 - 404 处理（放在最后） */
    uvhttp_router_add_route(ctx->router, "/*", not_found_handler);
    
    /* 设置路由器到服务器 */
    ctx->server->router = ctx->router;
    
    /* 启动服务器 */
    result = uvhttp_server_listen(ctx->server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("HTTP Methods E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("========================================\n");
    printf("\n支持的 HTTP 方法:\n");
    printf("  - GET\n");
    printf("  - POST\n");
    printf("  - PUT\n");
    printf("  - DELETE\n");
    printf("  - PATCH\n");
    printf("  - HEAD\n");
    printf("  - OPTIONS\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /api (JSON API)\n");
    printf("  - /users/:id (路径参数)\n");
    printf("  - /search (查询参数)\n");
    printf("  - /redirect (重定向)\n");
    printf("  - /error (500 错误)\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    uvhttp_free(ctx);
    
    printf("\n========================================\n");
    printf("服务器已停止\n");
    printf("========================================\n");
    
    return 0;
}