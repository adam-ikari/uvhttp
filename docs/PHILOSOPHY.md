# UVHTTP Design Philosophy

> This document states the core design philosophy of UVHTTP: what it **is**, what it **is not**, and **why it works this way**.
> Every decision and code review uses it as the standard.

---

## 1. Project Scope: A Component, Not a Framework

UVHTTP is a **C library for HTTP/1.1 and WebSocket servers**. It is not an application server (nginx, Apache) and not a framework (Express, Flask).

**What the scope means:**

| The project is | The project is not |
|---------|-----------|
| An embeddable C library | A standalone daemon |
| A protocol handling component | A business-logic container |
| Part of your application | Your whole application |

**Consequence:** UVHTTP does not provide authentication, authorization, databases, template engines, session management or configuration hot reload. Those belong to the business layer. UVHTTP does one thing, handle HTTP and WebSocket correctly, and does it well.

---

## 2. Distribution Model: Source Only, Built by the User

UVHTTP **does not ship binaries**: no .deb or .rpm packages, no prebuilt static libraries. Users get it this way:

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp && mkdir build && cd build
cmake .. && make
```

### What this decides

**1. Version control covers the API, not the ABI**

Users always compile from source and link statically with their own application, so **ABI compatibility is not a promise**. The version number (SemVer) promises **source-level API compatibility**:

```
MAJOR.minor.patch
  │      │      └── fixes that do not break the API (bug fixes, tests, docs)
  │      └──────── features that do not break the API (new functions, new modules)
  └─────────────── API-breaking changes (signature changes, removals, incompatible behavior)
```

- **PATCH release:** fully backward compatible; user code does not change
- **MINOR release:** backward compatible; new features are optional and existing code keeps working
- **MAJOR release:** API incompatible; users must change their code

**2. Compile-time trimming is a core feature**

Users choose the features they need with CMake options:
```cmake
cmake .. -DBUILD_WITH_WEBSOCKET=OFF -DBUILD_WITH_COMPRESSION=OFF
```

Unused features are not compiled and add no code size. This is embedded-first in practice.

**3. Dependency management belongs to the user**

UVHTTP manages dependencies (libuv, llhttp, mbedtls, xxhash, miniz) as git submodules. A single `--recurse-submodules` clone fetches all of them. This matches the "zero external dependencies" philosophy: no dependency comes from a system package manager.

---

## 3. Embedded First

UVHTTP optimizes for **embedded devices and long-running services**, not peak throughput.

### Priority order

```
memory safety > memory stability > binary size > startup time > peak throughput
```

### Targets

| Metric | Target | Measurement |
|------|------|---------|
| Memory leaks | 0 (proven by ASan) | ASan gate on every release |
| Long-run RSS | No growth | `scripts/performance/long_run_memory.sh` |
| Static library size | ~257 KB | Release build, stripped |
| 32-bit support | Required | Manual `-m32` build; no CI job |
| Cold start | ~310 ms | Process start to first byte |
| Compile-time trimming | Pay for what you use | CMake feature switches |

### Low resource use first

"Embedded first" at UVHTTP covers **all resources**, not just **memory**. On a constrained device, CPU, memory, bandwidth and battery each cost something:

| Resource | UVHTTP's approach | How it shows |
|------|--------------|---------|
| **Memory (RAM)** | Minimum static footprint + zero leaks | Compile-time trimming, no global variables, cache-line alignment |
| **CPU** | Zero-cost abstraction, no middle layer | Direct libuv calls, compile-time macro elimination |
| **Bandwidth** | Zero-copy paths | Native sendfile, no unnecessary buffer copies |
| **Storage** | Small static library | ~257 KB stripped static library |
| **Startup** | Fast cold start | ~310 ms to first byte |

**Core belief: in a low-resource setting, every byte and every CPU cycle is a measurable cost.**

Every UVHTTP choice follows that belief:

- **Compile-time trimming**: unused features are not compiled, so they use no ROM or RAM.
- **No global variables**: state lives in the instance, so multiple instances work and destruction releases everything.
- **Zero copy**: sendfile moves large files to the socket without a user-space buffer.
- **A single allocator hook**: embedded users inject their own memory pool and control every allocation.
- **32-bit support**: pointers halve and structs shrink with them.

> By contrast, an "optimization" that makes code more complex, larger or more dependent violates low-resource-first even if it lifts peak throughput a little, unless measured data shows total resource use fell.

### What follows

- **Peak throughput is not the selling point** (nginx and h2o are stronger there).
- **A 1 KB leak per connection is a bug.** On an embedded device it exhausts memory within 10 hours.
- **32-bit support is not optional.** Many embedded devices are still 32-bit.
- **Compile-time trimming** means users link only the features they need and waste no bytes.

---

## 4. Memory Safety Is Infrastructure

Memory safety is not a feature of UVHTTP. It is **infrastructure, as essential as the compiler**.

### Three layers

| Layer | Mechanism | Frequency |
|------|------|------|
| **Compile time** | Build matrix, cppcheck, test-validity check | Every PR |
| **Release** | Fast tests over the tested matrix, ASan gate | Every release |
| **Nightly** | UBSan test run, coverage, stress, long-run RSS check | Nightly CI |

### Principles

- **Zero ASan/UBSan findings is a release gate**, not an optional target.
- **`no_sanitize("address")` is not allowed.** Disabling a sanitizer is a last resort, and it requires an explanation and a tracking issue.
- **Valgrind helps but does not replace ASan.** ASan catches more bugs and runs faster.
- **A memory leak is a functional bug**, on par with a logic error.

---

## 5. Testing Is Infrastructure

UVHTTP's test code (52K lines) is **2.5 times** its production code (21K lines). That is not over-engineering. It is **infrastructure**.

### Testing philosophy

| Production code | Test code |
|---------|---------|
| 21K lines of C | 52K lines of C++ (gtest) |
| 47 files | 128 files |
| Implements behavior | Proves behavior is correct |

### Principles

- **Tests are not an appendage.** Test code ranks with production code.
- **Test gate.** A release ships only when every test passes; PR CI runs compile, cppcheck, test validity, docs sync and the build matrix.
- **Test separation.** Production code contains no test code (no `#ifdef TEST`).
- **Coverage is not the goal; correctness is.** High coverage is a side effect worth having.
- **New features ship with tests.** A feature without tests does not exist.

