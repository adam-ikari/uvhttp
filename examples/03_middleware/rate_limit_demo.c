/*
 * UVHTTP 请求过滤器示例：按客户端 IP 限流
 *
 * v2.10 起库不再内置限流，只提供一个分派前接缝
 * uvhttp_server_set_request_filter()。本例完全由用户代码实现限流：
 * 每个客户端 IP 一个槽位，1 秒固定窗口，窗口内最多 5 个请求。
 *
 * 为什么限流放在过滤器而不是中间件里：
 * UVHTTP_EXECUTE_MIDDLEWARE 是在 handler 函数体内展开的宏，中间件返回
 * STOP 只跳出中间件循环，handler 体照样往下执行。本例的拒绝发生在分派
 * 之前，handler 根本不会被调用，因此不依赖那个语义。
 *
 * 状态表用固定容量数组：海量源地址无法把内存撑大，表满时回收最久未活动的
 * 槽位。要换 LRU、按网段聚合或接 Redis，改 find_or_open_slot() 就够了。
 */

#include "uvhttp_middleware.h"

#include "uvhttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 256
#define WINDOW_MS 1000
#define MAX_REQUESTS_PER_WINDOW 5

/* ============ 每客户端固定窗口 ============ */

typedef struct {
    char ip[UVHTTP_IPV6_MAX_STRING_LENGTH];
    uint64_t window_start_ms; /* 0 表示槽位空闲 */
    int count;
} client_slot_t;

typedef struct {
    client_slot_t slots[MAX_CLIENTS];
} limiter_t;

static limiter_t* limiter_create(void) {
    return (limiter_t*)calloc(1, sizeof(limiter_t));
}

/* 找到该 IP 的槽位：命中已有槽位就返回它，否则用空槽，全满则复用最久未活动的 */
static client_slot_t* find_or_open_slot(limiter_t* lim, const char* ip) {
    client_slot_t* oldest = &lim->slots[0];
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        client_slot_t* s = &lim->slots[i];
        if (strcmp(s->ip, ip) == 0) {
            return s;
        }
        if (s->window_start_ms == 0) {
            return s;
        }
        if (s->window_start_ms < oldest->window_start_ms) {
            oldest = s;
        }
    }
    return oldest;
}

/* ============ 分派前过滤器 ============ */

static uvhttp_error_t rate_limit_filter(uvhttp_request_t* request,
                                        uvhttp_response_t* response,
                                        void* user_data) {
    limiter_t* lim = (limiter_t*)user_data;

    /* 键取真实对端地址：未开启 trust_proxy_headers 时不受 X-Forwarded-For
     * 影响，IPv6 客户端也各自独立计数 */
    const char* ip = uvhttp_request_get_client_ip(request);
    if (!ip) {
        return UVHTTP_OK; /* 没有可计数的键，不要拿请求开刀 */
    }

    uint64_t now_ms = uv_hrtime() / 1000000ULL;
    client_slot_t* s = find_or_open_slot(lim, ip);

    if (s->window_start_ms == 0 || now_ms - s->window_start_ms >= WINDOW_MS) {
        strncpy(s->ip, ip, sizeof(s->ip) - 1);
        s->ip[sizeof(s->ip) - 1] = '\0';
        s->window_start_ms = now_ms;
        s->count = 0;
    }

    if (++s->count <= MAX_REQUESTS_PER_WINDOW) {
        return UVHTTP_OK; /* 放行，继续正常分派 */
    }

    /* 拒绝必须自己把响应发完；若忘了发，库会兜底 403 */
    char body[128];
    int body_len =
        snprintf(body, sizeof(body),
                 "{\"error\":\"请求过于频繁\",\"client\":\"%s\"}", ip);
    uvhttp_response_set_status(response, 429);
    uvhttp_response_set_header(response, UVHTTP_HEADER_CONTENT_TYPE,
                               "application/json; charset=utf-8");
    uvhttp_response_set_header(response, UVHTTP_HEADER_RETRY_AFTER, "1");
    uvhttp_response_set_body(response, body, (size_t)body_len);
    uvhttp_response_send(response);

    printf("[LIMIT] %s 超出窗口配额\n", ip);
    return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
}

/* ============ 普通中间件（handler 内部） ============ */

static int logging_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                              uvhttp_middleware_context_t* ctx) {
    (void)resp;
    (void)ctx;

    printf("[LOG] %s %s\n", uvhttp_request_get_method(req),
           uvhttp_request_get_path(req));

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ============ 请求处理器 ============ */

static int api_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* 能被调用到这里，说明过滤器已经放行 */
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, logging_middleware);

    const char* json = "{\"message\":\"API 响应\"}";

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, UVHTTP_HEADER_CONTENT_TYPE,
                               "application/json; charset=utf-8");
    uvhttp_response_set_body(resp, json, strlen(json));

    return uvhttp_response_send(resp);
}

/* ============ 主函数 ============ */

int main(int argc, char* argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 8085;

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;

    limiter_t* lim = limiter_create();
    if (!lim) {
        fprintf(stderr, "Failed to create limiter state\n");
        return 1;
    }

    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server\n");
        free(lim);
        return 1;
    }

    /* 接缝在这里：库负责调用，策略全是上面那份用户代码 */
    err = uvhttp_server_set_request_filter(server, rate_limit_filter, lim);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Failed to install request filter\n");
        uvhttp_server_free(server);
        free(lim);
        return 1;
    }

    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        free(lim);
        return 1;
    }

    uvhttp_router_add_route(router, "/api", api_handler);
    uvhttp_server_take_router(server, router);

    if (uvhttp_server_listen(server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen on 0.0.0.0:%d\n", port);
        uvhttp_server_free(server);
        free(lim);
        return 1;
    }

    printf("服务器运行在 http://localhost:%d\n", port);
    printf("限流配置：每个客户端 IP 每秒最多 %d 个请求\n",
           MAX_REQUESTS_PER_WINDOW);
    printf("测试:\n");
    printf(
        "  for i in $(seq 10); do curl -s -o /dev/null -w '%%{http_code}\\n' "
        "http://localhost:%d/api; done\n",
        port);
    printf("  换一台机器（不同 IP）请求，配额互不影响\n");

    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_free(server);
    free(lim); /* user_data 是借用的，库不会释放 */

    return 0;
}
