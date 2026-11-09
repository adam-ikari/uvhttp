---
layout: home

hero:
  name: UVHTTP
  text: A C HTTP server with a memory-safety gate
  tagline: A small, embeddable C99 HTTP/1.1 and WebSocket library. Runs on 32-bit targets; 119 tests pass under ASan and UBSan; about 140K RPS on a GitHub CI runner.
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
  - title: Memory safety you can rerun
    details: 119/119 tests pass with ASan and UBSan, zero leaks, checked in CI.
  - title: Throughput that holds
    details: About 140K RPS on GitHub CI runners; CV 0.9–2.6% across 10 rounds in one run.
  - title: Small, and 32-bit
    details: ~257 KB static library, runs on 32-bit, ~310 ms cold start.
  - title: Zero-copy files
    details: sendfile above 1 MB; two-iovec writev for large in-memory bodies.
  - title: Cache in front
    details: LRU cache with prewarming for hot static files.
  - title: TLS and header guards
    details: Overflow checks, response-splitting prevention, TLS 1.2/1.3.

---

## Performance Benchmarks

### Key Metrics (v3.0.0 release gate, GitHub CI runner)

These figures come from the v3.0.0 release benchmark on **GitHub Actions `ubuntu-latest`** (Release build, system allocator, 10 concurrent connections, 10 s per round, 10 rounds per endpoint). Within one run the medians are stable (CV 0.9–2.6%). Between runs on shared runners they can move by tens of percent, so take the absolute RPS as a reference, not a promise. The regression gate compares head against base **on the same runner in the same job**: v3.0.0 measured 100.8% (`/`), 102.0% (`/json`) and 101.4% (`/large`) of v2.9.2, with 0 of 30 paired rounds below the 90% limit.

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

v3.0.0 breaks compatibility on purpose. It removes API that could not keep its contract instead of leaving it in place and pretending otherwise.

- **The built-in rate limiter is gone.** One counter per server meant a single client over quota got 429 responses sent to every other client. A pre-dispatch [request filter](/guide/REQUEST_FILTER_API) replaces it; window length, keying and storage are application policy now.
- **Ownership is in the name.** `uvhttp_server_set_router` / `set_context` became `take_router` / `take_context`. The builder layer and `uvhttp_serve` were removed.
- **Two atomicity bugs fixed.** `uvhttp_response_set_body()` no longer frees the old body before allocating the new one, and the LRU cache setters validate at the write point.
- **A compressed response can no longer advertise a stale `Content-Length`**, and the static-file prewarm no longer leaks every file it handles.

The [v3.0.0 release notes](/releases/v3.0.0-release-notes) list the full migration; the [changelog](/guide/CHANGELOG) tracks every PR.

### Memory-Safety & Quality Highlights

- **AddressSanitizer**: the full 119-test suite passes with leak detection on. Zero leaks, zero use-after-free, zero buffer overflows.
- **UndefinedBehaviorSanitizer**: the full suite passes with no undefined behavior.
- **Test cases**: 119 unit and integration tests, all passing.
- **CI/CD**: ASan runs in the pre-release gate; UBSan, coverage and stress tests run nightly. Sanitizer builds enable `BUILD_WITH_STATIC_FILES` and `BUILD_WITH_ROUTER_CACHE`, so that code is actually instrumented.
- **One command to check**: `make verify-memory-safety` (see [Memory Safety](./MEMORY_SAFETY.md)).
- **Coverage**: measured nightly and published as a CI artifact.

### Why UVHTTP (vs. other lightweight C HTTP libraries)

Most lightweight C HTTP libraries chase peak RPS and stop there. UVHTTP starts with the property that breaks production: memory safety. A per-connection leak or use-after-free survives a 10-second benchmark and then OOMs an embedded device within a week. UVHTTP is the lightweight, embeddable, 32-bit-capable C library that runs ASan and UBSan over its whole test suite in CI, on every release and every night, so those bugs get caught before they ship.

| Library | Embeddable C library | 32-bit | ASan-clean (verified) | UBSan-clean (verified) |
|---------|:----------------:|:------:|:---------------------:|:-----------------------:|
| **UVHTTP** | ✅ | ✅ | ✅ 119/119, CI | ✅ 119/119, CI |
| libuv-http | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| microhttpd | ✅ | ⚠️ | ❓ not advertised | ❓ not advertised |
| mongoose | ✅ | ✅ | ❓ not advertised | ❓ not advertised |
| nginx | ❌ (standalone) | ✅ | ✅ (large team) | ❓ |

> "not advertised" means the project publishes no sanitizer-clean test gate, so a clean result cannot be checked. UVHTTP's is reproducible with `make verify-memory-safety`.

### Performance Optimizations

- **Keep-Alive**: connections are reused, so each request does not pay for a new TCP handshake.
- **TCP**: `TCP_NODELAY` and `TCP_KEEPALIVE` on by default.
- **Router**: O(1) prefix matching to resolve a route.
- **Allocation**: mimalloc is optional.
- **libuv**: called directly, with no abstraction layer.

---

## Core Principles

### 1. Focus on Core Functionality
UVHTTP takes care of HTTP/1.1 and WebSocket details and leaves the application in charge of authentication, databases and everything else.

### 2. Zero Overhead Abstractions
They are compile-time macros. Production builds carry no runtime overhead, and the library calls libuv directly.

### 3. Minimalist Engineering
The codebase favors simplicity. Dependencies ship in-tree and the architecture stays clean, which keeps maintenance cheap.

### 4. Test Separation
Production code contains no test hooks. Tests use linker wrapping and an external mock framework, so the shipped library stays clean.

### 5. Zero Global Variables
State lives in libuv data pointers (`loop->data` or `server->context`). That supports multiple instances and unit tests without global-state pollution.

### 6. Error Handling
A single error type carries a code, a description and a recovery hint. Every failure point is checked and reported.

---

## Platform Support

### Current Status
- **Linux**: fully supported, and the primary platform.
- **macOS**: planned, lowest priority, no schedule.
- **Windows**: planned.
- **FreeBSD**: planned.
- **WebAssembly**: planned.

### Architecture Support
- **x86_64 (64-bit)**: fully supported.
- **x86 (32-bit)**: fully supported, with embedded optimizations.
- **ARM64**: planned for a future release.

UVHTTP targets Linux and runs on 32-bit embedded targets. Cross-platform support is on the roadmap.

---

## Quick Installation

```bash
# Clone repository with submodules
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# Build with default options
make build

# Run example server
./build/dist/bin/hello_world
```

For the options and the full build steps, see the [Installation Guide](/guide/build).

---

## Documentation

- **[Design Philosophy](/PHILOSOPHY)**: what UVHTTP is, what it is not, and why.
- **[Embedding Checklist](/embedding-checklist)**: check your integration.
- **[Getting Started](/guide/getting-started)**: install and run the first server.
- **[API Reference](/api/introduction)**: the complete API.
- **[Installation Guide](/guide/INSTALL_CMAKE)**: installation and build options.
- **[Performance Guide](/guide/performance)**: how to tune it.
- **[Release Notes](/releases/)**: changes and migration notes per version.
- **[FAQ](/guide/FAQ)**: common questions.
