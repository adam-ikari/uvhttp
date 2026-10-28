---
layout: home

hero:
  name: UVHTTP
  text: Memory-Safety-Verified C HTTP Server
  tagline: A lightweight, embeddable C99 HTTP/1.1 & WebSocket library — the only one in its class verified clean under ASan and UBSan. Runs 32-bit embedded, serves ~140K RPS on GitHub CI runners, leaks nothing. Throughput you can measure, memory safety you can prove.
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
    details: ASan + UBSan clean — 119/119 tests, zero leaks, verified in CI.
  - title: 🚀 Stable Throughput
    details: ~140K RPS on GitHub CI runners; within-run CV 0.9–2.6% across 10 rounds.
  - title: 📦 Lightweight & 32-bit
    details: ~257 KB static lib, runs on 32-bit, ~310 ms cold start.
  - title: 💾 Zero-Copy Files
    details: sendfile for files over 1 MB, two-iovec writev for large in-memory bodies.
  - title: 🧠 Smart Caching
    details: LRU cache with preheating for hot static content.
  - title: 🔒 Security-First
    details: Overflow guards, response-splitting prevention, TLS 1.2/1.3.
  - title: 🔧 Modular
    details: Toggle WebSocket / static files / TLS / router cache at compile time.
  - title: 📐 Clean API
    details: Consistent naming, unified errors, easy to learn and misuse-proof.

---

## 📊 Performance Benchmarks

### Key Metrics (v3.0.0 release gate, GitHub CI runner)

These figures come from the v3.0.0 release benchmark on **GitHub Actions `ubuntu-latest`** (Release build, system allocator, 10 concurrent connections, 10 s per round, 10 rounds per endpoint). Within a single run the medians are stable (CV 0.9–2.6%); across runs on shared runners they drift by tens of percent, so absolute RPS is documentation only. The regression gate compares head against base **on the same runner in the same job**: v3.0.0 measured 100.8% (`/`), 102.0% (`/json`) and 101.4% (`/large`) of v2.9.2, with 0 of 30 paired rounds below the 90% limit.

| Metric | Value | Notes |
|--------|-------|-------|
| **Throughput (`/`)** | ~139K RPS | 10 conn, HTTP/1.1 |
| **Throughput (`/json`)** | ~140K RPS | JSON endpoint |
| **Throughput (`/large`)** | ~18K RPS | ~100 KB body, zero-copy writev |
| **SSE (`/sse`)** | ~157K RPS | Server-sent events |
| **Streaming (`/stream`)** | ~21K RPS | Streamed body |
| **WebSocket** | ~28K msg/s | Echo, 10 conn × 200 msgs |
| **Average Latency** | ~70 µs | `/`, 10 conn; P99 ~150 µs |
| **High Concurrency** | ~117K RPS | 1000 conn; client read timeouts under overload |
| **Test Suite** | 119/119 pass | ASan + UBSan verified clean |

### What's new in v3.0.0

v3.0.0 is a deliberately breaking release: it removes API that could not keep its contract, rather than keeping it and pretending.

- **The built-in rate limiter is gone.** One counter per server meant a single client over quota returned 429 to every other client. A pre-dispatch [request filter](/guide/REQUEST_FILTER_API) replaces it, so window length, keying and storage stay application policy.
- **Ownership is in the name.** `uvhttp_server_set_router` / `set_context` became `take_router` / `take_context`, and the builder layer plus `uvhttp_serve` were removed.
- **Atomicity bugs fixed.** `uvhttp_response_set_body()` no longer frees the old body before allocating the new one, and the LRU cache setters validate at the write point.
- **A compressed response can no longer advertise a stale `Content-Length`**, and the static-file prewarm no longer leaks every file it handles.

Read the [v3.0.0 release notes](/releases/v3.0.0-release-notes) for the full migration list, or the [changelog](/guide/CHANGELOG) for every PR.

### Memory-Safety & Quality Highlights

- **AddressSanitizer**: full 119-test suite passes with leak detection enabled — zero leaks, zero use-after-free, zero buffer overflows
- **UndefinedBehaviorSanitizer**: full suite passes — zero undefined behavior
- **Test Cases**: 119 unit/integration tests, all passing
- **CI/CD**: ASan runs in the pre-release gate; UBSan, coverage and stress tests run nightly. Sanitizer builds enable `BUILD_WITH_STATIC_FILES` and `BUILD_WITH_ROUTER_CACHE`, so the static-file and router-cache code is actually instrumented
- **One-command verify**: `make verify-memory-safety` — see [Memory Safety](./MEMORY_SAFETY.md)
- **Coverage**: measured on every nightly run and published as a CI artifact

### Why UVHTTP (vs. other lightweight C HTTP libraries)

Most lightweight C HTTP libraries optimize for peak RPS and stop there. UVHTTP
optimizes for **the property that breaks production**: memory safety. A
per-connection leak or use-after-free that survives a 10-second benchmark will
OOM an embedded device over a week. UVHTTP is the lightweight, embeddable,
32-bit-capable C library that proves — under both AddressSanitizer and
UndefinedBehaviorSanitizer, in CI on every release and nightly — that those
bugs are gone.

| Library | Embeddable C library | 32-bit | ASan-clean (verified) | UBSan-clean (verified) |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 119/119, CI | ✅ 119/119, CI |
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
- **🔨 macOS**: Planned (lowest priority, unscheduled)
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

- **[Design Philosophy](/PHILOSOPHY)** — What UVHTTP is, isn't, and why
- **[Embedding Checklist](/embedding-checklist)** — Verify your integration
- **[Getting Started](/guide/getting-started)** — Introduction and quick start guide
- **[API Reference](/api/introduction)** — Complete API documentation
- **[Installation Guide](/guide/INSTALL_CMAKE)** — Installation and build guide
- **[Performance Guide](/guide/performance)** — Performance optimization tips
- **[Release Notes](/releases/)** — Version-by-version changes and migration notes
- **[FAQ](/guide/FAQ)** — Frequently asked questions
