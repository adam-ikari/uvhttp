# UVHTTP API 参考文档

## 概述

UVHTTP 提供了一套简洁、高效的 C API，用于构建 HTTP/1.1 服务器。

## 核心类型

### uvhttp_server_t

服务器对象，管理整个 HTTP 服务器的生命周期。

```c
typedef struct uvhttp_server uvhttp_server_t;
```

### uvhttp_router_t

路由对象，管理 URL 路径和处理函数的映射。

```c
typedef struct uvhttp_router uvhttp_router_t;
```

### uvhttp_context_t

上下文对象，存储服务器运行时状态。

```c
typedef struct uvhttp_context uvhttp_context_t;
```

### uvhttp_request_t

请求对象，封装 HTTP 请求信息。

```c
typedef struct uvhttp_request uvhttp_request_t;
```

### uvhttp_response_t

响应对象，封装 HTTP 响应信息。

```c
typedef struct uvhttp_response uvhttp_response_t;
```

## 服务器 API

### uvhttp_server_new

```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

创建新的服务器对象。

**参数**:
- `loop`: libuv 事件循环

**返回值**:
- 成功: 服务器对象指针
- 失败: `NULL`

**示例**:
```c
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
if (!server) {
    fprintf(stderr, "Failed to create server\n");
    return 1;
}
```

### uvhttp_server_free

```c
void uvhttp_server_free(uvhttp_server_t* server);
```

释放服务器对象。

**参数**:
- `server`: 服务器对象

**示例**:
```c
uvhttp_server_free(server);
```

### uvhttp_server_listen

```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server,
                                   const char* host,
                                   int port);
```

启动服务器监听指定地址和端口。

**参数**:
- `server`: 服务器对象
- `host`: 监听地址（如 "0.0.0.0"）
- `port`: 监听端口

**返回值**:
- `UVHTTP_OK`: 成功
- 其他值: 错误码

**示例**:
```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to listen: %d\n", result);
    return 1;
}
```

## 路由 API

### uvhttp_router_new

```c
uvhttp_router_t* uvhttp_router_new(void);
```

创建新的路由对象。

**返回值**:
- 成功: 路由对象指针
- 失败: `NULL`

### uvhttp_router_free

```c
void uvhttp_router_free(uvhttp_router_t* router);
```

释放路由对象。

### uvhttp_router_add_route

```c
void uvhttp_router_add_route(uvhttp_router_t* router,
                             const char* path,
                             uvhttp_route_handler_t handler);
```

添加路由规则。

**参数**:
- `router`: 路由对象
- `path`: URL 路径（如 "/api"）
- `handler`: 处理函数

**示例**:
```c
uvhttp_router_add_route(router, "/", home_handler);
uvhttp_router_add_route(router, "/api", api_handler);
```

## 请求处理 API

### uvhttp_request_get_method

```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
```

获取 HTTP 方法。

**返回值**: HTTP 方法字符串（如 "GET", "POST"）

### uvhttp_request_get_path

```c
const char* uvhttp_request_get_path(uvhttp_request_t* request);
```

获取请求路径。

**返回值**: URL 路径字符串

### uvhttp_request_get_header

```c
const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                     const char* name);
```

获取请求头。

**参数**:
- `request`: 请求对象
- `name`: 头部名称

**返回值**: 头部值，不存在返回 `NULL`

**示例**:
```c
const char* content_type = uvhttp_request_get_header(request, "Content-Type");
```

### uvhttp_request_get_body

```c
const char* uvhttp_request_get_body(uvhttp_request_t* request,
                                   size_t* len);
```

获取请求体。

**参数**:
- `request`: 请求对象
- `len`: 输出参数，返回请求体长度

**返回值**: 请求体数据指针

## 响应处理 API

### uvhttp_response_set_status

```c
void uvhttp_response_set_status(uvhttp_response_t* response,
                               int status_code);
```

设置响应状态码。

**参数**:
- `response`: 响应对象
- `status_code`: HTTP 状态码（如 200, 404）

**示例**:
```c
uvhttp_response_set_status(response, 200);
```

### uvhttp_response_set_header

```c
void uvhttp_response_set_header(uvhttp_response_t* response,
                               const char* name,
                               const char* value);
```

设置响应头。

**参数**:
- `response`: 响应对象
- `name`: 头部名称
- `value`: 头部值

**示例**:
```c
uvhttp_response_set_header(response, "Content-Type", "application/json");
```

### uvhttp_response_set_body

```c
void uvhttp_response_set_body(uvhttp_response_t* response,
                             const char* body,
                             size_t len);
