/*
 * UVHTTP 限流中间件示例
 *
 * 演示如何由应用层实现限流功能
 *
 * 注意：此示例使用全局变量以简化代码。
 * 在生产环境中，建议使用 libuv 数据指针模式或依赖注入来管理应用状态。
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <cJSON.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ============ 限流中间件实现 ============ */

/* 令牌桶限流器 */
typedef struct {
    int max_tokens;           /* 最大令牌数 */
    int current_tokens;       /* 当前令牌数 */
    int refill_rate;          /* 每秒补充令牌数 */
    uint64_t last_refill_time; /* 上次补充时间 */
} rate_limiter_t;

/* 全局限流器（简化示例） */
static rate_limiter_t* g_rate_limiter = NULL;

/* 创建限流器 */
static rate_limiter_t* rate_limiter_create(int max_tokens, int refill_rate) {
    rate_limiter_t* limiter = (rate_limiter_t*)malloc(sizeof(rate_limiter_t));
    if (!limiter) return NULL;
    
    limiter->max_tokens = max_tokens;
    limiter->current_tokens = max_tokens;
    limiter->refill_rate = refill_rate;
    limiter->last_refill_time = (uint64_t)time(NULL);
    
    return limiter;
}

/* 销毁限流器 */
static void rate_limiter_destroy(rate_limiter_t* limiter) {
    if (limiter) {
        free(limiter);
    }
}

/* 检查并消耗令牌 */
static int rate_limiter_try_consume(rate_limiter_t* limiter) {
    uint64_t now = (uint64_t)time(NULL);
    uint64_t elapsed = now - limiter->last_refill_time;
    
    /* 补充令牌 */
    if (elapsed > 0) {
        int tokens_to_add = (int)(elapsed * limiter->refill_rate);
        limiter->current_tokens += tokens_to_add;
        if (limiter->current_tokens > limiter->max_tokens) {
            limiter->current_tokens = limiter->max_tokens;
        }
        limiter->last_refill_time = now;
    }
    
    /* 检查是否有令牌 */
    if (limiter->current_tokens > 0) {
        limiter->current_tokens--;
        return 1; /* 成功 */
    }
    
    return 0; /* 失败 */
}

/* 限流中间件 */
static int rate_limit_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)req;  // 未使用参数

    if (!g_rate_limiter) {
        return UVHTTP_MIDDLEWARE_CONTINUE;
    }

    /* 检查限流 */
    if (!rate_limiter_try_consume(g_rate_limiter)) {
        // 使用 cJSON 创建 JSON 错误响应
        cJSON* error_obj = cJSON_CreateObject();
        if (!error_obj) {
            uvhttp_response_set_status(resp, 429);
            uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_header(resp, "Retry-After", "1");
            const char* error = "{\"error\":\"请求过于频繁\"}";
            uvhttp_response_set_body(resp, error, strlen(error));
            uvhttp_response_send(resp);
            return UVHTTP_MIDDLEWARE_STOP;
        }

        cJSON_AddStringToObject(error_obj, "error", "请求过于频繁");
        cJSON_AddStringToObject(error_obj, "message", "请稍后再试");

        char* error_string = cJSON_PrintUnformatted(error_obj);
        cJSON_Delete(error_obj);

        if (!error_string) {
            uvhttp_response_set_status(resp, 429);
            uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_header(resp, "Retry-After", "1");
            const char* error = "{\"error\":\"请求过于频繁\"}";
            uvhttp_response_set_body(resp, error, strlen(error));
            uvhttp_response_send(resp);
            return UVHTTP_MIDDLEWARE_STOP;
        }

        uvhttp_response_set_status(resp, 429);
        uvhttp_response_set_header(resp, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_header(resp, "Retry-After", "1");
        uvhttp_response_set_body(resp, error_string, strlen(error_string));

        uvhttp_response_send(resp);
        free(error_string);
        return UVHTTP_MIDDLEWARE_STOP;
    }

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ============ 其他中间件 ============ */

static int logging_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)resp;
    
    const char* method = uvhttp_request_get_method(req);
    const char* path = uvhttp_request_get_path(req);
    
    printf("[LOG] %s %s\n", method, path);
    
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ============ 请求处理器 ============ */

static int api_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 执行中间件链 */
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        logging_middleware,
        rate_limit_middleware
    );
    
    const char* json = "{\"message\":\"API 响应\"}";
    
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
    
    /* 创建限流器：每秒最多 5 个请求 */
    g_rate_limiter = rate_limiter_create(5, 5);
    if (!g_rate_limiter) {
        fprintf(stderr, "Failed to create rate limiter\n");
        return 1;
    }
    
    /* 创建服务器 */
    err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server\n");
        rate_limiter_destroy(g_rate_limiter);
        return 1;
    }
    
    /* 创建路由 */
    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        rate_limiter_destroy(g_rate_limiter);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/api", api_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8085);
    
    printf("服务器运行在 http://localhost:8085\n");
    printf("限流配置：每秒最多 5 个请求\n");
    printf("测试:\n");
    printf("  curl http://localhost:8085/api\n");
    printf("  for i in {1..10}; do curl http://localhost:8085/api; done\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    uvhttp_server_free(server);
    rate_limiter_destroy(g_rate_limiter);
    
    return 0;
}