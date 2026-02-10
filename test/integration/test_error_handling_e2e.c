/*
 * 错误处理端到端测试
 * 测试各种错误场景和错误处理机制
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
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

/* 400 Bad Request 处理器 */
static int bad_request_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Bad Request\", \"message\": \"Invalid request parameters\"}";
    uvhttp_response_set_status(response, 400);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("400 Bad Request\n");
    return 0;
}

/* 401 Unauthorized 处理器 */
static int unauthorized_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Unauthorized\", \"message\": \"Authentication required\"}";
    uvhttp_response_set_status(response, 401);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "WWW-Authenticate", "Bearer");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("401 Unauthorized\n");
    return 0;
}

/* 403 Forbidden 处理器 */
static int forbidden_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Forbidden\", \"message\": \"Access denied\"}";
    uvhttp_response_set_status(response, 403);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("403 Forbidden\n");
    return 0;
}

/* 404 Not Found 处理器 */
static int not_found_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* body = "{\"error\": \"Not Found\", \"message\": \"Resource not found\"}";
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("404 Not Found: %s\n", uvhttp_request_get_path(request));
    return 0;
}

/* 405 Method Not Allowed 处理器 */
static int method_not_allowed_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Method Not Allowed\", \"message\": \"HTTP method not supported\"}";
    uvhttp_response_set_status(response, 405);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Allow", "GET, POST");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("405 Method Not Allowed\n");
    return 0;
}

/* 409 Conflict 处理器 */
static int conflict_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Conflict\", \"message\": \"Resource already exists\"}";
    uvhttp_response_set_status(response, 409);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("409 Conflict\n");
    return 0;
}

/* 413 Payload Too Large 处理器 */
static int payload_too_large_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    size_t body_len = uvhttp_request_get_body_length(request);
    
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "{\"error\": \"Payload Too Large\", \"message\": \"Request body exceeds limit\", \"size\": %zu}",
             body_len);
    
    uvhttp_response_set_status(response, 413);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    printf("413 Payload Too Large: %zu bytes\n", body_len);
    return 0;
}

/* 415 Unsupported Media Type 处理器 */
static int unsupported_media_type_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Unsupported Media Type\", \"message\": \"Content-Type not supported\"}";
    uvhttp_response_set_status(response, 415);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("415 Unsupported Media Type\n");
    return 0;
}

/* 429 Too Many Requests 处理器 */
static int too_many_requests_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Too Many Requests\", \"message\": \"Rate limit exceeded\"}";
    uvhttp_response_set_status(response, 429);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Retry-After", "60");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("429 Too Many Requests\n");
    return 0;
}

/* 500 Internal Server Error 处理器 */
static int internal_error_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Internal Server Error\", \"message\": \"Server encountered an error\"}";
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("500 Internal Server Error\n");
    return 0;
}

/* 501 Not Implemented 处理器 */
static int not_implemented_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Not Implemented\", \"message\": \"Feature not implemented\"}";
    uvhttp_response_set_status(response, 501);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("501 Not Implemented\n");
    return 0;
}

/* 503 Service Unavailable 处理器 */
static int service_unavailable_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Service Unavailable\", \"message\": \"Server is temporarily unavailable\"}";
    uvhttp_response_set_status(response, 503);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Retry-After", "30");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("503 Service Unavailable\n");
    return 0;
}

/* 504 Gateway Timeout 处理器 */
static int gateway_timeout_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "{\"error\": \"Gateway Timeout\", \"message\": \"Upstream server timeout\"}";
    uvhttp_response_set_status(response, 504);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("504 Gateway Timeout\n");
    return 0;
}

/* 测试所有错误码的处理器 */
static int test_all_errors_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Error Codes Test</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".error { margin: 10px 0; padding: 10px; border-radius: 5px; }"
        ".error-4xx { background: #fff3cd; border-left: 4px solid #ffc107; }"
        ".error-5xx { background: #f8d7da; border-left: 4px solid #dc3545; }"
        "a { color: #0066cc; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🚨 Error Codes Test</h1>"
        "<p>测试所有 HTTP 错误码</p>"
        ""
        "<h2>4xx Client Errors</h2>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>400 Bad Request</strong> - "
        "<a href=\"/400\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>401 Unauthorized</strong> - "
        "<a href=\"/401\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>403 Forbidden</strong> - "
        "<a href=\"/403\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>404 Not Found</strong> - "
        "<a href=\"/404\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>405 Method Not Allowed</strong> - "
        "<a href=\"/405\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>409 Conflict</strong> - "
        "<a href=\"/409\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>413 Payload Too Large</strong> - "
        "<a href=\"/413\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>415 Unsupported Media Type</strong> - "
        "<a href=\"/415\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-4xx\">"
        "<strong>429 Too Many Requests</strong> - "
        "<a href=\"/429\">Test</a>"
        "</div>"
        ""
        "<h2>5xx Server Errors</h2>"
        ""
        "<div class=\"error error-5xx\">"
        "<strong>500 Internal Server Error</strong> - "
        "<a href=\"/500\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-5xx\">"
        "<strong>501 Not Implemented</strong> - "
        "<a href=\"/501\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-5xx\">"
        "<strong>503 Service Unavailable</strong> - "
        "<a href=\"/503\">Test</a>"
        "</div>"
        ""
        "<div class=\"error error-5xx\">"
        "<strong>504 Gateway Timeout</strong> - "
        "<a href=\"/504\">Test</a>"
        "</div>"
        ""
        "<h2>测试命令示例：</h2>"
        "<pre>"
        "# 测试 400 Bad Request\n"
        "curl http://localhost:8085/400\n"
        ""
        "# 测试 401 Unauthorized\n"
        "curl http://localhost:8085/401\n"
        ""
        "# 测试 403 Forbidden\n"
        "curl http://localhost:8085/403\n"
        ""
        "# 测试 404 Not Found\n"
        "curl http://localhost:8085/404\n"
        ""
        "# 测试 429 Too Many Requests\n"
        "curl http://localhost:8085/429\n"
        ""
        "# 测试 500 Internal Server Error\n"
        "curl http://localhost:8085/500\n"
        ""
        "# 测试 503 Service Unavailable\n"
        "curl http://localhost:8085/503\n"
        "</pre>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    printf("Error codes test page accessed\n");
    return 0;
}

