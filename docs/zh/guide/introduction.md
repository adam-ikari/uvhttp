---
title: 简介
sync_hash: 4582a5a23dbdba1621fdb667c665ba2fde61b1e2
description: UVHTTP 是基于 libuv 的 HTTP/1.1 与 WebSocket 服务器库，C99 编写。ASan/UBSan 验证内存安全，支持 32 位嵌入式，零拷贝，模块化特性。
---

# UVHTTP 简介

UVHTTP 是基于 libuv 的 HTTP/1.1 与 WebSocket 服务器库，面向现代 C 应用。

## 什么是 UVHTTP？

- **性能**：峰值约 20K RPS，亚毫秒级延迟
- **资源**：零拷贝优化，低内存与 CPU 占用
- **架构**：64 位与 32 位嵌入式系统
- **API**：简洁直观，错误处理完善
- **质量**：零编译警告，91 项测试通过，ASan/UBSan 验证

### 核心理念

1. **聚焦协议**：HTTP/1.1 与 WebSocket 处理，不约束业务逻辑
2. **零开销抽象**：编译期宏，运行时零成本
3. **极简工程**：消除不必要的复杂性
4. **测试分离**：生产代码不含测试代码或调试插桩
5. **零全局变量**：状态通过 libuv 数据指针管理，支持多实例
6. **错误处理**：统一错误系统，提供诊断与恢复指引

---

## 架构概览

### 事件驱动设计

UVHTTP 借助 libuv 的事件驱动架构实现高并发：

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
| **Router** | O(1) 前缀匹配，快速路由解析 |
| **Connection** | TCP 连接管理，支持 Keep-Alive |
| **Request/Response** | HTTP 消息解析与生成 |
| **WebSocket** | 基于 HTTP 升级的全双工通信 |
| **Static Files** | 零拷贝文件服务，带缓存 |
| **TLS** | 基于 mbedtls 的加密层 |

---

## 平台支持

| 平台 | 状态 | 备注 |
|----------|--------|-------|
| **Linux x86_64** | 稳定 | 主要平台 |
| **Linux x86（32 位）** | 稳定 | 嵌入式优化 |
| **macOS** | 已规划 | |
| **Windows** | 已规划 | |
| **FreeBSD** | 已规划 | 社区需求 |

### 跨平台考量

- **C99 标准**：无需编译器专用扩展
- **自包含依赖**：外部库以子模块形式内置
- **条件编译**：平台专用代码隔离在特性宏之后
- **POSIX**：在可用处复用 POSIX API

---

## 性能特征

### 基准测试（v2.5.0）

```yaml
配置:
  - Server: 4-core CPU, 16GB RAM
  - Client: wrk, 4 threads, 100 connections
  - Duration: 30 seconds

HTTP/1.1:
  低并发:
    - Throughput: ~20,000 RPS
    - Latency: ~0.4 ms avg (P50)
    - Error Rate: 0.00%

  高并发:
    - Throughput: ~17,000-19,000 RPS
    - Latency: ~9 ms avg (P50), ~58 ms (P99)
    - Error Rate: 0.00%

  静态文件 (1MB):
    - Throughput: 12,510 RPS
    - Latency: 15.3 ms avg
    - Zero-Copy: Enabled (sendfile)

API 路由:
  - Throughput: 13,950 RPS
  - Route Lookup: O(1)
  - Middleware Overhead: < 10μs
```

### 性能优化

1. **Keep-Alive**：连接复用
2. **TCP 优化**：默认启用 TCP_NODELAY 与 TCP_KEEPALIVE
3. **路由优化**：O(1) 前缀匹配
4. **内存分配**：可选 mimalloc
5. **直接调用 libuv**：无抽象层
6. **LRU 缓存**：静态文件自动缓存
7. **零拷贝 I/O**：sendfile 大文件传输

---

## 安全特性

### 内置安全

