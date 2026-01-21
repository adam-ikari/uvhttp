# UVHTTP API 完整参考

## 核心 API

### 统一 API（简化开发）

#### uvhttp_server_create
```c
uvhttp_server_builder_t* uvhttp_server_create(const char* host, int port);
```
快速创建服务器构建器。

**参数:**
- `host`: 监听地址
- `port`: 监听端口

**返回值:**
- 成功: 服务器构建器指针
- 失败: NULL

---

#### 链式路由 API

##### uvhttp_get
```c
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加 GET 路由。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_post
```c
uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server,
                                     const char* path,
                                     uvhttp_request_handler_t handler);
```
添加 POST 路由。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_put
```c
uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加 PUT 路由。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_delete
```c
uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
```
添加 DELETE 路由。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_any
```c
uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加任意方法路由。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

#### 配置 API

##### uvhttp_set_max_connections
```c
uvhttp_server_builder_t* uvhttp_set_max_connections(uvhttp_server_builder_t* server,
                                                    int max_conn);
```
设置最大连接数。

**参数:**
- `server`: 服务器构建器
- `max_conn`: 最大连接数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_set_timeout
```c
uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server,
                                            int timeout);
```
设置超时时间。

**参数:**
- `server`: 服务器构建器
- `timeout`: 超时时间（秒）

**返回值:**
- 服务器构建器指针（支持链式调用）

---

##### uvhttp_set_max_body_size
```c
uvhttp_server_builder_t* uvhttp_set_max_body_size(uvhttp_server_builder_t* server,
                                                   size_t size);
```
设置最大请求体大小。

**参数:**
- `server`: 服务器构建器
- `size`: 最大请求体大小（字节）

**返回值:**
- 服务器构建器指针（支持链式调用）

---

#### 快速响应 API

##### uvhttp_quick_response
```c
void uvhttp_quick_response(uvhttp_response_t* response,
                           int status,
                           const char* content_type,
                           const char* body);
```
快速发送响应。

**参数:**
- `response`: 响应实例
- `status`: HTTP 状态码
- `content_type`: 内容类型
- `body`: 响应体

---

##### uvhttp_html_response
```c
void uvhttp_html_response(uvhttp_response_t* response,
                          const char* html_body);
```
快速发送 HTML 响应。

**参数:**
- `response`: 响应实例
- `html_body`: HTML 内容

---

##### uvhttp_file_response
```c
void uvhttp_file_response(uvhttp_response_t* response,
                          const char* file_path);
```
快速发送文件响应。

**参数:**
- `response`: 响应实例
- `file_path`: 文件路径

---

#### 便捷请求 API

##### uvhttp_get_param
```c
const char* uvhttp_get_param(uvhttp_request_t* request,
                             const char* name);
```
获取请求参数。

**参数:**
- `request`: 请求实例
- `name`: 参数名称

**返回值:**
- 参数值，未找到返回 NULL

---

##### uvhttp_get_header
```c
const char* uvhttp_get_header(uvhttp_request_t* request,
                              const char* name);
```
获取请求头。

**参数:**
- `request`: 请求实例
- `name`: 头部名称

**返回值:**
- 头部值，未找到返回 NULL

---

##### uvhttp_get_body
```c
const char* uvhttp_get_body(uvhttp_request_t* request);
```
获取请求体。

**参数:**
- `request`: 请求实例

**返回值:**
- 请求体数据

---

#### 服务器运行和清理

##### uvhttp_server_run
```c
int uvhttp_server_run(uvhttp_server_builder_t* server);
```
运行服务器。

**参数:**
- `server`: 服务器构建器

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

##### uvhttp_server_stop_simple
```c
void uvhttp_server_stop_simple(uvhttp_server_builder_t* server);
```
停止服务器。

**参数:**
- `server`: 服务器构建器

---

##### uvhttp_server_simple_free
```c
void uvhttp_server_simple_free(uvhttp_server_builder_t* server);
```
释放服务器资源。

**参数:**
- `server`: 服务器构建器

---

#### 一键启动

##### uvhttp_serve
```c
int uvhttp_serve(const char* host, int port);
```
一键启动服务器（最简 API）。

**参数:**
- `host`: 监听地址
- `port`: 监听端口

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

### 统一 API 使用示例

#### 链式 API 示例
```c
void hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 200, "text/plain", "Hello, World!");
}

