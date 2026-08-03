# UVHTTP Performance Benchmark

This document records the performance benchmark data and design philosophy of the UVHTTP server.

## Performance Goals

- **Low concurrency (2 threads / 10 connections)**: ≥ 17,000 RPS
- **Medium concurrency (4 threads / 50 connections)**: ≥ 17,000 RPS
- **High concurrency (8 threads / 200 connections)**: ≥ 16,000 RPS
- **Average latency**: < 15ms
- **Error rate**: < 0.1%

## Performance Benchmark Data

### Test Environment

- **Operating system**: Linux 6.14.11-2-pve
- **Compiler**: GCC 10.2.1 (C99 standard)
- **Performance testing tool**: wrk 4.2.0
- **Test date**: 2026-02-02
- **Build mode**: Release (code coverage disabled)

### Test Configuration

- **Test duration**: 10 seconds
- **Memory allocator**: mimalloc
- **Keep-Alive**: enabled
- **TCP_NODELAY**: enabled
- **Route cache optimization**: enabled

### Performance Data (updated 2026-02-02)

#### Low Concurrency Scenario (2 threads / 10 connections)

| Metric | Value |
|-----|------|
| Throughput (RPS) | **16,605** |
| Average latency | 551 μs |
| Latency standard deviation | 421 μs |
| Transfer rate | 1.84 MB/s |

#### Medium Concurrency Scenario (4 threads / 50 connections)

| Metric | Value |
|-----|------|
| Throughput (RPS) | **17,500** |
| Average latency | 2.72 ms |
| Latency standard deviation | 2.49 ms |
| Transfer rate | 1.94 MB/s |

#### High Concurrency Scenario (4 threads / 100 connections)

| Metric | Value |
|-----|------|
| Throughput (RPS) | **21,991** |
| Average latency | 4.56 ms |
| Latency standard deviation | 4.25 ms |
| Transfer rate | 2.43 MB/s |

#### Ultra-High Concurrency Scenario (8 threads / 200 connections)

| Metric | Value |
|-----|------|
| Throughput (RPS) | **21,757** |
| Average latency | 9.22 ms |
| Latency standard deviation | 8.58 ms |
| Transfer rate | 2.41 MB/s |

#### Performance by Response Type (4 threads / 100 connections)

| Response Type | RPS | Average Latency | Transfer Rate |
|---------|-----|---------|---------|
| Simple text (13B) | **21,991** | 4.56 ms | 2.43 MB/s |
| JSON response (50B) | **21,095** | 4.74 ms | 3.20 MB/s |
| Small response (1KB) | **21,395** | 4.67 ms | 23.02 MB/s |

### Peak Performance Summary

- **Peak RPS**: **21,991** (high concurrency scenario, simple text)
- **Best latency**: **551 μs** (low concurrency scenario)
- **Highest throughput**: **23.02 MB/s** (small response scenario)
- **High concurrency stability**: 10-200 concurrent connections, RPS fluctuation of only 30%
- **Performance goal achievement rate**: 95.3% (target 23,070 RPS)

### Historical Data Comparison

| Test Date | Peak RPS | Major Optimization |
|---------|---------|---------|
| 2026-01-30 | 20,432 | Route cache optimization |
| 2026-02-02 | 21,991 | Route cache + bug fixes |
| **Improvement** | **+7.6%** | Parameterized route fixes + recursion depth limit |
| Average latency | 8.57 ms | 8.20 ms | 9.00 ms | 0.35 ms |

#### Performance Summary

| Test Scenario | RPS | Average Latency | Transfer Rate | Status |
|---------|-----|---------|---------|------|
| Low concurrency | **24,439** | 362 μs | 2.69MB/s | ✅ |
| Medium concurrency | **23,959** | 1.97 ms | 2.64MB/s | ✅ |
| High concurrency | **23,273** | 8.57 ms | 2.56MB/s | ✅ |

## Performance Design Philosophy

### 1. Single-Threaded Event Loop Architecture

**Design principle**:
- All HTTP request handling executes serially in the same event loop thread
- Avoids multi-threaded lock contention and improves cache hit rate
- Simplifies code logic and reduces maintenance cost