```

设置响应体。

**参数**:
- `response`: 响应对象
- `body`: 响应体数据
- `len`: 响应体长度

**示例**:
```c
const char* body = "Hello, World!";
uvhttp_response_set_body(response, body, strlen(body));
```

### uvhttp_response_send

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

发送响应。

**返回值**:
- `UVHTTP_OK`: 成功
- 其他值: 错误码

## 上下文 API

### uvhttp_context_create

```c
uvhttp_error_t uvhttp_context_create(uv_loop_t* loop,
                                    uvhttp_context_t** context);
```

创建上下文对象。

**参数**:
- `loop`: libuv 事件循环
- `context`: 输出参数，返回上下文对象

**返回值**:
- `UVHTTP_OK`: 成功
- 其他值: 错误码

### uvhttp_context_free

```c
void uvhttp_context_free(uvhttp_context_t* context);
```

释放上下文对象。

## 错误处理 API

### uvhttp_error_string

```c
const char* uvhttp_error_string(uvhttp_error_t error);
```

获取错误名称。

**返回值**: 错误名称字符串

### uvhttp_error_description

```c
const char* uvhttp_error_description(uvhttp_error_t error);
```

获取错误描述。

**返回值**: 错误描述字符串

### uvhttp_error_suggestion

```c
const char* uvhttp_error_suggestion(uvhttp_error_t error);
```

获取修复建议。

**返回值**: 修复建议字符串

### uvhttp_error_is_recoverable

```c
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```

检查错误是否可恢复。

**返回值**:
- `1`: 可恢复
- `0`: 不可恢复

## 内存管理 API

### 基本操作

UVHTTP 提供统一的内存管理接口，通过编译期选择分配器类型。

```c
void* uvhttp_alloc(size_t size);
void uvhttp_realloc(void* ptr, size_t size);
void uvhttp_free(void* ptr);
void* uvhttp_calloc(size_t nmemb, size_t size);
```

#### uvhttp_alloc

```c
void* uvhttp_alloc(size_t size);
```

分配内存。

**参数**:
- `size`: 要分配的字节数

**返回值**:
- 成功: 内存指针
- 失败: `NULL`

**示例**:
```c
void* ptr = uvhttp_alloc(1024);
if (!ptr) {
    // 处理内存不足
}
```

#### uvhttp_free

```c
void uvhttp_free(void* ptr);
```

释放内存。

**参数**:
- `ptr`: 要释放的内存指针

**示例**:
```c
uvhttp_free(ptr);
```

#### uvhttp_realloc

```c
void* uvhttp_realloc(void* ptr, size_t size);
```

重新分配内存。

**参数**:
- `ptr`: 原始内存指针
- `size`: 新的大小

**返回值**:
- 成功: 新的内存指针
- 失败: `NULL`

**示例**:
```c
ptr = uvhttp_realloc(ptr, 2048);
if (!ptr) {
    // 处理内存不足
}
```

#### uvhttp_calloc

```c
void* uvhttp_calloc(size_t nmemb, size_t size);
```

分配并初始化内存为零。

**参数**:
- `nmemb`: 元素数量
- `size`: 每个元素的大小

**返回值**:
- 成功: 内存指针
- 失败: `NULL`

**示例**:
```c
int* array = uvhttp_calloc(100, sizeof(int));
if (!array) {
    // 处理内存不足
}
```

### 分配器信息

#### uvhttp_allocator_name

```c
const char* uvhttp_allocator_name(void);
```

获取当前分配器名称。

**返回值**: 分配器名称字符串（"system" 或 "mimalloc"）

**示例**:
```c
printf("Using allocator: %s\n", uvhttp_allocator_name());
```

### 编译配置

通过 CMake 编译宏选择分配器类型：

```cmake
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### 性能特性

- **零运行时开销**: 所有函数都是内联函数
- **编译期优化**: 编译器可以完全优化
- **类型安全**: 编译期类型检查
- **可预测性**: 无动态分发

### 最佳实践

1. **统一使用**: 始终使用 `uvhttp_alloc/uvhttp_free`，不要混用 `malloc/free`
2. **成对分配**: 每个分配都有对应的释放
3. **检查返回值**: 检查分配是否成功
4. **避免泄漏**: 确保所有路径都释放内存

### 完整示例

