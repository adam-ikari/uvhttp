# UVHTTP API 完整参考

## 核心 API

### 服务器管理

#### uvhttp_server_new
```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```
创建新的 HTTP 服务器实例。

**参数:**
- `loop`: libuv 事件循环

**返回值:**
- 成功: 服务器实例指针
- 失败: NULL

**示例:**
```c
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
```

---

#### uvhttp_server_free
```c
void uvhttp_server_free(uvhttp_server_t* server);
```
释放服务器资源。

**参数:**
- `server`: 服务器实例

---

#### uvhttp_server_listen
```c
int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```
启动服务器监听。

**参数:**
- `server`: 服务器实例
- `host`: 监听地址 (如 "0.0.0.0")
- `port`: 监听端口 (如 8080)

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_server_stop
```c
void uvhttp_server_stop(uvhttp_server_t* server);
```
停止服务器。

---

### 路由管理

#### uvhttp_router_new
```c
uvhttp_router_t* uvhttp_router_new(void);
```
创建新的路由器。

**返回值:**
- 成功: 路由器实例指针
- 失败: NULL

---

#### uvhttp_router_free
```c
void uvhttp_router_free(uvhttp_router_t* router);
```
释放路由器资源。

---

#### uvhttp_router_add_route
```c
int uvhttp_router_add_route(uvhttp_router_t* router, 
                           const char* path, 
                           uvhttp_request_handler_t handler);
```
添加路由规则。

**参数:**
- `router`: 路由器实例
- `path`: 路由路径 (如 "/api/users")
- `handler`: 请求处理函数

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_router_find_handler
```c
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, 
                                                   const char* path);
```
查找路径对应的处理器。

**参数:**
- `router`: 路由器实例
- `path`: 请求路径

**返回值:**
- 找到: 处理器函数指针
- 未找到: NULL

---

### 请求处理

#### uvhttp_request_get_method
```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
```
获取 HTTP 方法。

**返回值:**
- HTTP 方法字符串 ("GET", "POST", 等)

---

#### uvhttp_request_get_url
```c
const char* uvhttp_request_get_url(uvhttp_request_t* request);
```
获取请求 URL。

**返回值:**
- URL 字符串

---

#### uvhttp_request_get_header
```c
const char* uvhttp_request_get_header(uvhttp_request_t* request, 
                                     const char* name);
```
获取请求头。

**参数:**
- `request`: 请求实例
- `name`: 头部名称 (如 "Content-Type")

**返回值:**
- 找到: 头部值字符串
- 未找到: NULL

---

#### uvhttp_request_get_body
```c
const char* uvhttp_request_get_body(uvhttp_request_t* request);
```
获取请求体。

**返回值:**
- 请求体数据指针

---

#### uvhttp_request_get_body_length
```c
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);
```
获取请求体长度。

**返回值:**
- 请求体字节数

---

### 响应处理

#### uvhttp_response_set_status
```c
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
```
设置响应状态码。

**参数:**
- `response`: 响应实例
- `status_code`: HTTP 状态码 (200, 404, 等)

---

#### uvhttp_response_set_header
```c
void uvhttp_response_set_header(uvhttp_response_t* response, 
                               const char* name, 
                               const char* value);
```
设置响应头。

**参数:**
- `response`: 响应实例
- `name`: 头部名称
- `value`: 头部值

---

#### uvhttp_response_set_body
```c
int uvhttp_response_set_body(uvhttp_response_t* response, 
                            const char* body, 
                            size_t length);
```
设置响应体。

**参数:**
- `response`: 响应实例
- `body`: 响应体数据
- `length`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_response_send
```c
void uvhttp_response_send(uvhttp_response_t* response);
```
发送响应。

---

## WebSocket API

### 连接管理

#### uvhttp_websocket_new
```c
uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, 
                                       uvhttp_response_t* response);
```
创建 WebSocket 连接。

**参数:**
- `request`: HTTP 请求
- `response`: HTTP 响应

**返回值:**
- 成功: WebSocket 实例指针
- 失败: NULL

---

#### uvhttp_websocket_free
```c
void uvhttp_websocket_free(uvhttp_websocket_t* ws);
```
释放 WebSocket 连接。

---

