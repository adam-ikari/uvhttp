/*
 * UVHTTP 编译期中间件配置示例
 *
 * 演示如何使用编译期宏配置中间件管线
 *
 * 注意：此示例使用全局变量以简化代码。
 * 在生产环境中，建议使用 libuv 数据指针模式或依赖注入来管理应用状态。
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <cJSON.h>
#include <stdio.h>
#include <string.h>

/* ============ 中间件实现 ============ */

/* 日志中间件 */
static int logging_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)resp;
    
    const char* method = uvhttp_request_get_method(req);
    const char* path = uvhttp_request_get_path(req);
    
    printf("[LOG] %s %s\n", method, path);
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 认证中间件 */
static int auth_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;

    const char* auth = uvhttp_request_get_header(req, "Authorization");

    if (!auth || strcmp(auth, "Bearer secret-token") != 0) {
        // 使用 cJSON 创建 JSON 错误响应
        cJSON* error_obj = cJSON_CreateObject();
        if (!error_obj) {
            uvhttp_response_set_status(resp, 401);
            uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_header(resp, "WWW-Authenticate", "Bearer");
            const char* error = "{\"error\":\"未授权\"}";
            uvhttp_response_set_body(resp, error, strlen(error));
            uvhttp_response_send(resp);
            return UVHTTP_MIDDLEWARE_STOP;
        }

        cJSON_AddStringToObject(error_obj, "error", "未授权");
        cJSON_AddStringToObject(error_obj, "message", "无效的认证令牌");

        char* error_string = cJSON_PrintUnformatted(error_obj);
        cJSON_Delete(error_obj);

        if (!error_string) {
            uvhttp_response_set_status(resp, 401);
            uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_header(resp, "WWW-Authenticate", "Bearer");
            const char* error = "{\"error\":\"未授权\"}";
            uvhttp_response_set_body(resp, error, strlen(error));
            uvhttp_response_send(resp);
            return UVHTTP_MIDDLEWARE_STOP;
        }

        uvhttp_response_set_status(resp, 401);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_header(resp, "WWW-Authenticate", "Bearer");
        uvhttp_response_set_body(resp, error_string, strlen(error_string));

        uvhttp_response_send(resp);
        free(error_string);
        return UVHTTP_MIDDLEWARE_STOP;
    }

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* CORS 中间件 */
static int cors_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)req;
    
    uvhttp_response_set_header(resp, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(resp, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ============ 请求处理器 ============ */

/* 公开处理器 - 只需要日志和 CORS */
static int public_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 编译期配置中间件管线 */
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        logging_middleware,
        cors_middleware
    );

    // 使用 cJSON 创建 JSON 响应
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(resp, 500);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(resp, error, strlen(error));
        return uvhttp_response_send(resp);
    }

    cJSON_AddStringToObject(json_obj, "message", "公开访问");

    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(resp, 500);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(resp, error, strlen(error));
        return uvhttp_response_send(resp);
    }

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json_string, strlen(json_string));

    int result = uvhttp_response_send(resp);
    free(json_string);

    return result;
}

/* 受保护处理器 - 需要完整的中间件管线 */
static int protected_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 编译期配置中间件管线 - 顺序固定 */
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        logging_middleware,
        auth_middleware,
        cors_middleware
    );

    // 使用 cJSON 创建 JSON 响应
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(resp, 500);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(resp, error, strlen(error));
        return uvhttp_response_send(resp);
    }

    cJSON_AddStringToObject(json_obj, "message", "访问成功");
    cJSON_AddStringToObject(json_obj, "data", "敏感信息");

    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(resp, 500);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(resp, error, strlen(error));
        return uvhttp_response_send(resp);
    }

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json_string, strlen(json_string));

    int result = uvhttp_response_send(resp);
    free(json_string);

    return result;
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
    uvhttp_router_add_route(router, "/public", public_handler);
    uvhttp_router_add_route(router, "/protected", protected_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8082);
    
    printf("服务器运行在 http://localhost:8082\n");
    printf("测试:\n");
    printf("  curl http://localhost:8082/public\n");
    printf("  curl http://localhost:8082/protected\n");
    printf("  curl -H 'Authorization: Bearer secret-token' http://localhost:8082/protected\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    uvhttp_server_free(server);
    
    return 0;
}