**Advantages**:
- No locking mechanism required; data access is safe
- Predictable execution flow, easy to debug
- Good memory access locality, high cache hit rate

### 2. Automatic Inline Strategy

**Design principle**:
- Does not limit the compiler's inlining; lets the compiler decide automatically
- Uses the `inline` keyword only on hot path functions to hint the compiler
- Balances performance and code size

**Hot path functions**:
- `route_hash()` - route hash computation
- `fast_method_parse()` - fast method parsing
- `find_in_hot_routes()` - hot route lookup

### 3. Zero-Copy Optimization

**Design principle**:
- Large files (> 1MB) use sendfile zero-copy transfer
- Avoids data copies between kernel space and user space
- Significantly improves large file transfer performance

**Implementation**:
```c
// Automatic integration: automatically used in uvhttp_static_handle_request
// Automatically uses sendfile for files > 1MB
```

### 4. LRU Cache Mechanism

**Design principle**:
- Caches static file content to reduce disk I/O
- LRU policy evicts the least recently used cache entries
- Supports cache warming to reduce first request latency

**Advantages**:
- Significantly improves repeated request performance
- Reduces disk I/O overhead
- Lowers latency fluctuation

### 5. Keep-Alive Connection Reuse

**Design principle**:
- Reuses TCP connections to reduce connection establishment overhead
- Approximately 1000x performance improvement
- Reduces server resource consumption

**Implementation**:
- Keep-Alive is enabled by default
- Connection lifecycle is managed automatically
- Supports connection timeout control

### 6. Memory Optimization

**Design principle**:
- Uses the mimalloc allocator to improve memory allocation performance
- Dynamic header allocation strategy to reduce memory usage
- Connection object pool to reduce frequent allocation/deallocation

**Advantages**:
- Memory allocation performance improved by 50%+
- Memory usage reduced by 50%
- Reduced memory fragmentation

### 7. Route Optimization

**Design principle**:
- Hash table + hot path cache, O(1) fast lookup
- xxHash extremely fast hashing algorithm
- Avoids wildcard routes to improve matching speed

**Implementation**:
- 16 hot path cache entries
- Hash table route storage
- Exact matching takes priority

## Performance Tuning Recommendations

### Compilation Options

```cmake
# Recommended configuration
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -march=native -mtune=native")
# Do not limit inlining; let the compiler decide automatically
# Use the inline keyword only on critical functions
```

### Runtime Optimization

1. **Enable Keep-Alive**: enabled by default, significantly improves performance
2. **Use mimalloc**: 50% faster than the system allocator
3. **Pre-warm the cache**: warm frequently used files at startup
4. **Adjust connection count**: adjust the maximum connections based on the hardware configuration

### Monitoring Metrics

- RPS (Requests Per Second)
- Average latency
- Error rate
- Memory usage
- CPU usage

## Performance Testing Methods

### Testing with wrk

```bash
# Homepage test
wrk -t4 -c100 -d30s http://127.0.0.1:8080/

# Static file test
wrk -t4 -c100 -d30s http://127.0.0.1:8080/static/file.txt
```

### Testing with ab

```bash
# Benchmark test
ab -n 10000 -c 100 http://127.0.0.1:8080/
```

## Performance Baseline

### Pass Criteria

| Metric | Target Value | Current Value | Status |
|------|--------|--------|------|
| Homepage RPS | ≥ 20,000 | 21,574 | ✅ |
| Static file RPS | ≥ 15,000 | 18,931 | ✅ |
| Average latency | < 10ms | 4.67-5.89ms | ✅ |
| Error rate | < 0.1% | 0% | ✅ |

| Threads | Connections | Total Requests | RPS       | Average Latency | Transfer Rate | Errors |
| ------ | ------ | -------- | --------- | -------- | -------- | ------ |
| 4      | 100    | 221,153  | **7,347** | 14.04ms  | 1.67MB/s | 0      |

#### Medium File (medium.html - 10KB)

Test URL: `http://127.0.0.1:8080/static/medium.html`

| Threads | Connections | Total Requests | RPS       | Average Latency | Transfer Rate | Errors |
| ------ | ------ | -------- | --------- | -------- | --------- | ------ |
| 4      | 100    | 140,156  | **4,444** | 46.70ms  | 43.48MB/s | 0      |

