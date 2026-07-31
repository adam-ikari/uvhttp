# UVHTTP Performance Targets

This document defines the performance targets and benchmarks for UVHTTP.

## Current Baseline

Measured 2026-07-25 on Linux x86_64 (Release build, system allocator, 2 threads, 10 concurrent connections, 5s test):

| Endpoint | Throughput (RPS) | Avg Latency | P99 Latency |
|----------|-----------------|-------------|-------------|
| `/simple` (text/plain) | 21,529 | 401µs | 0.95ms |
| `/json` (application/json) | 21,451 | 402µs | 0.95ms |
| `/large` (1KB body) | 21,314 | 405µs | 0.95ms |

## Targets

### Throughput

| Tier | Simple RPS | JSON RPS | Large RPS |
|------|-----------|----------|-----------|
| Bronze | 15,000 | 15,000 | 15,000 |
| Silver | 20,000 | 20,000 | 20,000 |
| **Gold** | **25,000** | **25,000** | **25,000** |
| Platinum | 35,000 | 35,000 | 35,000 |

**Current tier: Silver** (21,500 RPS)

### Latency

| Metric | Bronze | Silver | **Gold** | Platinum |
|--------|--------|--------|----------|----------|
| Avg latency | <2ms | <1ms | **<500µs** | <200µs |
| P99 latency | <10ms | <5ms | **<1ms** | <500µs |
| P999 latency | <50ms | <25ms | **<5ms** | <2ms |

**Current tier: Silver** (~400µs avg, ~0.95ms P99)

### Memory

| Metric | Bronze | Silver | **Gold** | Platinum |
|--------|--------|--------|----------|----------|
| Per-connection overhead | <64KB | <32KB | **<16KB** | <8KB |
| Memory per 10K connections | <640MB | <320MB | **<160MB** | <80MB |
| Request object size | <128KB | <96KB | **<64KB** | <32KB |

### Concurrency

| Concurrent Connections | Bronze | Silver | **Gold** | Platinum |
|----------------------|--------|--------|----------|----------|
| 100 | 15,000 RPS | 20,000 RPS | **25,000 RPS** | 35,000 RPS |
| 1,000 | 10,000 RPS | 15,000 RPS | **20,000 RPS** | 30,000 RPS |
| 10,000 | 5,000 RPS | 10,000 RPS | **15,000 RPS** | 25,000 RPS |

### TLS Performance

| Metric | Bronze | Silver | **Gold** | Platinum |
|--------|--------|--------|----------|----------|
| HTTPS RPS (vs HTTP) | 50% | 60% | **70%** | 80% |
| TLS handshake/s | 500 | 1,000 | **2,000** | 5,000 |
| Session resumption rate | N/A | 50% | **70%** | 90% |

## Key Performance Indicators

1. **P99 latency < 1ms** for simple requests at 10 concurrent connections
2. **Throughput > 20,000 RPS** for simple requests at 10 concurrent connections
3. **Scaling**: throughput drops < 50% when going from 10 to 1,000 concurrent connections
4. **Memory**: per-connection overhead < 64KB under load
5. **TLS overhead**: HTTPS achieves > 60% of HTTP throughput

## Benchmarking Methodology

### Environment
- Hardware: x86_64, 4+ cores, 8GB+ RAM
- OS: Linux kernel 5.x+
- Tool: wrk (or ab)
- Duration: 30s minimum for stable results
- Run: 3 times, report median

### Commands
```bash
# Simple response
wrk -t4 -c100 -d30s http://localhost:8086/simple

# JSON response
wrk -t4 -c100 -d30s http://localhost:8086/json

# Large response
wrk -t4 -c100 -d30s http://localhost:8086/large

# High concurrency
wrk -t4 -c1000 -d30s http://localhost:8086/simple

# TLS (if available)
wrk -t4 -c100 -d30s https://localhost:8443/simple
```

### Test Server
```bash
cd uvhttp
cmake -B build_bench -DCMAKE_BUILD_TYPE=Release
cmake --build build_bench -j$(nproc) --target test_performance_e2e
./build_bench/dist/bin/test_performance_e2e 8086
```

## Historical Tracking

| Date | Version | Simple RPS | JSON RPS | Large RPS | Notes |
|------|---------|-----------|----------|-----------|-------|
| 2026-07-25 | 2.5.1 | 21,529 | 21,451 | 21,314 | Baseline (Gold tier goal) |
| 2026-07-31 | 2.5.1 | 18,108 | — | — | Removed GCC extensions. ~6.6% perf cost, acceptable for portability. |