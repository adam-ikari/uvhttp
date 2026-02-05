# UVHTTP 常见问题解答 (FAQ)

本文档回答了关于使用 UVHTTP 库的常见问题。

## 目录

1. [安装和设置](#安装和设置)
2. [服务器管理](#服务器管理)
3. [请求处理](#请求处理)
4. [响应处理](#响应处理)
5. [TLS/SSL 配置](#tlsssl-配置)
6. [WebSocket](#websocket)
7. [性能优化](#性能优化)
8. [常见问题](#常见问题)

---

## 安装和设置

### Q1: UVHTTP 的系统要求是什么？

**回答：**
- **操作系统**: Linux, macOS, Windows
- **编译器**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **构建系统**: CMake 3.10+
- **依赖**: libuv (已包含), llhttp (已包含)

**构建命令：**
```bash
mkdir build && cd build
cmake ..
make
```

### Q2: 如何启用可选功能如 WebSocket？

**回答：**
使用 CMake 选项配置构建：

```bash
# 启用 WebSocket
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 启用 mimalloc 分配器
cmake -DBUILD_WITH_MIMALLOC=ON ..

# 启用所有功能
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

### Q3: 如何在系统分配器和 mimalloc 之间切换？

**回答：**
使用编译时的分配器类型标志：

```bash
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

---

## 服务器管理

### Q4: 如何优雅地关闭服务器？

**回答：**
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
    uvhttp_server_t* server = uvhttp_server_new(loop);
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

**回答：**
```c
// 绑定到特定 IP
uvhttp_server_listen(server, "192.168.1.100", 8080);

// 绑定到所有接口
uvhttp_server_listen(server, "0.0.0.0", 8080);

// 仅绑定本地回环
uvhttp_server_listen(server, "127.0.0.1", 8080);
```

### Q6: 如何在同一循环上运行多个服务器？

**回答：**
```c
uv_loop_t* loop = uv_default_loop();

// 创建多个服务器
uvhttp_server_t* server1 = uvhttp_server_new(loop);
uvhttp_server_t* server2 = uvhttp_server_new(loop);

// 配置每个服务器
uvhttp_server_listen(server1, "0.0.0.0", 8080);
uvhttp_server_listen(server2, "0.0.0.0", 8081);

// 所有服务器共享同一个事件循环
uv_run(loop, UV_RUN_DEFAULT);
```

---

## 请求处理

### Q7: 如何访问请求头？

**回答：**
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

**回答：**
```c
// 获取客户端 IP 地址
const char* client_ip = uvhttp_request_get_client_ip(request);
```

### Q9: 如何处理请求体？

**回答：**
```c
// 获取请求体
const char* body = uvhttp_request_get_body(request);
size_t body_len = uvhttp_request_get_body_length(request);

// 获取 Content-Length 头
const char* content_length = uvhttp_request_get_header(request, "Content-Length");

// 处理请求体...
```

### Q10: 如何处理查询参数？

**回答：**
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

**回答：**
```c
uvhttp_response_set_header(response, "Set-Cookie", 
    "session=abc123; Path=/; HttpOnly; Secure; SameSite=Strict");
uvhttp_response_set_header(response, "Set-Cookie", 
    "theme=dark; Path=/; Max-Age=31536000");
```

### Q12: 如何启用 CORS？

**回答：**
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

### Q13: 如何发送文件附件？

**回答：**
```c
uvhttp_response_set_header(response, "Content-Disposition", 
    "attachment; filename=\"example.txt\"");

const char* file_content = "文件内容...";
uvhttp_response_set_body(response, file_content, strlen(file_content));
uvhttp_response_send(response);
```

### Q14: 如何处理大响应？

**回答：**
```c
// 对于大响应，直接设置响应体
// UVHTTP 自动高效处理大响应
const char* large_data = get_large_data();  // 您的数据源
size_t data_length = get_data_length();

uvhttp_response_set_status(response, 200);
uvhttp_response_set_body(response, large_data, data_length);
uvhttp_response_send(response);
```

---

## TLS/SSL 配置

### Q15: 如何为测试生成自签名证书？

**回答：**
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

**回答：**
```c
// 启用客户端认证
uvhttp_tls_context_enable_client_auth(tls_ctx, 1);

// 设置验证深度
uvhttp_tls_context_set_verify_depth(tls_ctx, 3);

// 加载客户端 CA 证书
uvhttp_tls_context_load_ca_file(tls_ctx, "client_ca.crt");
```

### Q17: 如何配置密码套件？

**回答：**
```c
// 定义密码套件
static const int cipher_suites[] = {
    MBEDTLS_TLS_AES_256_GCM_SHA384,
    MBEDTLS_TLS_CHACHA20_POLY1305_SHA256,
    MBEDTLS_TLS_AES_128_GCM_SHA256,
    0  // 终止符
};

// 设置密码套件
uvhttp_tls_context_set_cipher_suites(tls_ctx, cipher_suites);
```

### Q18: 如何启用 TLS 1.3？

**回答：**
```c
// 启用 TLS 1.3
uvhttp_tls_context_enable_tls13(tls_ctx, 1);

// 设置最低 TLS 版本
// 启用 TLS 1.3 时会自动处理
```

---

## WebSocket

### Q19: 如何向所有连接的 WebSocket 客户端发送消息？

**回答：**
```c
// 向特定路径上的所有客户端广播消息
uvhttp_server_ws_broadcast(server, "/ws", "大家好", 9);
```

### Q20: 如何处理 WebSocket ping/pong？

**回答：**
```c
// 发送 ping（需要 context）
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

**回答：**
```c
// 通过设置配置启用 Keep-Alive
uvhttp_config_t* config = uvhttp_config_create();
config->keepalive_timeout = 60;  // 60 秒

uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_server_set_config(server, config);
```

### Q22: 如何优化静态文件服务？

**回答：**
```c
// 为静态文件启用 LRU 缓存
// 使用 uvhttp_static 时自动启用

// 启动时预热缓存
uvhttp_static_context_t* static_ctx = uvhttp_static_create("/var/www/static");
uvhttp_static_prewarm_directory(static_ctx, "/var/www/static", 100);

// 大文件（>1MB）自动使用 sendfile
// 无需额外配置
```

### Q23: 如何监控服务器性能？

**回答：**
```c
// 获取活动连接数
size_t active_connections = server->active_connections;

// 定期记录统计信息
printf("活动连接数: %zu\n", active_connections);
```

### Q24: 如何配置速率限制？

**回答：**
```c
// 在服务器上启用限流（每秒 1000 请求）
uvhttp_server_enable_rate_limit(server, 1000, 1);

// 添加 IP 到白名单（可选）
uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.100");
```

---

## 常见问题

### Q25: 服务器启动失败，提示"地址已被使用"

**回答：**
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

**回答：**
```c
// 增加超时值
uvhttp_config_t* config = uvhttp_config_create();
uvhttp_config_set_keepalive_timeout(config, 300);  // 5 分钟
uvhttp_config_set_request_timeout(config, 60);    // 1 分钟

uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_server_set_config(server, config);
```

### Q27: 内存使用持续增长

**回答：**
```bash
# 检查内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./your_server

# 或使用 AddressSanitizer 构建
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
make
./your_server
```

### Q28: CPU 使用率过高

**回答：**
```c
// 检查忙等待循环
// 确保使用 UV_RUN_DEFAULT，而不是在紧密循环中使用 UV_RUN_NOWAIT

// 禁用不必要的日志
#define UVHTTP_FEATURE_LOGGING 0

// 减少轮询频率
// 检查连接超时并相应调整
```

### Q29: TLS 握手失败，提示"证书验证失败"

**回答：**
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

**回答：**
```bash
# 1. 在 include/uvhttp_features.h 中启用日志
#define UVHTTP_FEATURE_LOGGING 1

# 2. 以 Debug 模式构建
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# 3. 运行并查看调试输出
./your_server
```

---

## 最佳实践

### 安全性
- 生产环境始终使用 HTTPS
- 验证所有用户输入
- 实现速率限制
- 保持依赖项更新
- 使用强密码套件

### 性能
- 启用 Keep-Alive 连接
- 使用连接池
- 为静态文件启用缓存
- 监控资源使用
- 分析性能瓶颈

### 可靠性
- 实现优雅关闭
- 添加适当的错误处理
- 记录重要事件
- 负载测试
- 监控服务器健康状况

---

## 相关文档

- [API 参考](api/API_REFERENCE.md)
- [贡献者指南](guide/DEVELOPER_GUIDE.md)
- [教程](guide/TUTORIAL.md)

---

## 版本

- **文档版本**: 1.0.0
- **最后更新**: 2026-02-03
- **UVHTTP 版本**: 2.2.0+