#### uvhttp_websocket_close
```c
uvhttp_websocket_error_t uvhttp_websocket_close(uvhttp_websocket_t* ws, 
                                              int code, 
                                              const char* reason);
```
关闭 WebSocket 连接。

**参数:**
- `ws`: WebSocket 实例
- `code`: 关闭代码
- `reason`: 关闭原因

**返回值:**
- 成功: UVHTTP_WEBSOCKET_OK
- 失败: 错误码

---

### 消息处理

#### uvhttp_websocket_send
```c
uvhttp_websocket_error_t uvhttp_websocket_send(uvhttp_websocket_t* ws, 
                                             const char* data, 
                                             size_t length, 
                                             uvhttp_websocket_type_t type);
```
发送 WebSocket 消息。

**参数:**
- `ws`: WebSocket 实例
- `data`: 消息数据
- `length`: 数据长度
- `type`: 消息类型

**返回值:**
- 成功: UVHTTP_WEBSOCKET_OK
- 失败: 错误码

---

#### uvhttp_websocket_set_handler
```c
uvhttp_websocket_error_t uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, 
                                                    uvhttp_websocket_handler_t handler, 
                                                    void* user_data);
```
设置消息处理器。

**参数:**
- `ws`: WebSocket 实例
- `handler`: 处理器函数
- `user_data`: 用户数据

**返回值:**
- 成功: UVHTTP_WEBSOCKET_OK
- 失败: 错误码

---

### 便捷宏

```c
#define uvhttp_websocket_send_text(ws, text) \
    uvhttp_websocket_send(ws, text, strlen(text), UVHTTP_WEBSOCKET_TEXT)

#define uvhttp_websocket_send_binary(ws, data, len) \
    uvhttp_websocket_send(ws, data, len, UVHTTP_WEBSOCKET_BINARY)
```

---

## 错误处理 API

### 错误码

```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NOT_FOUND = -3,
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_INIT = -301,
    // ... 更多错误码
} uvhttp_error_t;
```

### 错误查询

#### uvhttp_error_string
```c
const char* uvhttp_error_string(uvhttp_error_t error);
```
获取错误描述字符串。

---

## 配置管理

### 配置结构体

#### uvhttp_config_t
```c
typedef struct {
    /* 服务器配置 */
    int max_connections;              // 最大并发连接数 (1-10000)
    int max_requests_per_connection;  // 每个连接的最大请求数
    
    /* 性能配置 */
    size_t max_body_size;             // 最大请求体大小
    size_t max_header_size;           // 最大请求头大小
    
    /* 安全配置 */
    int rate_limit_window;            // 速率限制窗口时间(秒)
    
    /* 其他配置... */
} uvhttp_config_t;
```

### 配置管理函数

#### uvhttp_config_new
```c
uvhttp_config_t* uvhttp_config_new(void);
```
创建新的配置实例。

**返回值:**
- 成功: 配置实例指针
- 失败: NULL

#### uvhttp_config_set_defaults
```c
void uvhttp_config_set_defaults(uvhttp_config_t* config);
```
设置默认配置值。

**默认值:**
- `max_connections`: 2048
- `max_requests_per_connection`: 100
- `max_body_size`: 1MB
- `max_header_size`: 8KB

#### uvhttp_config_update_max_connections
```c
int uvhttp_config_update_max_connections(int max_connections);
```
动态更新最大并发连接数。

**参数:**
- `max_connections`: 新的最大连接数 (1-10000)

**返回值:**
- `UVHTTP_OK`: 更新成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数超出范围
- `UVHTTP_ERROR_INVALID_STATE`: 配置未初始化

**示例:**
```c
// 运行时动态调整连接限制
int result = uvhttp_config_update_max_connections(5000);
if (result == UVHTTP_OK) {
    printf("连接限制已更新为5000\n");
}
```

#### uvhttp_config_load_file
```c
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename);
```
从配置文件加载设置。

**配置文件格式:**
```
max_connections=3000
max_requests_per_connection=200
max_body_size=2097152
max_header_size=16384
```

#### uvhttp_config_load_env
```c
int uvhttp_config_load_env(uvhttp_config_t* config);
```
从环境变量加载配置。

**支持的环境变量:**
- `UVHTTP_MAX_CONNECTIONS`: 最大连接数
- `UVHTTP_MAX_REQUESTS_PER_CONNECTION`: 每连接最大请求数

