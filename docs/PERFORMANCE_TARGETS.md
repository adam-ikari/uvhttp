# UVHTTP Performance Targets

This document defines the performance targets and benchmarks for UVHTTP.

## Current Baseline (GitHub CI)

Measured 2026-08-21 on GitHub Actions `ubuntu-latest` runner (Release build, system allocator, 2 threads, 10 concurrent connections, 10s test, 10 rounds per endpoint, `benchmark_unified` binary).

### Why GitHub CI is the authoritative baseline

GitHub CI runners provide **consistent hardware** without the thermal throttling variance that plagues local benchmarks. Local benchmarks on AMD Ryzen 7 5800H showed 40%+ coefficient of variation due to CPU thermal throttling; CI runs show 0.4–2.4% CV.

> **That CV is within a single run.** The 10 rounds of one endpoint share one runner and one warm server, so they agree closely. Across *runs* of the same commit the medians do not: re-running `ci-benchmark.yml` on the unchanged 2026-08-21 control commit (2026-09-28) produced small-response medians in the ~40–54K band against the recorded 83K. Runners are shared VMs whose neighbours, CPU model, and frequency differ per allocation.
>
> Consequence: **absolute RPS numbers are comparable only as documentation, not as a gate.** The regression gate therefore measures head and base on the same runner in the same job and compares paired rounds — see [Regression Gate](#regression-gate-paired-same-runner).

### CI Runner Environment

Performance is measured on GitHub Actions `ubuntu-latest` runners. Standard runner specs:

| Item | Value |
|------|-------|
| OS | Ubuntu 22.04 LTS (or 24.04) |
| Architecture | x86_64 |
| CPU | 4-core (Intel Xeon or AMD EPYC) |
| RAM | 16 GB |
| Disk | 14 GB SSD |

> **Note**: GitHub Actions runners are shared VMs; actual CPU model and frequency may vary between runs. The `ci-benchmark.yml` workflow collects the exact environment info (CPU model, frequency, memory, kernel, compiler, wrk version) in each run and stores it as `benchmark-env.md` in the run artifacts.

> **Reproducibility**: To compare results, download the `benchmark-env.md` artifact from the specific CI run and verify the CPU model matches. Results from different runner generations (e.g., Intel Xeon vs AMD EPYC) are not directly comparable.

### Build Configuration

| Item | Value |
|------|-------|
| Build type | Release (`-DCMAKE_BUILD_TYPE=Release`) |
| Build options | `-DBUILD_BENCHMARKS=ON -DBUILD_EXAMPLES=OFF` |
| Binary | `benchmark_unified` (port 18081) |
| wrk params | `-t2 -c10 -d10s` (standard), `-t4 -c1000 -d10s` (high concurrency) |

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

### Regression Gate (paired, same runner)

`ci-benchmark.yml` gates PRs (with the `benchmark` label) and releases by
measuring **both revisions on the same runner in the same job**:

1. Resolve a base revision — PR base SHA, previous published release tag, or
   the `base_ref` input of a manual run.
2. Check out and build it next to head (`bench-base/`, Release, same flags).
3. Start both `benchmark_unified` servers (head on 18081, base on 18082),
   warm both up, then run the gated endpoints (`/`, `/json`, `/large`)
   alternately: head round *i*, base round *i*, 10 rounds each — odd rounds
   load head first, even rounds load base first, so a within-pair ordering
   effect is shared rather than assigned to one side.
4. `scripts/performance/regression_check.py head-paired.csv --compare base-paired.csv`
   pairs samples by round number and gates on the **median** of the per-round
   head/base ratios: an endpoint fails only when that median is below 90% **and**
   a majority of the individual pairs are also below 90%. The majority condition
   is what keeps heavy tails off the verdict — measured 2026-09-28, ten paired
   rounds of two byte-identical builds gave `/` a median of 97.4% with a median
   absolute deviation of 14.4%, i.e. single unlucky rounds land far off the
   median in both directions. A real regression (the writev small-body case,
   −14% with ~2% spread) puts nearly every pair under the limit and still fails.
   A confidence bound (median − 1.7·MAD-SE) was tried first and rejected: with
   heavy-tailed samples it inherits the tail and failed those identical builds.
   Fewer than 3 pairs, or no base sample, fails the gate too — an inconclusive
   measurement is not a pass.
5. The absolute `DEFAULT_BASELINE` comparison still runs, but as report-only
   output in the same log, so trends stay visible without gating on them.

If no base revision can be resolved the job falls back to the old absolute
baseline gate and says so in the log.

> Five self-test runs of this gate on PR #388, which changes no C code at all
> (base = the PR's own base SHA), all on 2026-09-28:
>
> | rule under test | `/` | `/json` | `/large` | verdict |
> |---|---|---|---|---|
> | 6 rounds, hard 90% cutoff on the median | 89.6% | 112.8% | 99.9% | false FAIL |
> | 10 rounds, median − 1.7·MAD-SE bound | 97.4% (MAD 14.4%) | 106.9% | 101.9% | false FAIL (bound 88.2%) |
> | 10 rounds, median + majority | 98.4% (3/10 under limit) | 95.9% | 100.6% | PASS |
> | same rule, re-run | 103.3% (1/10) | 107.9% | 100.9% | PASS |
> | same rule, re-run | 103.5% (0/10) | 100.5% | 99.4% | PASS |
>
> The raw per-round `/` ratios in the first passing run were
> 93 80 103 72 97 123 100 88 100 109 (% of base) — single rounds up to 28% below
> and 23% above the median, while `/large` in the same run stayed inside 95–107%.
> Absolute baseline in that same run reported 74.1% for `/`, which is the
> cross-run layer, not a regression. The rule has since passed three consecutive
> self-tests on identical code, and still fails the injected −14%/2%-spread case.

```bash
# Paired check outside CI: two CSVs measured on the same machine
python3 scripts/performance/regression_check.py head.csv --compare base.csv --threshold 0.10

# Legacy absolute comparison
python3 scripts/performance/regression_check.py benchmark-raw.csv --threshold 0.10
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
