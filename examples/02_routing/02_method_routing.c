/**
 * @file 02_method_routing.c
 * @brief HTTP 方法路由示例
 * 
 * 展示如何为同一个路径设置不同 HTTP 方法的处理器
 * 
 * 学习目标：
 * 1. 理解 HTTP 方法（GET、POST、PUT、DELETE）
 * 2. 如何为同一个路径设置多个处理方法
 * 3. 如何获取请求体和发送不同的响应
 * 
 * 运行方法：
 * gcc -o method_routing 02_method_routing.c -I../../include -L../../build -luvhttp -luv -lpthread
 * ./method_routing
 * 
 * 测试：
 * curl http://localhost:8080/resource
 * curl -X POST http://localhost:8080/resource -d '{"name":"test"}'
 * curl -X PUT http://localhost:8080/resource -d '{"name":"updated"}'
 * curl -X DELETE http://localhost:8080/resource
 */

#include "../../include/uvhttp.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/**
 * @brief 应用上下文结构
 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    int is_running;
} app_context_t;

static app_context_t* g_ctx = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_ctx && g_ctx->server) {
        uvhttp_server_stop(g_ctx->server);
        uvhttp_server_free(g_ctx->server);
        g_ctx->server = NULL;
    }
    exit(0);
}

/**
 * @brief GET 请求处理器 - 获取资源
 */
int get_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用的参数
    printf("处理 GET 请求\n");

    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "method", "GET");
    cJSON_AddStringToObject(json_obj, "action", "获取资源");
    cJSON_AddStringToObject(json_obj, "status", "success");

    // 创建嵌套的 data 对象
    cJSON* data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "id", 1);
    cJSON_AddStringToObject(data, "name", "示例资源");
    cJSON_AddStringToObject(data, "description", "这是一个示例资源");
    cJSON_AddItemToObject(json_obj, "data", data);

    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief POST 请求处理器 - 创建资源
 */
int post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    printf("处理 POST 请求\n");

    // 获取请求体
    const char* body = uvhttp_request_get_body(req);

    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "method", "POST");
    cJSON_AddStringToObject(json_obj, "action", "创建资源");

    if (body && strlen(body) > 0) {
        printf("  请求体: %s\n", body);
        cJSON_AddStringToObject(json_obj, "status", "success");
        cJSON_AddStringToObject(json_obj, "message", "资源创建成功");
        cJSON_AddStringToObject(json_obj, "received", body);
        cJSON_AddNumberToObject(json_obj, "created_id", 123);
    } else {
        printf("  请求体为空\n");
        cJSON_AddStringToObject(json_obj, "status", "error");
        cJSON_AddStringToObject(json_obj, "message", "请求体为空");
    }

    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief PUT 请求处理器 - 更新资源
 */
int put_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    printf("处理 PUT 请求\n");

    // 获取请求体
    const char* body = uvhttp_request_get_body(req);

    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "method", "PUT");
    cJSON_AddStringToObject(json_obj, "action", "更新资源");

    if (body && strlen(body) > 0) {
        printf("  请求体: %s\n", body);
        cJSON_AddStringToObject(json_obj, "status", "success");
        cJSON_AddStringToObject(json_obj, "message", "资源更新成功");
        cJSON_AddStringToObject(json_obj, "received", body);
        cJSON_AddNumberToObject(json_obj, "updated_id", 1);
    } else {
        printf("  请求体为空\n");
        cJSON_AddStringToObject(json_obj, "status", "error");
        cJSON_AddStringToObject(json_obj, "message", "请求体为空");
    }

    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief DELETE 请求处理器 - 删除资源
 */
int delete_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用的参数
    printf("处理 DELETE 请求\n");

    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "method", "DELETE");
    cJSON_AddStringToObject(json_obj, "action", "删除资源");
    cJSON_AddStringToObject(json_obj, "status", "success");
    cJSON_AddStringToObject(json_obj, "message", "资源删除成功");
    cJSON_AddNumberToObject(json_obj, "deleted_id", 1);

    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief 不支持的方法处理器
 */