/* 主页处理器 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Error Handling E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🚨 Error Handling End-to-End Test Server</h1>"
        "<p>测试各种错误场景和错误处理机制</p>"
        ""
        "<h2>测试端点：</h2>"
        "<ul>"
        "<li><a href=\"/errors\">/errors</a> - 查看所有错误码测试</li>"
        "<li><a href=\"/400\">/400</a> - 400 Bad Request</li>"
        "<li><a href=\"/401\">/401</a> - 401 Unauthorized</li>"
        "<li><a href=\"/403\">/403</a> - 403 Forbidden</li>"
        "<li><a href=\"/404\">/404</a> - 404 Not Found</li>"
        "<li><a href=\"/405\">/405</a> - 405 Method Not Allowed</li>"
        "<li><a href=\"/409\">/409</a> - 409 Conflict</li>"
        "<li><a href=\"/413\">/413</a> - 413 Payload Too Large</li>"
        "<li><a href=\"/415\">/415</a> - 415 Unsupported Media Type</li>"
        "<li><a href=\"/429\">/429</a> - 429 Too Many Requests</li>"
        "<li><a href=\"/500\">/500</a> - 500 Internal Server Error</li>"
        "<li><a href=\"/501\">/501</a> - 501 Not Implemented</li>"
        "<li><a href=\"/503\">/503</a> - 503 Service Unavailable</li>"
        "<li><a href=\"/504\">/504</a> - 504 Gateway Timeout</li>"
        "</ul>"
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
    int port = 8085;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
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
    
    /* 设置服务器用户数据 */
    ctx->server->user_data = ctx;
    
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
    
    /* 添加路由 - 错误码测试页面 */
    uvhttp_router_add_route(ctx->router, "/errors", test_all_errors_handler);
    
    /* 添加路由 - 4xx 错误 */
    uvhttp_router_add_route(ctx->router, "/400", bad_request_handler);
    uvhttp_router_add_route(ctx->router, "/401", unauthorized_handler);
    uvhttp_router_add_route(ctx->router, "/403", forbidden_handler);
    uvhttp_router_add_route(ctx->router, "/404", not_found_handler);
    uvhttp_router_add_route(ctx->router, "/405", method_not_allowed_handler);
    uvhttp_router_add_route(ctx->router, "/409", conflict_handler);
    uvhttp_router_add_route(ctx->router, "/413", payload_too_large_handler);
    uvhttp_router_add_route(ctx->router, "/415", unsupported_media_type_handler);
    uvhttp_router_add_route(ctx->router, "/429", too_many_requests_handler);
    
    /* 添加路由 - 5xx 错误 */
    uvhttp_router_add_route(ctx->router, "/500", internal_error_handler);
    uvhttp_router_add_route(ctx->router, "/501", not_implemented_handler);
    uvhttp_router_add_route(ctx->router, "/503", service_unavailable_handler);
    uvhttp_router_add_route(ctx->router, "/504", gateway_timeout_handler);
    
    /* 添加路由 - 404 处理（放在最后） */
    uvhttp_router_add_route(ctx->router, "/*", not_found_handler);
    
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
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("Error Handling E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("========================================\n");
    printf("\n测试功能:\n");
    printf("  - 4xx 客户端错误\n");
    printf("  - 5xx 服务器错误\n");
    printf("  - 错误响应格式\n");
    printf("  - 错误头信息\n");
    printf("  - 404 处理\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /errors (所有错误码)\n");
    printf("  - /400, /401, /403, /404, /405, /409\n");
    printf("  - /413, /415, /429\n");
    printf("  - /500, /501, /503, /504\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx) {
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