/*
 * UVHTTP 中间件系统
 * 提供可插拔的中间件架构
 */

#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 中间件类型 */
typedef enum {
    UVHTTP_MIDDLEWARE_HTTP = 0,    /* HTTP 中间件 */
    UVHTTP_MIDDLEWARE_WEBSOCKET = 1, /* WebSocket 中间件 */
    UVHTTP_MIDDLEWARE_STATIC = 2   /* 静态文件中间件 */
} uvhttp_middleware_type_t;

/* 中间件优先级 */
typedef enum {
    UVHTTP_MIDDLEWARE_PRIORITY_LOW = 0,
    UVHTTP_MIDDLEWARE_PRIORITY_NORMAL = 1,
    UVHTTP_MIDDLEWARE_PRIORITY_HIGH = 2
} uvhttp_middleware_priority_t;

/* 中间件返回值 */
#define UVHTTP_MIDDLEWARE_CONTINUE 0    /* 继续执行下一个中间件 */
#define UVHTTP_MIDDLEWARE_STOP 1        /* 停止执行中间件链 */

/* 中间件上下文 */
typedef struct uvhttp_middleware_context {
    void* data;  /* 中间件私有数据 */
    void (*cleanup)(void* data);  /* 清理函数 */
} uvhttp_middleware_context_t;

/* HTTP 中间件处理函数 */
typedef int (*uvhttp_http_middleware_handler_t)(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
);

/* HTTP 中间件结构 */
typedef struct uvhttp_http_middleware {
    const char* path;                          /* 路径模式 */
    uvhttp_http_middleware_handler_t handler;  /* 处理函数 */
    uvhttp_middleware_priority_t priority;     /* 优先级 */
    uvhttp_middleware_context_t context;       /* 上下文 */
    struct uvhttp_http_middleware* next;       /* 下一个中间件 */
} uvhttp_http_middleware_t;

/* 创建 HTTP 中间件 */
uvhttp_http_middleware_t* uvhttp_http_middleware_create(
    const char* path,
    uvhttp_http_middleware_handler_t handler,
    uvhttp_middleware_priority_t priority
);

/* 销毁 HTTP 中间件 */
void uvhttp_http_middleware_destroy(uvhttp_http_middleware_t* middleware);

/* 设置中间件上下文 */
void uvhttp_http_middleware_set_context(
    uvhttp_http_middleware_t* middleware,
    void* data,
    void (*cleanup)(void*)
);

/* 执行 HTTP 中间件链 */
int uvhttp_http_middleware_execute(
    uvhttp_http_middleware_t* middleware,
    uvhttp_request_t* request,
    uvhttp_response_t* response
);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */