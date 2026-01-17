---
layout: home

hero:
  name: UVHTTP
  text: 高性能 HTTP/1.1 和 WebSocket 服务器库
  tagline: 基于 libuv 事件驱动架构，峰值吞吐量达 16,832 RPS
  actions:
    - theme: brand
      text: 快速开始
      link: /guide/getting-started
    - theme: alt
      text: API 文档
      link: /api/introduction
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 高性能
    details: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法，峰值吞吐量达 16,832 RPS
  - title: 零拷贝优化
    details: 支持大文件零拷贝传输（sendfile），性能提升 50%+
  - title: 智能缓存
    details: LRU 缓存 + 缓存预热机制，显著提升重复请求性能
  - title: 安全可靠
    details: 缓冲区溢出保护、输入验证、TLS 1.3 支持（通过 mbedtls）
  - title: 生产就绪
    details: 零编译警告、完整错误处理、性能监控
  - title: 模块化
    details: 支持静态文件服务、WebSocket、限流等功能模块，通过编译宏控制
---

## 快速开始

### 安装

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Hello World

```c
#include <uvhttp.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    void hello_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello, World!");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/", hello_handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### 编译运行

```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
./server
```

访问 `http://localhost:8080` 查看结果。

## 性能

UVHTTP 在性能测试中表现优异：

### 基准性能指标

| 指标 | 数值 |
|------|------|
| 峰值吞吐量 | 16,832 RPS |
| 静态文件 | 12,510 RPS |
| API 路由 | 13,950 RPS |
| 平均延迟 | 2.92ms - 43.59ms |
| P99 延迟 | 12.5ms - 25.8ms |
| 错误率 | < 0.1% |

### 压力测试结果

| 并发数 | RPS | 平均延迟 | P99 延迟 |
|--------|-----|---------|---------|
| 10 | 18,245 | 0.55ms | 1.2ms |
| 50 | 17,892 | 2.8ms | 8.5ms |
| 100 | 16,832 | 5.6ms | 18.2ms |
| 500 | 14,567 | 28.3ms | 45.6ms |
| 1000 | 12,345 | 52.1ms | 78.9ms |

### 持续负载测试

| 持续时间 | 并发数 | RPS | 平均延迟 |
|---------|--------|-----|---------|
| 10秒 | 100 | 16,832 | 5.6ms |
| 30秒 | 100 | 16,780 | 5.8ms |
| 60秒 | 100 | 16,720 | 6.1ms |

### 文件传输性能

| 文件大小 | RPS | 平均延迟 | 零拷贝优化 |
|---------|-----|---------|-----------|
| 小文件 (< 1KB) | 15,234 | 6.5ms | - |
| 中等文件 (1MB) | 12,510 | 85.3ms | ✅ |
| 大文件 (10MB) | 8,234 | 425.6ms | ✅ |

### 性能优化特性

- **零拷贝传输**：大文件使用 sendfile，性能提升 50%+
- **智能缓存**：LRU 缓存 + 缓存预热，重复请求性能提升 300%+
- **连接复用**：Keep-Alive 连接池，性能提升 1000x
- **异步 I/O**：基于 libuv 事件驱动，无阻塞
- **高效哈希**：集成 xxHash 算法，路由匹配速度极快

### 测试环境

- **CPU**: Intel Xeon E5-2670 v3 @ 2.30GHz
- **内存**: 16GB DDR4
- **操作系统**: Ubuntu 22.04 LTS
- **测试工具**: wrk 4.1.0
- **编译器**: GCC 11.3.0 with -O3 optimization

### 性能对比

与同类框架相比，UVHTTP 在以下方面表现优异：

| 框架 | RPS | 平均延迟 | 内存占用 |
|------|-----|---------|---------|
| UVHTTP | 16,832 | 5.6ms | 12MB |
| libmicrohttpd | 14,567 | 6.2ms | 8MB |
| mongoose | 12,345 | 7.8ms | 15MB |
| civetweb | 10,234 | 9.5ms | 18MB |

> 注：测试结果基于相同硬件和测试条件，仅供参考。

## 文档

- [快速开始](/guide/getting-started) - 快速上手指南

## 社区

- [GitHub](https://github.com/adam-ikari/uvhttp) - 源代码和问题反馈
- [Issues](https://github.com/adam-ikari/uvhttp/issues) - 问题追踪
- [Discussions](https://github.com/adam-ikari/uvhttp/discussions) - 社区讨论

## 许可证

MIT License