void api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_get_param(req, "name");
    char response[256];
    snprintf(response, sizeof(response), "Hello, %s!", name ? name : "Guest");
    uvhttp_quick_response(res, 200, "application/json", response);
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_get(server, "/", hello_handler)
           ->post(server, "/api", api_handler)
           ->put(server, "/api", api_handler)
           ->delete(server, "/api", api_handler);

    uvhttp_set_max_connections(server, 5000);
    uvhttp_set_timeout(server, 30);

    return uvhttp_server_run(server);
}
```

#### 一键启动示例
```c
int main() {
    return uvhttp_serve("0.0.0.0", 8080);
}
```

---

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
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
```
释放服务器资源。

**参数:**
- `server`: 服务器实例

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_listen
```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```
启动服务器监听。

**参数:**
- `server`: 服务器实例
- `host`: 监听地址 (如 "0.0.0.0")
- `port`: 监听端口 (如 8080)

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_stop
```c
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
```
停止服务器。

**参数:**
- `server`: 服务器实例

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

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
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                           const char* path,
                           uvhttp_request_handler_t handler);
```
添加路由规则。

**参数:**
- `router`: 路由器实例
- `path`: 路由路径 (如 "/api/users")
- `handler`: 请求处理函数

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_router_find_handler
```c
uvhttp_request_handler_t uvhttp_router_find_handler(const uvhttp_router_t* router,
                                                   const char* path,
                                                   const char* method);
```
查找路径对应的处理器。

**参数:**
- `router`: 路由器实例
- `path`: 请求路径
- `method`: HTTP 方法 ("GET", "POST", 等)

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

### 服务器端 WebSocket 管理

#### uvhttp_server_register_ws_handler
```c
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server,
                                                 const char* path,
                                                 uvhttp_ws_handler_t* handler);
```
注册 WebSocket 处理器。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径 (如 "/ws")
- `handler`: WebSocket 处理器

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_send
```c
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn,
                                     const char* data,
                                     size_t len);
```
发送 WebSocket 消息。

**参数:**
- `ws_conn`: WebSocket 连接
- `data`: 消息数据
- `len`: 数据长度

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_close
```c
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn,
                                      int code,
                                      const char* reason);
```
关闭 WebSocket 连接。

**参数:**
- `ws_conn`: WebSocket 连接
- `code`: 关闭代码
- `reason`: 关闭原因

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### WebSocket 连接管理

#### uvhttp_server_ws_enable_connection_management
```c
uvhttp_error_t uvhttp_server_ws_enable_connection_management(uvhttp_server_t* server,
                                                             int timeout_seconds,
                                                             int heartbeat_interval);
```
启用 WebSocket 连接管理。

**参数:**
- `server`: 服务器实例
- `timeout_seconds`: 超时时间（秒）
- `heartbeat_interval`: 心跳间隔（秒）

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_get_connection_count
```c
int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server);
```
获取 WebSocket 连接总数。

**参数:**
- `server`: 服务器实例

**返回值:**
- 连接数量

---

#### uvhttp_server_ws_broadcast
```c
uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server,
                                         const char* path,
                                         const char* data,
                                         size_t len);
```
向指定路径的所有连接广播消息。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `data`: 消息数据
- `len`: 数据长度

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### WebSocket 认证

#### uvhttp_server_ws_set_auth_config
```c
uvhttp_error_t uvhttp_server_ws_set_auth_config(uvhttp_server_t* server,
                                                const char* path,
                                                uvhttp_ws_auth_config_t* config);
```
设置 WebSocket 认证配置。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `config`: 认证配置

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_enable_token_auth
```c
uvhttp_error_t uvhttp_server_ws_enable_token_auth(uvhttp_server_t* server,
                                                  const char* path,
                                                  uvhttp_ws_token_validator_callback validator,
                                                  void* user_data);
