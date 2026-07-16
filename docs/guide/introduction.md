---
title: Introduction
description: UVHTTP is a production-grade, event-driven HTTP/1.1 and WebSocket server library for modern C, built on libuv. ASan/UBSan-verified memory safety, 32-bit embedded support, zero-copy, modular features. Architecture overview and core principles.
---

# Introduction to UVHTTP

UVHTTP is an event-driven HTTP/1.1 and WebSocket server library for C, built on libuv. These docs cover building web services with it.

## 🎯 What is UVHTTP?

UVHTTP is an **event-driven HTTP server library** for C, built on libuv:

- **Throughput**: peak 23,226 RPS, sub-millisecond latency
- **Resource use**: small memory footprint and CPU usage via zero-copy
- **Architecture**: 64-bit and 32-bit embedded support
- **API**: clean, with error handling built in
- **Production state**: zero warnings, full test coverage, security-first defaults

### Core Philosophy

UVHTTP follows these principles:

1. **Focus on Core Protocol Handling**: HTTP/1.1 and WebSocket processing only; no business logic
2. **Zero Overhead Abstractions**: all abstractions are compile-time macros with no runtime cost
3. **Minimalist Engineering**: cut unnecessary complexity, keep functionality
4. **Test Separation**: production code contains no test-specific code or debug instrumentation
5. **Zero Global Variables**: all state held in libuv data pointers for multi-instance support
6. **Error Handling**: unified error system with diagnostics and recovery guidance

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
| **macOS** | 🔨 In Progress | Testing phase |
| **Windows** | 📋 Planned | Next major version |
| **FreeBSD** | 📋 Planned | Community requested |
| **WebAssembly** | 📋 Planned | Browser environments |

### Cross-Platform Considerations

UVHTTP is built for portability:

- **Standard C99**: no compiler-specific extensions required
- **Self-contained dependencies**: all external libraries included as submodules
- **Conditional compilation**: platform-specific code isolated behind feature macros
- **POSIX**: uses POSIX APIs where available

---

## 🚀 Performance Characteristics

### Benchmark Results (v2.5.0)

```yaml
Configuration:
  - Server: 4-core CPU, 16GB RAM
  - Client: wrk, 4 threads, 100 connections
  - Test Duration: 30 seconds

HTTP/1.1 Benchmarks:
  Low Concurrency:
    - Throughput: 23,226 RPS
    - Latency: 2.92 ms avg (P50)
    - Error Rate: 0.00%
  
  High Concurrency:
    - Throughput: 31,409 RPS
    - Latency: 43.59 ms avg (P99)
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

### Performance Optimizations

1. **Keep-Alive**: connection reuse avoids per-request TCP setup
2. **TCP**: `TCP_NODELAY` and `TCP_KEEPALIVE` enabled by default
3. **Router**: O(1) prefix matching, no linear search
4. **Allocation**: optional mimalloc
5. **Direct libuv calls**: no abstraction layer between application and libuv
6. **LRU caching**: static files cached with preheating
7. **Zero-copy I/O**: `sendfile` for large file transfers

---

## 🔒 Security Features

### Built-in Security

- **Buffer Overflow Protection**: bounds checking on all buffers
- **Input Validation**: HTTP headers and request data validated
- **TLS 1.3 Support**: via mbedtls
- **Memory Safety**: AddressSanitizer and Valgrind compatible
- **Error Handling**: detailed errors without information leakage
- **Resource Limits**: configurable connections, headers, and body size

### Security Best Practices

```c
// Example: Enable TLS for production deployments
uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
uvhttp_tls_context_load_cert(tls_ctx, "server.crt", "server.key");
server->tls_ctx = tls_ctx;

// Example: Set reasonable resource limits
server->max_connections = 1000;
server->max_headers = 100;
server->max_body_size = 10 * 1024 * 1024; // 10MB
```

---

## 📚 Documentation Structure

### User Guides
- **[Quick Start](getting-started.md)**: Get running in 5 minutes
- **[Installation](installation.md)**: Detailed installation instructions
- **[First Server](first-server.md)**: Build your first HTTP server
- **[WebSocket](websocket.md)**: Real-time communication setup

### Core Concepts
- **[Tutorial](TUTORIAL.md)**: Progressive learning from basics to advanced
- **[libuv Data Pointer](LIBUV_DATA_POINTER.md)**: Understanding context passing
- **[Middleware System](MIDDLEWARE_SYSTEM.md)**: Request/response processing pipeline
- **[Unified Response Guide](UNIFIED_RESPONSE_GUIDE.md)**: Standard response patterns

### Advanced Features
- **[Rate Limit API](RATE_LIMIT_API.md)**: Token bucket rate limiting
- **[Static File Server](STATIC_FILE_SERVER.md)**: Efficient file serving
- **[Compression](../dev/COMPRESSION_FEATURE_REPORT.md)**: Zero-overhead compression

### Developer Resources
- **[Developer Guide](DEVELOPER_GUIDE.md)**: Development best practices
- **[CMake Configuration](CMAKE_CONFIGURATION.md)**: Build system customization
- **[API Reference](../api/introduction)**: Complete API documentation

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
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.5.0");
    
    // Set response body
    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.5.0\"}";
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
    uvhttp_server_set_router(server, router);
    
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

We welcome contributions! Please read our [Contributing Guidelines](../../CONTRIBUTING.md) before submitting pull requests.

### Getting Help

- **GitHub Issues**: report bugs and request features
- **Discussions**: ask questions and share ideas
- **Documentation**: guides and API reference

### License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.

---

## 📖 Next Steps

- **[Quick Start Guide](getting-started.md)**: Begin building your first server
- **[API Reference](../api/introduction)**: Explore the complete API
- **[Examples](../../examples/)**: Browse practical examples
- **[Performance Benchmarks](../performance.md)**: Understand performance characteristics