---

## 6. Zero-Cost Abstraction

Zero-cost abstraction is UVHTTP's core engineering principle: **an abstraction costs nothing at runtime; it is only a compile-time organization device**. The idea comes from the C++ zero-overhead principle, implemented in C: preprocessor macros and static inline functions eliminate the abstraction before code generation.

### Why it is core

UVHTTP targets embedded, low-resource settings (section 3). An abstraction added for writing convenience that costs a few branches, an indirect call or a stack frame per request erodes throughput, increases instruction-cache pressure and grows the binary. Therefore:

> **Abstractions are allowed, but an abstraction must cost zero. Otherwise it is not an abstraction; it is debt.**

### Three zero-cost tools

| Tool | How it works | Example |
|------|------|------|
| **Compile-time macros** | Feature switches remove unused code during preprocessing | `UVHTTP_FEATURE_*` switches, `UVHTTP_LIKELY/UNLIKELY` branch prediction |
| **Static inline functions** | `static inline` inlines in optimized builds with no call overhead | `uvhttp_hash`, `uvhttp_method_to_string` fast paths |
| **Direct calls, no middle layer** | Call libuv / llhttp / mbedtls directly instead of wrapping and forwarding | `uv_tcp_*`, `llhttp_execute` used directly |

### Verification

Zero-cost abstraction needs **evidence**, not claims:

- **Disassembly check**: hot paths contain no leftover calls to abstraction layers.
- **Benchmark comparison**: enabling or disabling an abstraction shows no throughput difference (see `docs/PERFORMANCE_TARGETS.md`).
- **Code review**: the compiler must fully eliminate abstractions on performance-critical paths.

### Relation to low-resource-first

Zero-cost abstraction is the **means**; low-resource-first is the **end**:

- Zero-cost abstraction → CPU and instruction-cache efficiency
- Low-resource-first → a combined view of memory, size and startup time

---

## 7. Code Design Rules

### No global variables

All state travels through libuv `data` pointers (`loop->data`, `handle->data`). This lets UVHTTP support:
- multiple instances (several servers in one process)
- unit tests (no global state to contaminate)
- testability (all state can be injected)

### Unified error system

Every function that can fail returns `uvhttp_error_t`, one error type carrying a code, a description and a recovery hint. No `-1`, `NULL` or `errno` signals an error.

### Cache-line alignment

Hot-path structs (`uvhttp_request_t`, `uvhttp_response_t`, `uvhttp_connection_t`) are explicitly aligned and padded to a cache line (64 bytes). This is not a micro-optimization. It defends against false sharing on multicore systems.

### Compile-time feature flags

Compile-time macros control every optional feature (`UVHTTP_FEATURE_COMPRESSION`, `UVHTTP_FEATURE_WEBSOCKET` and others). Disabled features disappear in preprocessing with zero runtime cost.

