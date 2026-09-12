---
title: Performance Benchmarks
description: UVHTTP performance benchmarks — ~83K RPS on GitHub CI runners, 81K RPS JSON, 8.8K RPS large responses, zero socket errors, P50 ~117µs latency. Includes reproduce commands, optimization tips, and monitoring.
---

# Performance

UVHTTP is designed for high performance and low latency. This document provides performance metrics and optimization tips.

## Performance Metrics

### CI Baseline (Authoritative)

The authoritative baseline is measured on **GitHub Actions `ubuntu-latest` runners** with `benchmark_unified` (Release build, system allocator, 2 threads, 10 concurrent connections, 10s per round, 10 rounds per endpoint). CI runners remove the CPU thermal-throttling variance that plagues local benchmarks (CV 0.4–2.4% on CI vs 40%+ locally). Full methodology and the exact runner environment are recorded in [Performance Targets](../PERFORMANCE_TARGETS.md).

| Endpoint | RPS | Avg Latency | Notes |
|----------|-----|-------------|-------|
| `/` (simple text) | **~83K** | ~117µs | HTTP/1.1, 10 conn, GitHub CI runner |
| `/json` | **~81K** | ~117µs | JSON endpoint |
| `/large` (~100KB body) | **~8.8K** | — | Large responses; zero-copy writev optimization (5.1K → 8.8K, +72.7%) |
| High concurrency (1000 conn) | **~55K** | ~27ms | graceful degradation |
| Socket errors | **0** | — | zero errors under load |

> **Note**: `benchmark_unified` `/large` returns a ~100KB body — much larger than the ~1KB body in `test_performance_e2e`. The two binaries are not directly comparable.

### Local Benchmark (Development Reference)

The following local measurements are kept for development-time reference only; they are **not** the authoritative baseline. Measured on the original benchmark host (AMD Ryzen 7 5800H, 12 cores, Linux 6.17.13-2-pve) with `wrk 4.1.0` against the built-in `test_performance_e2e` server, GCC 11.4.0 Release build (`-O2 -DNDEBUG`), system allocator. Reproduce: `wrk -t4 -c<N> -d10s http://127.0.0.1:18090/simple`.

| Scenario | RPS | Avg Latency | Max Latency | Notes |
|----------|-----|-------------|-------------|-------|
| **Low concurrency** (10 conn) | **19,887** | 0.35 ms | 6.59 ms | P50 0.32 / P99 0.86 ms |
| **Medium concurrency** (100 conn) | **19,834** | 5.03 ms | 19.25 ms | |
| **High concurrency** (500 conn) | **19,810** | 25.31 ms | 62.81 ms | flat vs 100 conn |
| **Extreme concurrency** (1000 conn) | **18,518** | 56.31 ms | 322 ms | graceful degradation |
| **JSON endpoint** (100 conn) | 19,451 | 5.15 ms | 20.48 ms | 2.84 MB/s transfer |
| **Large response 1KB** (100 conn) | 19,524 | 5.13 ms | 14.50 ms | 9.92 MB/s transfer |
| **Socket errors** | **0** | — | — | zero errors across all levels |
| **Total requests served** | 1,341,713 | — | — | zero server-side errors |

**Test Environment**:
- OS: Linux 6.17.13-2-pve
- CPU: AMD Ryzen 7 5800H (12 cores)
- Compiler: GCC 11.4.0
- Tool: wrk 4.1.0
- Test Duration: 10 seconds per test
- Build Type: Release (-O2 -DNDEBUG)
- Memory Allocator: System allocator
- Router Cache: Hash table only (hot path cache removed)

**Note**: For production performance testing, use Release mode:
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .
cmake --build . -j$(nproc) --target test_performance_e2e
./dist/bin/test_performance_e2e 18090
wrk -t4 -c100 -d10s http://127.0.0.1:18090/simple
```

### Memory-Safety Verification

Performance is meaningless without correctness. The full 101-test suite is verified
clean under both sanitizers before any performance work is considered done:

```bash
# AddressSanitizer (leaks, use-after-free, overflows)
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc) && (cd build_asan && ctest -j4)

