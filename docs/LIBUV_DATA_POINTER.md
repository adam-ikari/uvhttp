# 使用 libuv 循环数据指针

## 概述

libuv 的事件循环提供了一个 `data` 指针字段，可以用来存储用户自定义数据。这是避免使用全局变量的最佳实践，特别适合在多线程环境中使用。

## 基本概念

### uv_loop_t 的 data 字段

```c
typedef struct uv_loop_s {
    // ... 其他字段 ...
    void* data;  // 用户数据指针
    // ... 其他字段 ...
} uv_loop_t;
```

### 为什么要使用 data 指针？

1. **避免全局变量**：全局变量在多线程环境中会导致竞争条件
2. **线程安全**：每个事件循环有自己的数据，互不干扰
3. **代码封装**：将相关数据封装在一起，提高代码可维护性
4. **上下文传递**：方便在回调函数中访问应用上下文

## 基本用法

### 1. 定义应用上下文结构

```c
/**
 * @brief 应用上下文结构
 * 
 * 封装所有应用相关的数据
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    time_t start_time;
    // 其他应用数据...
} app_context_t;
```

### 2. 创建并设置上下文

```c
int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = malloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    // 初始化上下文
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    
    // 创建服务器
    ctx->server = uvhttp_server_new(loop);
    ctx->router = uvhttp_router_new();
    uvhttp_server_set_router(ctx->server, ctx->router);
    
    // 将上下文设置到事件循环的 data 指针
    loop->data = ctx;
    
    // 添加路由（需要访问上下文）
    uvhttp_router_add_route(ctx->router, "/", home_handler);
    
    // 启动服务器
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
    
    return 0;
}
```

### 3. 在回调函数中访问上下文

```c
/**
 * @brief 请求处理器
 * 
 * 从事件循环的 data 指针获取应用上下文
 */
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 从事件循环获取应用上下文
    app_context_t* ctx = (app_context_t*)loop->data;
    
    // 访问上下文数据
    ctx->request_count++;
    
    // 使用上下文中的数据
    char response[256];
    snprintf(response, sizeof(response),
        "{\"request_count\":%d,\"uptime\":%ld}",
        ctx->request_count,
        time(NULL) - ctx->start_time);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}
```

## 完整示例

### 示例 1：简单的计数器

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

/**
 * @brief 应用上下文
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    time_t start_time;
} app_context_t;

/**
 * @brief 统计处理器
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;
    
    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"request_count\": %d,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"active_connections\": %zu\n"
        "}",
        ctx->request_count,
        time(NULL) - ctx->start_time,
        ctx->server ? ctx->server->active_connections : 0);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    ctx->request_count++;
    
    return uvhttp_response_send(res);
}

/**
 * @brief 信号处理
 */
void signal_handler(int sig) {
    printf("\n收到信号 %d\n", sig);
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    // 创建并初始化上下文
    app_context_t* ctx = malloc(sizeof(app_context_t));
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    
    // 设置到事件循环
    loop->data = ctx;
    
    // 创建服务器
    ctx->server = uvhttp_server_new(loop);
    ctx->router = uvhttp_router_new();
    uvhttp_server_set_router(ctx->server, ctx->router);
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    
    // 启动服务器
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080/stats\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
    
    return 0;
}
```

### 示例 2：多线程环境

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4

/**
 * @brief 工作线程上下文
 */
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} worker_context_t;

/**
 * @brief 请求处理器
 */
int worker_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    worker_context_t* ctx = (worker_context_t*)loop->data;
    
    char response[256];
    snprintf(response, sizeof(response),
        "{\"thread_id\":%d,\"request_count\":%d}",
        ctx->thread_id,
        ++ctx->request_count);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

/**
 * @brief 工作线程函数
 */
void* worker_thread(void* arg) {
    worker_context_t* ctx = (worker_context_t*)arg;
    
    // 创建独立的事件循环
    ctx->loop = uv_loop_new();
    
    // 将上下文设置到事件循环
    ctx->loop->data = ctx;
    
    // 创建服务器
    ctx->server = uvhttp_server_new(ctx->loop);
    ctx->router = uvhttp_router_new();
    uvhttp_server_set_router(ctx->server, ctx->router);
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/", worker_handler);
    
    // 启动服务器（不同端口）
    int port = 8080 + ctx->thread_id;
    uvhttp_server_listen(ctx->server, "0.0.0.0", port);
    
    printf("线程 %d 监听端口 %d\n", ctx->thread_id, port);
    
    // 运行事件循环
    uv_run(ctx->loop, UV_RUN_DEFAULT);
    
    // 清理
    uvhttp_server_free(ctx->server);
    uv_loop_close(ctx->loop);
    free(ctx->loop);
    
    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    worker_context_t contexts[THREAD_COUNT];
    
    // 创建工作线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        contexts[i].thread_id = i;
        contexts[i].request_count = 0;
        
        pthread_create(&threads[i], NULL, worker_thread, &contexts[i]);
    }
    
    printf("多线程服务器启动，共 %d 个线程\n", THREAD_COUNT);
    
    // 等待所有线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
```

