/*
 * UVHTTP 简化版编译宏中间件使用示例
 * 
 * 演示如何使用简化版的编译宏中间件系统
 */

#include "uvhttp.h"
#include "uvhttp_middleware_macros.h"
#include <stdio.h>
#include <string.h>

/* ==================== 自定义中间件 ==================== */

/* 认证中间件 */
static int auth_middleware(const uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* token = uvhttp_request_get_header((uvhttp_request_t*)request, "Authorization");
    
    if (!token || strncmp(token, "Bearer ", 7) != 0) {
        uvhttp_response_set_status(response, 401);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Unauthorized", 12);
        uvhttp_response_send(response);
        return UVHTTP_MIDDLEWARE_STOP;
    }
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 内容类型验证中间件 */
static int content_type_middleware(const uvhttp_request_t* request, uvhttp_response_t* response) {
    if (request->method == UVHTTP_POST || request->method == UVHTTP_PUT) {
        const char* content_type = uvhttp_request_get_header((uvhttp_request_t*)request, "Content-Type");
        
        if (!content_type || strstr(content_type, "application/json") == NULL) {
            uvhttp_response_set_status(response, 415);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "Unsupported Media Type", 23);
            uvhttp_response_send(response);
            return UVHTTP_MIDDLEWARE_STOP;
        }
    }
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ==================== 中间件链定义 ==================== */

/* API 公开端点中间件链 */
UVHTTP_MIDDLEWARE_CHAIN(api_public_chain,
    uvhttp_middleware_logging,
    uvhttp_middleware_cors,
    uvhttp_middleware_preflight
);

/* API 受保护端点中间件链 */
UVHTTP_MIDDLEWARE_CHAIN(api_protected_chain,
    uvhttp_middleware_logging,
    uvhttp_middleware_cors,
    uvhttp_middleware_preflight,
    auth_middleware,
    content_type_middleware
);

/* ==================== 路由处理器 ==================== */

/* 公开端点处理器 */
static int public_api_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 方式1：直接使用宏 */
    UVHTTP_MIDDLEWARE(req, resp,
        uvhttp_middleware_logging,
        uvhttp_middleware_cors,
        uvhttp_middleware_preflight
    );
    
    /* 处理请求 */
    const char* json = "{\"message\":\"Public API\",\"version\":\"1.0.0\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);
    
    return 0;
}

/* 受保护端点处理器 - 方式1：直接使用宏 */
static int protected_api_handler_v1(uvhttp_request_t* req, uvhttp_response_t* resp) {
    UVHTTP_MIDDLEWARE(req, resp,
        uvhttp_middleware_logging,
        uvhttp_middleware_cors,
        uvhttp_middleware_preflight,
        auth_middleware,
        content_type_middleware
    );
    
    /* 处理请求 */
    const char* json = "{\"message\":\"Protected API\",\"data\":\"secret\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);
    
    return 0;
}

/* 受保护端点处理器 - 方式2：使用预定义的链 */
static int protected_api_handler_v2(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 使用预定义的中间件链 */
    for (size_t i = 0; i < api_protected_chain_count; i++) {
        if (api_protected_chain[i](req, resp) != UVHTTP_MIDDLEWARE_CONTINUE) {
            return 0;
        }
    }
    
    /* 处理请求 */
    const char* json = "{\"message\":\"Protected API v2\",\"data\":\"secret\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);
    
    return 0;
}

/* 简单处理器 - 无中间件 */
static int simple_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html><head><title>简单中间件示例</title></head>"
        "<body>"
        "<h1>简化版编译宏中间件系统</h1>"
        "<h2>特性：</h2>"
        "<ul>"
        "<li>✅ 零动态分配</li>"
        "<li>✅ 零运行时开销</li>"
        "<li>✅ 静态配置</li>"
        "<li>✅ 类型安全</li>"
        "<li>✅ 易于使用</li>"
        "</ul>"
        "<h2>端点：</h2>"
        "<ul>"
        "<li><a href=\"/api/public\">/api/public</a> - 公开端点</li>"
        "<li><a href=\"/api/protected\">/api/protected</a> - 受保护端点（需要认证）</li>"
        "</ul>"
        "</body></html>";
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/html");
    uvhttp_response_set_body(resp, html, strlen(html));
    uvhttp_response_send(resp);
    
    return 0;
}

/* ==================== 主函数 ==================== */

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", simple_handler);
    uvhttp_router_add_route(router, "/api/public", public_api_handler);
    uvhttp_router_add_route(router, "/api/protected", protected_api_handler_v1);
    
    /* 启动服务器 */
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    printf("服务器已启动: http://0.0.0.0:8080\n");
    printf("\n测试方法：\n");
    printf("1. 访问 /api/public - 公开端点\n");
    printf("2. 访问 /api/protected - 受保护端点（需要认证）\n");
    printf("3. 使用 curl 测试：\n");
    printf("   curl http://localhost:8080/api/public\n");
    printf("   curl -H \"Authorization: Bearer token\" http://localhost:8080/api/protected\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_server_free(server);
    
    return 0;
}