```c
#include "uvhttp_allocator.h"

void example_memory_usage(void) {
    // 分配内存
    char* buffer = uvhttp_alloc(1024);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }

    // 使用内存
    strcpy(buffer, "Hello, World!");

    // 重新分配
    buffer = uvhttp_realloc(buffer, 2048);
    if (!buffer) {
        fprintf(stderr, "Failed to reallocate memory\n");
        return;
    }

    // 释放内存
    uvhttp_free(buffer);
}
```

## 工具函数 API

### 字符串处理

#### uvhttp_safe_strcpy
```c
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);
```
安全的字符串复制。

#### uvhttp_url_decode
```c
int uvhttp_url_decode(const char* src, char* dest, size_t dest_size);
```
URL 解码。

### 哈希函数

#### uvhttp_hash_string
```c
uint64_t uvhttp_hash_string(const char* str);
```
计算字符串哈希值。

## 错误码

| 错误码 | 值 | 描述 |
|--------|-----|------|
| UVHTTP_OK | 0 | 成功 |
| UVHTTP_ERROR_INVALID_PARAM | -1 | 无效参数 |
| UVHTTP_ERROR_OUT_OF_MEMORY | -2 | 内存不足 |
| UVHTTP_ERROR_IO | -3 | I/O 错误 |
| UVHTTP_ERROR_TLS | -4 | TLS 错误 |
| UVHTTP_ERROR_WEBSOCKET | -5 | WebSocket 错误 |
| UVHTTP_ERROR_ROUTER | -6 | 路由错误 |
| UVHTTP_ERROR_STATIC_FILE | -7 | 静态文件错误 |

## 常量

### HTTP 方法

```c
#define UVHTTP_METHOD_GET "GET"
#define UVHTTP_METHOD_POST "POST"
#define UVHTTP_METHOD_PUT "PUT"
#define UVHTTP_METHOD_DELETE "DELETE"
#define UVHTTP_METHOD_HEAD "HEAD"
#define UVHTTP_METHOD_OPTIONS "OPTIONS"
```

### HTTP 状态码

```c
#define UVHTTP_STATUS_OK 200
#define UVHTTP_STATUS_CREATED 201
#define UVHTTP_STATUS_BAD_REQUEST 400
#define UVHTTP_STATUS_NOT_FOUND 404
#define UVHTTP_STATUS_INTERNAL_SERVER_ERROR 500
```

### 常量限制

```c
#define UVHTTP_MAX_HEADERS 64
#define UVHTTP_MAX_HEADER_NAME_SIZE 256
#define UVHTTP_MAX_HEADER_VALUE_SIZE 8192
#define UVHTTP_MAX_URL_SIZE 8192
```

## 编译选项

### CMake 选项

```cmake
BUILD_WITH_WEBSOCKET=ON          # 启用 WebSocket 支持
BUILD_WITH_MIMALLOC=ON           # 启用 mimalloc 分配器
BUILD_WITH_HTTPS=ON                # 启用 TLS 支持
ENABLE_DEBUG=OFF                 # 调试模式
ENABLE_COVERAGE=OFF              # 代码覆盖率
BUILD_EXAMPLES=ON               # 构建示例程序
```

### 编译宏

```c
UVHTTP_FEATURE_WEBSOCKET          # WebSocket 支持
UVHTTP_FEATURE_STATIC_FILES       # 静态文件服务
UVHTTP_FEATURE_TLS                # TLS 支持
UVHTTP_FEATURE_LRU_CACHE          # LRU 缓存
UVHTTP_FEATURE_ROUTER_CACHE       # 路由缓存
UVHTTP_FEATURE_LOGGING            # 日志系统
UVHTTP_ALLOCATOR_TYPE             # 分配器类型 (0=系统, 1=mimalloc)
```

## 示例

### 基本 HTTP 服务器

```c
#include "uvhttp.h"

void home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
}

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", home_handler);
    server->router = router;
    
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %d\n", result);
        return 1;
    }
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    uvhttp_server_free(server);
    return 0;
}
```

## 参考资料

- [架构设计文档](../dev/ARCHITECTURE.md)
- [贡献者指南 (中文)](../zh/guide/DEVELOPER_GUIDE.md)
- [教程 (中文)](../zh/guide/TUTORIAL.md)
- [安全策略](../SECURITY.md)
- [libuv 文档](https://docs.libuv.org/)
- [HTTP/1.1 规范](https://tools.ietf.org/html/rfc7230)