**示例:**
```bash
export UVHTTP_MAX_CONNECTIONS=4000
export UVHTTP_MAX_REQUESTS_PER_CONNECTION=150
./your_server
```

#### uvhttp_config_validate
```c
int uvhttp_config_validate(const uvhttp_config_t* config);
```
验证配置参数的有效性。

**返回值:**
- `UVHTTP_OK`: 配置有效
- `UVHTTP_ERROR_INVALID_PARAM`: 存在无效参数

#### uvhttp_config_get_current
```c
const uvhttp_config_t* uvhttp_config_get_current(void);
```
获取当前全局配置实例。

#### uvhttp_config_monitor_changes
```c
int uvhttp_config_monitor_changes(uvhttp_config_change_callback_t callback);
```
监控配置变化。

**回调函数类型:**
```c
typedef void (*uvhttp_config_change_callback_t)(const char* key, 
                                                 const void* old_value, 
                                                 const void* new_value);
```

**示例:**
```c
void on_config_change(const char* key, const void* old_value, const void* new_value) {
    if (strcmp(key, "max_connections") == 0) {
        printf("最大连接数从 %d 变更为 %d\n", 
               *(int*)old_value, *(int*)new_value);
    }
}

uvhttp_config_monitor_changes(on_config_change);
```

### 配置常量

#### 连接相关常量
```c
#define UVHTTP_MAX_CONNECTIONS           2048    // 编译时默认最大连接数
#define UVHTTP_DEFAULT_MAX_CONNECTIONS   2048    // 运行时默认最大连接数
#define UVHTTP_DEFAULT_BACKLOG           1024    // 监听队列大小
```

### 错误处理

#### 连接限制错误
```c
#define UVHTTP_ERROR_CONNECTION_LIMIT    -103    // 连接数达到上限
```

当活动连接数达到 `max_connections` 限制时，服务器会：
1. 记录警告日志
2. 拒绝新连接
3. 返回 `UVHTTP_ERROR_CONNECTION_LIMIT` 错误

### 使用示例

#### 基本配置使用
```c
#include "uvhttp.h"
#include "uvhttp_config.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建配置
    uvhttp_config_t* config = uvhttp_config_new();
    uvhttp_config_set_defaults(config);
    
    // 自定义连接限制
    config->max_connections = 3000;
    config->max_requests_per_connection = 200;
    
    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        fprintf(stderr, "配置验证失败\n");
        return 1;
    }
    
    // 应用配置
    server->config = config;
    
    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

#### 动态配置调整
```c
void adjust_connections_based_on_load() {
    // 获取当前配置
    const uvhttp_config_t* current = uvhttp_config_get_current();
    
    // 根据系统负载动态调整
    if (system_load > 0.8) {
        // 高负载时降低连接数
        uvhttp_config_update_max_connections(current->max_connections * 0.8);
    } else if (system_load < 0.3) {
        // 低负载时增加连接数
        uvhttp_config_update_max_connections(current->max_connections * 1.2);
    }
}
```

#### 配置文件和环境变量
```c
int load_config_from_multiple_sources(uvhttp_config_t* config) {
    // 1. 设置默认值
    uvhttp_config_set_defaults(config);
    
    // 2. 从配置文件加载
    if (uvhttp_config_load_file(config, "uvhttp.conf") != UVHTTP_OK) {
        printf("配置文件加载失败，使用默认配置\n");
    }
    
    // 3. 从环境变量加载 (会覆盖配置文件设置)
    if (uvhttp_config_load_env(config) != UVHTTP_OK) {
        printf("环境变量加载失败\n");
    }
    
    // 4. 验证最终配置
    return uvhttp_config_validate(config);
}
```

---

### 错误恢复

#### uvhttp_set_error_recovery_config
```c
void uvhttp_set_error_recovery_config(int max_retries, 
                                     int base_delay_ms, 
                                     int max_delay_ms, 
                                     double backoff_multiplier);
```
配置错误重试策略。

---

#### uvhttp_retry_operation
```c
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*), 
                                     void* context, 
                                     const char* operation_name);
