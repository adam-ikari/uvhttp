---
title: Introduction
description: UVHTTP is a production-grade, event-driven HTTP/1.1 and WebSocket server library for modern C, built on libuv. ASan/UBSan-verified memory safety, 32-bit embedded support, zero-copy, modular features. Architecture overview and core principles.
---

# Introduction to UVHTTP

UVHTTP is an event-driven HTTP/1.1 and WebSocket server library for C, built on libuv. These docs cover building web services with it.

## 🎯 What is UVHTTP?

UVHTTP is an **event-driven HTTP server library** for C, built on libuv:

- Requests reach ~139K RPS on GitHub CI runners with sub-millisecond latency; see [Performance](performance.md)
- Memory and CPU use stay small through zero-copy
- It builds for 64-bit and 32-bit embedded targets
- The API is clean, and error handling is built in
- The production build has zero warnings, passes 119/119 tests, and is verified under ASan/UBSan with security-first defaults

### Core Philosophy

UVHTTP follows these principles:

1. Handle the protocol and nothing else: HTTP/1.1 and WebSocket processing, no business logic
2. Abstract at compile time: all abstractions are macros with no runtime cost
3. Keep the engineering minimal: cut unnecessary complexity, keep functionality
4. Keep tests out of production code: no test-specific code or debug instrumentation
5. Hold no global variables: all state lives in libuv data pointers, so multiple instances work
6. Report errors through one system, with diagnostics and recovery guidance

---

## 🏗️ Architecture Overview

### Event-Driven Design

UVHTTP uses libuv's event loop for high concurrency without threads:

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

### Key Components

| Component | Responsibility |
|-----------|---------------|
| **Server** | HTTP server lifecycle management, connection pooling |
| **Router** | O(1) prefix matching for fast route resolution |
| **Connection** | TCP connection management with Keep-Alive support |
| **Request/Response** | HTTP message parsing and generation |
| **WebSocket** | Full-duplex communication over HTTP upgrade |
| **Static Files** | Zero-copy file serving with caching |
| **TLS** | Encryption layer using mbedtls |

---

## 🌍 Platform Support

### Current Status

| Platform | Status | Notes |
|----------|--------|-------|
| **Linux x86_64** | ✅ Fully Supported | Primary platform |
| **Linux x86 (32-bit)** | ✅ Fully Supported | Embedded-optimized |
| **macOS** | 📋 Planned | |
| **Windows** | 📋 Planned | |
| **FreeBSD** | 📋 Planned | Community requested |

### Cross-Platform Considerations

UVHTTP is built for portability:

- The code is standard C99, with no compiler-specific extensions required
- All external libraries ship as submodules, so dependencies are self-contained
- Platform-specific code sits behind feature macros for conditional compilation
- POSIX APIs are used where available

---

## 🚀 Performance Characteristics

### Benchmark Results (v3.0.0)

Measured on GitHub Actions `ubuntu-latest` with `benchmark_unified` (Release build, 10 connections, 10 s per round, 10 rounds). Absolute RPS drifts between runs on shared runners, so the regression gate uses a same-runner paired comparison; see [Performance](performance.md) for the methodology.

| Scenario | Throughput | Latency | Notes |
|----------|-----------|---------|-------|
| `/` (HTTP/1.1) | ~139K RPS | ~70 µs avg | 10 conn |
| `/json` | ~140K RPS | ~70 µs avg | JSON endpoint |
| `/large` (~100KB) | ~18K RPS | — | zero-copy writev |
| `/sse` | ~157K RPS | ~63 µs avg | server-sent events |
| `/stream` | ~21K RPS | ~468 µs avg | streamed body |
| High concurrency | ~117K RPS | ~28 ms avg | 1000 conn; client read timeouts under overload |

### Performance Optimizations

1. Keep-Alive: connections are reused, so a request skips TCP setup
2. `TCP_NODELAY` and `TCP_KEEPALIVE` are on by default
3. The router matches prefixes in O(1), with no linear search
4. Allocation can use mimalloc
5. Application calls reach libuv directly, with no abstraction layer
6. Static files are cached in an LRU and preheated
7. `sendfile` serves large files with zero copy

---

## 🔒 Security Features

### Built-in Security

- Bounds are checked on every buffer
- HTTP headers and request data are validated
- TLS 1.2/1.3 support comes from mbedtls
- The code is compatible with AddressSanitizer and Valgrind
- Errors carry detail without leaking information
- Connection, header, and body sizes can be limited

### Security Best Practices