```
启用 Token 认证。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `validator`: Token 验证回调
- `user_data`: 用户数据

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### 底层 WebSocket API

#### uvhttp_ws_connection_create
```c
struct uvhttp_ws_connection* uvhttp_ws_connection_create(int fd,
                                                         mbedtls_ssl_context* ssl,
                                                         int is_server);
```
创建 WebSocket 连接。

**参数:**
- `fd`: 文件描述符
- `ssl`: SSL 上下文
- `is_server`: 是否为服务器端

**返回值:**
- 成功: WebSocket 连接指针
- 失败: NULL

---

#### uvhttp_ws_send_text
```c
int uvhttp_ws_send_text(struct uvhttp_ws_connection* conn,
                        const char* text,
                        size_t len);
```
发送文本消息。

**参数:**
- `conn`: WebSocket 连接
- `text`: 文本数据
- `len`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_send_binary
```c
int uvhttp_ws_send_binary(struct uvhttp_ws_connection* conn,
                          const uint8_t* data,
                          size_t len);
```
发送二进制消息。

**参数:**
- `conn`: WebSocket 连接
- `data`: 二进制数据
- `len`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_close
```c
int uvhttp_ws_close(struct uvhttp_ws_connection* conn,
                    int code,
                    const char* reason);
```
关闭 WebSocket 连接。

**参数:**
- `conn`: WebSocket 连接
- `code`: 关闭代码
- `reason`: 关闭原因

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_set_callbacks
```c
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection* conn,
                             uvhttp_ws_on_message_callback on_message,
                             uvhttp_ws_on_close_callback on_close,
                             uvhttp_ws_on_error_callback on_error);
```
设置 WebSocket 回调函数。

**参数:**
- `conn`: WebSocket 连接
- `on_message`: 消息回调
- `on_close`: 关闭回调
- `on_error`: 错误回调

---

### WebSocket 类型定义

#### WebSocket 操作码
```c
typedef enum {
    UVHTTP_WS_OPCODE_CONTINUATION = 0x0,
    UVHTTP_WS_OPCODE_TEXT = 0x1,
    UVHTTP_WS_OPCODE_BINARY = 0x2,
    UVHTTP_WS_OPCODE_CLOSE = 0x8,
    UVHTTP_WS_OPCODE_PING = 0x9,
    UVHTTP_WS_OPCODE_PONG = 0xA
} uvhttp_ws_opcode_t;
```

#### WebSocket 处理器
```c
typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg);
    void* user_data;
} uvhttp_ws_handler_t;
```

### 使用示例

#### 基本 WebSocket 服务器
```c
void on_ws_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    if (opcode == UVHTTP_WS_OPCODE_TEXT) {
        // 回显消息
        uvhttp_server_ws_send(ws_conn, data, len);
    }
}

void on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket 连接关闭\n");
}

void on_ws_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    fprintf(stderr, "WebSocket 错误: %s\n", error_msg);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    uvhttp_ws_handler_t ws_handler = {
        .on_connect = NULL,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .on_error = on_ws_error,
        .user_data = NULL
    };

    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

---

## 错误处理 API

### 错误码

