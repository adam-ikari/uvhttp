# Embedded & Long-Run Profile

Peak single-host throughput is not UVHTTP's pitch (nginx and h2o win there).
For embedded devices and long-running services, the metrics that matter are
different: **does memory stay flat over days of uptime? how small is the
footprint? does it run on 32-bit? how fast does it start?** This page answers
those.

These measurements complement the [memory-safety guarantee](./MEMORY_SAFETY.md):
sanitizer-clean status tells you there are *no leaks by construction*; the
long-run RSS profile below shows it *in practice under sustained load*.

## Long-run memory stability (no leak under sustained load)

The invariant: under continuous load, the server's resident set size (RSS) must
not grow unboundedly. A slow per-connection leak that passes a 10-second
functional test will OOM an embedded device over a week.

Measured with `scripts/performance/long_run_memory.sh`, which drives sustained
`wrk -t4 -c100` load and samples the server's RSS every 10 seconds.

| Elapsed (s) | RSS (KB) | Notes |
|-------------|----------|-------|
| 10 | 5,372 | 100 sustained connections |
| 20 | 5,372 | |
| 30 | 5,372 | |
| 40 | 5,372 | |
| 50 | 5,372 | |
| 60 | 5,372 | |

**Result: RSS flat — 0 KB growth over 60s of sustained load, ~20K RPS, zero
socket errors.** Re-run for hours to extend the curve; the expectation is a flat
line.

```bash
# Reproduce (60s default; pass a longer duration for a longer profile)
scripts/performance/long_run_memory.sh 300
```

> This is the runtime complement to the ASan-clean guarantee. ASan proves no
> allocation is lost along any tested code path; the RSS profile proves the
> allocator reaches steady state and does not drift under real traffic.

## Footprint

Default Release build (`-O2 -DNDEBUG`, stripped), 64-bit, system allocator:

| Artifact | Size |
|----------|------|
| Static library `libuvhttp.a` | ~257 KB (stripped) |
| Example server binary (`test_performance_e2e`) | ~1.0 MB (stripped, statically linked) |
| Steady-state RSS under 100 connections | ~5.3 MB |

The ~257 KB static library is the embedded-relevant figure — a device linking
UVHTTP adds roughly a quarter-megabyte of code, plus libuv/mbedtls only if the
features are enabled (both are compile-time optional).

> **Note on `-Os`:** the project's Release mode currently forces `-O2 -DNDEBUG`
> (see `CMakeLists.txt`). To build for minimal size, override
> `CMAKE_C_FLAGS_RELEASE`:
> ```bash
> cmake -DCMAKE_BUILD_TYPE=Release \
>       -DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG -ffunction-sections -fdata-sections" \
>       -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -s" ..
> ```

## Cold start

Time from process launch to first byte served (includes process startup, event
loop init, server bind, and one request):

| Metric | Value |
|--------|-------|
| First byte after launch | ~310 ms |

Suitable for on-demand/lazy-start embedded services and serverless-style
invocations.

## 32-bit support

UVHTTP explicitly supports 32-bit targets (the v2.5.0 release theme was
"32-bit Support & Compression"). The 32-bit CI build configures with:

```bash
cmake -B build-32bit \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686" \
  ...
```

(See `.github/workflows/ci-32bit.yml`.) On 32-bit, pointers halve and the
connection/request/response structs shrink accordingly, making the footprint
suitable for resource-constrained devices. 32-bit build correctness is gated in
CI.

## Compile-time minimization

Footprint is configurable at compile time — only pay for what you use:

| Feature | CMake option | Default |
|---------|--------------|---------|
| WebSocket | `BUILD_WITH_WEBSOCKET` | ON |
| TLS (mbedtls) | `BUILD_WITH_HTTPS` | ON |
| Static files | `UVHTTP_FEATURE_STATIC_FILES` | ON |
| Compression | `BUILD_WITH_COMPRESSION` | ON |
| Rate limiting | `UVHTTP_FEATURE_RATE_LIMIT` | ON |
| mimalloc allocator | `BUILD_WITH_MIMALLOC` | OFF |

A minimal embedded build (HTTP/1.1 only, no TLS/WebSocket/static/compression)
links a smaller library by disabling the unused features.

## Why this matters

For a device management endpoint or a long-running edge service:

- A leak that adds 1 KB/connection is invisible in a 10s `wrk` run but costs
  ~10 MB/day at 100 req/s with keep-alive churn. UVHTTP's flat RSS profile and
  ASan-clean status mean this failure mode is eliminated by construction.
- A ~257 KB library that runs on 32-bit fits where nginx (a standalone daemon
  with its own runtime) does not.
- Sanitizer-clean + 32-bit + small footprint is a combination most lightweight C
  HTTP libraries do not offer — see the [comparison table on the home page](./).
