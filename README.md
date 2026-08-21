# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-2.7.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-linux%20%7C%2032--bit-orange.svg)
![Tests](https://img.shields.io/badge/tests-101%2F101%20passing-success.svg)
[![ASan](https://img.shields.io/badge/ASan-clean-success.svg)](https://github.com/adam-ikari/uvhttp/actions/workflows/ci-nightly.yml)
[![UBSan](https://img.shields.io/badge/UBSan-clean-success.svg)](https://github.com/adam-ikari/uvhttp/actions/workflows/ci-nightly.yml)
![Performance](https://img.shields.io/badge/performance-~83K%20RPS%20(CI)-brightgreen.svg)

**Memory-Safety-Verified C HTTP/1.1 & WebSocket Server Library**

Lightweight & Embeddable • 32-bit Support • Zero-Copy • ASan/UBSan-Verified Production Grade

</div>

## 🎯 Overview

UVHTTP is a production-grade, event-driven HTTP server library built on libuv for modern C applications. It delivers exceptional performance with minimal resource consumption, making it ideal for both high-performance servers and embedded systems.

### Key Metrics (v2.7.0, GitHub CI baseline)

Performance baselines are measured on **GitHub Actions `ubuntu-latest` runners** for hardware consistency. Previous local baselines (v2.6.x, ~20K RPS) were measured on developer hardware with 40%+ variance from CPU thermal throttling. The CI runner eliminates this variance (CV 0.4–2.4%), providing an authoritative, reproducible baseline.

| Metric | Value | Context |
|--------|-------|---------|
| **Peak Throughput** | ~83K RPS | HTTP/1.1, 10 conn, GitHub CI runner |
| **High Concurrency** | ~55K RPS | 1000 concurrent connections |
| **Static Files** | 5.7K RPS | ~100KB body, `benchmark_unified` |
| **API Routing** | 82K RPS | JSON endpoint |
| **Average Latency** | ~117µs | P50, 10 connections |
| **Error Rate** | 0% | Zero socket errors under load (10 conn) |
| **Test Suite** | 101/101 pass | ASan + UBSan verified clean |

## 🌍 Platform Support

| Platform | Status | Architecture |
|----------|--------|--------------|
| **Linux** | ✅ Fully Supported | x86_64, x86 (32-bit) |
| **macOS** | 🔨 In Progress | x86_64, ARM64 |
| **Windows** | 📋 Planned | x86_64 |
| **FreeBSD** | 📋 Planned | x86_64 |
| **WebAssembly** | 📋 Planned | wasm32, wasm64 |

### 32-bit Embedded Systems
UVHTTP provides full support for 32-bit architectures with optimizations for resource-constrained environments, making it suitable for embedded devices and IoT applications.

## ✨ Core Features

### Performance
- ⚡ **Exceptional Performance**: 83K RPS on GitHub CI runners (Platinum tier), with 0.4–2.4% variance across 10 rounds
- 💾 **Zero-Copy Transmission**: Native sendfile integration for large files (>1MB)
- 🧠 **Intelligent Caching**: LRU cache with automatic preheating mechanisms
- 🚀 **Keep-Alive Optimization**: ~1000x performance improvement through connection reuse

### Architecture
- 🔧 **Modular Design**: Compile-time feature selection for WebSocket, static files, rate limiting
- ⚙️ **Zero Overhead**: All abstractions are compile-time macros with zero runtime cost
- 📐 **Event-Driven**: Non-blocking I/O based on libuv event loop
- 🎯 **Direct API Calls**: No abstraction layer between application and libuv

### Security
- 🔒 **Security-First**: Comprehensive buffer overflow protection and input validation
- 🛡️ **TLS 1.2/1.3 Support**: Encryption through mbedtls integration
- ✅ **Memory Safety**: Verified clean under AddressSanitizer (no leaks, no use-after-free, no overflows) and UndefinedBehaviorSanitizer across the full 101-test suite
- 🚨 **Resource Limits**: Configurable limits for connections, headers, and body size

### Developer Experience
- 📘 **Professional API**: Consistent naming conventions and intuitive design
- 📝 **Comprehensive Documentation**: Extensive guides, API reference, and examples
- 🔍 **Detailed Error Handling**: Unified error system with diagnostics and recovery guidance
- 🧪 **Zero Compilation Warnings**: Strict code quality standards

### Advanced Features
- 🔄 **Connection Management**: Connection pool, timeout detection, heartbeat monitoring
- 📊 **Rate Limiting**: Token bucket algorithm with whitelist support
- 🌐 **WebSocket**: Full-duplex communication with Ping/Pong support
- ⚙️ **Highly Configurable**: 36 compile-time options for different deployment scenarios
- 🎛️ **Memory Optimization**: Optional mimalloc for faster allocations

## 🚀 Quick Start

### Build System

UVHTTP uses **CMake** as the build system and **Make** as the command entry point. The `Makefile` wraps CMake with familiar targets — no extra tools needed.

#### Option 1: Make (Recommended)

```bash
# Clone repository
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Build (Debug)
make build

# Run tests
make test

# Verify memory safety (ASan + UBSan)
make verify-memory-safety

# Clean build artifacts
make clean
```

**Advantages:**
- ✅ Universal — pre-installed on virtually all Linux/macOS systems
- ✅ Familiar to embedded C developers
- ✅ Zero additional dependencies
- ✅ Wraps CMake directly — transparent, debuggable
- ✅ Cross-platform (Linux, macOS, WSL)

#### Option 2: Direct CMake

For CI scripts or custom workflows:

```bash
# Clone repository
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Configure
mkdir build && cd build
cmake ..

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Build Examples

```bash
# After building UVHTTP, compile examples easily
cd examples
make -f Makefile.examples

# Run an example
export LD_LIBRARY_PATH=../build/dist/lib:$LD_LIBRARY_PATH
./bin/simple_server
```

### Prerequisites

- **Just Command Runner**: Optional, for developers who prefer `just` (`cargo install just` or see [just.systems](https://just.systems))
- **C Compiler**: GCC 4.8+ or Clang 3.4+ with C99 support
- **CMake**: Version 3.10 or higher
- **Build Tools**: make, git
- **Optional**: mimalloc for improved memory performance
- **Node.js** (for llhttp): Required for building llhttp from source

### Building llhttp

Before building UVHTTP, you need to build the llhttp library:

```bash
# Option 1: Using npm (recommended)
cd deps/llhttp
npm install
npm run build

# Option 2: Using make
cd deps/llhttp
make build/libllhttp.a

# Option 3: Using Python (if npm not available)
cd deps/llhttp
python3 -m http.server 8080 &
npm install
npm run build
```

**Note**: The llhttp library is cached after the first build, so you only need to build it once.

### Building llhttp (HTTP Parser)

UVHTTP uses llhttp as the HTTP parser. You need to build it before compiling UVHTTP:

```bash
# Navigate to llhttp directory
cd deps/llhttp

# Option 1: Using npm (recommended)
npm install
npm run build

# Option 2: Using make (if npm not available)
make build/libllhttp.a

# Return to project root
cd ../..
```

**Note**: llhttp is only needed for the first build. The compiled library will be cached for subsequent builds.

### Advanced Build Options

```bash
# Enable mimalloc allocator
cmake -DBUILD_WITH_MIMALLOC=ON ..

# Build with debugging symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Enable code coverage
cmake -DENABLE_COVERAGE=ON ..

# Disable WebSocket support
cmake -DBUILD_WITH_WEBSOCKET=OFF ..

# 32-bit build for embedded systems
cmake -DCMAKE_C_FLAGS="-m32" ..
```

### Custom Configuration

For advanced users, you can create a custom configuration file:

```bash
# Copy the user options template
cp cmake/UserOptions.cmake cmake/UserOptions.local.cmake

# Edit the file to customize build options
vim cmake/UserOptions.local.cmake

# Build with custom configuration
cmake -DCMAKE_USER_CONFIG=ON ..
```

### Basic Usage

```c
#include <uvhttp.h>
#include <uv.h>

// Request handler
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello from UVHTTP v2.7.0!");
    return uvhttp_response_send(res);
}

int main() {
    // Create event loop
    uv_loop_t* loop = uv_default_loop();
    
    // Create server and router
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;
    
    // Add route
    uvhttp_router_add_route(router, "/hello", hello_handler);
    
    // Start server
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

**Compile and Run**:
```bash
gcc -o server server.c -I./include -L./build/dist/lib -luvhttp -luv
export LD_LIBRARY_PATH=./build/dist/lib:$LD_LIBRARY_PATH
./server
```

### Getting Help

- **Make Targets**: `make help` (shows all available targets)
- **CMake Options**: `make cmake-options` (shows all CMake build options)
- **Design Philosophy**: See [docs/PHILOSOPHY.md](docs/PHILOSOPHY.md)
- **Embedding Checklist**: See [docs/embedding-checklist.md](docs/embedding-checklist.md)
- **Quick Start Guide**: See [docs/guide/getting-started.md](docs/guide/getting-started.md)
- **Examples Makefile**: `make -f examples/Makefile.examples help`
- **Documentation**: See [docs/guide/getting-started.md](docs/guide/getting-started.md)

## 🏗️ Architecture

UVHTTP follows a modular, event-driven architecture designed for performance and flexibility:

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                      │
│  ┌───────────────────────────────────────────────────┐  │
│  │  Business Logic & Request Handlers                │  │
│  │  - Authentication                                 │  │
│  │  - Data Processing                                │  │
│  │  - Response Generation                            │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                 UVHTTP Framework Layer                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │ Router   │  │Middleware│  │WebSocket │            │  │
│  │ O(1)     │  │ Pipeline  │  │ Support  │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │ Static   │  │ Rate     │  │  TLS     │            │  │
│  │ Files    │  │ Limit    │  │ Support  │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                   libuv Event Loop                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │  I/O     │  │  Timer   │  │  Signal  │            │  │
│  │ Events   │  │ Events   │  │ Events   │            │  │
│  └──────────┘  └──────────┘  └──────────┘            │  │
└─────────────────────────────────────────────────────────┘
```

### Key Design Principles

1. **Zero Global Variables**: All state managed through libuv data pointers
2. **Zero Overhead Abstractions**: Compile-time macros, no runtime cost
3. **Modular Design**: Feature selection at compile time
4. **Direct libuv Integration**: No intermediate abstraction layers
5. **Resource Safety**: Comprehensive error handling and memory management

## 📚 Documentation

### User Guides
- **[Getting Started](docs/guide/getting-started.md)** - 5-minute quick start guide
- **[API Reference](docs/api/introduction.md)** - Complete API documentation
- **[Build Guide](docs/guide/CMAKE_CONFIGURATION.md)** - Build system configuration
- **[Performance Benchmarks](docs/performance.md)** - Performance analysis and metrics

### Developer Resources
- **[Architecture Design](docs/dev/ARCHITECTURE.md)** - System architecture and design decisions
- **[Developer Guide](docs/guide/DEVELOPER_GUIDE.md)** - Development best practices
- **[Testing Standards](docs/dev/TESTING_STANDARDS.md)** - Testing guidelines and coverage
- **[Migration Guide](docs/MIGRATION_GUIDE.md)** - Upgrading between versions

### Advanced Topics
- **[WebSocket Guide](docs/guide/websocket.md)** - Real-time communication
- **[Static File Server](docs/guide/STATIC_FILE_SERVER.md)** - File serving optimization
- **[Rate Limit API](docs/guide/RATE_LIMIT_API.md)** - Rate limiting implementation
- **[Compression Features](COMPRESSION_FEATURE_REPORT.md)** - Zero-overhead compression

## 🏗️ Project Structure

```
uvhttp/
├── include/              # Public API headers (27 files)
│   ├── uvhttp.h         # Main header file
│   ├── uvhttp_*.h       # Module headers
│   └── uvhttp_features.h # Feature configuration
├── src/                 # Implementation (23 .c files)
│   ├── uvhttp_*.c       # Core modules
│   └── uvhttp_websocket.c # WebSocket implementation
├── docs/                # Documentation
│   ├── api/             # API documentation
│   ├── guide/           # User guides
│   └── dev/             # Developer documentation
├── examples/            # Example programs (organized by topic)
│   ├── 01_basics/       # Basic examples
│   ├── 02_routing/      # Routing examples
│   └── 05_websocket/    # WebSocket examples
├── test/                # Test suite
│   ├── unit/            # Unit tests (37 active)
│   └── integration/     # Integration tests
├── benchmark/           # Performance benchmarks
├── deps/                # Third-party dependencies (submodules)
│   ├── libuv/           # Asynchronous I/O
│   ├── llhttp/          # HTTP parser
│   ├── mbedtls/         # TLS/SSL
│   └── mimalloc/        # Memory allocator
└── CMakeLists.txt       # Build configuration
```

## 🧪 Testing & Quality Assurance

### Test Coverage
- **Test Suite**: 101 unit/integration tests, all passing
- **Memory Safety**: Full suite verified clean under AddressSanitizer (no leaks, no use-after-free, no buffer overflows) and UndefinedBehaviorSanitizer (no undefined behavior)
- **CI/CD**: Automated testing on multiple platforms; nightly ASan + UBSan jobs
- **Code Quality**: Zero compilation warnings, strict linting (`-Werror`)

### Running Tests

```bash
# Run all tests
./run_tests.sh

# Run tests with coverage report
./run_tests.sh --detailed

# Run specific test
cd build
./uvhttp_unit_tests --gtest_filter=TestSuite.TestName

# Memory-safety verification (AddressSanitizer, with leak detection)
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
cd build_asan && ctest --output-on-failure

# Undefined-behavior verification (UBSan)
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc)
cd build_ubsan && ctest --output-on-failure
```

### Performance Testing

```bash
# Start the performance test server (built-in endpoints: /simple /json /large ...)
./build/dist/bin/test_performance_e2e 8080
#   ...or the unified benchmark server:
./build/dist/bin/benchmark_unified 8080

# Run wrk benchmark
wrk -t4 -c100 -d30s http://localhost:8080/simple

# Run Apache Bench
ab -n 10000 -c 100 http://localhost:8080/
```

## 🤝 Contributing

We welcome contributions! Please follow these guidelines:

1. Read [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines
2. Follow the code style: C99 standard, 4-space indentation, K&R braces
3. Ensure all tests pass: `./run_tests.sh`
4. Zero compilation warnings: `-Werror` enabled
5. Add tests for new features
6. Update documentation for API changes

### Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'feat: Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### License Summary

- ✅ Free for commercial and personal use
- ✅ No attribution required (but appreciated)
- ✅ Can modify and distribute
- ✅ No warranty provided

## 🙏 Acknowledgments

UVHTTP is built upon excellent open-source projects:

- **[libuv](https://github.com/libuv/libuv)** - Asynchronous I/O library
- **[llhttp](https://github.com/nodejs/llhttp)** - HTTP parser
- **[mbedtls](https://github.com/Mbed-TLS/mbedtls)** - TLS/SSL library
- **[mimalloc](https://github.com/microsoft/mimalloc)** - Memory allocator
- **[xxHash](https://github.com/Cyan4973/xxHash)** - Fast hashing algorithm
- **[Google Test](https://github.com/google/googletest)** - Testing framework

## 📞 Support & Community

### Getting Help
- **GitHub Issues**: [https://github.com/adam-ikari/uvhttp/issues](https://github.com/adam-ikari/uvhttp/issues)
- **Discussions**: [https://github.com/adam-ikari/uvhttp/discussions](https://github.com/adam-ikari/uvhttp/discussions)
- **Documentation**: [https://adam-ikari.github.io/uvhttp](https://adam-ikari.github.io/uvhttp)

### Community
- Star us on [GitHub](https://github.com/adam-ikari/uvhttp)
- Fork us and contribute
- Share your projects using UVHTTP
- Report bugs and suggest features

## 🗺️ Roadmap

### v2.7.0 (Released 2026-08-21)
- [x] TLS session cache (2048 entries, 24h timeout)
- [x] CI performance benchmark workflow
- [x] Code quality fixes (L3-L5)
- [x] Brain knowledge base documentation
- [x] Platinum tier baseline (83K RPS on CI)

### v2.8.0 (Planned)
- [ ] Performance regression gate (CI)
- [ ] Embedding verification round 2
- [ ] io_uring exploration for static file path
- [ ] New embedder integration docs
- [ ] Memory allocation optimization

### v2.9.0 (Future)
- [ ] FreeBSD support
- [ ] Fuzz testing enhancement
- [ ] Community contribution guide
- [ ] Chinese/English doc completeness

## 📊 Version History

| Version | Date | Highlights |
|---------|------|------------|
| **v2.7.0** | 2026-08-21 | TLS session cache re-enabled, CI benchmark workflow (ci-benchmark.yml), code quality fixes (L3-L5), brain documentation, Platinum tier baseline (83K RPS on CI) |
| **v2.6.2** | 2026-08-17 | Connection-limit memory safety fix (uv_close on accept failure), WebSocket RFC 6455/memory-safety fixes (PR #336), uv_strerror_r consistency |
| **v2.6.0** | 2026-07-31 | Health check endpoint, SSE example, mock testing infrastructure, Makefile build entry |
| **v2.5.1** | 2026-07-27 | Coverage 86% lines / 99% functions, 101/101 tests |
| **v2.5.0** | 2026-03-15 | 32-bit embedded support, compression features |
| **v2.4.4** | 2026-01-28 | Performance optimizations, code cleanup |
| **v2.3.0** | 2026-02-10 | Performance fix for connection cleanup |
| **v2.2.0** | 2026-01-27 | Major refactor, zero-overhead abstractions |

See [CHANGELOG.md](docs/CHANGELOG.md) for detailed release notes.

---

<div align="center">

**Built with ❤️ for high-performance applications**

[⬆ Back to Top](#uvhttp)

</div>