```c
typedef enum {
    UVHTTP_OK = 0,

    /* General errors */
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NOT_FOUND = -3,
    UVHTTP_ERROR_ALREADY_EXISTS = -4,
    UVHTTP_ERROR_NULL_POINTER = -5,
    UVHTTP_ERROR_BUFFER_TOO_SMALL = -6,
    UVHTTP_ERROR_TIMEOUT = -7,
    UVHTTP_ERROR_CANCELLED = -8,

    /* Server errors */
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_SERVER_LISTEN = -101,
    UVHTTP_ERROR_SERVER_STOP = -102,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,
    UVHTTP_ERROR_SERVER_ALREADY_RUNNING = -104,
    UVHTTP_ERROR_SERVER_NOT_RUNNING = -105,
    UVHTTP_ERROR_SERVER_INVALID_CONFIG = -106,

    /* Connection errors */
    UVHTTP_ERROR_CONNECTION_INIT = -200,
    UVHTTP_ERROR_CONNECTION_ACCEPT = -201,
    UVHTTP_ERROR_CONNECTION_START = -202,
    UVHTTP_ERROR_CONNECTION_CLOSE = -203,
    UVHTTP_ERROR_CONNECTION_RESET = -204,
    UVHTTP_ERROR_CONNECTION_TIMEOUT = -205,
    UVHTTP_ERROR_CONNECTION_REFUSED = -206,
    UVHTTP_ERROR_CONNECTION_BROKEN = -207,

    /* Request/Response errors */
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_INIT = -301,
    UVHTTP_ERROR_RESPONSE_SEND = -302,
    UVHTTP_ERROR_INVALID_HTTP_METHOD = -303,
    UVHTTP_ERROR_INVALID_HTTP_VERSION = -304,
    UVHTTP_ERROR_HEADER_TOO_LARGE = -305,
    UVHTTP_ERROR_BODY_TOO_LARGE = -306,
    UVHTTP_ERROR_MALFORMED_REQUEST = -307,

    /* TLS errors */
    UVHTTP_ERROR_TLS_INIT = -400,
    UVHTTP_ERROR_TLS_CONTEXT = -401,
    UVHTTP_ERROR_TLS_HANDSHAKE = -402,
    UVHTTP_ERROR_TLS_CERT_LOAD = -403,
    UVHTTP_ERROR_TLS_KEY_LOAD = -404,
    UVHTTP_ERROR_TLS_VERIFY_FAILED = -405,
    UVHTTP_ERROR_TLS_EXPIRED = -406,
    UVHTTP_ERROR_TLS_NOT_YET_VALID = -407,

    /* Router errors */
    UVHTTP_ERROR_ROUTER_INIT = -500,
    UVHTTP_ERROR_ROUTER_ADD = -501,
    UVHTTP_ERROR_ROUTE_NOT_FOUND = -502,
    UVHTTP_ERROR_ROUTE_ALREADY_EXISTS = -503,
    UVHTTP_ERROR_INVALID_ROUTE_PATTERN = -504,

    /* Rate limit errors */
    UVHTTP_ERROR_RATE_LIMIT_EXCEEDED = -550,

    /* Allocator errors */
    UVHTTP_ERROR_ALLOCATOR_INIT = -600,
    UVHTTP_ERROR_ALLOCATOR_SET = -601,
    UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED = -602,

    /* WebSocket errors */
    UVHTTP_ERROR_WEBSOCKET_INIT = -700,
    UVHTTP_ERROR_WEBSOCKET_HANDSHAKE = -701,
    UVHTTP_ERROR_WEBSOCKET_FRAME = -702,
    UVHTTP_ERROR_WEBSOCKET_TOO_LARGE = -703,
    UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE = -704,
    UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED = -705,
    UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED = -706,
    UVHTTP_ERROR_WEBSOCKET_CLOSED = -707,

    /* HTTP/2 errors */
    UVHTTP_ERROR_HTTP2_INIT = -800,
    UVHTTP_ERROR_HTTP2_STREAM = -801,
    UVHTTP_ERROR_HTTP2_SETTINGS = -802,
    UVHTTP_ERROR_HTTP2_FLOW_CONTROL = -803,
    UVHTTP_ERROR_HTTP2_HEADER_COMPRESS = -804,
    UVHTTP_ERROR_HTTP2_PRIORITY = -805,

    /* Configuration errors */
    UVHTTP_ERROR_CONFIG_PARSE = -900,
    UVHTTP_ERROR_CONFIG_INVALID = -901,
    UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND = -902,
    UVHTTP_ERROR_CONFIG_MISSING_REQUIRED = -903,

    /* Middleware errors */
    UVHTTP_ERROR_MIDDLEWARE_INIT = -1000,
    UVHTTP_ERROR_MIDDLEWARE_REGISTER = -1001,
    UVHTTP_ERROR_MIDDLEWARE_EXECUTE = -1002,
    UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND = -1003,

    /* Logging errors */
    UVHTTP_ERROR_LOG_INIT = -1100,
    UVHTTP_ERROR_LOG_WRITE = -1101,
    UVHTTP_ERROR_LOG_FILE_OPEN = -1102,
    UVHTTP_ERROR_LOG_NOT_INITIALIZED = -1103,

    UVHTTP_ERROR_MAX
} uvhttp_error_t;
```