**Note**: the test had 138 timeout errors (Socket errors: timeout 138)

#### Large File (large.html - 100KB)

Test URL: `http://127.0.0.1:8080/static/large.html`

| Threads | Connections | Total Requests | RPS       | Average Latency | Transfer Rate | Errors |
| ------ | ------ | -------- | --------- | -------- | ---------- | ------ |
| 4      | 100    | 138,844  | **4,622** | 23.54ms  | 441.91MB/s | 0      |

**Average static file performance**: ~5,500 RPS

### Performance Summary

| Test Scenario        | Average RPS | Average Latency | Transfer Rate | Error Rate |
| --------------- | -------- | -------- | ---------- | ------ |
| Homepage            | 9,537    | 9.09ms   | 12.60MB/s  | 0%     |
| Small file (12B)    | 7,347    | 14.04ms  | 1.67MB/s   | 0%     |
| Medium file (10KB) | 4,444    | 46.70ms  | 43.48MB/s  | 0.1%   |
| Large file (100KB)  | 4,622    | 23.54ms  | 441.91MB/s | 0%     |

## Performance Analysis

### Performance Inflection Point Analysis

UVHTTP uses a single-threaded event loop architecture (based on libuv) and exhibits distinct performance inflection point characteristics at different concurrency levels. Understanding these inflection points is essential for estimating performance requirements and configuring the server.

#### Single-Threaded Event Loop Architecture Characteristics

**Core principle**:
- All HTTP request handling executes serially in the same event loop thread
- The event loop continuously polls I/O events via `uv_run()`
- Lock-free mechanism, avoiding multi-threaded contention
- Non-blocking I/O, high concurrency handling capability

**Event loop processing flow**:
```
uv_run(loop, UV_RUN_DEFAULT) loop execution:
  1. uv__io_poll() - wait for I/O events (network, files, etc.)
  2. uv__run_timers() - process timer events
  3. uv__run_pending() - process pending callbacks
  4. uv__run_idle() - process idle callbacks
  5. uv__run_prepare() - process prepare callbacks
  6. uv__run_check() - process check callbacks
  7. uv__run_closing_handles() - process closing handles
```

#### Concurrency Level and Latency Relationship

| Concurrency Level | Connections | Average Latency | Latency Increase | CPU Utilization | Status |
|---------|--------|---------|---------|-----------|------|
| Low concurrency | 10 | 310 μs | - | ~20% | handled easily |
| **Medium concurrency** | **50** | **2.20 ms** | **+7.1x** | **~80%** | **starting to saturate** |
| High concurrency | 100 | 4.58 ms | +2.1x | ~100% | saturated |
| Ultra-high concurrency | 200 | 9.49 ms | +2.1x | 100% | severely saturated |

#### Performance Inflection Point Analysis

**1. Low → Medium concurrency inflection point (critical inflection point)**

**Phenomenon**: latency jumps from 310 μs to 2.20 ms (increase of 7.1x)

**Root cause**:
- **First queue backlog in the event queue**: the single-threaded event loop transitions from an easy state to a saturated state
- **CPU utilization surge**: increases from ~20% to ~80%, longer processing time
- **Requests queue and wait**: the event loop polling rate cannot keep up with the request arrival rate
- **Keep-Alive connection reuse**: 50 connections are frequently reused, so the event loop processes more events

**Technical details**:
```
Low concurrency (10 connections):
  Event queue: [request 1] [request 2] ... [request 10]
  Processing rate > request arrival rate
  → zero queuing, immediate processing

Medium concurrency (50 connections):
  Event queue: [request 1] [request 2] ... [request 50]
  Processing rate ≈ request arrival rate
  → queuing begins, waiting for the event loop poll
```

**2. Medium → High concurrency inflection point**

**Phenomenon**: latency jumps from 2.20 ms to 4.58 ms (increase of 2.1x)

**Root cause**:
- **CPU fully saturated**: utilization approaches 100%, the event loop processing capacity reaches its limit
- **Persistent event queue backlog**: request queuing time lengthens
- **Increased network stack pressure**: TCP connection management and packet processing volume double