# UndefinedBehaviorSanitizer
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc) && (cd build_ubsan && ctest -j4)
```

### Stability

- **Concurrency Range**: 10-1000 concurrent connections (tested)
- **RPS Fluctuation**: < 5% across all concurrency levels locally; CV 0.4–2.4% on CI
- **Memory Usage**: Stable, no leaks detected (no CLOSE_WAIT connections)
- **CPU Usage**: Efficient, scales with load
- **Socket Errors**: Zero errors at all tested concurrency levels

### Performance Improvements (v2.3.1)

- **Event Loop Blocking Fix**: Removed synchronous `uv_run()` calls from connection cleanup
- **Performance Recovery**: Restored performance from 7-10,691 RPS to 31,000+ RPS
- **Zero Socket Errors**: Eliminated socket read errors (from 95%+ to 0%)
- **No Connection Leaks**: Eliminated CLOSE_WAIT state connections
- **Code Simplification**: Reduced `uvhttp_connection.c` by 38 lines

### Performance Improvements (v2.3.0)

- **Router Cache Optimization**: Removed hot path cache to avoid negative performance impact
- **Benchmark Compilation**: Unified compilation options with project for consistency
- **Memory Optimization**: Reduced memory footprint by removing redundant cache layers
- **Simplified Architecture**: Code simplification improves maintainability without sacrificing performance

## Performance Features

### 1. Zero-Copy Optimization

Large files (> 1MB) use `sendfile` for zero-copy transmission:

```c
// Automatically used in uvhttp_static_handle_request
// Files > 1MB use sendfile automatically
```

**Performance Gain**: 50%+ improvement for large files

### 2. Smart Caching

LRU cache with cache preheating:

```c
// Preheat cache on startup
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
```

**Performance Gain**: 300%+ improvement for repeated requests

### 3. Connection Pooling

Keep-Alive connections reduce connection overhead:

```c
// Automatically managed by UVHTTP
// Connections are reused when possible
```

**Performance Gain**: 1000x improvement for repeated requests

### 4. Fast Hashing

Integrated xxHash for ultra-fast hash operations:

```c
// Used internally for routing and caching
// xxHash is one of the fastest non-cryptographic hash functions
```

**Performance Gain**: 10x faster than standard hash functions

## Optimization Tips

### 1. Enable mimalloc

Use mimalloc for better memory allocation performance:

```bash
cmake -DBUILD_WITH_MIMALLOC=ON ..
```

**Performance Gain**: 20-30% improvement in allocation-heavy workloads

### 2. Use Zero-Copy for Large Files

For serving large files, register a normal C handler with the static file module:

```c
int static_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    return uvhttp_static_handle_request(static_ctx, req, res);
}

uvhttp_router_add_route(router, "/static/*", static_file_handler);
```

### 3. Preheat Cache

Preheat frequently accessed files:

```c
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
uvhttp_static_prewarm_cache(ctx, "/static/css/style.css");
```

### 4. Optimize Routes

Use specific routes instead of wildcards:

```c
// Good: Specific routes
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);

// Avoid: Wildcard routes (slower)
// uvhttp_router_add_route(router, "/api/*", api_handler);
```

### 5. Configure Keep-Alive

Adjust keep-alive timeout based on your workload:

```c
uvhttp_config_t* config = NULL;
uvhttp_config_new(&config);
config->keepalive_timeout = 60; // seconds
```

## Performance Testing

Run performance tests:

```bash
# Start test server
./build/dist/bin/benchmark_unified > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 3

# Run wrk benchmark
wrk -t4 -c100 -d30s http://localhost:18081/

# Cleanup
kill $SERVER_PID 2>/dev/null || true
```

## Performance Comparison

### vs Other HTTP Libraries

| Library | Throughput (RPS) | Latency (ms) | Memory Usage |
|---------|------------------|--------------|--------------|
| **UVHTTP** | **~83,000** | **~0.117 (P50, CI)** | **Low** |
| libuv-http | 18,500 | 3.45 | Medium |
| microhttpd | 15,200 | 4.20 | Low |
| mongoose | 12,800 | 5.10 | Medium |

*Note: Results may vary based on hardware, host load, and test duration. The
UVHTTP figure is the GitHub CI baseline (~83K RPS, 10 connections, `benchmark_unified`).
Competitor figures are indicative local measurements and are not directly
comparable to the CI numbers. Full historical baselines are in
`docs/performance/baseline-history.json`.*

## Monitoring Performance

### Built-in Metrics

UVHTTP tracks the active connection count directly on the server object:

```c
// Active connection count
size_t active_connections = server->active_connections;
```

### External Tools

Use standard tools for monitoring:

```bash
# CPU usage
top

# Memory usage
valgrind --tool=massif ./your_server

# Network performance
netstat -s
```

## Performance Tuning

### Compiler Optimizations

Enable compiler optimizations:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### System Configuration

Optimize system settings:

```bash
# Increase file descriptor limit
ulimit -n 65536

# Optimize TCP settings
sysctl -w net.core.somaxconn=4096
```

## Next Steps

- [Performance Benchmark (Chinese)](../zh/dev/PERFORMANCE_BENCHMARK.md) - Detailed benchmark results
- [Performance Testing Standard (Chinese)](../zh/dev/PERFORMANCE_TESTING_STANDARD.md) - Testing methodology
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Security Policy](../SECURITY.md) - Security guidelines
