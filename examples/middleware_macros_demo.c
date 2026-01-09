/*
 * UVHTTP 编译宏中间件使用示例
 * 演示如何使用优化后的编译宏中间件系统
 * 
 * 优化特性：
 * 1. 使用 static const，只分配一次
 * 2. 支持上下文（与动态系统兼容）
 * 3. 使用 do-while + continue，避免宏中的 return 问题
 * 4. 统一函数签名，中间件可在两系统中复用
 */

#include "uvhttp.h"
#include "uvhttp_middleware_macros.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* 日志中间件 - 演示上下文使用 */
static int logging_middleware(const uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    /* 使用上下文存储开始时间（使用栈分配，避免内存泄漏）*/
    uint64_t start_time = uv_hrtime();  // libuv 提供的高精度时间
    ctx->data = (void*)(intptr_t)start_time;  // 存储为指针值
    ctx->cleanup = NULL;  // 无需清理
    
    printf("[LOG] %s %s\n", uvhttp_method_to_string(req->method), req->url);
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 认证中间件 - 演示上下文数据共享 */
static int auth_middleware(const uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    const char* token = uvhttp_request_get_header((uvhttp_request_t*)req, "Authorization");
    if (!token || strncmp(token, "Bearer ", 7) != 0) {
        uvhttp_response_set_status(resp, 401);
        uvhttp_response_set_header(resp, "Content-Type", "text/plain");
        uvhttp_response_set_body(resp, "Unauthorized", 12);
        uvhttp_response_send(resp);
        return UVHTTP_MIDDLEWARE_STOP;
    }
    
    /* 将用户信息存储在上下文中，供后续中间件使用 */
    ctx->data = (void*)(intptr_t)12345;  // 模拟用户ID
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* CORS 中间件 */
static int cors_middleware(const uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    uvhttp_response_set_header(resp, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    if (req->method == UVHTTP_METHOD_OPTIONS) {
        uvhttp_response_set_status(resp, 200);
        uvhttp_response_send(resp);
        return UVHTTP_MIDDLEWARE_STOP;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 清理中间件 - 演示上下文清理 */
static int cleanup_middleware(const uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    /* 执行上下文清理 */
    if (ctx->cleanup) {
        ctx->cleanup(ctx->data);
        ctx->data = NULL;
        ctx->cleanup = NULL;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 根路径处理函数 */
static int root_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html><head><title>编译宏中间件示例</title></head>"
        "<body>"
        "<h1>编译宏中间件系统（优化版）</h1>"
        "<ul>"
        "<li>✅ 使用 static const，零运行时分配</li>"
        "<li>✅ 支持上下文，与动态系统兼容</li>"
        "<li>✅ 使用 do-while + continue，避免宏中的 return 问题</li>"
        "<li>✅ 统一函数签名，中间件可复用</li>"
        "</ul>"
        "</body></html>";
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/html");
    uvhttp_response_set_body(resp, html, strlen(html));
    uvhttp_response_send(resp);
    
    return 0;
}

/* API 处理器 - 使用编译宏中间件 */
static int api_handler_with_middleware(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 执行中间件链 - 使用优化的宏 */
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        logging_middleware,
        auth_middleware,
        cors_middleware
    );
    
    /* 处理请求 */
    const char* json = "{\"message\": \"API response\", \"status\": \"success\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);
    
    return 0;
}

/* 定义中间件链 - 演示 UVHTTP_DEFINE_MIDDLEWARE_CHAIN 宏 */
UVHTTP_DEFINE_MIDDLEWARE_CHAIN(admin_chain,
    logging_middleware,
    auth_middleware,
    cors_middleware
);

/* 管理员处理器 - 使用预定义的中间件链 */
static int admin_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 使用预定义的中间件链 */
    for (size_t i = 0; i < admin_chain_count; i++) {
        uvhttp_middleware_context_t ctx = {0};
        if (admin_chain_handlers[i] && admin_chain_handlers[i](req, resp, &ctx) != UVHTTP_MIDDLEWARE_CONTINUE) {
            return 0;
        }
    }
    
    /* 处理请求 */
    const char* json = "{\"message\": \"Admin access granted\"}";
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_response_set_body(resp, json, strlen(json));
    uvhttp_response_send(resp);
    
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", root_handler);
    uvhttp_router_add_route(router, "/api", api_handler_with_middleware);
    uvhttp_router_add_route(router, "/admin", admin_handler);
    
    /* 启动服务器 */
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    printf("服务器已启动: http://0.0.0.0:8080\n");
    printf("测试端点:\n");
    printf("  - http://0.0.0.0:8080/\n");
    printf("  - http://0.0.0.0:8080/api\n");
    printf("  - http://0.0.0.0:8080/admin\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