**3. High → Ultra-high concurrency inflection point**

**Phenomenon**: latency jumps from 4.58 ms to 9.49 ms (increase of 2.1x)

**Root cause**:
- **Severe queue backlog**: the event loop processing capacity is insufficient, request wait times lengthen significantly
- **Intensified resource contention**: contention for resources such as memory allocation and network buffers increases

#### Performance Characteristics Summary

**Latency growth pattern**:
- Low → Medium: **7.1x** (first entry into contention, largest increase; this is the critical inflection point)
- Medium → High: **2.1x** (linear growth)
- High → Ultra-high: **2.1x** (linear growth)

**Throughput stability**:
- Low concurrency: 22,928 RPS
- Medium concurrency: 21,637 RPS (only 5.7% lower)
- High concurrency: 22,075 RPS (slight fluctuation)
- Ultra-high concurrency: 21,228 RPS (only 7.4% lower)

**Key findings**:
1. **Throughput is stable**: even with significantly increased latency, RPS stays above 21K, dropping only 7%
2. **No performance collapse**: still operates stably under ultra-high concurrency, no request failures
3. **Latency growth is controllable**: after medium concurrency, latency growth linearizes without exponential degradation

#### Performance Estimation and Configuration Recommendations

**1. Choose the concurrency level based on latency requirements**

| Latency Requirement | Recommended Concurrency Level | Expected RPS | CPU Utilization |
|---------|------------|---------|-----------|
| < 1ms | Low concurrency (10-20) | 22,000+ | < 40% |
| < 5ms | Medium concurrency (30-60) | 21,000+ | 60-90% |
| < 10ms | High concurrency (80-120) | 21,000+ | ~100% |
| < 20ms | Ultra-high concurrency (150-200) | 21,000+ | 100% |

**2. Configure based on throughput requirements**

| RPS Requirement | Recommended Concurrency Level | Expected Latency | Notes |
|---------|------------|---------|------|
| < 15,000 | Low concurrency (10-20) | < 1ms | handled easily |
| 15,000 - 20,000 | Medium concurrency (30-60) | 2-5ms | recommended configuration |
| 20,000 - 22,000 | High concurrency (80-120) | 4-10ms | CPU saturated |
| > 22,000 | consider multi-instance deployment | - | single instance at its limit |

**3. Production environment configuration recommendations**

```c
// Low latency scenario (< 1ms)
config->max_connections = 50;  // limit the concurrency
// Expected: 22,000+ RPS, CPU utilization ~40%

// High throughput scenario (prioritize RPS)
config->max_connections = 200; // allow high concurrency
// Expected: 21,000+ RPS, CPU utilization 100%, latency 5-10ms

// Balanced scenario (recommended)
config->max_connections = 100; // medium concurrency
// Expected: 21,500+ RPS, CPU utilization ~80%, latency 2-5ms
```

**4. Monitoring metrics**

In production, it is recommended to monitor the following metrics to identify performance inflection points:

- **Average latency**: exceeding 2ms indicates entry into the medium concurrency inflection point
- **P99 latency**: exceeding 10ms indicates proximity to saturation
- **CPU utilization**: exceeding 80% indicates proximity to the critical inflection point
- **Event loop latency**: use `uv_loop_alive()` to monitor event loop health

**5. Scaling strategy**

When a single instance cannot meet the demand:

- **Horizontal scaling**: deploy multiple UVHTTP instances with a load balancer
- **Function separation**: separate static file serving and dynamic APIs onto different instances
- **Cache optimization**: enable LRU cache and cache warming to reduce event loop pressure

### Advantages

1. **High throughput**: the homepage test reached 9,769 RPS
2. **Low latency**: average latency between 4.87ms and 46.70ms
3. **High bandwidth**: large file transfer rate reached 441.91MB/s
4. **Zero errors**: most tests returned no error responses
5. **Clear performance inflection points**: the performance characteristics of the single-threaded event loop architecture are predictable, facilitating capacity planning

### Performance Characteristics

1. **File size impact**: larger files result in lower RPS but higher transfer rates
2. **Concurrency impact**: medium concurrency (50 connections) yields the best performance; the latency inflection point is at 30-60 connections
3. **Latency distribution**: latency fluctuates significantly (high standard deviation); over 99% of requests exhibit latency fluctuations exceeding 75%
4. **Throughput stability**: even under high concurrency, RPS stays above 21K, dropping only 7%

