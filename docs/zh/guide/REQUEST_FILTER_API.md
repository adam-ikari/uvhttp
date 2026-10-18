---
title: 请求过滤器 API
description: UVHTTP 的分派前请求过滤器 —— 应用层准入策略（限流、黑白名单、配额）的运行位置，附一个按客户端计数的固定窗口示例。
---

# 请求过滤器 API

## 概述

UVHTTP 不提供限流。v2.10 起内置限流整体删除，取而代之的是一个在请求分派之前
运行的接缝：

```c
typedef uvhttp_error_t (*uvhttp_request_filter_t)(uvhttp_request_t* request,
                                                 uvhttp_response_t* response,
                                                 void* user_data);

uvhttp_error_t uvhttp_server_set_request_filter(uvhttp_server_t* server,
                                                uvhttp_request_filter_t filter,
                                                void* user_data);
```

删除是刻意的。旧的限流器把整个服务器共用一个计数器，一个恶意客户端能让所有访客
都收到 429；它的白名单又只匹配 IPv4 对端地址。想做得比这更好，就得回答嵌入库
无权代答的问题：窗口多长、按什么维度计数（客户端 IP、租户、API key）、是否信任
代理头、这张表最多占多少内存、满了淘汰谁。所以库只负责跑接缝，语义归你。

## 契约

| 项目 | 行为 |
| --- | --- |
| 时机 | 每个解析完成的请求调用一次，位于协议升级探测之前、路由查找之前。被拒的请求既不会到达 detector，也不会到达 handler。 |
| 放行 | 返回 `UVHTTP_OK`。 |
| 停止 | 返回任何其他错误码。`UVHTTP_ERROR_RATE_LIMIT_EXCEEDED`（-550）保留在错误码表里，就是这个场景的惯用值。 |
| 响应 | 停止请求的 filter 应当自己把响应发完（状态码、`Retry-After`、响应体）再返回。 |
| 兜底 | 若返回时什么都没发送，库代为回复：filter 写过的状态码原样保留，否则发 403 `Forbidden`，`text/plain` 响应体。`uvhttp_response_send()` 的 `sent` 保护会在 filter 已发送时把兜底变成空操作，因此不存在重复发送。 |
| 连接 | 拒绝请求不会关闭连接。keep-alive 客户端可以在同一条连接上继续发下一个请求。 |
| 槽位 | 只有一个 filter，不是链。要串多步就在自己的回调里串。 |
| 线程 | 在事件循环线程上运行。保持 O(1)，不要在里面阻塞。 |
| `user_data` | 借用，与 `server->handler` 一致：库永不 free。它必须比 server 活得久，或者先安装 `NULL` filter 把它清掉。 |

`filter` 传 `NULL` 即移除当前安装的过滤器；`uvhttp_server_new()` 出来的 server
默认接缝是关着的。

识别客户端用 `uvhttp_request_get_client_ip()`。它返回 IPv6 或 IPv4 的对端地址，
只有在 `config->trust_proxy_headers` 置位时才认 `X-Forwarded-For` /
`X-Real-IP` —— 代理头由客户端控制，极易伪造，不是真的站在代理后面就别开。返回
的指针可能指向内部缓冲区，下一次调用就会覆盖它，需要保留就立刻拷走。

## 按客户端的固定窗口

下面的示例全是用户代码，除了接缝之外不依赖任何库支持。每个客户端 IP 一个槽位，
1 秒窗口，每窗口 100 个请求，表容量固定，因此海量源地址无法无限撑大内存。

```c
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"

#define MAX_CLIENTS 256
#define WINDOW_MS 1000
#define MAX_REQUESTS_PER_WINDOW 100

typedef struct {
    char ip[UVHTTP_IPV6_MAX_STRING_LENGTH];
    uint64_t window_start_ms; /* 0 表示槽位空闲 */
    int count;
} client_slot_t;

typedef struct {
    client_slot_t slots[MAX_CLIENTS];
} limiter_t;

static client_slot_t* find_or_open_slot(limiter_t* lim, const char* ip) {
    client_slot_t* oldest = &lim->slots[0];
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        client_slot_t* s = &lim->slots[i];
        if (strcmp(s->ip, ip) == 0) {
            return s;
        }
        if (s->window_start_ms == 0) {
            return s; /* 空槽 */
        }
        if (s->window_start_ms < oldest->window_start_ms) {
            oldest = s;
        }
    }
    /* 表满了：回收最久未活动的客户端。若你要的是别的策略（LRU、按网段聚合），
     * 换掉这一步就是了。 */
    return oldest;
}

static uvhttp_error_t limit_filter(uvhttp_request_t* request,
                                   uvhttp_response_t* response,
                                   void* user_data) {
    limiter_t* lim = (limiter_t*)user_data;
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

    if (++s->count > MAX_REQUESTS_PER_WINDOW) {
        uvhttp_response_set_status(response, 429);
        uvhttp_response_set_header(response, "Retry-After", "1");
        uvhttp_response_set_header(response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_body(response, "Too Many Requests",
                                 strlen("Too Many Requests"));
        uvhttp_response_send(response);
        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
    }
    return UVHTTP_OK;
}

int main(void) {
    limiter_t* lim = calloc(1, sizeof(*lim));
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_server_set_request_filter(server, limit_filter, lim);
    /* ... 路由、listen、跑循环 ... */

    uvhttp_server_free(server);
    free(lim); /* 库不会替你 free，自己来 */
    return 0;
}
```

因为键是真实对端地址，一个吵闹的客户端不再牵连他人，IPv6 客户端也有自己的槽位，
而不是从检查里漏过去。

## 从被删的 API 迁移

| 已删除 | 替代 |
| --- | --- |
| `uvhttp_server_enable_rate_limit(server, max, window)` | 装一个自己维护计数的 filter，`max` 与 `window` 变成你的常量。 |
| `uvhttp_server_disable_rate_limit(server)` | `uvhttp_server_set_request_filter(server, NULL, NULL)`。 |
| `uvhttp_server_check_rate_limit(server)` | filter 本就在分派前运行，没有需要调用的东西。 |
| `uvhttp_server_add_rate_limit_whitelist()` / `clear_rate_limit_all()` | 白名单就是 filter 开头的一次 `strcmp`。 |
| `uvhttp_server_get_rate_limit_status()` / `reset_rate_limit_client()` | 直接读写你自己的表。 |
| `server->rate_limit_*` 字段、`UVHTTP_FEATURE_RATE_LIMIT`、`BUILD_WITH_RATE_LIMIT` | 没了。接缝只是一对指针，不需要编译选项。 |
| `uvhttp_has_feature("rate_limit")` | 仍可调用，现在恒定回答「不支持」。 |

`UVHTTP_ERROR_RATE_LIMIT_EXCEEDED`（-550）及其 `uvhttp_error_string()` 文案保留：
它从库内部返回值变成应用层的约定码，库自己不再返回它。

## 参见

- [教程](/zh/guide/TUTORIAL) —— handler 内部的中间件组合。注意中间件的
  `STOP` 只跳出中间件循环，handler 函数体照样执行；要中止分派本身请用本过滤器。
- [CMake 配置](/zh/guide/CMAKE_CONFIGURATION) —— `trust_proxy_headers`。