- **缓冲区溢出防护**：边界检查
- **输入校验**：HTTP 头部与请求数据
- **TLS 1.3**：mbedtls 集成
- **内存安全**：AddressSanitizer 与 Valgrind 兼容
- **错误处理**：不泄露敏感信息
- **资源限制**：连接数、头部数量、请求体大小上限

### 安全示例

```c
// 启用 TLS
uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
uvhttp_tls_context_load_cert(tls_ctx, "server.crt", "server.key");
server->tls_ctx = tls_ctx;

// 设置资源限制
server->max_connections = 1000;
server->max_headers = 100;
server->max_body_size = 10 * 1024 * 1024; // 10MB
```

---

## 文档结构

### 用户指南
- **[快速开始](getting-started.md)**：5 分钟运行
- **[安装](installation.md)**：安装说明
- **[第一个服务器](first-server.md)**：构建 HTTP 服务器
- **[WebSocket](websocket.md)**：实时通信

### 核心概念
- **[教程](TUTORIAL.md)**：渐进式学习
- **[libuv 数据指针](LIBUV_DATA_POINTER.md)**：上下文传递
- **[中间件系统](MIDDLEWARE_SYSTEM.md)**：请求/响应流水线
- **[统一响应指南](UNIFIED_RESPONSE_GUIDE.md)**：标准响应模式

### 高级特性
- **[限流 API](RATE_LIMIT_API.md)**：令牌桶限流
- **[静态文件服务器](STATIC_FILE_SERVER.md)**：文件服务
- **[压缩](../dev/COMPRESSION_FEATURE_REPORT.md)**：零开销压缩

### 开发者资源
- **[开发者指南](DEVELOPER_GUIDE.md)**：开发实践
- **[CMake 配置](CMAKE_CONFIGURATION.md)**：构建系统定制
- **[API 参考](../api/introduction)**：完整 API 文档

---

## 快速上手

```c
#include <uvhttp.h>
#include <uv.h>
#include <string.h>

int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.5.0");

    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.5.0\"}";
    uvhttp_response_set_body(res, body, strlen(body));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);

    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_set_router(server, router);

    uvhttp_router_add_route(router, "/hello", hello_handler);

    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

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

## 社区与支持

### 贡献

在提交 Pull Request 之前，请先阅读[贡献指南](../../CONTRIBUTING.md)。

### 获取帮助

- **GitHub Issues**：报告 Bug 与功能需求
- **Discussions**：提问与分享想法
- **文档**：指南与 API 参考

### 许可证

MIT 许可证，详见 [LICENSE](../../LICENSE)。

---

## 兼容性

### 平台支持

| 平台 | 版本 | 状态 |
|----------|---------|--------|
| Linux x86_64 | 2.2.0+ | ✅ 稳定 |
| Linux i386 | 2.2.0+ | ✅ 稳定 |
| macOS x86_64 | 2.2.0+ | ✅ 稳定 |
| macOS ARM64 | 2.2.0+ | ✅ 稳定 |
| Windows x86_64 | 2.2.0+ | ⚠️ 实验性 |

### 编译器支持

| 编译器 | 版本 | 状态 |
|----------|---------|--------|
| GCC | 4.8+ | ✅ 稳定 |
| Clang | 3.4+ | ✅ 稳定 |
| MSVC | 2019+ | ⚠️ 实验性 |

### 依赖版本

| 依赖 | 版本 | 状态 |
|------|------|------|
| libuv | 1.44.0+ | ✅ 必需 |
| llhttp | 8.1.0+ | ✅ 必需 |
| mbedtls | 3.0.0+ | ✅ 可选（TLS） |
| mimalloc | 2.0.0+ | ✅ 可选（分配器） |
| cjson | 1.7.0+ | ✅ 可选（JSON） |

---

## 下一步

- **[快速开始指南](getting-started.md)**：构建第一个服务器
- **[API 参考](../api/introduction)**：探索完整 API
- **[示例](../../examples/)**：浏览实用示例
- **[性能基准](../performance.md)**：了解性能特征
