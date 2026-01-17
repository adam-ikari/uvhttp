# API 文档

## 概述

UVHTTP 提供了一套简洁的 C API，用于构建高性能的 HTTP/1.1 和 WebSocket 服务器。

## 核心模块

### 服务器 (uvhttp_server)

`uvhttp_server_t` 是服务器的核心结构体。

#### 创建服务器

```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

#### 启动服务器

```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```

#### 停止服务器

```c
void uvhttp_server_close(uvhttp_server_t* server);
```

### 路由 (uvhttp_router)

`uvhttp_router_t` 提供路由功能。

#### 创建路由

```c
uvhttp_router_t* uvhttp_router_new(void);
```

#### 添加路由

```c
void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_handler_t handler);
```

### 请求 (uvhttp_request)

`uvhttp_request_t` 表示 HTTP 请求。

#### 获取请求方法

```c
uvhttp_method_t uvhttp_request_get_method(uvhttp_request_t* req);
```

#### 获取请求路径

```c
const char* uvhttp_request_get_path(uvhttp_request_t* req);
```

#### 获取请求头

```c
const char* uvhttp_request_get_header(uvhttp_request_t* req, const char* name);
```

#### 获取请求体

```c
const char* uvhttp_request_get_body(uvhttp_request_t* req, size_t* len);
```

### 响应 (uvhttp_response)

`uvhttp_response_t` 用于构建 HTTP 响应。

#### 创建响应

```c
uvhttp_response_t* uvhttp_response_new(uvhttp_request_t* req);
```

#### 设置状态码

```c
void uvhttp_response_set_status(uvhttp_response_t* res, int status);
```

#### 设置响应头

```c
void uvhttp_response_set_header(uvhttp_response_t* res, const char* name, const char* value);
```

#### 设置响应体

```c
void uvhttp_response_set_body(uvhttp_response_t* res, const char* body);
```

#### 发送响应

```c
void uvhttp_response_send(uvhttp_response_t* res);
```

## 错误处理

所有可能失败的函数都返回 `uvhttp_error_t`：

```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR = -1,
    UVHTTP_ERR_INVALID_PARAM = -2,
    UVHTTP_ERR_OUT_OF_MEMORY = -3,
    // ... 更多错误码
} uvhttp_error_t;
```

### 错误检查

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}
```

## 完整示例

```c
#include <uvhttp.h>
#include <stdio.h>

void index_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html");
    uvhttp_response_set_body(res, "<h1>Hello, UVHTTP!</h1>");
    uvhttp_response_send(res);
}

void api_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "{\"message\":\"API response\"}");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/", index_handler);
    uvhttp_router_add_route(router, "/api", api_handler);

    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_close(server);
    return 0;
}
```

## 更多 API

- [服务器 API](/api/server) - 服务器相关 API
- [路由 API](/api/router) - 路由相关 API
- [请求 API](/api/request) - 请求相关 API
- [响应 API](/api/response) - 响应相关 API
- [WebSocket API](/api/websocket) - WebSocket 相关 API