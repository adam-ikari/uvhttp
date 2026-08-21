# UVHTTP Performance Targets

This document defines the performance targets and benchmarks for UVHTTP.

## Current Baseline

Measured 2026-08-21 on Linux x86_64 (Release build, system allocator, 2 threads, 10 concurrent connections, 10s test, 10 rounds per endpoint).

### Observations

**CPU thermal throttling dominates variance.** The benchmark host's CPU enters turbo boost for the first 20-30s (~3 rounds), then thermally throttles to a lower sustained frequency. This causes ~40% coefficient of variation across runs. Two values are reported per endpoint:

- **Peak**: median of the first 3 rounds (turbo boost active)
- **Steady state**: median of the last 7 rounds (after thermal equilibrium)

| Endpoint | Peak (RPS) | Peak Latency | Steady (RPS) | Steady Latency | CV |
|----------|-----------|-------------|-------------|-------------|-----|
| `/simple` (text/plain) | 33,678 | 475µs | **15,265** | 1.04ms | 40.6% |
| `/json` (application/json) | 35,166 | 430µs | **14,478** | 1.13ms | 46.6% |
| `/large` (1KB body) | 31,459 | 565µs | **28,409** | 669µs | 34.9% |

> **Note on methodology**: The `/large` endpoint shows higher steady-state throughput than `/simple`/`/json`. This is likely because the 1KB body triggers a different code path (writev vs small-write) that happens to be more CPU-cache-friendly on this test host. This is not a generalizable result — verify on target hardware.

### Latency (steady state, P50–P99)

| Metric | Simple | JSON | Large |
|--------|--------|------|-------|
| Average | 1.04ms | 1.13ms | 669µs |
| P99 | ~2ms | ~2.5ms | ~1.5ms |

### High concurrency (1000 connections, steady state)

| Metric | Value |
|--------|-------|
| Throughput | ~14,300 RPS |
| Avg latency | ~35ms |
| Timeout errors | ~260/run (out of ~143K requests) |

The high-concurrency test shows that the single-threaded event loop saturates at ~14K RPS regardless of connection count. Timeout errors suggest the connection backlog exceeds the kernel's accept queue under load.

## Targets

### Throughput

| Tier | Simple RPS | JSON RPS | Large RPS |
|------|-----------|----------|-----------|
| Bronze | 10,000 | 10,000 | 10,000 |
| Silver | 15,000 | 15,000 | 15,000 |
| **Gold** | **25,000** | **25,000** | **25,000** |
| Platinum | 30,000 | 30,000 | 30,000 |
| Diamond | 40,000 | 40,000 | 40,000 |

**Current tier: Silver** (steady state ~15K RPS)  
**Peak tier: Gold** (turbo-boosted ~30K RPS)

### Latency

| Metric | Bronze | Silver | **Gold** | Platinum | Diamond |
|--------|--------|--------|----------|----------|---------|
| Avg latency | <2ms | <1.5ms | **<1ms** | <500µs | <300µs |
| P99 latency | <10ms | <5ms | **<2ms** | <1ms | <800µs |
| P999 latency | <50ms | <25ms | **<5ms** | <5ms | <2ms |

**Current tier: Silver** (~1ms avg, ~2ms P99 steady state)

### Memory

| Metric | Bronze | Silver | **Gold** | Platinum | Diamond |
|--------|--------|--------|----------|----------|---------|
| Per-connection overhead | <64KB | <32KB | **<16KB** | <8KB | <4KB |
| Memory per 10K connections | <640MB | <320MB | **<160MB** | <80MB | <40MB |
| Request object size | <128KB | <96KB | **<64KB** | <32KB | <16KB |

### TLS Performance

| Metric | Bronze | Silver | **Gold** | Platinum | Diamond |
|--------|--------|--------|----------|----------|---------|
| HTTPS RPS (vs HTTP) | 50% | 60% | **70%** | 80% | 90% |
| TLS handshake/s | 500 | 1,000 | **2,000** | 5,000 | 10,000 |
| Session resumption rate | N/A | 50% | **70%** | 90% | 95% |

## Key Performance Indicators

1. **P99 latency < 2ms** for simple requests at 10 concurrent connections (steady state)
2. **Throughput > 15,000 RPS** for simple requests at 10 concurrent connections (steady state)
3. **Scaling**: throughput drops < 50% when going from 10 to 1,000 concurrent connections
4. **Memory**: per-connection overhead < 64KB under load
5. **TLS overhead**: HTTPS achieves > 60% of HTTP throughput

## Benchmarking Methodology

### Environment
- Hardware: x86_64, 4+ cores, 8GB+ RAM
- OS: Linux kernel 5.x+
- Tool: wrk (or ab)
- Duration: 10s minimum for stable results
- Run: 10 rounds per endpoint, report median of last 7 rounds (steady state)
- Note: CPU thermal throttling causes 30-40% variance between cold and warm runs

### Commands
```bash
# Simple response
wrk -t2 -c10 -d10s http://localhost:8086/simple

# JSON response
wrk -t2 -c10 -d10s http://localhost:8086/json

# Large response
wrk -t2 -c10 -d10s http://localhost:8086/large

# High concurrency
wrk -t4 -c1000 -d10s http://localhost:8086/simple

# TLS (if available)
wrk -t4 -c100 -d10s https://localhost:8443/simple
```

### Test Server
```bash
cd uvhttp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/dist/bin/test_performance_e2e 8086
```

## Historical Tracking

| Date | Version | Simple RPS | JSON RPS | Large RPS | Notes |
|------|---------|-----------|----------|-----------|-------|
| 2026-07-25 | 2.5.1 | 21,529 | 21,451 | 21,314 | Baseline (single 5s run, turbo zone) |
| 2026-07-31 | 2.5.1 | 18,108 | — | — | Removed GCC extensions. ~6.6% perf cost. |
| 2026-08-12 | 2.6.1 | 25,949 | 26,088 | 25,154 | Single 5s run, turbo zone. Gold tier. |
| 2026-08-21 | 2.6.2 | 15,265* | 14,478* | 28,409* | 10 rounds, 10s each. Steady state. *Silver tier. |
| 2026-08-21 | 2.6.2 | 33,678† | 35,166† | 31,459† | †Peak turbo (first 3 rounds). Gold tier. |

> **Historical note**: Earlier baselines (2026-07-25, 2026-08-12) used single 5s runs which captured only the turbo-boosted zone. The 2026-08-21 multi-round baseline reveals that steady-state throughput is ~15K RPS (Silver tier), while peak turbo throughput is ~33K RPS (Gold tier). The difference is entirely due to CPU thermal throttling on the benchmark host, not code changes.