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
    printf("处理 GET 请求\n");
    
    const char* json = "{\n"
        "  \"method\": \"GET\",\n"
        "  \"action\": \"获取资源\",\n"
        "  \"status\": \"success\",\n"
        "  \"data\": {\n"
        "    \"id\": 1,\n"
        "    \"name\": \"示例资源\",\n"
        "    \"description\": \"这是一个示例资源\"\n"
        "  }\n"
        "}\n";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

/**
 * @brief POST 请求处理器 - 创建资源
 */
int post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    printf("处理 POST 请求\n");
    
    // 获取请求体
    const char* body = uvhttp_request_get_body(req);
    
    char response[1024];
    if (body && strlen(body) > 0) {
        printf("  请求体: %s\n", body);
        snprintf(response, sizeof(response),
            "{\n"
            "  \"method\": \"POST\",\n"
            "  \"action\": \"创建资源\",\n"
            "  \"status\": \"success\",\n"
            "  \"message\": \"资源创建成功\",\n"
            "  \"received\": \"%s\",\n"
            "  \"created_id\": 123\n"
            "}", body);
    } else {
        printf("  请求体为空\n");
        snprintf(response, sizeof(response),
            "{\n"
            "  \"method\": \"POST\",\n"
            "  \"action\": \"创建资源\",\n"
            "  \"status\": \"error\",\n"
            "  \"message\": \"请求体为空\"\n"
            "}");
    }
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

/**
 * @brief PUT 请求处理器 - 更新资源
 */
int put_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    printf("处理 PUT 请求\n");
    
    // 获取请求体
    const char* body = uvhttp_request_get_body(req);
    
    char response[1024];
    if (body && strlen(body) > 0) {
        printf("  请求体: %s\n", body);
        snprintf(response, sizeof(response),
            "{\n"
            "  \"method\": \"PUT\",\n"
            "  \"action\": \"更新资源\",\n"
            "  \"status\": \"success\",\n"
            "  \"message\": \"资源更新成功\",\n"
            "  \"received\": \"%s\",\n"
            "  \"updated_id\": 1\n"
            "}", body);
    } else {
        printf("  请求体为空\n");
        snprintf(response, sizeof(response),
            "{\n"
            "  \"method\": \"PUT\",\n"
            "  \"action\": \"更新资源\",\n"
            "  \"status\": \"error\",\n"
            "  \"message\": \"请求体为空\"\n"
            "}");
    }
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

/**
 * @brief DELETE 请求处理器 - 删除资源
 */
int delete_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    printf("处理 DELETE 请求\n");
    
    const char* json = "{\n"
        "  \"method\": \"DELETE\",\n"
        "  \"action\": \"删除资源\",\n"
        "  \"status\": \"success\",\n"
        "  \"message\": \"资源删除成功\",\n"
        "  \"deleted_id\": 1\n"
        "}\n";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

/**
 * @brief 不支持的方法处理器
 */
int method_not_allowed_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    
    printf("不支持的方法: %s\n", method);
    
    char response[256];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"error\": \"Method Not Allowed\",\n"
        "  \"message\": \"不支持的 HTTP 方法: %s\",\n"
        "  \"allowed_methods\": [\"GET\", \"POST\", \"PUT\", \"DELETE\"]\n"
        "}", method);
    
    uvhttp_response_set_status(res, 405);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_header(res, "Allow", "GET, POST, PUT, DELETE");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
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
    ctx->server = uvhttp_server_new(ctx->loop);
    
    if (!ctx->server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        free(ctx);
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加路由（所有方法都指向同一个处理器，在处理器内部分发）
    printf("添加路由...\n");
    uvhttp_router_add_route(router, "/resource", resource_handler);
    printf("  ✓ /resource (支持 GET, POST, PUT, DELETE)\n");
    printf("\n");
    
    // 设置路由器
    uvhttp_server_set_router(ctx->server, router);
    
    // 启动服务器
    int result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
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
    printf("  curl -X POST http://localhost:8080/resource -d '{"name":"test"}'\n\n");
    
    printf("  # PUT 请求 - 更新资源\n");
    printf("  curl -X PUT http://localhost:8080/resource -d '{"name":"updated"}'\n\n");
    
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