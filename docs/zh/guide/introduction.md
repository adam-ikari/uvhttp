# UVHTTP 简介

欢迎使用 UVHTTP，这是一个基于 libuv 构建的高性能 HTTP/1.1 与 WebSocket 服务器库，面向现代 C 应用。本套文档为构建可扩展、高效 Web 服务的开发者提供全面指引。

## 🎯 什么是 UVHTTP？

UVHTTP 是一个**生产级、事件驱动的 HTTP 服务器库**，专为有以下需求的开发者而设计：

- **卓越性能**：峰值吞吐量约 20K RPS，亚毫秒级延迟
- **资源高效**：通过零拷贝优化，实现极低的内存占用与 CPU 消耗
- **架构灵活**：同时支持 64 位与 32 位嵌入式系统
- **开发体验**：简洁直观的 API，配合完善的错误处理
- **生产就绪**：零编译警告，91 项测试全部通过，ASan/UBSan 验证通过，并以安全为先的设计理念

### 核心理念

UVHTTP 遵循以下基本原则：

1. **聚焦核心协议处理**：提供 HTTP/1.1 与 WebSocket 协议处理，不施加业务逻辑约束
2. **零开销抽象**：所有抽象均为编译期宏，运行时零成本
3. **极简工程**：在保持功能的前提下消除不必要的复杂性
4. **测试分离**：生产代码中不含任何测试专用代码或调试插桩
5. **零全局变量**：所有状态通过 libuv 数据指针管理，以支持多实例
6. **全面的错误处理**：统一的错误系统，提供详尽诊断与恢复指引

---

## 🏗️ 架构概览

### 事件驱动设计

UVHTTP 借助 libuv 的事件驱动架构，在不引入线程复杂性的前提下实现高并发：

```
┌─────────────────────────────────────────────────┐
│           Event Loop (libuv)                    │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐         │
│  │ Timer   │  │  I/O    │  │ Signal  │         │
│  │ Events  │  │ Events  │  │ Events  │         │
│  └────┬────┘  └────┬────┘  └────┬────┘         │
│       │            │            │                │
│  ┌────▼────────────▼────────────▼────┐          │
│  │    HTTP Request Handlers          │          │
│  │  - Route Matching                 │          │
│  │  - Middleware Processing          │          │
│  │  - Response Generation            │          │
│  └───────────────────────────────────┘          │
└─────────────────────────────────────────────────┘
```

### 关键组件

| 组件 | 职责 |
|-----------|---------------|
| **Server** | HTTP 服务器生命周期管理、连接池 |
| **Router** | O(1) 前缀匹配，实现快速路由解析 |
| **Connection** | TCP 连接管理，支持 Keep-Alive |
| **Request/Response** | HTTP 消息解析与生成 |
| **WebSocket** | 基于 HTTP 升级的全双工通信 |
| **Static Files** | 零拷贝文件服务，带缓存 |
| **TLS** | 基于 mbedtls 的加密层 |

---

## 🌍 平台支持

### 当前状态

| 平台 | 状态 | 备注 |
|----------|--------|-------|
| **Linux x86_64** | ✅ 完全支持 | 主要平台 |
| **Linux x86（32 位）** | ✅ 完全支持 | 面向嵌入式优化 |
| **macOS** | 🔨 开发中 | 测试阶段 |
| **Windows** | 📋 已规划 | 下一主要版本 |
| **FreeBSD** | 📋 已规划 | 社区需求 |
| **WebAssembly** | 📋 已规划 | 浏览器环境 |

### 跨平台考量

UVHTTP 在设计上注重可移植性：

- **符合标准 C11**：无需任何编译器专用扩展
- **自包含依赖**：所有外部库均以子模块形式内置
- **条件编译**：平台专用代码隔离在特性宏之后
- **符合 POSIX**：在可用处复用 POSIX API

---

## 🚀 性能特征

### 基准测试结果（v2.5.0）

```yaml
Configuration:
  - Server: 4-core CPU, 16GB RAM
  - Client: wrk, 4 threads, 100 connections
  - Test Duration: 30 seconds

HTTP/1.1 Benchmarks:
  Low Concurrency:
    - Throughput: ~20,000 RPS
    - Latency: ~0.4 ms avg (P50)
    - Error Rate: 0.00%
  
  High Concurrency:
    - Throughput: ~17,000-19,000 RPS
    - Latency: ~9 ms avg (P50), ~58 ms (P99)
    - Error Rate: 0.00%
  
  Static Files (1MB):
    - Throughput: 12,510 RPS
    - Latency: 15.3 ms avg
    - Zero-Copy: Enabled (sendfile)

API Routing:
  - Throughput: 13,950 RPS
  - Route Lookup: O(1) complexity
  - Middleware Overhead: < 10μs per request
```

