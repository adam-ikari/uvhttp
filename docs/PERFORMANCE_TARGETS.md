# UVHTTP Performance Targets

This document defines the performance targets and benchmarks for UVHTTP.

## Current Baseline (GitHub CI)

Measured 2026-08-21 on GitHub Actions `ubuntu-latest` runner (Release build, system allocator, 2 threads, 10 concurrent connections, 10s test, 10 rounds per endpoint, `benchmark_unified` binary).

### Why GitHub CI is the authoritative baseline

GitHub CI runners provide **consistent hardware** without the thermal throttling variance that plagues local benchmarks. Local benchmarks on AMD Ryzen 7 5800H showed 40%+ coefficient of variation due to CPU thermal throttling; CI runners show 0.4–2.4% CV.

### Results

| Endpoint | Median (RPS) | Mean (RPS) | CV | Avg Latency | Steady (RPS) |
|----------|-------------|------------|-----|-------------|-------------|
| `/` (simple text) | **83,099** | 82,769 | 2.0% | 117µs | 83,008 |
| `/json` (application/json) | **81,848** | 81,711 | 2.4% | 117µs | 80,688 |
| `/large` (~100KB body) | **5,721** | 5,719 | 0.4% | 1.74ms | 5,732 |

> **Note**: The `/large` endpoint in `benchmark_unified` returns a ~100KB body, much larger than the ~1KB body in `test_performance_e2e`. The two binaries are not directly comparable.

### High concurrency (1000 connections, 3 rounds)

| Metric | Value |
|--------|-------|
| Throughput | ~55,000 RPS |
| Avg latency | ~27ms |
| Timeout errors | ~310/run (out of ~550K requests) |

### Peak vs Steady State

On CI runners, **peak and steady state are nearly identical** (no thermal throttling):

| Endpoint | Peak (first 3 median) | Steady (last 7 median) | Drop |
|----------|----------------------|------------------------|------|
| `/` | 83,190 | 83,008 | 0.2% |
| `/json` | 84,179 | 80,688 | 4.1% |
| `/large` | 5,716 | 5,732 | 0% |

## Targets

### Throughput

| Tier | Simple RPS | JSON RPS | Large RPS |
|------|-----------|----------|-----------|
| Bronze | 20,000 | 20,000 | 2,000 |
| Silver | 40,000 | 40,000 | 3,000 |
| Gold | 60,000 | 60,000 | 4,000 |
| **Platinum** | **80,000** | **80,000** | **5,000** |
| Diamond | 100,000 | 100,000 | 8,000 |

**Current tier: Platinum** (83K RPS simple, 82K RPS JSON, 5.7K RPS large)

### Latency

| Metric | Bronze | Silver | Gold | **Platinum** | Diamond |
|--------|--------|--------|------|----------|---------|
| Avg latency | <1ms | <500µs | <200µs | **<150µs** | <100µs |
| P99 latency | <5ms | <2ms | <1ms | **<500µs** | <200µs |

**Current tier: Platinum** (~117µs avg for simple/JSON)

### High Concurrency

| Concurrent Connections | Bronze | Silver | Gold | **Platinum** | Diamond |
|----------------------|--------|--------|------|----------|---------|
| 1,000 | 20,000 RPS | 30,000 RPS | 40,000 RPS | **50,000 RPS** | 80,000 RPS |

**Current tier: Platinum** (~55K RPS at 1000 connections)

## Key Performance Indicators

1. **Avg latency < 150µs** for simple requests at 10 concurrent connections
2. **Throughput > 80,000 RPS** for simple requests at 10 concurrent connections
3. **CV < 5%** across 10 rounds (CI runner stability)
4. **Scaling**: throughput drops < 40% when going from 10 to 1,000 concurrent connections
5. **Memory**: per-connection overhead < 64KB under load

## Benchmarking Methodology

### Primary: GitHub CI (authoritative)

Run via `ci-benchmark.yml` workflow:

- **Trigger**: `workflow_dispatch`, PR with `benchmark` label, push to `pre-release`
- **Runner**: `ubuntu-latest` (consistent hardware, no thermal throttling)
- **Build**: Release, `BUILD_BENCHMARKS=ON`
- **Binary**: `benchmark_unified` (port 18081)
- **Rounds**: 10 per endpoint, 10s each
- **Stats**: median, mean, CV, peak (first 3 median), steady (last 7 median)
- **Artifacts**: retained 90 days for trend tracking

```bash
# Trigger manually
gh workflow run ci-benchmark.yml --ref main -f rounds=10 -f duration=10

# Or via PR label
gh pr edit <PR_NUMBER> --add-label benchmark
```

### Secondary: Local benchmark (development)

For development-time quick checks. **Not authoritative** due to hardware variance.

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build -j$(nproc)

# Run
./build/dist/bin/benchmark_unified 18081 &
wrk -t2 -c10 -d10s http://localhost:18081/
wrk -t2 -c10 -d10s http://localhost:18081/json
wrk -t2 -c10 -d10s http://localhost:18081/large
```

> **Warning**: Local benchmarks on mobile/laptop CPUs (e.g., AMD Ryzen 7 5800H) may show 40%+ variance due to thermal throttling. Use CI results as the authoritative baseline.

### Endpoints (benchmark_unified)

| Endpoint | Response | Body Size |
|----------|----------|-----------|
| `/` | Simple text | ~2 bytes |
| `/json` | JSON | ~37 bytes |
| `/large` | Filled text | ~100KB |
| `/small` | Small text | ~128 bytes |
| `/medium` | Medium text | ~1KB |
| `/compression/text` | Compressible text | ~50KB |
| `/compression/json` | Compressible JSON | ~50KB |

## Historical Tracking

| Date | Version | Runner | Simple RPS | JSON RPS | Large RPS | CV | Notes |
|------|---------|--------|-----------|----------|-----------|-----|-------|
| 2026-07-25 | 2.5.1 | Local | 21,529 | 21,451 | 21,314 | — | Baseline (single 5s run) |
| 2026-08-12 | 2.6.1 | Local | 25,949 | 26,088 | 25,154 | — | Single 5s run, turbo zone |
| 2026-08-21 | 2.6.2 | Local | 15,265† | 14,478† | 28,409† | 40% | †Steady state, AMD 5800H throttled |
| 2026-08-21 | 2.6.2 | Local | 33,678‡ | 35,166‡ | 31,459‡ | 40% | ‡Peak turbo (first 3 rounds) |
| **2026-08-21** | **2.6.2** | **CI** | **83,099** | **81,848** | **5,721** | **2.0%** | **Authoritative baseline. GitHub CI runner.** |

> **Note on /large**: Local `test_performance_e2e` has ~1KB `/large` body; CI `benchmark_unified` has ~100KB `/large` body. The 5,721 RPS CI result for `/large` reflects the larger payload, not a performance regression.

## Change Log

| Date | Change |
|------|--------|
| 2026-08-21 | Switched primary baseline from local to GitHub CI. Added Platinum tier (80K RPS). Added CV as KPI. |
| 2026-08-21 | Added multi-round methodology (10 rounds, peak vs steady). Documented thermal throttling impact. |
| 2026-08-12 | Initial Gold tier baseline (25K RPS, single 5s run). |