```
执行带重试的操作。

---

## 内存管理 API

### 基本操作

```c
void* uvhttp_malloc(size_t size);
void* uvhttp_realloc(void* ptr, size_t size);
void uvhttp_free(void* ptr);
void* uvhttp_calloc(size_t nmemb, size_t size);
```

### 内存统计 (调试模式)

```c
void uvhttp_get_memory_stats(size_t* total_allocated, 
                             size_t* current_allocated,
                             size_t* allocation_count, 
                             size_t* free_count);
int uvhttp_check_memory_leaks(void);
void uvhttp_reset_memory_stats(void);
```

---

## 工具函数 API

### 字符串处理

#### uvhttp_safe_strcpy
```c
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);
```
安全的字符串复制。

---

#### uvhttp_url_decode
```c
int uvhttp_url_decode(const char* src, char* dest, size_t dest_size);
```
URL 解码。

---

### 编码转换

#### uvhttp_base64_encode
```c
int uvhttp_base64_encode(const unsigned char* input, 
                        size_t input_length,
                        char* output, 
                        size_t output_size);
```
Base64 编码。

---

#### uvhttp_base64_decode
```c
int uvhttp_base64_decode(const char* input, 
                        unsigned char* output, 
                        size_t output_size);
```
Base64 解码。

---

### 验证函数

#### uvhttp_validate_header_value
```c
int uvhttp_validate_header_value(const char* name, const char* value);
```
验证 HTTP 头部值。

---

## TLS/SSL API

### TLS 配置

```c
typedef struct {
    const char* cert_file;
    const char* key_file;
    const char* ca_file;
    int verify_peer;
} uvhttp_tls_config_t;
```

#### uvhttp_server_enable_tls
```c
int uvhttp_server_enable_tls(uvhttp_server_t* server, 
                            const uvhttp_tls_config_t* config);
```
启用 TLS 支持。

---

## 中间件 API

### CORS 中间件

```c
void uvhttp_cors_middleware(uvhttp_request_t* request, 
                           uvhttp_response_t* response,
                           const char* allowed_origins,
                           const char* allowed_methods,
                           const char* allowed_headers);
```

### 限流中间件

```c
void uvhttp_rate_limit_middleware(uvhttp_request_t* request, 
                                 uvhttp_response_t* response,
                                 int max_requests_per_minute);
```

---

## 常量定义

### HTTP 状态码

```c
#define UVHTTP_STATUS_OK                 200
#define UVHTTP_STATUS_CREATED            201
#define UVHTTP_STATUS_NO_CONTENT          204
#define UVHTTP_STATUS_BAD_REQUEST        400
#define UVHTTP_STATUS_UNAUTHORIZED       401
#define UVHTTP_STATUS_FORBIDDEN           403
#define UVHTTP_STATUS_NOT_FOUND           404
#define UVHTTP_STATUS_INTERNAL_ERROR      500
```

### 内容类型

```c
#define UVHTTP_CONTENT_TYPE_TEXT         "text/plain"
#define UVHTTP_CONTENT_TYPE_HTML         "text/html"
#define UVHTTP_CONTENT_TYPE_JSON         "application/json"
#define UVHTTP_CONTENT_TYPE_XML          "application/xml"
```

### 限制常量

```c
#define UVHTTP_MAX_CONNECTIONS           2048
#define UVHTTP_MAX_BODY_SIZE             (1024 * 1024)  // 1MB
#define UVHTTP_MAX_HEADERS               64
#define UVHTTP_MAX_URL_SIZE              2048
```

---

## 使用示例

### 基本 HTTP 服务器

```c
#include "uvhttp.h"

void handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello", 5);
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### WebSocket 服务器

```c
void ws_handler(uvhttp_websocket_t* ws, 
               const uvhttp_websocket_message_t* msg, 
               void* user_data) {
    if (msg->type == UVHTTP_WEBSOCKET_TEXT) {
        uvhttp_websocket_send_text(ws, "Echo: ");
        uvhttp_websocket_send(ws, msg->data, msg->length, msg->type);
    }
}

void upgrade_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_websocket_t* ws = uvhttp_websocket_new(req, res);
    uvhttp_websocket_set_handler(ws, ws_handler, NULL);
}
```

---

## 版本信息

```c
#define UVHTTP_VERSION_MAJOR    1
#define UVHTTP_VERSION_MINOR    0
#define UVHTTP_VERSION_PATCH    0
#define UVHTTP_VERSION_STRING   "1.0.0"
```