### 性能优化

1. **Keep-Alive 连接**：通过连接复用实现约 1000 倍提升
2. **TCP 优化**：默认启用 TCP_NODELAY 与 TCP_KEEPALIVE
3. **路由优化**：O(1) 前缀匹配，消除线性搜索
4. **内存分配**：可选 mimalloc，带来更快的分配与更好的碎片控制
5. **直接调用 libuv**：应用与 libuv 之间零抽象层
6. **LRU 缓存**：自动缓存静态文件并预热
7. **零拷贝 I/O**：集成 sendfile，用于大文件传输

---

## 🔒 安全特性

### 内置安全

- **缓冲区溢出防护**：对所有缓冲区进行全面边界检查
- **输入校验**：对 HTTP 头部与请求数据进行严格校验
- **TLS 1.3 支持**：通过集成 mbedtls 实现现代加密
- **内存安全**：可选 AddressSanitizer 与 Valgrind 兼容
- **错误处理**：详尽的错误信息，且不泄露敏感信息
- **资源限制**：可配置连接数、头部数量与请求体大小上限

### 安全最佳实践

```c
// 示例：为生产部署启用 TLS
uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
uvhttp_tls_context_load_cert(tls_ctx, "server.crt", "server.key");
server->tls_ctx = tls_ctx;

// 示例：设置合理的资源限制
server->max_connections = 1000;
server->max_headers = 100;
server->max_body_size = 10 * 1024 * 1024; // 10MB
```

---

## 📚 文档结构

### 用户指南
- **[快速开始](getting-started.md)**：5 分钟即可运行
- **[安装](installation.md)**：详细的安装说明
- **[第一个服务器](first-server.md)**：构建你的第一个 HTTP 服务器
- **[WebSocket](websocket.md)**：实时通信配置

### 核心概念
- **[教程](TUTORIAL.md)**：从基础到进阶的渐进式学习
- **[libuv 数据指针](LIBUV_DATA_POINTER.md)**：理解上下文传递
- **[中间件系统](MIDDLEWARE_SYSTEM.md)**：请求/响应处理流水线
- **[统一响应指南](UNIFIED_RESPONSE_GUIDE.md)**：标准响应模式

### 高级特性
- **[限流 API](RATE_LIMIT_API.md)**：令牌桶限流
- **[静态文件服务器](STATIC_FILE_SERVER.md)**：高效文件服务
- **[压缩](../dev/COMPRESSION_FEATURE_REPORT.md)**：零开销压缩

### 开发者资源
- **[开发者指南](DEVELOPER_GUIDE.md)**：开发最佳实践
- **[CMake 配置](CMAKE_CONFIGURATION.md)**：构建系统定制
- **[API 参考](../api/introduction)**：完整的 API 文档

---

## 🛠️ 快速上手示例

```c
#include <uvhttp.h>
#include <uv.h>
#include <string.h>

// 请求处理函数
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 设置响应状态
    uvhttp_response_set_status(res, 200);
    
    // 设置响应头
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.5.0");
    
    // 设置响应体
    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.5.0\"}";
    uvhttp_response_set_body(res, body, strlen(body));
    
    // 发送响应
    return uvhttp_response_send(res);
}

int main() {
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    
    // 创建路由器
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_set_router(server, router);
    
    // 添加路由
    uvhttp_router_add_route(router, "/hello", hello_handler);
    
    // 开始监听
    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 运行事件循环
    printf("Server listening on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

**编译与运行**：
```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv
export LD_LIBRARY_PATH=./build/dist/lib:$LD_LIBRARY_PATH
./server
```

---

## 🤝 社区与支持

### 贡献

欢迎贡献代码！在提交 Pull Request 之前，请先阅读我们的[贡献指南](../../CONTRIBUTING.md)。

### 获取帮助

- **GitHub Issues**：报告 Bug 与提出功能需求
- **Discussions**：提问与分享想法
- **文档**：全面的指南与 API 参考

### 许可证

本项目基于 MIT 许可证发布，详见 [LICENSE](../../LICENSE) 文件。

---

## 📖 下一步

- **[快速开始指南](getting-started.md)**：开始构建你的第一个服务器
- **[API 参考](../api/introduction)**：探索完整 API
- **[示例](../../examples/)**：浏览实用示例
- **[性能基准](../performance.md)**：了解性能特征