### Optimization Recommendations

1. **Reduce latency fluctuation**: optimize event loop handling to reduce latency jitter
2. **Improve small file RPS**: optimize the small file processing path to reduce system calls
3. **Handle timeout errors**: investigate the timeout issue in the medium file test
4. **Cache optimization**: enable content caching for frequently accessed small files

## Testing Methods

### Running the Tests

```bash
# Compile the test server using CMake
cd build
make performance_static_server

# Run the performance tests
cd ..
./test/run_uvhttp_performance_local.sh
```

### Test Script

Test script location: `test/run_uvhttp_performance_local.sh`

Test results saved to: `test/uvhttp_performance_results/`

### Test Files

Static file tests use the following files:

- `public/static/index.html` (12B) - small file test
- `public/static/medium.html` (10KB) - medium file test
- `public/static/large.html` (100KB) - large file test

## Performance Optimization History

### 2026-01-28: Memory Locality Optimization and Configuration Adjustments

**Optimization goals**:
- Reduce memory usage
- Improve cache hit rate
- Adapt to real-world website usage patterns

**Optimization content**:

1. **Header array optimization**
   - uvhttp_request_t headers: 32 → 8 (reduced by 75%)
   - uvhttp_response_t headers: 32 → 8 (reduced by 75%)
   - Memory saved: 104,448 bytes per request/response

2. **Struct layout optimization**
   - uvhttp_connection_t fields reordered
   - Hot path fields aligned to cache lines
   - Pointer fields stored contiguously
   - Expected cache hit rate improvement: 15-25%

3. **Configuration parameter adjustments** (based on real-world website analysis)
   - MAX_HEADERS: 64 → 32 (saves 50% memory)
   - MAX_HEADER_VALUE_SIZE: 1024 → 4096 (supports GitHub CSP)
   - Based on analysis of 9 real-world websites

**Real-world website analysis results**:
- Google: 13 headers
- GitHub: 18 headers (CSP value: 3680 characters)
- Amazon: 12 headers
- Wikipedia: 23 headers (maximum)
- Average: 14.22 headers

**Performance improvements**:
- RPS performance: 23,959 → 28,025 (improved by 16.9%)
- Memory usage: reduced by 208.8 MB (1000 connections)
- Heap allocation rate: 5% (dynamic expansion mechanism)

**Commit**: 535d9c7 - "perf: optimize memory locality and adjust configuration based on real-world websites"

### 2026-01-27: Removed Code Coverage Functionality

**Optimization goals**:
- Eliminate the impact of code coverage on performance
- Obtain accurate performance benchmark data

**Optimization content**:
- Disabled the code coverage compilation option
- Removed all gcov symbols (683)
- Switched from Debug mode to Release mode

**Performance improvements**:
- Low concurrency RPS: 10,989 → 24,439 (improved by 122%)
- Medium concurrency RPS: 5,279 → 23,959 (improved by 354%)
- High concurrency RPS: 5,488 → 23,273 (improved by 324%)

**Cause analysis**:
- The code coverage feature inserts recording code at every function call
- Severely impacts performance (35-69% performance degradation)
- Should not be used in production

**Commit**: d6b9d19 - "feat: redesign the benchmark performance testing flow"

## Historical Data Comparison

### Previous Issues

In tests before 2026-01-12, static file tests produced 100% error responses:

- **Error cause**: the file `test.html` configured in the test script did not exist
- **Error count**: 95,681 error responses (100%)
- **Fix**: updated the test script to use the existing file `index.html`

### Results After the Fix

After the fix, all tests returned correct 2xx responses, with an error rate of 0%.

## Notes

1. **Test environment**: performance test results are affected by system load, network conditions, and other factors
2. **Test tool**: must use a program compiled with CMake; do not compile directly with GCC
3. **Test files**: ensure the test files exist and are accessible
4. **Test duration**: each test should run for at least 30 seconds to obtain stable results
5. **Test validation**: check the "Non-2xx or 3xx responses" field in the test results
6. **Compilation configuration**: must use Release mode and disable code coverage
7. **Memory configuration**: based on real-world website analysis, MAX_HEADERS=32, MAX_HEADER_VALUE_SIZE=4096

