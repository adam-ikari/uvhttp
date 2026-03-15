# Introduction to UVHTTP

Welcome to UVHTTP, a high-performance HTTP/1.1 and WebSocket server library built on libuv for modern C applications. This documentation provides comprehensive guidance for developers building scalable, efficient web services.

## 🎯 What is UVHTTP?

UVHTTP is a **production-grade, event-driven HTTP server library** designed for developers who require:

- **Exceptional Performance**: Peak throughput of 23,226 RPS with sub-millisecond latency
- **Resource Efficiency**: Minimal memory footprint and CPU usage through zero-copy optimizations
- **Architecture Flexibility**: Support for both 64-bit and 32-bit embedded systems
- **Developer Experience**: Clean, intuitive API with comprehensive error handling
- **Production Readiness**: Zero compilation warnings, complete test coverage, and security-first design

### Core Philosophy

UVHTTP follows these fundamental principles:

1. **Focus on Core Protocol Handling**: Provides HTTP/1.1 and WebSocket protocol processing without imposing business logic constraints
2. **Zero Overhead Abstractions**: All abstractions are compile-time macros with zero runtime cost
3. **Minimalist Engineering**: Eliminates unnecessary complexity while maintaining functionality
4. **Test Separation**: Production code contains no test-specific code or debug instrumentation
5. **Zero Global Variables**: All state managed through libuv data pointers for multi-instance support
6. **Comprehensive Error Handling**: Unified error system with detailed diagnostics and recovery guidance

---

## 🏗️ Architecture Overview

### Event-Driven Design

UVHTTP leverages libuv's event-driven architecture to achieve high concurrency without threading complexity:

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

UVHTTP is designed with portability in mind:

- **Standard C11 Compliance**: No compiler-specific extensions required
- **Self-Contained Dependencies**: All external libraries included as submodules
- **Conditional Compilation**: Platform-specific code isolated behind feature macros
- **POSIX Compliance**: Leverages POSIX APIs where available

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

1. **Keep-Alive Connections**: ~1000x improvement through connection reuse
2. **TCP Optimizations**: TCP_NODELAY and TCP_KEEPALIVE enabled by default
3. **Router Optimization**: O(1) prefix matching eliminates linear search
4. **Memory Allocation**: Optional mimalloc for faster allocations and better fragmentation
5. **Direct libuv Calls**: Zero abstraction layer between application and libuv
6. **LRU Caching**: Automatic caching of static files with preheating
7. **Zero-Copy I/O**: sendfile integration for large file transfers

---

## 🔒 Security Features

### Built-in Security

- **Buffer Overflow Protection**: Comprehensive bounds checking on all buffers
- **Input Validation**: Strict validation of HTTP headers and request data
- **TLS 1.3 Support**: Modern encryption through mbedtls integration
- **Memory Safety**: Optional AddressSanitizer and Valgrind compatibility
- **Error Handling**: Detailed error messages without information leakage
- **Resource Limits**: Configurable limits for connections, headers, and body size

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

// Request handler function
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set response status
    uvhttp_response_set_status(res, 200);
    
    // Set response headers
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.5.0");
    
    // Set response body
    const char* body = "{\"message\":\"Hello from UVHTTP\",\"version\":\"2.5.0\"}";
    uvhttp_response_set_body(res, body);
    
    // Send response
    return uvhttp_response_send(res);
}

int main() {
    // Create event loop
    uv_loop_t* loop = uv_default_loop();
    
    // Create server
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // Create router
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
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

- **GitHub Issues**: Report bugs and request features
- **Discussions**: Ask questions and share ideas
- **Documentation**: Comprehensive guides and API reference

### License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.

---

## 📖 Next Steps

- **[Quick Start Guide](getting-started.md)**: Begin building your first server
- **[API Reference](../api/introduction)**: Explore the complete API
- **[Examples](../../examples/)**: Browse practical examples
- **[Performance Benchmarks](../performance.md)**: Understand performance characteristics