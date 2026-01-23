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
void uvhttp_server_free(uvhttp_server_t* server);
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

`uvhttp_response_t` 用于构建 HTTP 响应。响应对象由框架创建并传递给请求处理器。

#### 设置状态码

```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status);
```

#### 设置响应头

```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
```

#### 设置响应体

```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
```

#### 发送响应

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
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
#include <string.h>

int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, "<h1>Hello, UVHTTP!</h1>", strlen("<h1>Hello, UVHTTP!</h1>"));
    uvhttp_response_send(response);
    return UVHTTP_OK;
}

int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    const char* json_body = "{\"message\":\"API response\"}";
    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);
    return UVHTTP_OK;
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

    uvhttp_server_free(server);
    return 0;
}
```

## 完整 API 参考

详细的 API 参考文档已使用 Doxygen 自动生成，包含所有公共 API 的完整说明：

- **[查看完整 API 文档](/api-reference/)** - 包含所有模块、函数、数据结构的详细文档
- **[模块索引](/api-reference/modules.html)** - 按功能模块浏览 API
- **[函数索引](/api-reference/globals.html)** - 按字母顺序查找函数
- **[数据结构](/api-reference/annotated.html)** - 所有结构体和类型定义

### 核心模块

- **服务器 (uvhttp_server)** - 服务器创建、启动、停止
- **路由 (uvhttp_router)** - 路由管理、参数提取
- **请求 (uvhttp_request)** - HTTP 请求处理
- **响应 (uvhttp_response)** - HTTP 响应构建
- **WebSocket (uvhttp_websocket)** - WebSocket 连接管理
- **TLS (uvhttp_tls)** - TLS/SSL 支持
- **静态文件 (uvhttp_static)** - 静态文件服务
- **中间件 (uvhttp_middleware)** - 中间件系统
- **限流 (uvhttp_rate_limit)** - 请求限流
- **CORS (uvhttp_cors)** - 跨域资源共享
- **日志 (uvhttp_log)** - 日志系统
- **配置 (uvhttp_config)** - 配置管理
- **缓存 (uvhttp_lru_cache)** - LRU 缓存
- **内存池 (uvhttp_mempool)** - 内存池管理
- **分配器 (uvhttp_allocator)** - 内存分配器
- **错误处理 (uvhttp_error)** - 错误处理机制