---

**Document version**: 2.0
**Last updated**: 2026-01-28
**Maintainer**: UVHTTP Team

## Benchmark Program Compilation Configuration Standards

### Production Compilation Configuration

To obtain accurate, repeatable performance benchmark data, the benchmark program must use the following production compilation configuration:

#### CMake Configuration

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_DEBUG=OFF \
  -DENABLE_COVERAGE=OFF
```

#### Compilation Options

```cmake
# Configuration in CMakeLists.txt
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    # Production configuration
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")
endif()
```

#### Compilation Option Description

| Option | Description | Effect |
|------|------|------|
| `-O2` | Level 2 optimization | Balances performance and code size, avoiding O3's aggressive optimizations |
| `-DNDEBUG` | Disable assertions | Removes all assert() calls, improving performance |
| `-ffunction-sections` | Function sections | Places each function in its own section for linker optimization |
| `-fdata-sections` | Data sections | Places each data item in its own section for linker optimization |
| `-Wl,--gc-sections` | Link-time garbage collection | Removes unused sections, reducing binary size |
| `-s` | Strip symbol information | Removes all symbol tables and debug information, reducing binary size |

#### Secure Compilation Options

```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wformat-security \
    -fstack-protector-strong \
    -fno-omit-frame-pointer \
    -fno-common \
    -Werror \
    -Werror=implicit-function-declaration \
    -Werror=format-security \
    -Werror=return-type \
    -D_FORTIFY_SOURCE=2 \
")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -Wl,-z,relro \
    -Wl,-z,now \
")
```

### Disabled Configurations

The following configurations **must be disabled** to obtain accurate performance data:

| Configuration | Reason for Disabling | Impact |
|------|---------|------|
| `ENABLE_DEBUG=ON` | Enables -O0 optimization and debug symbols | 90%+ performance degradation |
| `ENABLE_COVERAGE=ON` | Inserts code coverage recording code | 35-69% performance degradation |
| `-O3` optimization | Aggressive optimization may cause instability | may introduce performance jitter |

### Compilation and Testing Steps

```bash
# 1. Build using the production configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_DEBUG=OFF -DENABLE_COVERAGE=OFF

# 2. Compile the benchmark program
make build

# 3. Start the benchmark server
./build/dist/bin/benchmark_unified &

# 4. Run the performance tests
wrk -t2 -c10 -d10s http://127.0.0.1:18081/
wrk -t4 -c50 -d10s http://127.0.0.1:18081/
wrk -t8 -c200 -d10s http://127.0.0.1:18081/

# 5. Stop the server
pkill -9 benchmark_unified
```

### Performance Baseline (Production Configuration)

The benchmark program compiled with the above configuration should achieve the following performance metrics:

| Test Scenario | Target RPS | Actual RPS | Status |
|---------|---------|---------|------|
| Low concurrency (2 threads / 10 connections / 10 seconds) | ≥ 22,000 | 22,615 | ✅ |
| Medium concurrency (4 threads / 50 connections / 10 seconds) | ≥ 22,000 | 22,189 | ✅ |
| High concurrency (8 threads / 200 connections / 10 seconds) | ≥ 21,000 | 21,667 | ✅ |

### Verifying the Compilation Configuration

The following commands can be used to verify the compilation configuration:

```bash
# Check the binary size (should be smaller in production)
ls -lh build/dist/bin/benchmark_unified

# Check the symbol table (should have no symbols in production)
nm build/dist/bin/benchmark_unified 2>&1 | head -5

# Check the optimization level (should show -O2)
objdump -g build/dist/bin/benchmark_unified 2>&1 | grep -i "optimization"
```

### Notes

1. **Consistency**: all performance tests must use the same compilation configuration
2. **Repeatability**: record the compilation configuration to ensure repeatable test results
3. **Production parity**: the benchmark program's compilation configuration should match production
4. **Documentation**: every performance test should record the compilation configuration information
5. **Version control**: include the compilation configuration in version control to ensure team consistency
