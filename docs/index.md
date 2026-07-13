---
layout: home

hero:
  name: UVHTTP
  text: Memory-Safety-Verified C HTTP Server
  tagline: A lightweight, embeddable C99 HTTP/1.1 & WebSocket library — the only one in its class verified clean under ASan and UBSan. Runs 32-bit embedded, serves ~20K RPS, leaks nothing. Throughput you can measure, memory safety you can prove.
  actions:
    - theme: brand
      text: Get Started
      link: /guide/getting-started
    - theme: alt
      text: Memory Safety
      link: /MEMORY_SAFETY
    - theme: alt
      text: View on GitHub
      link: https://github.com/adam-ikari/uvhttp

features:
  - title: 🛡️ Memory-Safety Verified
    details: |
      The full 91-test suite passes clean under AddressSanitizer (zero leaks,
      use-after-free, or overflows) and UndefinedBehaviorSanitizer — verified
      nightly in CI, reproducible with one command: `make verify-memory-safety`.
      The property most lightweight C HTTP libraries don't provide, and the one
      that matters for long-running and embedded deployments.
  - title: 🚀 Stable, Leak-Free Throughput
    details: |
      ~20K RPS, flat from 100 to 500 connections, zero socket errors across
      1M+ requests. Not chasing the single-host peak (nginx/h2o win there) —
      chasing predictable, leak-free, long-uptime behavior.
  - title: 📦 Lightweight & 32-bit Embedded
    details: |
      ~257 KB static library, runs on 32-bit targets, cold-starts in ~310 ms.
      Compile-time feature selection means you only pay for what you use —
      perfect for resource-constrained devices.
  - title: 💾 Zero-Copy File Transfer
    details: |
      Native sendfile integration moves large files (>1MB) directly from kernel
      to socket, eliminating user-space copies and cutting CPU use 50%+.
  - title: 🧠 Intelligent Caching
    details: |
      LRU cache with automatic preheating serves hot static content from memory,
      shrinking disk I/O and response times for repeated requests.
  - title: 🔒 Security-First Design
    details: |
      Buffer-overflow guards on every string path, response-splitting
      prevention, strict input validation, and TLS 1.3 via mbedtls — with
      zero compiler warnings under `-Werror`.
  - title: 🔧 Modular Architecture
    details: |
      Toggle WebSocket, static files, rate limiting, TLS, and compression at
      compile time. Macro-based modularization adds zero runtime overhead.
  - title: 📐 Professional API
    details: |
      Consistent naming, intuitive request/response model, and a unified error
      system with detailed codes and recovery guidance — easy to learn, hard to
      misuse.

---

## 📊 Performance Benchmarks

### Key Metrics (v2.5.0)

Throughput varies by hardware. Values below are representative; on a comparable VM
the library sustains ~17K–20K RPS (100 connections) / ~20K peak (low concurrency)
with **zero socket errors**. Reproduce with `wrk -t4 -c100 -d10s`.

| Metric | Value | Notes |
|--------|-------|-------|
| **Peak Throughput** | ~20K RPS | Low concurrency (10 conn), HTTP/1.1 |
| **High Concurrency** | ~17–19K RPS | 100 concurrent connections |
| **Static Files** | 12,510 RPS | Medium concurrency, 1MB files |
| **API Routing** | 13,950 RPS | REST endpoints |
| **Average Latency** | ~9–21 ms | P50–P90, 100 connections |
| **Error Rate** | 0% | Zero socket errors under load |
| **Test Suite** | 91/91 pass | ASan + UBSan verified clean |

### Memory-Safety & Quality Highlights

- **AddressSanitizer**: Full 91-test suite passes with leak detection enabled — zero leaks, zero use-after-free, zero buffer overflows
- **UndefinedBehaviorSanitizer**: Full suite passes — zero undefined behavior
- **Test Cases**: 91 unit/integration tests, all passing
- **CI/CD**: Nightly ASan + UBSan jobs (see `.github/workflows/ci-nightly.yml`)
- **One-command verify**: `make verify-memory-safety` — see [Memory Safety](./MEMORY_SAFETY.md)
- **High-Coverage Modules** (≥95%):
  - uvhttp_utils.c: 100.0%
  - uvhttp_error.c: 98.8%
  - uvhttp_version.c: 98.3%
  - uvhttp_error_helpers.c: 95.9%

### Why UVHTTP (vs. other lightweight C HTTP libraries)

Most lightweight C HTTP libraries optimize for peak RPS and stop there. UVHTTP
optimizes for **the property that breaks production**: memory safety. A
per-connection leak or use-after-free that survives a 10-second benchmark will
OOM an embedded device over a week. UVHTTP is the lightweight, embeddable,
32-bit-capable C library that proves — under both AddressSanitizer and
UndefinedBehaviorSanitizer, on every nightly CI run — that those bugs are gone.

| Library | Embeddable C lib | 32-bit | ASan-clean (verified) | UBSan-clean (verified) |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 91/91, nightly CI | ✅ 91/91, nightly CI |
| libuv-http | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| microhttpd | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| mongoose | ✅ | ✅ | ❓ not advertised | ❓ not advertised |
| nginx | ❌ (standalone) | ✅ | ✅ (large team) | ❓ |

> "not advertised" means the project publishes no sanitizer-clean test gate, so
> the absence of a finding is not verifiable. UVHTTP's is reproducible with
> `make verify-memory-safety`.

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
- **[Installation Guide](/guide/INSTALL_CMAKE)** - Installation and build guide
- **[Performance Guide](/guide/performance)** - Performance optimization tips
- **[FAQ](/guide/FAQ)** - Frequently asked questions