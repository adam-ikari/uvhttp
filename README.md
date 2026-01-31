# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.2.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Tests](https://img.shields.io/badge/tests-280%20passing-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-linux-orange.svg)

**High-performance HTTP/1.1 and WebSocket server library**

Focus on Core • High Performance • Configurable • Production Ready

</div>

## 📌 Platform Support

**Current Support**: Linux (x86_64, x86, ARM64, ARM32)

**Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for Linux platforms. We plan to expand support to other operating systems and platforms in future releases. Contributions for cross-platform compatibility are welcome!

## ✨ Features

- ⚡ **High Performance**: Built on libuv event-driven architecture with integrated xxHash ultra-fast hashing, peak throughput up to 23,226 RPS
- 🔒 **Secure**: Buffer overflow protection, input validation, TLS 1.3 support
- 🛡️ **Production Ready**: Zero compilation warnings, complete error handling, performance monitoring
- 🔧 **Easy to Use**: Clean API, rich examples, comprehensive documentation
- 🔄 **Connection Management**: Connection pool, timeout detection, heartbeat detection, broadcast functionality
- ⚙️ **Configurable**: 36 compile-time configuration options for different scenarios
- 💾 **Smart Caching**: LRU cache + cache preheating, zero-copy large file transmission

## 📊 Performance Metrics

- **Peak Throughput**: 23,226 RPS (low concurrency scenario)
- **High Concurrency Stability**: 10-500 concurrent, RPS fluctuation only 5%
- **Minimum Latency**: 352 μs (low concurrency)

## 🚀 Quick Start

### Installation

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Basic Usage

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

## 📚 Documentation

- **[API Reference](docs/api/API_REFERENCE.md)**: Complete API documentation
- **[Developer Guide](docs/guide/DEVELOPER_GUIDE.md)**: Development guide and best practices
- **[Tutorial](docs/guide/TUTORIAL.md)**: Progressive tutorial from basics to advanced
- **[Architecture Design](docs/dev/ARCHITECTURE.md)**: System architecture documentation

## 🏗️ Architecture

```
uvhttp/
├── include/           # Public headers
├── src/              # Source code
├── docs/             # Documentation
├── examples/         # Example programs
├── test/             # Unit and integration tests
└── deps/             # Third-party dependencies
```

## 🧪 Testing

```bash
# Run all tests
./run_tests.sh

# Run with coverage
./run_tests.sh --detailed

# Run specific test
./build/dist/bin/test_allocator
```

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [libuv](https://github.com/libuv/libuv) - Asynchronous I/O library
- [llhttp](https://github.com/nodejs/llhttp) - HTTP parser
- [mbedtls](https://github.com/Mbed-TLS/mbedtls) - TLS/SSL library
- [Google Test](https://github.com/google/googletest) - Testing framework

## 📞 Contact

- **GitHub**: [https://github.com/adam-ikari/uvhttp](https://github.com/adam-ikari/uvhttp)
- **Issues**: [https://github.com/adam-ikari/uvhttp/issues](https://github.com/adam-ikari/uvhttp/issues)

---

<div align="center">
Made with ❤️ by UVHTTP Contributors
</div>