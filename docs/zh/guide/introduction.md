---
title: 简介
description: UVHTTP 是基于 libuv 的 HTTP/1.1 与 WebSocket 服务器库，C99 编写。ASan/UBSan 验证内存安全，支持 32 位嵌入式，零拷贝，模块化特性。
---

# UVHTTP 简介

UVHTTP 是基于 libuv 的 HTTP/1.1 与 WebSocket 服务器库，用标准 C 编写。协议解析、连接管理、路由分派都在库里完成，业务逻辑留给应用。

## 什么是 UVHTTP？

- **性能**：GitHub CI runner 上约 139K RPS，延迟在亚毫秒级（见[性能](performance.md)）
- **资源**：零拷贝优化，内存与 CPU 占用低
- **架构**：64 位与 32 位嵌入式系统
- **API**：路由、请求、响应各有独立对象，错误码统一
- **生产状态**：零编译警告，119/119 测试通过，ASan/UBSan 验证，默认值按安全优先设置

### 核心理念

1. **聚焦协议**：只处理 HTTP/1.1 与 WebSocket，业务逻辑不归它管
2. **零开销抽象**：开关是编译期宏，运行时没有额外成本
3. **极简工程**：用不上的复杂度直接删掉
4. **测试分离**：生产代码不含测试代码，也不含调试插桩
5. **零全局变量**：状态挂在 libuv 数据指针上，同进程可跑多个实例
6. **错误处理**：统一错误系统，出错时给出诊断和恢复方向

---

## 架构概览

### 事件驱动设计

UVHTTP 跑在 libuv 的事件循环上：定时器、I/O 和信号事件汇到同一处，再交给请求处理器完成路由匹配、中间件处理和响应生成。

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

### 当前状态

| 平台 | 状态 | 备注 |
|----------|--------|-------|
| **Linux x86_64** | 稳定 | 主要平台 |
| **Linux x86（32 位）** | 稳定 | 嵌入式优化 |
| **macOS** | 已规划 | |
| **Windows** | 已规划 | |
| **FreeBSD** | 已规划 | 社区需求 |

### 跨平台考量

- **C99 标准**：不依赖编译器专用扩展
- **自包含依赖**：外部库以子模块形式内置
- **条件编译**：平台专用代码隔在特性宏后面
- **POSIX**：能用 POSIX API 的地方直接复用

---

## 性能特征

### 基准测试（v3.0.0）

在 GitHub Actions `ubuntu-latest` 上使用 `benchmark_unified` 测量（Release 构建、10 连接、每轮 10 秒、10 轮）。共享 runner 上绝对 RPS 会跨 run 漂移，因此回归门禁使用同机配对比较；方法论见[性能](performance.md)。

| 场景 | 吞吐量 | 延迟 | 备注 |
|----------|-----------|---------|-------|
| `/`（HTTP/1.1） | 约 139K RPS | 约 70 µs 平均 | 10 连接 |
| `/json` | 约 140K RPS | 约 70 µs 平均 | JSON 端点 |
| `/large`（约 100KB） | 约 18K RPS | — | 零拷贝 writev |
| `/sse` | 约 157K RPS | 约 63 µs 平均 | 服务器发送事件 |
| `/stream` | 约 21K RPS | 约 468 µs 平均 | 流式 body |
| 高并发 | 约 117K RPS | 约 28 ms 平均 | 1000 连接；过载时客户端出现读超时 |

### 性能优化

1. **Keep-Alive**：复用连接，省掉反复握手
2. **TCP 优化**：默认打开 TCP_NODELAY 与 TCP_KEEPALIVE
3. **路由优化**：O(1) 前缀匹配
4. **内存分配**：可选 mimalloc
5. **直接调用 libuv**：中间不隔抽象层
6. **LRU 缓存**：静态文件自动缓存
7. **零拷贝 I/O**：大文件走 sendfile

---

## 安全特性

### 内置安全

