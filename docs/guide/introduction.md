# UVHTTP User Documentation

Welcome to the UVHTTP user documentation! This documentation is for developers building applications with UVHTTP.

## 📌 Platform Support

**Current Support**: Linux

**Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for Linux platforms. We plan to expand support to other operating systems and platforms in future releases.

## 📚 Documentation Navigation

### Getting Started
- [Quick Start](getting-started.md) - Get up and running in 5 minutes
- [Installation](installation.md) - Installation instructions
- [First Server](first-server.md) - Create your first HTTP server

### Core Concepts
- [Tutorial](TUTORIAL.md) - Progressive tutorial from basics to advanced
- [libuv Data Pointer](LIBUV_DATA_POINTER.md) - Understanding libuv data pointer pattern
- [Middleware System](MIDDLEWARE_SYSTEM.md) - Middleware system architecture
- [Unified Response Guide](UNIFIED_RESPONSE_GUIDE.md) - Standard response handling

### Features
- [Rate Limit API](RATE_LIMIT_API.md) - Rate limiting functionality
- [Static File Server](STATIC_FILE_SERVER.md) - Static file serving
- [WebSocket](websocket.md) - WebSocket support

### Development
- [Developer Guide](DEVELOPER_GUIDE.md) - Development guide and best practices
- [CMake Configuration](CMAKE_CONFIGURATION.md) - Build configuration

## 🚀 Quick Start

```c
#include <uvhttp.h>
#include <uv_loop.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    server->router = router;
    
    // Add a simple route
    uvhttp_router_add_route(router, "/api", [](uvhttp_request_t* req) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, "{\"message\":\"Hello World\"}");
        uvhttp_response_send(res);
    });
    
    // Start server
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // Run event loop
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## 📖 More Information

- **[API Reference](../api/API_REFERENCE.md)**: Complete API documentation
- **[Architecture Design](../dev/ARCHITECTURE.md)**: System architecture
- **[Performance Benchmark](../dev/PERFORMANCE_BENCHMARK.md)**: Performance metrics

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](../../CONTRIBUTING.md) for details.

## 📄 License

This project is licensed under the MIT License.