### 错误查询

#### uvhttp_error_string
```c
const char* uvhttp_error_string(uvhttp_error_t error);
```
获取错误名称字符串。

**参数:**
- `error`: 错误码

**返回值:**
- 错误名称字符串

---

#### uvhttp_error_category_string
```c
const char* uvhttp_error_category_string(uvhttp_error_t error);
```
获取错误分类字符串。

**参数:**
- `error`: 错误码

**返回值:**
- 错误分类字符串（如 "Server", "Connection", "Request" 等）

---

#### uvhttp_error_description
```c
const char* uvhttp_error_description(uvhttp_error_t error);
```
获取错误描述。

**参数:**
- `error`: 错误码

**返回值:**
- 错误描述字符串

---

#### uvhttp_error_suggestion
```c
const char* uvhttp_error_suggestion(uvhttp_error_t error);
```
获取修复建议。

**参数:**
- `error`: 错误码

**返回值:**
- 修复建议字符串

---

#### uvhttp_error_is_recoverable
```c
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```
检查错误是否可恢复。

**参数:**
- `error`: 错误码

**返回值:**
- 1: 可恢复
- 0: 不可恢复

---

### 错误恢复和重试

#### uvhttp_set_error_recovery_config
```c
void uvhttp_set_error_recovery_config(int max_retries,
                                     int base_delay_ms,
                                     int max_delay_ms,
                                     double backoff_multiplier);
```
配置错误重试策略。

**参数:**
- `max_retries`: 最大重试次数
- `base_delay_ms`: 基础延迟（毫秒）
- `max_delay_ms`: 最大延迟（毫秒）
- `backoff_multiplier`: 退避乘数

---

#### uvhttp_retry_operation
```c
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*),
                                     void* context,
                                     const char* operation_name);
```
执行带重试的操作。

**参数:**
- `operation`: 操作函数
- `context`: 上下文数据
- `operation_name`: 操作名称（用于日志）

**返回值:**
- 成功: UVHTTP_OK
- 失败: 错误码

---

### 错误日志和统计

#### uvhttp_log_error
```c
void uvhttp_log_error(uvhttp_error_t error, const char* context);
```
记录错误日志。

**参数:**
- `error`: 错误码
- `context`: 上下文信息

---

#### uvhttp_get_error_stats
```c
void uvhttp_get_error_stats(uvhttp_context_t* context,
                           size_t* error_counts,
                           time_t* last_error_time,
                           const char** last_error_context);
```
获取错误统计信息。

**参数:**
- `context`: 应用上下文
- `error_counts`: 错误计数数组（输出）
- `last_error_time`: 最后错误时间（输出）
- `last_error_context`: 最后错误上下文（输出）

---

#### uvhttp_reset_error_stats
```c
void uvhttp_reset_error_stats(uvhttp_context_t* context);
```
重置错误统计信息。

**参数:**
- `context`: 应用上下文

---

#### uvhttp_get_most_frequent_error
```c
uvhttp_error_t uvhttp_get_most_frequent_error(uvhttp_context_t* context);
```
获取最频繁的错误。

**参数:**
- `context`: 应用上下文

**返回值:**
- 最频繁的错误码

---

### 使用示例

#### 基本错误处理
```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "分类: %s\n", uvhttp_error_category_string(result));
    fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "建议: %s\n", uvhttp_error_suggestion(result));
    return 1;
}
```