- **缓冲区溢出防护**：边界检查
- **输入校验**：HTTP 头部与请求数据都检查
- **TLS 1.2/1.3**：集成 mbedtls
- **内存安全**：兼容 AddressSanitizer 与 Valgrind
- **错误处理**：不泄露敏感信息
- **资源限制**：连接数、头部数量、请求体大小都有上限

### 安全示例

```c
// 启用 TLS
uvhttp_tls_context_t* tls_ctx = NULL;
uvhttp_tls_context_new(&tls_ctx);
uvhttp_tls_context_load_cert_chain(tls_ctx, "server.crt");
uvhttp_tls_context_load_private_key(tls_ctx, "server.key");
server->tls_ctx = tls_ctx;

// 设置资源限制（只列执行路径真正读取的字段）。请求体 / 请求头 / URL 上限
// 是编译期常量（UVHTTP_MAX_BODY_SIZE 等），不是配置项 —— v3.0.0 删除了那些
// 从未生效的配置字段。
server->config->max_connections = 1000;
server->config->max_file_size = 10 * 1024 * 1024; // 10MB
```

---

## 文档结构

### 用户指南
- **[快速开始](getting-started.md)**：5 分钟跑起来
- **[安装](installation.md)**：编译与依赖
- **[第一个服务器](first-server.md)**：从空文件写起
- **[WebSocket](websocket.md)**：双向实时通信

### 核心概念
- **[教程](TUTORIAL.md)**：从基础到高级
- **[libuv 数据指针](LIBUV_DATA_POINTER.md)**：上下文怎么传
- **中间件系统**：请求/响应流水线
- **[统一响应指南](UNIFIED_RESPONSE_GUIDE.md)**：标准响应写法

### 高级特性
- **[请求过滤器 API](REQUEST_FILTER_API.md)**：分派前准入策略，限流由此实现
- **[静态文件服务器](STATIC_FILE_SERVER.md)**：sendfile 与缓存
- **压缩**：零开销压缩

### 开发者资源
- **[开发者指南](DEVELOPER_GUIDE.md)**：开发规范
- **[CMake 配置](CMAKE_CONFIGURATION.md)**：构建选项
- **[API 参考](../api/introduction)**：逐个接口的说明

---

## 快速上手

```c
#include <uvhttp.h>
#include <uv.h>
#include <string.h>

int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.7.1");

    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.7.1\"}";
    uvhttp_response_set_body(res, body, strlen(body));

    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);

    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_take_router(server, router);

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

编译与运行：
```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv
export LD_LIBRARY_PATH=./build/dist/lib:$LD_LIBRARY_PATH
./server
```

---

## 社区与支持

### 贡献

在提交 Pull Request 之前，请先阅读[贡献指南](https://github.com/adam-ikari/uvhttp/blob/main/CONTRIBUTING.md)。

### 获取帮助

- **GitHub Issues**：报 Bug、提功能需求
- **Discussions**：提问，或者讲讲你怎么用它
- **文档**：指南与 API 参考

### 许可证

MIT 许可证，详见 [LICENSE](https://github.com/adam-ikari/uvhttp/blob/main/LICENSE)。

---

## 兼容性

### 平台支持

| 平台 | 版本 | 状态 |
|----------|---------|--------|
| Linux x86_64 | 3.0.0 | ✅ 稳定 |
| Linux i386 | 3.0.0 | ✅ 稳定 |
| macOS x86_64 | — | 已规划 |
| macOS ARM64 | — | 已规划 |
| Windows x86_64 | — | 已规划 |

### 编译器支持

| 编译器 | 版本 | 状态 |
|----------|---------|--------|
| GCC | 4.8+ | ✅ 稳定 |
| Clang | 3.4+ | ✅ 稳定 |
| MSVC | — | 已规划 |

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
- **[API 参考](../api/introduction)**：按模块查接口
- **[示例](https://github.com/adam-ikari/uvhttp/tree/main/examples)**：可直接编译的示例
- **[性能基准](./performance.md)**：实测数字与测量方法
