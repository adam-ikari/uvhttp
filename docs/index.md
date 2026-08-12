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
    details: ASan + UBSan clean — 101/101 tests, zero leaks, nightly CI.
  - title: 🚀 Stable Throughput
    details: ~20K RPS, flat 100→500 connections, zero socket errors.
  - title: 📦 Lightweight & 32-bit
    details: ~257 KB static lib, runs on 32-bit, ~310 ms cold start.
  - title: 💾 Zero-Copy Files
    details: Native sendfile for large transfers — 50%+ less CPU.
  - title: 🧠 Smart Caching
    details: LRU cache with preheating for hot static content.
  - title: 🔒 Security-First
    details: Overflow guards, response-splitting prevention, TLS 1.2/1.3.
  - title: 🔧 Modular
    details: Toggle WebSocket / static / TLS / rate-limit at compile time.
  - title: 📐 Clean API
    details: Consistent naming, unified errors, easy to learn and misuse-proof.

---

## 📊 Performance Benchmarks

### Key Metrics (v2.6.0)

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
| **Test Suite** | 101/101 pass | ASan + UBSan verified clean |

### Memory-Safety & Quality Highlights

- **AddressSanitizer**: Full 101-test suite passes with leak detection enabled — zero leaks, zero use-after-free, zero buffer overflows
- **UndefinedBehaviorSanitizer**: Full suite passes — zero undefined behavior
- **Test Cases**: 101 unit/integration tests, all passing
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
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 101/101, nightly CI | ✅ 101/101, nightly CI |
| libuv-http | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| microhttpd | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| mongoose | ✅ | ✅ | ❓ not advertised | ❓ not advertised |
| nginx | ❌ (standalone) | ✅ | ✅ (large team) | ❓ |

> "not advertised" means the project publishes no sanitizer-clean test gate, so
> the absence of a finding is not verifiable. UVHTTP's is reproducible with
> `make verify-memory-safety`.

### Performance Optimizations

- **Keep-Alive**: connection reuse avoids re-establishing TCP per request
- **TCP**: `TCP_NODELAY` and `TCP_KEEPALIVE` enabled by default
- **Router**: O(1) prefix matching for route resolution
- **Allocation**: optional mimalloc
- **libuv**: called directly, no abstraction layer

---

## 🎯 Core Principles

### 1. Focus on Core Functionality
UVHTTP handles HTTP/1.1 and WebSocket protocol details; it does not impose business logic. The application keeps control over authentication, databases, and other features.

### 2. Zero Overhead Abstractions
Abstractions are compile-time macros with no runtime cost in production builds. The library calls libuv directly, with no intermediate layers.

### 3. Minimalist Engineering
The codebase favors simplicity. Self-contained dependencies and a clean architecture keep maintenance cost low.

### 4. Test Separation
Production code contains no test-specific code. Tests use linker wrapping and external mock frameworks, so the library ships clean.

### 5. Zero Global Variables
All state is held in libuv data pointers (`loop->data` or `server->context`). This enables multi-instance support and unit testing without global-state pollution.

### 6. Error Handling
A unified error type carries codes, descriptions, and recovery hints. Every failure point is checked and reported.

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

UVHTTP targets Linux, with 32-bit embedded support. Cross-platform expansion is on the roadmap.

---

## 🔧 Quick Installation

```bash
# Clone repository with submodules
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Build with default options
make build

# Run example server
./build/dist/bin/hello_world
```

For detailed installation instructions and build options, see the [Installation Guide](/guide/build).

---

## 📚 Documentation

- **[Getting Started](/guide/getting-started)** - Introduction and quick start guide
- **[API Reference](/api/introduction)** - Complete API documentation
- **[Installation Guide](/guide/INSTALL_CMAKE)** - Installation and build guide
- **[Performance Guide](/guide/performance)** - Performance optimization tips
- **[FAQ](/guide/FAQ)** - Frequently asked questions
<!-- site updated: 1784942602 -->
