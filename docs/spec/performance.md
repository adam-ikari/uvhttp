# Performance Spec

## Overview

This spec defines the performance benchmarks, throughput targets, latency
budgets, and resource limits that UVHTTP must satisfy. All measurements are
taken on the reference benchmark host.

## Reference Benchmark Host

- **CPU**: AMD Ryzen 7 5800H, 12 cores
- **OS**: Linux 6.17.13-2-pve
- **Compiler**: GCC 11.4.0 Release build (`-O2 -DNDEBUG`)
- **Allocator**: System allocator
- **Test tool**: wrk 4.1.0
- **Test duration**: 10 seconds per run
- **Server**: built-in `test_performance_e2e`

## Throughput Requirements

| Scenario | Connections | Min RPS | Target RPS | Notes |
|----------|-------------|---------|------------|-------|
| Low concurrency | 10 | 18,000 | 19,887 | P50 < 0.5ms |
| Medium concurrency | 100 | 18,000 | 19,834 | Flat vs low |
| High concurrency | 500 | 18,000 | 19,810 | Flat vs 100 |
| Extreme concurrency | 1000 | 16,000 | 18,518 | Graceful degradation |
| JSON endpoint | 100 | 17,000 | 19,451 | |
| Large response 1KB | 100 | 17,000 | 19,524 | |
| **Socket errors** | **All** | **0** | **0** | Zero tolerance |

## Latency Requirements

| Percentile | Latency Budget |
|------------|----------------|
| P50 | < 5ms |
| P90 | < 10ms |
| P95 | < 15ms |
| P99 | < 25ms |

## Memory Requirements

| Metric | Budget | Notes |
|--------|--------|-------|
| Static library size | < 300 KB | Stripped Release build |
| Per-server instance | < 512 bytes | Plus per-connection allocations |
| Per-connection | < 4 KB | Request + response + buffers |
| Steady-state RSS | Flat | No growth under sustained load |
| Cold start | < 500ms | First byte after launch |

## Long-Run Stability

Under sustained load (100 connections, 60+ seconds), the server's RSS must
not grow. Measured with `scripts/performance/long_run_memory.sh`.

## Optimization Features

| Feature | Status | Impact |
|---------|--------|--------|
| Keep-Alive connection reuse | Active | ~1000x vs per-request |
| O(1) route lookup (trie) | Active | Eliminates linear scan |
| LRU cache for static files | Active | Reduces disk I/O |
| Zero-copy sendfile | Active | 50%+ CPU reduction for large files |
| Direct libuv calls | Design | No abstraction layer |
| Compile-time feature selection | Active | No runtime overhead for unused features |

## Test Requirements

- Performance regression test on every PR (via CI)
- Benchmark results must be reproducible
- Historical baseline stored in `docs/performance/baseline-history.json`
- RPS degradation > 5% from baseline must be investigated