---

## 8. C Standard: C11

UVHTTP builds with C11, not C99: `CMakeLists.txt` sets `CMAKE_C_STANDARD 11`. The code relies on C11 facilities such as `_Static_assert`, which C99 lacks.

**Chosen:**
- Portability: GCC 4.8+ and Clang 3.3+ support the C11 features the code uses.
- Embedded toolchains: cross-compilers in this space support C11.
- Least surprise: C11 behavior is known.

**Not used:**
- Optional C11/C17 features (anonymous structs, generic expressions): they raise compiler requirements for little gain.
- C23 features: most embedded compilers do not support them yet.

---

## 9. Project Boundaries: What We Do Not Do

UVHTTP deliberately does not do the following. If your use case needs them, UVHTTP may not fit.

| Area | Reason |
|------|------|
| **HTTP/2** | HTTP/2 multiplexing conflicts with libuv's event model; it needs application-visible multiplexing, not transparent connection reuse. |
| **HTTP/3** | Built on QUIC, which needs a different transport layer and falls outside a lightweight C library. |
| **Native Windows support** | libuv runs on Windows, but UVHTTP's zero-copy path (sendfile) and deep reliance on epoll would make Windows support a large amount of work. |
| **Async DNS** | libuv has built-in DNS resolution, but UVHTTP is a server library and does not take the client role. |
| **Automatic reconnect and health checks** | These are business-layer concerns, not protocol-layer ones. |
| **Configuration hot reload** | Embedded use rarely needs it; applications that need it can implement it in the application layer. |
| **Plugin system** | Dynamic loading conflicts with embedded security goals; users extend functionality by linking at compile time. |

> These boundaries are not permanent. If community contributions make a case possible without violating the core philosophy, the project can re-evaluate it.

---

## 10. Embedding Validation: Real Use Is the Only Valid Test

Three gates guard UVHTTP code quality (compile, test, CI), but all three validate **inside the library**. Embedding uvhttp in qwrt exposed 6 classes of problems. All of them passed uvhttp's own test suite and failed together in a real embedding.

**Lesson: passing tests does not mean an embedder can use it.**

### Principles

1. **Embedding validation is a test.** Every new feature, API or module must pass validation by at least one real embedder before release.
2. **Code without embedding validation counts as unvalidated**, even when internal tests pass.
3. **The embedding checklist is infrastructure.** Before each significant release, validate every known embedding dimension with the checklist.

### Problems exposed by the qwrt embedding

| Class | Problem | Root cause |
|------|------|------|
| **Enum alignment** | llhttp and uvhttp method enums did not align; casts corrupted POST/PUT/DELETE | Enum value coupling was undocumented and had no compile-time assertion |
| **Standards compatibility** | gzip output used the zlib wrapper (0x78) instead of standard gzip (0x1f8b) | Tests checked decompression, not a standard decoder |
| **Path normalization** | The index file path had no leading slash → 403 | Path validation and path generation disagreed |
| **Protocol compliance** | HTTP header matching was case-sensitive and rejected legal case variants | Test clients used canonical case, which hid the problem |
| **Pipeline** | No fallback handler when the router missed, so an embedder could not intercept | Design did not consider that embedders need a last line of defense |
| **Static globals** | The gzip cache used a module-level static global | Violated the no-global-variables rule; CI did not detect it |

Validate these classes at every embedding.

### Addition to this document

The first eight chapters define the **design philosophy** (what to do, what not to do, why). This chapter defines the **validation philosophy** (how to prove the project did it right).

UVHTTP's validation stack:

```
┌─────────────────────────────────────┐
│ Layer 3: embedding validation       │ ← defined in this chapter
├─────────────────────────────────────┤
│ Layer 2: build-configuration matrix │ ← covered (build-matrix)
├─────────────────────────────────────┤
│ Layer 1: tests + ASan/UBSan (base)  │ ← complete
└─────────────────────────────────────┘
```

Layer one is complete. Layer two is covered by the build-matrix job in ci-pr.yml, which validates all-on, minimal, ROUTER_CACHE, static files and no-compression builds. Layer three runs through the embedding checklist. This chapter bridges layer one and layer three.

---

## 11. Version History and Changes

| Version | Date | Change |
|------|------|------|
| 1.0 | 2026-08-20 | Initial document |
| 1.1 | 2026-08-20 | Added "low resource use first"; fixed section 9 formatting |
| 1.2 | 2026-08-20 | Added the top-level chapter "Zero-Cost Abstraction" |
| 1.3 | 2026-08-20 | Updated the validation stack: layer two build-matrix is covered by ci-pr.yml |
