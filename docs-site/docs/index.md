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

    uvhttp_router_add_route(router, "/", [](uvhttp_request_t* req) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/plain");
        uvhttp_response_set_body(res, "Hello, World!");
        uvhttp_response_send(res);
    });

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

| 指标 | 数值 |
|------|------|
| 峰值吞吐量 | 16,832 RPS |
| 静态文件 | 12,510 RPS |
| API 路由 | 13,950 RPS |
| 平均延迟 | 2.92ms - 43.59ms |
| 错误率 | < 0.1% |

## 文档

- [快速开始](/guide/getting-started) - 快速上手指南
- [API 文档](/api/introduction) - 完整的 API 参考
- [架构设计](/guide/architecture) - 系统架构说明
- [性能优化](/guide/performance) - 性能优化指南
- [最佳实践](/guide/best-practices) - 开发最佳实践

## 社区

- [GitHub](https://github.com/adam-ikari/uvhttp) - 源代码和问题反馈
- [Issues](https://github.com/adam-ikari/uvhttp/issues) - 问题追踪
- [Discussions](https://github.com/adam-ikari/uvhttp/discussions) - 社区讨论

## 许可证

MIT License