## 高级用法

### 1. 嵌套上下文

```c
typedef struct {
    app_context_t* app_ctx;
    int worker_id;
    int local_count;
} worker_context_t;

// 在主线程中
app_context_t* app_ctx = malloc(sizeof(app_context_t));
app_ctx->server = server;
app_ctx->router = router;
loop->data = app_ctx;

// 在工作线程中
worker_context_t* worker_ctx = malloc(sizeof(worker_context_t));
worker_ctx->app_ctx = app_ctx;  // 共享应用上下文
worker_ctx->worker_id = id;
worker_ctx->local_count = 0;
worker_loop->data = worker_ctx;
```

### 2. 上下文初始化函数

```c
/**
 * @brief 初始化应用上下文
 */
app_context_t* app_context_create(uv_loop_t* loop) {
    app_context_t* ctx = malloc(sizeof(app_context_t));
    if (!ctx) return NULL;
    
    ctx->server = uvhttp_server_new(loop);
    ctx->router = uvhttp_router_new();
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    
    if (ctx->server && ctx->router) {
        uvhttp_server_set_router(ctx->server, ctx->router);
        loop->data = ctx;
        return ctx;
    }
    
    // 创建失败，清理
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
    return NULL;
}

/**
 * @brief 销毁应用上下文
 */
void app_context_destroy(app_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->server) uvhttp_server_free(ctx->server);
    free(ctx);
}
```

### 3. 在异步回调中使用

```c
typedef struct {
    uv_work_t work_req;
    app_context_t* ctx;
    char query[256];
} async_query_t;

void async_work_cb(uv_work_t* req) {
    async_query_t* async = (async_query_t*)req->data;
    // 执行异步工作
    printf("执行查询: %s\n", async->query);
}

void async_after_cb(uv_work_t* req, int status) {
    async_query_t* async = (async_query_t*)req->data;
    app_context_t* ctx = async->ctx;
    
    // 访问上下文
    ctx->request_count++;
    
    free(async);
    free(req);
}

int async_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;
    
    // 创建异步请求
    async_query_t* async = malloc(sizeof(async_query_t));
    async->ctx = ctx;
    strcpy(async->query, "SELECT * FROM users");
    
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = async;
    
    uv_queue_work(loop, work_req, async_work_cb, async_after_cb);
    
    return 0;
}
```

## 最佳实践

### 1. 始终检查 data 指针

```c
int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = (app_context_t*)loop->data;
    
    // 检查上下文是否存在
    if (!ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 使用上下文
    // ...
}
```

### 2. 在清理时重置 data 指针

```c
void cleanup(app_context_t* ctx, uv_loop_t* loop) {
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    
    // 重置 data 指针
    loop->data = NULL;
    
    free(ctx);
}
```

### 3. 使用宏简化访问

```c
#define GET_CTX(loop, ctx_type) ((ctx_type*)((loop)->data))

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop, app_context_t);
    
    if (!ctx) {
        // 错误处理
    }
    
    // 使用 ctx
}
```

### 4. 线程安全的上下文访问

```c
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    pthread_mutex_t mutex;
} app_context_t;

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uv_default_loop();
    app_context_t* ctx = GET_CTX(loop, app_context_t);
    
    // 加锁保护共享数据
    pthread_mutex_lock(&ctx->mutex);
    ctx->request_count++;
    int count = ctx->request_count;
    pthread_mutex_unlock(&ctx->mutex);
    
    // 使用数据
    // ...
}
```

## 常见问题

### Q: 如何在多个事件循环间共享数据？

A: 使用共享上下文结构，每个事件循环的 data 指针指向同一个上下文：

```c
app_context_t* shared_ctx = malloc(sizeof(app_context_t));

// 线程 1
loop1->data = shared_ctx;

// 线程 2
loop2->data = shared_ctx;

// 记得使用互斥锁保护共享数据
```

### Q: data 指针会被 libuv 修改吗？

A: 不会。libuv 不会修改 data 指针，它完全由用户控制。

### Q: 可以在 data 指针中存储任意数据吗？

A: 可以，data 是 `void*` 类型，可以存储任何指针。建议使用结构体来组织相关数据。

### Q: 如何处理 data 指针为 NULL 的情况？

A: 始终检查 data 指针是否为 NULL，特别是在库函数或回调函数中：

```c
void callback(uv_async_t* handle) {
    uv_loop_t* loop = handle->loop;
    app_context_t* ctx = (app_context_t*)loop->data;
    
    if (!ctx) {
        fprintf(stderr, "错误: 上下文未初始化\n");
        return;
    }
    
    // 继续处理
}
```

## 总结

使用 libuv 循环的 data 指针是避免全局变量的最佳实践：

1. **封装性**：将相关数据封装在结构体中
2. **线程安全**：每个事件循环有自己的数据副本
3. **灵活性**：可以在运行时动态修改上下文
4. **可维护性**：代码结构清晰，易于理解和维护

在多线程和复杂应用中，务必使用 data 指针来管理应用状态！