---
title: Embedded & Long-Run Profile
description: "UVHTTP's embedded and long-running metrics: flat RSS over sustained load (0 KB growth in 60s), ~257 KB static library, ~310 ms cold start, and 32-bit support. The runtime complement to the ASan-clean memory-safety guarantee."
---

# Embedded & Long-Run Profile

UVHTTP does not sell peak single-host throughput; nginx and h2o win there. On an embedded device or a service that runs for months, other numbers decide whether it works: does RSS stay flat, how large is the binary, does it run on 32-bit, how long does startup take. This page measures each.

The measurements complement the [memory-safety guarantee](./MEMORY_SAFETY.md). A sanitizer-clean suite says no allocation is lost along a tested path. The RSS curve below shows what happens under sustained traffic.

## Long-run memory stability (no leak under sustained load)

The invariant: under continuous load, the server's resident set size must not grow without bound. A slow per-connection leak can pass a 10-second functional test and still OOM an embedded device within a week.

`scripts/performance/long_run_memory.sh` drives sustained `wrk -t4 -c100` load and samples the server's RSS every 10 seconds.

| Elapsed (s) | RSS (KB) | Notes |
|-------------|----------|-------|
| 10 | 5,372 | 100 sustained connections |
| 20 | 5,372 | |
| 30 | 5,372 | |
| 40 | 5,372 | |
| 50 | 5,372 | |
| 60 | 5,372 | |

**Result: RSS flat, 0 KB growth over 60s of sustained load at ~20K RPS, zero socket errors.** Run it for hours to extend the curve; the expected shape is a horizontal line.

```bash
# Reproduce (60s default; pass a longer duration for a longer profile)
scripts/performance/long_run_memory.sh 300
```

> ASan proves no allocation is lost along a tested path. The RSS profile shows the allocator reaching steady state and staying there under real traffic.

## Footprint

Default Release build (`-O2 -DNDEBUG`, stripped), 64-bit, system allocator:

| Artifact | Size |
|----------|------|
| Static library `libuvhttp.a` | ~257 KB (stripped) |
| Example server binary (`test_performance_e2e`) | ~1.0 MB (stripped, statically linked) |
| Steady-state RSS under 100 connections | ~5.3 MB |

The ~257 KB static library is the number that matters on a device. Linking UVHTTP adds about a quarter of a megabyte of code, plus libuv or mbedtls only when those features are enabled.

> **Note on `-Os`:** Release mode forces `-O2 -DNDEBUG` (see `CMakeLists.txt`). For a smaller binary, override
> `CMAKE_C_FLAGS_RELEASE`:
> ```bash
> cmake -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG -ffunction-sections -fdata-sections" \
>       -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -s" ..
> ```

## Cold start

Time from process launch to the first byte served, including process startup, event-loop init, server bind and one request:

| Metric | Value |
|--------|-------|
| First byte after launch | ~310 ms |

That fits on-demand or lazy-start embedded services and serverless-style invocations.

## 32-bit support

UVHTTP supports 32-bit targets; the v2.5.0 release theme was "32-bit Support & Compression". A 32-bit build uses:

```bash
cmake -B build-32bit \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686" \
  ...
```

On 32-bit, pointers halve and the connection, request and response structs shrink with them. That matters on devices with little RAM. The `.github/workflows/ci-32bit.yml` workflow is not part of the current CI set, so a 32-bit build is something you run yourself.

## Compile-time minimization

Footprint is a compile-time choice. You link what you use:

| Feature | CMake option | Default |
|---------|--------------|---------|
| WebSocket | `BUILD_WITH_WEBSOCKET` | ON |
| TLS (mbedtls) | `BUILD_WITH_HTTPS` | ON |
| Static files | `BUILD_WITH_STATIC_FILES` | OFF |
| Compression | `BUILD_WITH_COMPRESSION` | ON |
| mimalloc allocator | `BUILD_WITH_MIMALLOC` | OFF |

A minimal embedded build (HTTP/1.1 only) disables TLS, WebSocket, static files and compression, and links a smaller library.

## Why this matters

For a device management endpoint or a long-running edge service:

- A leak of 1 KB per connection is invisible in a 10s `wrk` run and costs about 10 MB/day at 100 req/s with keep-alive churn. UVHTTP's flat RSS profile and ASan-clean status remove that failure mode by construction.
- A ~257 KB library that runs on 32-bit fits where nginx, a standalone daemon with its own runtime, does not.
- Sanitizer-clean, 32-bit, small footprint: most lightweight C HTTP libraries offer at most two of the three. The [comparison table on the home page](./) has the details.
