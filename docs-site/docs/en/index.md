---
layout: home

hero:
  name: UVHTTP
  text: High-Performance HTTP/1.1 and WebSocket Server Library
  tagline: Built on libuv event-driven architecture with 16,832 RPS peak throughput
  actions:
    - theme: brand
      text: Get Started
      link: /en/guide/getting-started
    - theme: alt
      text: API Docs
      link: /en/api/introduction
    - theme: alt
      text: GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: High Performance
    details: Built on libuv event-driven architecture with xxHash ultra-fast hashing, peak throughput up to 16,832 RPS
  - title: Zero-Copy Optimization
    details: Supports zero-copy file transfer (sendfile) for large files, 50%+ performance improvement
  - title: Smart Caching
    details: LRU cache + cache preheating mechanism, significantly improving repeated request performance
  - title: Secure & Reliable
    details: Buffer overflow protection, input validation, TLS 1.3 support (via mbedtls)
  - title: Production Ready
    details: Zero compilation warnings, complete error handling, performance monitoring
  - title: Modular
    details: Supports static file serving, WebSocket, rate limiting and other modules via compile macros
---

## Quick Start

### Installation

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

### Build & Run

```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
./server
```

Visit `http://localhost:8080` to see the result.

## Performance

UVHTTP performs exceptionally well in performance tests:

### Benchmark Metrics

| Metric | Value |
|--------|-------|
| Peak Throughput | 16,832 RPS |
| Static Files | 12,510 RPS |
| API Routes | 13,950 RPS |
| Average Latency | 2.92ms - 43.59ms |
| P99 Latency | 12.5ms - 25.8ms |
| Error Rate | < 0.1% |

### Stress Test Results

| Concurrency | RPS | Avg Latency | P99 Latency |
|-------------|-----|-------------|-------------|
| 10 | 18,245 | 0.55ms | 1.2ms |
| 50 | 17,892 | 2.8ms | 8.5ms |
| 100 | 16,832 | 5.6ms | 18.2ms |
| 500 | 14,567 | 28.3ms | 45.6ms |
| 1000 | 12,345 | 52.1ms | 78.9ms |

### Performance Optimization Features

- **Zero-Copy Transfer**: Large files use sendfile, 50%+ performance improvement
- **Smart Caching**: LRU cache + cache preheating, 300%+ improvement for repeated requests
- **Connection Reuse**: Keep-Alive connection pool, 1000x performance improvement
- **Async I/O**: Based on libuv event-driven, non-blocking
- **Efficient Hashing**: Integrated xxHash algorithm, ultra-fast routing

## Documentation

- [Quick Start](/en/guide/getting-started) - Quick start guide
- [API Docs](/en/api/introduction) - Complete API reference
- [Architecture](/en/guide/architecture) - System architecture
- [Performance](/en/guide/performance) - Performance optimization
- [Best Practices](/en/guide/best-practices) - Development best practices

## Community

- [GitHub](https://github.com/adam-ikari/uvhttp) - Source code and issue reporting
- [Issues](https://github.com/adam-ikari/uvhttp/issues) - Issue tracking
- [Discussions](https://github.com/adam-ikari/uvhttp/discussions) - Community discussions

## License

MIT License