#### 错误恢复
```c
uvhttp_set_error_recovery_config(3, 100, 5000, 2.0);

uvhttp_error_t connect_operation(void* ctx) {
    return uvhttp_server_connect((uvhttp_server_t*)ctx);
}

uvhttp_error_t result = uvhttp_retry_operation(connect_operation, server, "连接服务器");
if (result != UVHTTP_OK) {
    fprintf(stderr, "连接失败，已重试多次\n");
}
```

#### 错误统计
```c
size_t error_counts[UVHTTP_ERROR_COUNT] = {0};
time_t last_error_time = 0;
const char* last_error_context = NULL;

uvhttp_get_error_stats(context, error_counts, &last_error_time, &last_error_context);

uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error(context);
printf("最频繁的错误: %s (%zu 次)\n",
       uvhttp_error_string(most_frequent),
       error_counts[most_frequent]);
```

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
int uvhttp_config_update_max_connections(uvhttp_context_t* context, int max_connections);
```
动态更新最大并发连接数。

**参数:**
- `context`: 应用上下文
- `max_connections`: 新的最大连接数 (1-10000)

**返回值:**
- `UVHTTP_OK`: 更新成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数超出范围
- `UVHTTP_ERROR_INVALID_STATE`: 配置未初始化

**示例:**
```c
// 运行时动态调整连接限制
int result = uvhttp_config_update_max_connections(context, 5000);
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
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);
```
获取当前全局配置实例。

**参数:**
- `context`: 应用上下文

**返回值:**
- 成功: 配置实例指针
- 失败: NULL

#### uvhttp_config_monitor_changes
```c
int uvhttp_config_monitor_changes(uvhttp_context_t* context, uvhttp_config_change_callback_t callback);
```
监控配置变化。

**参数:**
- `context`: 应用上下文
- `callback`: 配置变化回调函数

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

uvhttp_config_monitor_changes(context, on_config_change);
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
    uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
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

### 哈希函数

#### uvhttp_hash
```c
uint64_t uvhttp_hash(const void* data, size_t length, uint64_t seed);
```
计算数据的64位哈希值（基于xxHash算法）。

**参数:**
- `data`: 要哈希的数据
- `length`: 数据长度
- `seed`: 哈希种子

**返回值:**
- 64位哈希值

**示例:**
```c
const char* data = "Hello, World!";
uint64_t hash = uvhttp_hash(data, strlen(data), 0x12345678);
```

---

#### uvhttp_hash_string
```c
uint64_t uvhttp_hash_string(const char* str);
```
计算字符串的64位哈希值（使用默认种子）。

**参数:**
- `str`: 要哈希的字符串

**返回值:**
- 64位哈希值

**示例:**
```c
uint64_t hash = uvhttp_hash_string("user_session_token");
```

---

#### uvhttp_hash_default
```c
uint64_t uvhttp_hash_default(const void* data, size_t length);
```
使用默认种子计算数据的哈希值。

**参数:**
- `data`: 要哈希的数据
- `length`: 数据长度

**返回值:**
- 64位哈希值

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

### 网络和服务器配置

```c
#define UVHTTP_DEFAULT_PORT              8080
#define UVHTTP_DEFAULT_HOST              "0.0.0.0"
```

### 缓冲区大小配置

```c
#define UVHTTP_DIR_LISTING_BUFFER_SIZE   4096
#define UVHTTP_DIR_ENTRY_HTML_OVERHEAD   200
#define UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN 256
```

### 连接和池配置

```c
#define UVHTTP_DEFAULT_CONNECTION_POOL_SIZE 100
#define UVHTTP_ROUTER_MAX_CHILDREN       16
```

### WebSocket配置

```c
#define UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE 1024
```

### 错误恢复配置

```c
#define UVHTTP_DEFAULT_BASE_DELAY_MS     100
#define UVHTTP_DEFAULT_MAX_DELAY_MS      5000
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
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    
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
#define UVHTTP_VERSION_MAJOR    2
#define UVHTTP_VERSION_MINOR    0
#define UVHTTP_VERSION_PATCH    0
#define UVHTTP_VERSION_STRING   "2.0.0"
```