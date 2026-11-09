# UVHTTP 常见问题解答 (FAQ)

下面按安装、服务器、请求、响应、TLS、WebSocket、性能的顺序排，每条配一段最小代码。

## 目录

1. [安装和设置](#安装和设置)
2. [服务器管理](#服务器管理)
3. [请求处理](#请求处理)
4. [响应处理](#响应处理)
5. [TLS/SSL 配置](#tls-ssl-配置)
6. [WebSocket](#websocket)
7. [性能优化](#性能优化)
8. [常见问题](#常见问题)

---

## 安装和设置

### Q1: UVHTTP 的系统要求是什么？

- **操作系统**: Linux, macOS, Windows
- **编译器**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **构建系统**: CMake 3.13+
- **依赖**: libuv (已包含), llhttp (已包含)

**构建命令：**
```bash
make build
```

### Q2: 如何启用可选功能如 WebSocket？

可选项在配置阶段定下来，改的是 `CMakeLists.txt` 里的 `option()` 默认值。改完重新跑 `make build`，配置阶段会读新的值。

```bash
# 启用 WebSocket — 在 CMakeLists.txt 中将 BUILD_WITH_WEBSOCKET 设为 ON
# 启用 mimalloc 分配器 — 在 CMakeLists.txt 中将 BUILD_WITH_MIMALLOC 设为 ON
# 启用所有功能 — 在 CMakeLists.txt 中将相应选项设为 ON
make build
```

### Q3: 如何在系统分配器和 mimalloc 之间切换？

分配器类型是编译期选项，改 `CMakeLists.txt` 里 `UVHTTP_ALLOCATOR_TYPE` 的值，再跑一次 `make build`。这个值不会在运行时切换，换分配器就得重编：

```bash
# 系统分配器（默认）— UVHTTP_ALLOCATOR_TYPE=0
# mimalloc 分配器 — UVHTTP_ALLOCATOR_TYPE=1
make build
```

---

## 服务器管理

### Q4: 如何优雅地关闭服务器？

信号处理器只做一件事：让事件循环停下来。uv_run 返回后再依次调用停止和释放接口，进程不会带着半开的连接退出。在信号处理器里直接退出会跳过后面这段清理。

```c
#include <signal.h>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
    uv_stop(loop);  // 停止事件循环
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_error_t r = uvhttp_server_new(loop, &server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "创建服务器失败: %s\n", uvhttp_error_string(r));
        return 1;
    }
    // ... 配置服务器 ...

    uvhttp_server_listen(server, "0.0.0.0", 8080);

    // 运行直到收到信号
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理
    uvhttp_server_stop(server);
    uvhttp_server_free(server);

    return 0;
}
```

### Q5: 如何更改服务器的地址绑定？

第二个参数决定监听范围。绑回环地址只收本机流量，绑所有接口则对同网段可见：

```c
// 绑定到特定 IP
uvhttp_server_listen(server, "192.168.1.100", 8080);

// 绑定到所有接口
uvhttp_server_listen(server, "0.0.0.0", 8080);

// 仅绑定本地回环
uvhttp_server_listen(server, "127.0.0.1", 8080);
```

### Q6: 如何在同一循环上运行多个服务器？

每个 server 有自己的监听句柄，共用一个事件循环。回调里靠数据指针分辨请求来自哪个实例：

```c
uv_loop_t* loop = uv_default_loop();

// 创建多个服务器
uvhttp_server_t* server1 = NULL;
uvhttp_server_t* server2 = NULL;
uvhttp_server_new(loop, &server1);
uvhttp_server_new(loop, &server2);

// 配置每个服务器
uvhttp_server_listen(server1, "0.0.0.0", 8080);
uvhttp_server_listen(server2, "0.0.0.0", 8081);

// 所有服务器共享同一个事件循环
uv_run(loop, UV_RUN_DEFAULT);
```

---

## 请求处理

### Q7: 如何访问请求头？

```c
// 获取特定请求头
const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
const char* content_type = uvhttp_request_get_header(request, "Content-Type");

// 遍历所有请求头
void header_callback(const char* name, const char* value, void* user_data) {
    printf("请求头: %s: %s\n", name, value);
}
uvhttp_request_foreach_header(request, header_callback, NULL);
```

### Q8: 如何获取客户端 IP 地址？

```c
const char* client_ip = uvhttp_request_get_client_ip(request);
```

### Q9: 如何处理请求体？

请求体指针和长度分开取。长度必须单独读，body 里有空字符时字符串函数会提前截断：

```c
// 获取请求体
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);

// 获取 Content-Length 头
const char* content_length = uvhttp_request_get_header(request, "Content-Length");
```

### Q10: 如何处理查询参数？

```c
// 获取所有查询参数
const char* query = uvhttp_request_get_query_string(request);

// 获取特定查询参数
const char* search = uvhttp_request_get_query_param(request, "q");
const char* page = uvhttp_request_get_query_param(request, "page");
```

---

## 响应处理

### Q11: 如何在响应中设置 Cookie？

```c
uvhttp_response_set_header(response, "Set-Cookie",
    "session=abc123; Path=/; HttpOnly; Secure; SameSite=Strict");
uvhttp_response_set_header(response, "Set-Cookie",
    "theme=dark; Path=/; Max-Age=31536000");
```

### Q12: 如何启用 CORS？

CORS 在应用层做：设置几个响应头，再拦下预检请求直接回状态码，不进业务处理器。

```c
uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");

// 处理预检请求
if (uvhttp_request_get_method(request) == UVHTTP_METHOD_OPTIONS) {
    uvhttp_response_set_status(response, 204);
    uvhttp_response_send(response);
    return 0;
}
```

要在多个服务间复用，可以写成中间件，`examples/03_middleware/` 里有现成的写法。

**注意：** UVHTTP 不内置 CORS。放行哪些来源、要不要带凭据，取决于你的应用，库替不了这个决定。

### Q13: 如何发送文件附件？

```c
uvhttp_response_set_header(response, "Content-Disposition",
    "attachment; filename=\"example.txt\"");

const char* file_content = "文件内容...";
uvhttp_response_set_body(response, file_content, strlen(file_content));
uvhttp_response_send(response);
```

### Q14: 如何处理大响应？

```c
const char* large_data = get_large_data();
size_t data_length = get_data_length();

uvhttp_response_set_status(response, 200);
uvhttp_response_set_body(response, large_data, data_length);
uvhttp_response_send(response);
```

---

## TLS/SSL 配置

### Q15: 如何为测试生成自签名证书？

```bash
# 生成 CA 私钥
openssl genrsa -out ca.key 4096

# 生成 CA 证书
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=MyCA"

# 生成服务器私钥
openssl genrsa -out server.key 2048

# 生成 CSR
openssl req -new -key server.key -out server.csr \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"

# 使用 CA 签名证书
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 365 \
  -extfile <(echo "subjectAltName=DNS:localhost,IP:127.0.0.1")
```

### Q16: 如何启用客户端证书认证？

```c
// 启用客户端认证
uvhttp_tls_context_enable_client_auth(tls_ctx, 1);

// 设置验证深度
uvhttp_tls_context_set_verify_depth(tls_ctx, 3);

// 加载客户端 CA 证书
uvhttp_tls_context_load_ca_file(tls_ctx, "client_ca.crt");
```

### Q17: 如何配置密码套件？

```c
static const int cipher_suites[] = {
    MBEDTLS_TLS_AES_256_GCM_SHA384,
    MBEDTLS_TLS_CHACHA20_POLY1305_SHA256,
    MBEDTLS_TLS_AES_128_GCM_SHA256,
    0
};

uvhttp_tls_context_set_cipher_suites(tls_ctx, cipher_suites);
```

### Q18: 如何启用 TLS 1.3？

```c
uvhttp_tls_context_enable_tls13(tls_ctx, 1);
```

---

## WebSocket

### Q19: 如何向所有连接的 WebSocket 客户端发送消息？

```c
uvhttp_server_ws_broadcast(server, "/ws", "大家好", 9);
```

### Q20: 如何处理 WebSocket ping/pong？

```c
// 发送 ping
uvhttp_ws_send_ping(context, ws_conn, (const uint8_t*)"心跳", 6);

// 在回调中处理 pong
int on_message(uvhttp_ws_connection_t* ws_conn, const char* data,
               size_t len, int opcode) {
    if (opcode == UVHTTP_WS_OPCODE_PONG) {
        printf("收到 pong: %s\n", data);
    }
    return 0;
}
```

---

## 性能优化

### Q21: 如何启用连接池？

```c
// Keep-Alive 默认开启。空闲超时由 connection_timeout 决定（秒，5-300）；
// 原来的 keepalive_timeout / request_timeout 字段在 v3.0.0 被删除，
// 因为没有任何代码路径读取它们。
uvhttp_config_t* config = NULL;
uvhttp_config_new(&config);
uvhttp_config_set_defaults(config);
config->connection_timeout = 60;  // 60 秒

uvhttp_server_t* server = NULL;
uvhttp_server_new(loop, &server);
server->config = config;
```

### Q22: 如何优化静态文件服务？

```c
// 为静态文件启用 LRU 缓存
// 使用 uvhttp_static 时自动启用

// 启动时预热缓存
uvhttp_static_config_t config = {0};
strncpy(config.root_directory, "/var/www/static", sizeof(config.root_directory) - 1);
uvhttp_static_context_t* static_ctx = NULL;
uvhttp_static_create(&config, &static_ctx);
uvhttp_static_prewarm_directory(static_ctx, "/var/www/static", 100);

// 大文件（>1MB）自动使用 sendfile
```

### Q23: 如何监控服务器性能？

```c
size_t active_connections = server->active_connections;
printf("活动连接数: %zu\n", active_connections);
```

### Q24: 如何实现速率限制？

库自身不再内置限流。它在分派每个请求之前调用你的过滤器，计数策略完全由你掌握：

```c
static uvhttp_error_t limit_filter(uvhttp_request_t* request,
                                   uvhttp_response_t* response,
                                   void* user_data) {
    if (client_is_over_budget(user_data,
                              uvhttp_request_get_client_ip(request))) {
        uvhttp_response_set_status(response, 429);
        uvhttp_response_set_header(response, "Retry-After", "1");
        uvhttp_response_send(response);
        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
    }
    return UVHTTP_OK;
}

uvhttp_server_set_request_filter(server, limit_filter, &my_state);
```

按客户端的完整固定窗口实现（含表容量上限与淘汰）见
[请求过滤器 API](REQUEST_FILTER_API.md)。

---

## 常见问题

### Q25: 服务器启动失败，提示"地址已被使用"

```bash
# 查找使用端口的进程
lsof -i :8080
# 或
netstat -tlnp | grep 8080

# 终止进程
kill -9 <PID>

# 或使用不同的端口
uvhttp_server_listen(server, "0.0.0.0", 8081);
```

### Q26: 连接超时错误

```c
// 调大空闲超时（connection_timeout，合法范围 5-300 秒）。
// 没有独立的 request_timeout / keepalive_timeout —— v3.0.0 删除了这两个字段，
// 连接计时器读的是 connection_timeout。
uvhttp_config_t* config = NULL;
uvhttp_config_new(&config);
uvhttp_config_set_defaults(config);
config->connection_timeout = 300;  // 5 分钟

uvhttp_server_t* server = NULL;
uvhttp_server_new(loop, &server);
server->config = config;
```

### Q27: 内存使用持续增长

```bash
# 检查内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./your_server

# 或使用 AddressSanitizer 构建 — 在 CMakeLists.txt 中将 ENABLE_DEBUG 和 ENABLE_ASAN 设为 ON，然后运行：
make build
./your_server
```

### Q28: CPU 使用率过高

```c
// 检查忙等待循环
// 确保使用 UV_RUN_DEFAULT，而不是在紧密循环中使用 UV_RUN_NOWAIT

// 禁用不必要的日志
#define UVHTTP_FEATURE_LOGGING 0

// 减少轮询频率
// 检查连接超时并相应调整
```

### Q29: TLS 握手失败，提示"证书验证失败"

```c
// 为测试禁用证书验证（不推荐用于生产环境）
uvhttp_tls_context_enable_client_auth(tls_ctx, 0);

// 对于生产环境，确保：
// 1. 服务器证书有效
// 2. CA 证书已加载
// 3. 证书链完整
// 4. 证书未过期
// 5. 通用名称（CN）与主机名匹配
```

### Q30: 如何启用调试日志？

```bash
# 1. 在 include/uvhttp_features.h 中启用日志
#define UVHTTP_FEATURE_LOGGING 1

# 2. 以 Debug 模式构建 — 在 CMakeLists.txt 中将 ENABLE_DEBUG 设为 ON，然后运行：
make build

# 3. 运行并查看调试输出
./your_server
```

---

## 最佳实践

### 安全性
- 线上流量走 HTTPS
- 所有用户输入都校验长度和内容
- 限流放在请求过滤器里，库不再提供
- 依赖跟着上游更新
- 密码套件只用安全的

### 性能
- 打开 Keep-Alive
- 复用连接，别每次请求都新建
- 静态文件走缓存
- 盯住 CPU 和内存的占用
- 先用性能分析找到瓶颈，再改代码

### 可靠性
- 实现优雅关闭
- 每个失败分支都要处理
- 关键事件写进日志
- 上线前跑负载测试
- 监控进程是否还活着

---

## 相关文档

- [API 参考](/api/API_REFERENCE.md)
- [贡献者指南](/zh/guide/DEVELOPER_GUIDE.md)
- [教程](/zh/guide/TUTORIAL.md)

---

## 版本

- **文档版本**: 1.0.0
- **最后更新**: 2026-10-30
- **UVHTTP 版本**: 3.0.0