```c
// Example: Enable TLS for production deployments
uvhttp_tls_context_t* tls_ctx = NULL;
uvhttp_tls_context_new(&tls_ctx);
uvhttp_tls_context_load_cert_chain(tls_ctx, "server.crt");
uvhttp_tls_context_load_private_key(tls_ctx, "server.key");
server->tls_ctx = tls_ctx;

// Example: Set reasonable resource limits (fields an execution path reads).
// Body/header/URL limits are compile-time constants (UVHTTP_MAX_BODY_SIZE etc.),
// not config entries — v3.0.0 removed the config fields that never took effect.
server->config->max_connections = 1000;
server->config->max_file_size = 10 * 1024 * 1024; // 10MB
```

---

## 📚 Documentation Structure

### User Guides
- [Quick Start](getting-started.md): build and run in 5 minutes
- [Installation](../zh/guide/installation.md): installation instructions
- [First Server](../zh/guide/first-server.md): build your first HTTP server
- [WebSocket](../zh/guide/websocket.md): real-time communication

### Core Concepts
- [Tutorial](../zh/guide/TUTORIAL.md): basics to advanced, step by step
- [libuv Data Pointer](../zh/guide/LIBUV_DATA_POINTER.md): how context passes through callbacks
- Middleware system: a request/response processing pipeline
- [Unified Response Guide](../zh/guide/UNIFIED_RESPONSE_GUIDE.md): standard response patterns

### Advanced Features
- [Request Filter API](REQUEST_FILTER_API.md): pre-dispatch admission policy, including rate limiting
- [Static File Server](STATIC_FILE_SERVER.md): file serving
- Compression: zero-overhead compression

### Developer Resources
- [Developer Guide](../zh/guide/DEVELOPER_GUIDE.md): development practices
- [CMake Configuration](../zh/guide/CMAKE_CONFIGURATION.md): build system options
- [API Reference](../api/introduction): the full API

---

## 🛠️ Quick Start Example

```c
#include <uvhttp.h>
#include <uv.h>
#include <string.h>

// Request handler function
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set response status
    uvhttp_response_set_status(res, 200);
    
    // Set response headers
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.7.1");
    
    // Set response body
    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.7.1\"}";
    uvhttp_response_set_body(res, body, strlen(body));
    
    // Send response
    return uvhttp_response_send(res);
}

int main() {
    // Create event loop
    uv_loop_t* loop = uv_default_loop();
    
    // Create server
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    
    // Create router
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_take_router(server, router);
    
    // Add route
    uvhttp_router_add_route(router, "/hello", hello_handler);
    
    // Start listening
    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // Run event loop
    printf("Server listening on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

**Compile and Run**:
```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv
export LD_LIBRARY_PATH=./build/dist/lib:$LD_LIBRARY_PATH
./server
```

---

## 🤝 Community and Support

### Contributing

Read the [Contributing Guidelines](https://github.com/adam-ikari/uvhttp/blob/main/CONTRIBUTING.md) before you open a pull request.

### Getting Help

- GitHub Issues: report bugs and request features
- Discussions: ask questions and share ideas
- Documentation: guides and API reference

### License

The project uses the MIT License; see the [LICENSE](https://github.com/adam-ikari/uvhttp/blob/main/LICENSE) file.

---

## Compatibility

### Platform Support

| Platform | Version | Status |
|----------|---------|--------|
| Linux x86_64 | 3.0.0 | ✅ Stable |
| Linux i386 | 3.0.0 | ✅ Stable |
| macOS x86_64 | — | 📋 Planned |
| macOS ARM64 | — | 📋 Planned |
| Windows x86_64 | — | 📋 Planned |

### Compiler Support

| Compiler | Version | Status |
|----------|---------|--------|
| GCC | 4.8+ | ✅ Stable |
| Clang | 3.4+ | ✅ Stable |
| MSVC | — | 📋 Planned |

### Dependency Versions

| Dependency | Version | Status |
|------------|---------|--------|
| libuv | 1.44.0+ | ✅ Required |
| llhttp | 8.1.0+ | ✅ Required |
| mbedtls | 3.0.0+ | ✅ Optional (TLS) |
| mimalloc | 2.0.0+ | ✅ Optional (Allocator) |
| cjson | 1.7.0+ | ✅ Optional (JSON) |

---

## 📖 Next Steps

- [Quick Start Guide](getting-started.md): build your first server
- [API Reference](../api/introduction): the complete API
- [Examples](https://github.com/adam-ikari/uvhttp/tree/main/examples): working examples
- [Performance Benchmarks](./performance.md): measured throughput and latency