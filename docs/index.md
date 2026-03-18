---
layout: home

hero:
  name: UVHTTP
  text: High-Performance HTTP/1.1 and WebSocket Server Library
  tagline: Event-driven architecture with 32-bit embedded support and zero-overhead compression
  actions:
    - theme: brand
      text: Get Started
      link: /guide/getting-started
    - theme: alt
      text: API Reference
      link: /api/introduction
    - theme: alt
      text: View on GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🚀 Exceptional Performance
    details: |
      Peak throughput: **23,226 RPS** with sub-millisecond latency.
      Optimized for high-concurrency scenarios with efficient event-driven architecture.
  - title: 💾 Zero-Copy Transmission
    details: |
      Native sendfile integration for large file transfers (>1MB).
      Eliminates data copying between kernel and user space, achieving 50%+ performance gains.
  - title: 🧠 Intelligent Caching
    details: |
      LRU cache with automatic preheating mechanisms.
      Reduces disk I/O for frequently accessed static content, improving response times.
  - title: 🔒 Security-First Design
    details: |
      Comprehensive buffer overflow protection and rigorous input validation.
      TLS 1.3 support through mbedtls for encrypted communications.
  - title: 🔧 Modular Architecture
    details: |
      Compile-time feature selection for WebSocket, static files, rate limiting, and more.
      Zero runtime overhead through macro-based modularization.
  - title: ⚡ Minimal Footprint
    details: |
      Self-contained dependencies with optional mimalloc integration.
      Suitable for embedded systems and resource-constrained environments.
  - title: 📐 Professional API
    details: |
      Consistent naming conventions and intuitive API design.
      Comprehensive error handling with detailed error codes and recovery suggestions.
  - title: ✅ Production-Grade
    details: |
      Zero compilation warnings with strict code quality standards.
      Complete resource management, memory safety, and extensive test coverage.

---

## 📊 Performance Benchmarks

### Key Metrics (v2.5.0)

| Metric | Value | Notes |
|--------|-------|-------|
| **Peak Throughput** | 23,226 RPS | Low concurrency, HTTP/1.1 |
| **Static Files** | 12,510 RPS | Medium concurrency, 1MB files |
| **API Routing** | 13,950 RPS | REST endpoints |
| **Average Latency** | 2.92 - 43.59 ms | P50-P99 range |
| **Error Rate** | < 0.1% | Under normal load |
| **Code Coverage** | 89.2% lines, 92.0% functions | Production-grade quality assurance |

### Test Coverage Highlights

- **Overall Coverage**: 89.2% (13,386/15,008 lines)
- **Function Coverage**: 92.0% (7,336/7,974 functions)
- **Test Cases**: 200+ comprehensive test cases
- **High Coverage Modules** (≥95%):
  - uvhttp_utils.c: 100.0%
  - uvhttp_error.c: 98.8%
  - uvhttp_version.c: 98.3%
  - uvhttp_error_helpers.c: 95.9%

### Performance Optimizations

- **Keep-Alive Connections**: ~1000x performance improvement through connection reuse
- **TCP Optimizations**: TCP_NODELAY and TCP_KEEPALIVE enabled by default
- **Router Optimization**: O(1) prefix matching for fast route resolution
- **Memory Allocation**: Optional mimalloc for faster allocations
- **Direct libuv Calls**: Zero abstraction layer overhead

---

## 🎯 Core Principles

### 1. Focus on Core Functionality
UVHTTP provides essential HTTP/1.1 and WebSocket protocol handling without imposing business logic constraints. Application developers maintain complete control over authentication, databases, and other features.

### 2. Zero Overhead Abstractions
All abstractions are compile-time macros with zero runtime cost in production builds. Direct libuv API calls ensure maximum performance without intermediate layers.

### 3. Minimalist Engineering
The codebase prioritizes simplicity and clarity, eliminating unnecessary complexity. Self-contained dependencies and clean architecture reduce maintenance burden.

### 4. Test Separation
Production code contains no test-specific code. Testing is achieved through linker wrapping and external mock frameworks, ensuring library purity.

### 5. Zero Global Variables
All state is managed through libuv data pointers (loop->data or server->context), enabling multi-instance support and unit testing without global state pollution.

### 6. Comprehensive Error Handling
Unified error type system with detailed error codes, descriptions, and recovery suggestions. Every potential failure point is properly checked and reported.

---

## 🌍 Platform Support

### Current Status
- **✅ Linux**: Fully supported (primary platform)
- **🔨 macOS**: Work in progress
- **🔨 Windows**: Planned
- **🔨 FreeBSD**: Planned
- **🔨 WebAssembly**: Planned

### Architecture Support
- **✅ x86_64 (64-bit)**: Fully supported
- **✅ x86 (32-bit)**: Fully supported with embedded optimizations
- **🔨 ARM64**: Planned for future releases

UVHTTP is currently optimized for Linux platforms with robust 32-bit embedded system support. We actively plan to expand cross-platform compatibility in upcoming releases.

---

## 🔧 Quick Installation

```bash
# Clone repository with submodules
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Build with default options
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run example server
./dist/bin/hello_world
```

For detailed installation instructions and build options, see the [Installation Guide](/guide/build).

---

## 📚 Documentation

- **[Getting Started](/guide/getting-started)** - Introduction and quick start guide
- **[API Reference](/api/introduction)** - Complete API documentation
- **[Developer Guide](/guide/DEVELOPER_GUIDE)** - Development best practices
- **[Performance Benchmarks](/dev/PERFORMANCE_BENCHMARK)** - Detailed performance analysis
- **[Architecture Design](/dev/ARCHITECTURE)** - System architecture documentation