# 中间件

## 概述

UVHTTP 提供强大的中间件系统，允许在请求处理前后执行自定义逻辑。

## 中间件基础

### 中间件结构

```c
typedef void (*uvhttp_middleware_handler_t)(
    uvhttp_request_t* req,
    uvhttp_middleware_next_t next
);
```

### 创建中间件

```c
void logging_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    printf("Request: %s %s\n",
           uvhttp_method_string(uvhttp_request_get_method(req)),
           uvhttp_request_get_path(req));

    next(req);  // 调用下一个中间件或处理器
}
```

### 注册中间件

```c
uvhttp_server_t* server = uvhttp_server_new(loop);

// 添加中间件
uvhttp_server_add_middleware(server, logging_middleware);
```

## 内置中间件

### 日志中间件

```c
#include <uvhttp_log_middleware.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建日志中间件
    uvhttp_log_middleware_t* log_middleware = uvhttp_log_middleware_new();
    uvhttp_log_middleware_set_format(log_middleware, "%s %s %d");

    // 添加到服务器
    uvhttp_server_add_middleware(server, log_middleware);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### CORS 中间件

```c
#include <uvhttp_cors_middleware.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建 CORS 中间件
    uvhttp_cors_middleware_t* cors = uvhttp_cors_middleware_new();

    // 配置 CORS
    uvhttp_cors_middleware_allow_origin(cors, "*");
    uvhttp_cors_middleware_allow_methods(cors, "GET, POST, PUT, DELETE");
    uvhttp_cors_middleware_allow_headers(cors, "Content-Type, Authorization");

    // 添加到服务器
    uvhttp_server_add_middleware(server, cors);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### 限流中间件

```c
#include <uvhttp_rate_limit.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建限流中间件
    uvhttp_rate_limit_t* rate_limit = uvhttp_rate_limit_new();

    // 配置限流：每分钟最多 100 个请求
    uvhttp_rate_limit_set_max_requests(rate_limit, 100);
    uvhttp_rate_limit_set_window(rate_limit, 60);

    // 添加到服务器
    uvhttp_server_add_middleware(server, rate_limit);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 自定义中间件

### 认证中间件

```c
void auth_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    const char* token = uvhttp_request_get_header(req, "Authorization");

    if (!token) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_body(res, "Unauthorized");
        uvhttp_response_send(res);
        return;  // 不调用 next，终止请求处理
    }

    // 验证 token
    if (!validate_token(token)) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 403);
        uvhttp_response_set_body(res, "Forbidden");
        uvhttp_response_send(res);
        return;
    }

    // 认证通过，继续处理
    next(req);
}
```

### 请求计时中间件

```c
void timing_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    clock_t start = clock();

    next(req);

    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("Request processed in %.3f seconds\n", elapsed);
}
```

### 响应头中间件

```c
void headers_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    next(req);

    // 在响应发送后添加额外的头
    // 注意：这需要在响应发送前调用
}
```

### 错误处理中间件

```c
void error_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    // 设置错误处理
    uvhttp_request_set_error_handler(req, [](uvhttp_request_t* req, int error_code) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, error_code);
        uvhttp_response_set_body(res, "Internal Server Error");
        uvhttp_response_send(res);
    });

    next(req);
}
```

## 中间件链

### 多个中间件

```c
int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 添加多个中间件
    uvhttp_server_add_middleware(server, logging_middleware);
    uvhttp_server_add_middleware(server, timing_middleware);
    uvhttp_server_add_middleware(server, auth_middleware);
    uvhttp_server_add_middleware(server, cors_middleware);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### 中间件执行顺序

中间件按照添加顺序执行：

```
Request → Middleware 1 → Middleware 2 → Middleware 3 → Handler
           ↓              ↓              ↓              ↓
         next()        next()        next()        Response
```

## 条件中间件

### 路径条件

```c
void conditional_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    const char* path = uvhttp_request_get_path(req);

    // 只对 /api 路径应用中间件
    if (strncmp(path, "/api", 4) == 0) {
        // 应用中间件逻辑
    }

    next(req);
}
```

### 方法条件

```c
void post_only_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    if (uvhttp_request_get_method(req) == UVHTTP_METHOD_POST) {
        // 只对 POST 请求应用中间件
    }

    next(req);
}
```

## 中间件配置

### 配置示例

```c
typedef struct {
    int max_requests;
    int window_seconds;
    char* log_format;
} middleware_config_t;

void configurable_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    middleware_config_t* config = (middleware_config_t*)uvhttp_request_get_middleware_data(req);

    // 使用配置
    printf("Max requests: %d\n", config->max_requests);

    next(req);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 设置配置
    middleware_config_t config = {
        .max_requests = 100,
        .window_seconds = 60,
        .log_format = "%s %s %d"
    };

    // 添加带配置的中间件
    uvhttp_server_add_middleware_with_config(server, configurable_middleware, &config);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 中间件最佳实践

### 1. 保持简单

```c
// 好：单一职责
void logging_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    printf("Request: %s\n", uvhttp_request_get_path(req));
    next(req);
}

// 避免：复杂逻辑
void complex_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    // 太多逻辑...
}
```

### 2. 正确调用 next()

```c
// 好：总是调用 next()
void good_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    // 处理逻辑
    next(req);  // 继续处理
}

// 避免：忘记调用 next()
void bad_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    // 处理逻辑
    // 忘记调用 next()，请求被挂起
}
```

### 3. 错误处理

```c
void safe_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    try {
        // 可能失败的操作
        next(req);
    } catch (...) {
        // 错误处理
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 500);
        uvhttp_response_send(res);
    }
}
```

### 4. 性能考虑

```c
// 好：快速处理
void fast_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    // 简单检查
    if (should_skip(req)) {
        next(req);
        return;
    }
    // 处理逻辑
    next(req);
}

// 避免：阻塞操作
void blocking_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    sleep(1);  // 阻塞！
    next(req);
}
```

## 示例项目

### 完整的中间件链

```c
#include <uvhttp.h>
#include <uvhttp_log_middleware.h>
#include <uvhttp_cors_middleware.h>
#include <uvhttp_rate_limit.h>

void logging_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    printf("[%s] %s %s\n",
           uvhttp_method_string(uvhttp_request_get_method(req)),
           uvhttp_request_get_path(req),
           uvhttp_request_get_header(req, "User-Agent"));
    next(req);
}

void auth_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    const char* token = uvhttp_request_get_header(req, "Authorization");
    if (!token) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_body(res, "Unauthorized");
        uvhttp_response_send(res);
        return;
    }
    next(req);
}

void handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, "Hello, World!");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    // 添加中间件
    uvhttp_server_add_middleware(server, logging_middleware);
    uvhttp_server_add_middleware(server, auth_middleware);

    uvhttp_router_add_route(router, "/", handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 更多资源

- [API 文档](/api/introduction) - API 介绍
- [路由](/guide/routing) - 路由系统
- [最佳实践](/guide/best-practices) - 最佳实践指南