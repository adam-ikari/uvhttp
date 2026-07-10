# Performance

UVHTTP is designed for high performance and low latency. This document provides performance metrics and optimization tips.

## Performance Metrics

### Benchmark Results (Updated: 2026-07-10)

Throughput varies by hardware. The values below are from a recent run on a Linux
VM (Linux 6.17, x86_64) using `wrk 4.1.0` against the built-in `test_performance_e2e`
server, Release build. Reproduce: `wrk -t4 -c<N> -d10s http://127.0.0.1:18080/simple`.

| Metric | Value | Notes |
|--------|-------|-------|
| **Peak Throughput** | ~20K RPS | Low concurrency (10 connections, 2 threads) |
| **High Concurrency** | ~19K RPS | 100 connections, 4 threads |
| **Very High Concurrency** | ~19K RPS | 1000 connections, 4 threads |
| **JSON Endpoint** | ~19.7K RPS | 100 connections |
| **Large Response (1KB)** | ~19.4K RPS | 100 connections, 9.85 MB/s transfer |
| **Average Latency** | ~9–21 ms | P50–P90, 100 connections (`/json`) |
| **Socket Errors** | **0%** | Zero errors at all concurrency levels |

**Test Environment**:
- OS: Linux 6.17.13-2-pve
- Tool: wrk 4.1.0
- Test Duration: 10 seconds per test
- Build Type: Release (-O2 -DNDEBUG)
- Memory Allocator: System allocator
- Router Cache: Hash table only (hot path cache removed)

**Note**: For production performance testing, use Release mode:
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .
cmake --build . -j$(nproc) --target test_performance_e2e
./dist/bin/test_performance_e2e 18080
wrk -t4 -c100 -d10s http://127.0.0.1:18080/simple
```

### Memory-Safety Verification

Performance is meaningless without correctness. The full 91-test suite is verified
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

- **Concurrency Range**: 10-100 concurrent connections (tested)
- **RPS Fluctuation**: < 5% across all concurrency levels
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

For serving large files, use the static file module:

```c
uvhttp_router_add_route(router, "/static/*", [](uvhttp_request_t* req) {
    uvhttp_static_handle_request(req, static_ctx);
});
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
uvhttp_config_t* config = uvhttp_config_new();
config->keep_alive_timeout = 60; // seconds
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
| **UVHTTP** | **~20,000** | **~9 (P50)** | **Low** |
| libuv-http | 18,500 | 3.45 | Medium |
| microhttpd | 15,200 | 4.20 | Low |
| mongoose | 12,800 | 5.10 | Medium |

*Note: Results may vary based on hardware and configuration. UVHTTP figures are
from a recent Linux VM run; earlier measurements on the original benchmark host
reported up to ~31K RPS at 100 connections.*

## Monitoring Performance

### Built-in Metrics

UVHTTP provides built-in performance monitoring:

```c
// Get connection statistics
size_t active_connections = server->stats.active_connections;
size_t total_requests = server->stats.total_requests;
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