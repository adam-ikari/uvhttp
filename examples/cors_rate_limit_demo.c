/*
 * UVHTTP CORS 和限流功能使用示例
 * 演示如何在实际应用中使用 CORS 中间件和服务器级别的限流功能
 */

#include "uvhttp.h"
#include "uvhttp_cors_middleware.h"
#include "uvhttp_simple_middleware.h"
#include <stdio.h>
#include <string.h>

/* ==================== 路由处理器 ==================== */

/* 根路径处理器 */
static int root_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    const char* html =
        "<!DOCTYPE html>"
        "<html><head><title>CORS 和限流功能示例</title></head>"
        "<body>"
        "<h1>CORS 和限流功能示例</h1>"
        "<h2>测试端点：</h2>"
        "<ul>"
        "<li><a href=\"/api/data\">/api/data</a> - API 数据端点</li>"
        "<li><a href=\"/api/admin\">/api/admin</a> - 管理员端点（严格限流）</li>"
        "<li><a href=\"/api/public\">/api/public</a> - 公开端点（宽松限流）</li>"
        "</ul>"
        "<h2>服务器限流配置：</h2>"
        "<ul>"
        "<li>默认限制：100 请求/60 秒</li>"
        "<li>白名单 IP：127.0.0.1（不受限流影响）</li>"
        "</ul>"
        "<h2>测试方法：</h2>"
        "<ul>"
        "<li>使用浏览器直接访问（测试 CORS）</li>"
        "<li>使用 curl 快速发送多个请求（测试限流）</li>"
        "</ul>"
        "</body></html>";

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/html");
    uvhttp_response_set_body(resp, html, strlen(html));
    uvhttp_response_send(resp);

    return 0;
}

/* API 数据处理器 */
static int api_data_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 使用编译宏中间件系统 - CORS 中间件 */
    UVHTTP_MIDDLEWARE(req, resp,
        uvhttp_cors_middleware_simple
    );

    /* 返回 API 数据 */
    const char* json = "{\"message\":\"Hello from API\",\"data\":{\"id\":1,\"name\":\"example\"}}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);

    return 0;
}

/* 管理员处理器（严格限流） */
static int api_admin_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 使用编译宏中间件系统 - CORS 中间件 */
    UVHTTP_MIDDLEWARE(req, resp,
        uvhttp_cors_middleware_simple
    );

    /* 返回管理员数据 */
    const char* json = "{\"message\":\"Admin access\",\"users\":[{\"id\":1,\"role\":\"admin\"}]}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);

    return 0;
}

/* 公开端点处理器（宽松限流） */
static int api_public_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 使用编译宏中间件系统 - CORS 中间件 */
    UVHTTP_MIDDLEWARE(req, resp,
        uvhttp_cors_middleware_simple
    );

    /* 返回公开数据 */
    const char* json = "{\"message\":\"Public access\",\"version\":\"1.0.0\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);

    return 0;
}

/* ==================== 主函数 ==================== */

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    /* 启用服务器级别的限流功能 */
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(
        server,
        100,  // 最大请求数
        60    // 时间窗口（秒）
    );

    if (result != UVHTTP_OK) {
        fprintf(stderr, "启用限流失败: %s\n", uvhttp_error_string(result));
        return 1;
    }

    /* 添加白名单 IP（本地访问不受限流影响） */
    uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");

    /* 添加路由 */
    uvhttp_router_add_route(router, "/", root_handler);
    uvhttp_router_add_route(router, "/api/data", api_data_handler);
    uvhttp_router_add_route(router, "/api/admin", api_admin_handler);
    uvhttp_router_add_route(router, "/api/public", api_public_handler);

    /* 启动服务器 */
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    printf("========================================\n");
    printf("CORS 和限流功能示例服务器\n");
    printf("========================================\n");
    printf("服务器已启动: http://0.0.0.0:8080\n\n");
    printf("限流配置:\n");
    printf("  - 最大请求数: 100 请求/60 秒\n");
    printf("  - 白名单 IP: 127.0.0.1\n\n");
    printf("测试端点:\n");
    printf("  - http://0.0.0.0:8080/\n");
    printf("  - http://0.0.0.0:8080/api/data\n");
    printf("  - http://0.0.0.0:8080/api/admin\n");
    printf("  - http://0.0.0.0:8080/api/public\n\n");
    printf("测试 CORS:\n");
    printf("  curl -H \"Origin: http://example.com\" http://localhost:8080/api/data\n\n");
    printf("测试限流（从非白名单 IP）:\n");
    printf("  for i in {1..150}; do curl http://localhost:8080/api/data; done\n");
    printf("  注意：从 127.0.0.1 访问不会触发限流\n");
    printf("========================================\n");

    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);

    return 0;
}