int method_not_allowed_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);

    printf("不支持的方法: %s\n", method);

    // 使用 cJSON 创建 JSON 对象
    cJSON* json_obj = cJSON_CreateObject();
    if (!json_obj) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to create JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    cJSON_AddStringToObject(json_obj, "error", "Method Not Allowed");

    char message[256];
    snprintf(message, sizeof(message), "不支持的 HTTP 方法: %s", method ? method : "unknown");
    cJSON_AddStringToObject(json_obj, "message", message);

    // 创建允许的方法数组
    cJSON* allowed = cJSON_CreateArray();
    cJSON_AddItemToArray(allowed, cJSON_CreateString("GET"));
    cJSON_AddItemToArray(allowed, cJSON_CreateString("POST"));
    cJSON_AddItemToArray(allowed, cJSON_CreateString("PUT"));
    cJSON_AddItemToArray(allowed, cJSON_CreateString("DELETE"));
    cJSON_AddItemToObject(json_obj, "allowed_methods", allowed);

    // 生成 JSON 字符串
    char* json_string = cJSON_PrintUnformatted(json_obj);
    cJSON_Delete(json_obj);

    if (!json_string) {
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        const char* error = "{\"error\":\"Failed to generate JSON\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }

    uvhttp_response_set_status(res, 405);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));

    int result = uvhttp_response_send(res);
    free(json_string);

    return result;
}

/**
 * @brief 统一的路由处理器 - 根据方法分发
 */
int resource_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    
    // 根据HTTP方法分发到不同的处理器
    if (strcmp(method, "GET") == 0) {
        return get_handler(req, res);
    } else if (strcmp(method, "POST") == 0) {
        return post_handler(req, res);
    } else if (strcmp(method, "PUT") == 0) {
        return put_handler(req, res);
    } else if (strcmp(method, "DELETE") == 0) {
        return delete_handler(req, res);
    } else {
        return method_not_allowed_handler(req, res);
    }
}

int main() {
    printf("========================================\n");
    printf("  UVHTTP HTTP 方法路由示例\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建应用上下文
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "错误: 无法分配内存\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    g_ctx = ctx;
    
    // 创建服务器
    ctx->loop = uv_default_loop();
    uvhttp_error_t server_result = uvhttp_server_new(ctx->loop, &ctx->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 添加路由（所有方法都指向同一个处理器，在处理器内部分发）
    printf("添加路由...\n");
    uvhttp_router_add_route(router, "/resource", resource_handler);
    printf("  ✓ /resource (支持 GET, POST, PUT, DELETE)\n");
    printf("\n");
    
    // 设置路由器
    uvhttp_server_set_router(ctx->server, router);
    
    // 启动服务器
    int listen_result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (listen_result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", listen_result);
        uvhttp_server_free(ctx->server);
        free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("  服务器运行在 http://localhost:8080\n");
    printf("========================================\n\n");
    
    printf("测试命令：\n");
    printf("  # GET 请求 - 获取资源\n");
    printf("  curl http://localhost:8080/resource\n\n");

    printf("  # POST 请求 - 创建资源\n");
    printf("  curl -X POST http://localhost:8080/resource -d '{\"name\":\"test\"}'\n\n");

    printf("  # PUT 请求 - 更新资源\n");
    printf("  curl -X PUT http://localhost:8080/resource -d '{\"name\":\"updated\"}'\n\n");

    printf("  # DELETE 请求 - 删除资源\n");
    printf("  curl -X DELETE http://localhost:8080/resource\n\n");
    
    printf("按 Ctrl+C 停止服务器\n\n");
    
    // 运行事件循环
    uv_run(ctx->loop, UV_RUN_DEFAULT);
    
    // 清理
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    free(ctx);
    
    return 0;
}