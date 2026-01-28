/*
 * UVHTTP 中间件链复用示例
 * 
 * 演示如何使用 UVHTTP_DEFINE_MIDDLEWARE_CHAIN 和 UVHTTP_EXECUTE_MIDDLEWARE_CHAIN
 * 在多个处理器中复用中间件链
 * 
 * 注意：此示例使用全局变量以简化代码。
 * 在生产环境中，建议使用 libuv 数据指针模式或依赖注入来管理应用状态。
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <stdio.h>
#include <string.h>

/* ============ 中间件实现 ============ */

static int logging_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)resp;
    
    const char* method = uvhttp_request_get_method(req);
    const char* path = uvhttp_request_get_path(req);
    
    printf("[LOG] %s %s\n", method, path);
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int auth_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    
    const char* auth = uvhttp_request_get_header(req, "Authorization");
    
    if (!auth || strcmp(auth, "Bearer secret-token") != 0) {
        const char* error = "{\"error\":\"未授权\",\"message\":\"无效的认证令牌\"}";
        
        uvhttp_response_set_status(resp, 401);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_header(resp, "WWW-Authenticate", "Bearer");
        uvhttp_response_set_body(resp, error, strlen(error));
        
        uvhttp_response_send(resp);
        return UVHTTP_MIDDLEWARE_STOP;
    }
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int cors_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)req;
    
    uvhttp_response_set_header(resp, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ============ 定义中间件链 ============ */

/* API 通用中间件链：日志 + 认证 + CORS */
UVHTTP_DEFINE_MIDDLEWARE_CHAIN(api_chain,
    logging_middleware,
    auth_middleware,
    cors_middleware
);

/* 公开 API 中间件链：日志 + CORS */
UVHTTP_DEFINE_MIDDLEWARE_CHAIN(public_chain,
    logging_middleware,
    cors_middleware
);

/* ============ 请求处理器 ============ */

/* 用户 API - 使用 api_chain */
static int user_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 执行预定义的中间件链 */
    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, api_chain);
    
    const char* json = "{\"message\":\"用户数据\"}";
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json, strlen(json));
    
    return uvhttp_response_send(resp);
}

/* 管理员 API - 使用 api_chain */
static int admin_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 执行预定义的中间件链 */
    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, api_chain);
    
    const char* json = "{\"message\":\"管理员数据\"}";
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json, strlen(json));
    
    return uvhttp_response_send(resp);
}

/* 健康检查 - 使用 public_chain */
static int health_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 执行预定义的中间件链 */
    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, public_chain);
    
    const char* json = "{\"status\":\"ok\"}";
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json, strlen(json));
    
    return uvhttp_response_send(resp);
}

/* ============ 主函数 ============ */

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    
    uvhttp_error_t err;
    
    /* 创建服务器 */
    err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    /* 创建路由 */
    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/api/user", user_handler);
    uvhttp_router_add_route(router, "/api/admin", admin_handler);
    uvhttp_router_add_route(router, "/health", health_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8083);
    
    printf("服务器运行在 http://localhost:8083\n");
    printf("测试:\n");
    printf("  curl http://localhost:8083/health\n");
    printf("  curl http://localhost:8083/api/user\n");
    printf("  curl -H 'Authorization: Bearer secret-token' http://localhost:8083/api/user\n");
    printf("  curl -H 'Authorization: Bearer secret-token' http://localhost:8083/api/admin\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    uvhttp_server_free(server);
    
    return 0;
}