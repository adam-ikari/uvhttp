# UVHTTP CI/CD Design Document

## Document Information

- **Project**: UVHTTP
- **Version**: 2.2.0
- **Creation Date**: 2026-02-23
- **Document Type**: Technical Solution Design
- **Status**: Implemented

---

## Table of Contents

1. [Overview](#overview)
2. [Design Principles](#design-principles)
3. [Workflow Architecture](#workflow-architecture)
4. [Core Workflows](#core-workflows)
5. [Reusable Actions](#reusable-actions)
6. [Configuration Matrix](#configuration-matrix)
7. [Trigger Conditions](#trigger-conditions)
8. [Concurrency Control](#concurrency-control)
9. [Build Configuration](#build-configuration)
10. [Testing Strategy](#testing-strategy)
11. [Coverage Management](#coverage-management)
12. [Performance Monitoring](#performance-monitoring)
13. [Security Scanning](#security-scanning)
14. [Release Process](#release-process)
15. [Monitoring and Alerts](#monitoring-and-alerts)
16. [Best Practices](#best-practices)
17. [Appendix](#appendix)

---

## Overview

### Background

UVHTTP is an HTTP/1.1 and WebSocket server library based on libuv, written in C99. To ensure code quality, performance stability, and continuous delivery capability, a CI/CD system needs to be built.

### Goals

1. **Code quality assurance**: ensure code quality through automated testing and static analysis
2. **Performance monitoring**: continuously monitor performance metrics to detect performance degradation in a timely manner
3. **Security scanning**: regularly scan for security vulnerabilities and check dependencies
4. **Fast feedback**: provide fast feedback at the PR stage to accelerate the development iteration cycle
5. **Automated release**: implement a fully automated flow from code to release
6. **Multi-platform support**: support Linux 32/64-bit, macOS, and Windows platforms
7. **Configuration validation**: verify the correctness of various compilation configurations

### Technology Stack

- **CI/CD platform**: GitHub Actions
- **Build system**: CMake 3.10+
- **Test framework**: Google Test
- **Performance testing**: wrk, ab
- **Code coverage**: lcov, gcov
- **Static analysis**: cppcheck, clang-tidy
- **Security scanning**: CodeQL, Dependency Check
- **Caching**: GitHub Actions Cache

---

## Design Principles

### 1. Fast Feedback

- **PR stage**: run only the necessary tests, completing within 10-15 minutes
- **Matrix validation**: use parallel strategies to validate multiple configurations simultaneously
- **Incremental testing**: prioritize testing of modules affected by the change

### 2. Comprehensive Coverage

- **Build matrix**: covers 14 compilation configuration combinations
- **Platform coverage**: Linux (32/64-bit), macOS, Windows
- **Test types**: unit tests, integration tests, performance tests, stress tests, memory tests

### 3. Tiered Validation

```
Level 1: PR fast validation (10-15 minutes)
  ├─ Build
  ├─ Fast tests
  ├─ Code format check
  └─ Dependency scan

Level 2: Push comprehensive validation (20-30 minutes)
  ├─ Build matrix (14 configurations)
  ├─ Full tests
  ├─ Static analysis
  └─ Performance benchmark tests

Level 3: Daily deep testing (60-90 minutes)
  ├─ Coverage reports
  ├─ Memory leak detection
  ├─ Stress tests
  ├─ Performance trend analysis
  └─ Security scanning

Level 4: Release validation (30-40 minutes)
  ├─ Multi-platform build
  ├─ Full test suite
  ├─ Release package packaging
  └─ Release creation
```

### 4. Maintainability

- **Reusable Actions**: encapsulate common logic as reusable Actions
- **Configuration driven**: use matrix configuration to avoid duplicate code
- **Complete documentation**: every workflow has detailed comments and documentation

### 5. Cost Optimization

- **Intelligent caching**: cache dependencies and build artifacts to reduce repeated compilation
- **Parallel execution**: leverage the matrix strategy to execute tasks in parallel
- **On-demand triggering**: select the appropriate workflow based on the event type

---

## Workflow Architecture

### Overall Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     GitHub Actions Triggers                 │
│  PR | Push | Tag | Schedule | Manual Dispatch               │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │   PR    │   │  Push   │   │  Nightly │
   │  Validate│   │  Validate│   │  Test   │
   └────┬────┘   └────┬────┘   └────┬────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
              ┌────────▼────────┐
              │   Shared Actions│
              │  setup-build    │
              │  cache-deps     │
              │  run-tests      │
              └────────┬────────┘
                       │
              ┌────────▼────────┐
              │  Build Matrix   │
              │  Validation (14 │
              │  configurations)│
              └────────┬────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │  Test   │   │ Analysis│   │Performance│
   │  Execute│   │ Report  │   │ Monitor │
   └────┬────┘   └────┬────┘   └────┬────┘
        │              │              │
        └──────────────┼──────────────┘
                       │
              ┌────────▼────────┐
              │  Result Summary │
              │   GitHub Summary│
              └─────────────────┘
```

### Workflow List

| Workflow Name | Trigger Condition | Purpose | Execution Time |
|-----------|---------|------|---------|
| `ci-pr.yml` | PR to main/develop | PR fast validation | 10-15 minutes |
| `ci-build-matrix.yml` | Push to main/develop/pre-release | Build matrix validation | 20-30 minutes |
| `ci-32bit.yml` | Push to main/develop/feature/* | 32-bit compilation validation | 2-3 minutes |
| `ci-performance.yml` | Push to develop | Performance benchmark tests | 15-20 minutes |
| `ci-performance-tls.yml` | Push to develop | TLS performance tests | 15-20 minutes |
| `ci-nightly.yml` | Daily UTC 0:00 | Deep testing and reporting | 60-90 minutes |
| `ci-release.yml` | Push tag (v*) | Release process | 30-40 minutes |
| `deploy-docs.yml` | Push to main | Documentation deployment | 5-10 minutes |
| `performance-benchmark.yml` | Manual | Performance baseline comparison | 20-30 minutes |
| `security-issue-creator.yml` | Security alerts | Automatically create security issues | Immediate |
| `notify.yml` | Workflow completion | Notifications and reporting | Immediate |

---

## Core Workflows

### 1. PR Fast Validation (ci-pr.yml)

#### Goals

Provide fast feedback when a PR is submitted, ensuring the code compiles and passes basic tests.

#### Trigger Condition

```yaml
on:
  pull_request:
    branches: [ main, develop ]
    types: [opened, synchronize, reopened]
```

#### Task List

```
Phase 1: Parallel execution
├─ ubuntu-build: Ubuntu build and test
├─ code-quality-check: Code quality check
├─ dependency-scan: Dependency vulnerability scan
├─ ubuntu-test-fast: Fast tests
└─ format-check: Code format check

Phase 2: Result summary
└─ generate-summary: Generate the PR summary
```

#### Execution Time

- **Total time**: 10-15 minutes
- **Parallel tasks**: 5
- **Critical path**: ubuntu-build → ubuntu-test-fast → generate-summary

#### Validation Content

| Category | Validation Item | Tool |
|-----|-------|------|
| Build | 64-bit Release build | CMake |
| Testing | Fast test suite | Google Test |
| Quality | cppcheck static analysis | cppcheck |
| Quality | Code format check | clang-format |
| Security | Dependency vulnerability scan | npm audit |

#### Output Artifacts

- Build artifacts: `build-ubuntu-pr-{PR_NUMBER}.zip`
- Test logs: `test-logs-ubuntu-pr-{PR_NUMBER}.zip`
- Quality report: `code-quality-pr-{PR_NUMBER}.zip`
- GitHub Summary: test results displayed on the PR page

#### Failure Strategy

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.event.pull_request.number }}
  cancel-in-progress: true
```

- New commits to the same PR cancel old runs
- Failure of any task causes the overall failure
- Failed tasks retain their logs for debugging

---

### 2. Build Matrix Validation (ci-build-matrix.yml)

#### Goals

Validate the correctness of all compilation configuration combinations, ensuring the library works correctly under various configurations.

#### Trigger Condition

```yaml
on:
  pull_request:
    branches: [ main, develop ]
    types: [opened, synchronize, reopened]
  push:
    branches: [ main, develop, pre-release ]
  workflow_dispatch:
```

#### Configuration Matrix

```yaml
strategy:
  fail-fast: false
  matrix:
    config:
      # Basic configurations
      - name: "Minimal (no optional features)"
        websocket: OFF
        mimalloc: OFF
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      # Full-feature configurations
      - name: "Full Features"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      # Individual feature tests
      - name: "WebSocket Only"
        websocket: ON
        mimalloc: OFF
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "HTTPS Only"
        websocket: OFF
        mimalloc: OFF
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "mimalloc Only"
        websocket: OFF
        mimalloc: ON
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      # Special modes
      - name: "Debug Mode"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: ON
        coverage: OFF
        examples: OFF

      - name: "Coverage Mode"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: ON
        examples: OFF

      # Combination tests
      - name: "WebSocket + HTTPS"
        websocket: ON
        mimalloc: OFF
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "WebSocket + mimalloc"
        websocket: ON
        mimalloc: ON
        https: OFF
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "HTTPS + mimalloc"
        websocket: OFF
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "WebSocket + HTTPS + mimalloc"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: OFF

      - name: "Debug + Coverage"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: ON
        coverage: ON
        examples: OFF

      - name: "Minimal + Debug"
        websocket: OFF
        mimalloc: OFF
        https: OFF
        debug: ON
        coverage: OFF
        examples: OFF

      - name: "Full + Examples"
        websocket: ON
        mimalloc: ON
        https: ON
        debug: OFF
        coverage: OFF
        examples: ON
```

#### Configuration Description

| Configuration | WebSocket | mimalloc | HTTPS | Debug | Coverage | Examples |
|-----|-----------|----------|-------|-------|----------|----------|
| Minimal | OFF | OFF | OFF | OFF | OFF | OFF |
| Full Features | ON | ON | ON | OFF | OFF | OFF |
| WebSocket Only | ON | OFF | OFF | OFF | OFF | OFF |
| HTTPS Only | OFF | OFF | ON | OFF | OFF | OFF |
| mimalloc Only | OFF | ON | OFF | OFF | OFF | OFF |
| Debug Mode | ON | ON | ON | ON | OFF | OFF |
| Coverage Mode | ON | ON | ON | OFF | ON | OFF |
| WebSocket + HTTPS | ON | OFF | ON | OFF | OFF | OFF |
| WebSocket + mimalloc | ON | ON | OFF | OFF | OFF | OFF |
| HTTPS + mimalloc | OFF | ON | ON | OFF | OFF | OFF |
| WebSocket + HTTPS + mimalloc | ON | ON | ON | OFF | OFF | OFF |
| Debug + Coverage | ON | ON | ON | ON | ON | OFF |
| Minimal + Debug | OFF | OFF | OFF | ON | OFF | OFF |
| Full + Examples | ON | ON | ON | OFF | OFF | ON |

#### Execution Time

- **Single configuration**: 2-3 minutes
- **Total time**: 20-30 minutes (14 configurations executed in parallel)

#### Validation Content

| Category | Validation Item | Standard |
|-----|-------|------|
| Build | CMake configuration succeeds | exit code 0 |
| Build | Compilation without warnings | no "warning:" output |
| Testing | Tests pass | all tests pass |
| Testing | Test skipping handling | exit code 8 indicates expected skips |
| Coverage | Coverage report generated | coverage.info exists |
| Artifacts | Build artifacts complete | libuvhttp.a exists |

#### Output Artifacts

- Build artifacts for each configuration: `build-{CONFIG_NAME}-{PR_NUMBER}.zip`
- Test logs: `test-logs-{CONFIG_NAME}-{PR_NUMBER}.zip`
- Coverage reports: generated for Coverage configurations only

#### Failure Strategy

```yaml
strategy:
  fail-fast: false
```

- `fail-fast: false`: even if one configuration fails, other configurations continue to execute
- Facilitates discovering all configuration issues at once
- Each configuration runs independently without affecting others

---

### 3. 32-bit Compilation Validation (ci-32bit.yml)

#### Goals

Validate compilation correctness on the 32-bit (i686) architecture, ensuring the library can be used on 32-bit systems.

#### Trigger Condition

```yaml
on:
  push:
    branches: [ main, develop, 'feature/**' ]
  pull_request:
    branches: [ main, develop ]
  workflow_dispatch:
```

#### Technical Challenges

1. **Cross-compilation**: compile 32-bit code on a 64-bit system
2. **Dependency libraries**: all dependencies (libuv, mbedtls, llhttp) must be compiled as 32-bit
3. **C++ linking**: 32-bit libstdc++ linking issues in the CI/CD environment
4. **Architecture validation**: must verify that the generated library is actually 32-bit

#### Solutions

##### 1. 32-bit Toolchain

```yaml
- name: Install 32-bit toolchain
  run: |
    sudo apt-get update
    sudo apt-get install -y gcc-multilib g++-multilib g++-12-multilib
```

##### 2. Compiler Flags

```yaml
cmake -B build-32bit \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE -Wno-format-truncation" \
  -DEXE_LINKER_FLAGS="-m32" \
  -DSHARED_LINKER_FLAGS="-m32" \
  -DMODULE_LINKER_FLAGS="-m32" \
  -DBUILD_WITH_WEBSOCKET=ON \
  -DBUILD_WITH_MIMALLOC=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DENABLE_COVERAGE=OFF
```

Key flag descriptions:

| Flag | Effect |
|-----|------|
| `-m32` | Generate 32-bit code |
| `-march=i686` | Specify the target architecture |
| `-D_GNU_SOURCE` | Enable GNU extensions |
| `-Wno-format-truncation` | Disable format truncation warnings |

##### 3. Dependency Library Compilation

Modify `cmake/Dependencies.cmake` to pass compiler flags to the dependency libraries:

```cmake
execute_process(
  COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv -B ${LIBUV_BUILD_DIR}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DBUILD_TESTING=OFF
    -DLIBUV_BUILD_SHARED=OFF
    -DLIBUV_BUILD_BENCH=OFF
    -DLIBUV_BUILD_EXAMPLES=OFF
    "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
  RESULT_VARIABLE LIBUV_CONFIG_RESULT
)
```

##### 4. Skip C++ Tests

Due to the 32-bit libstdc++ linking issue, only the core library is built; tests are not compiled:

```yaml
cmake --build build-32bit --config Release --target uvhttp -j$(nproc)
```

##### 5. Architecture Validation

Use `objdump` to verify that the library file is 32-bit:

```bash
objdump -f build-32bit/dist/lib/libuvhttp.a | grep -q "elf32-i386"
```

#### Task List

```
Phase 1: Build
└─ ubuntu-32bit-build: 32-bit compilation and validation

Phase 2: Testing
└─ ubuntu-32bit-test: basic validation

Phase 3: Summary
└─ generate-summary: generate the summary
```

#### Execution Time

- **Total time**: 2-3 minutes
- **Build**: 1.5 minutes
- **Testing**: 0.5 minutes
- **Validation**: immediate

#### Validation Content

| Category | Validation Item | Tool/Method |
|-----|-------|----------|
| Build | 32-bit compilation succeeds | CMake + gcc -m32 |
| Architecture | Main library is 32-bit | objdump -f |
| Architecture | libuv is 32-bit | objdump -f |
| Architecture | mbedtls is 32-bit | objdump -f |
| Architecture | llhttp is 32-bit | objdump -f |
| File | Library files exist | file + stat |
| File | Library files are non-empty | file size > 0 |

#### Output Artifacts

- Build artifacts: `build-32bit-{SHA}.zip`
- Test logs: `test-logs-32bit-{SHA}.zip`
- GitHub Summary: 32-bit validation summary

#### Limitations and Notes

1. **C++ tests skipped**: due to the 32-bit libstdc++ linking issue in the CI/CD environment, C++ tests are not compiled
2. **mimalloc disabled**: the mimalloc allocator is not used in the 32-bit environment
3. **WebSocket enabled**: 32-bit compilation with WebSocket support

#### Local Validation

Developers can validate 32-bit compilation locally:

```bash
# Install the 32-bit toolchain
sudo apt-get install gcc-multilib g++-multilib

# Configure and compile the 32-bit version
cmake -B build-32bit \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE" \
  -DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"

cmake --build build-32bit --config Release

# Verify the architecture
file build-32bit/dist/lib/libuvhttp.a
objdump -f build-32bit/dist/lib/libuvhttp.a | head -1
```

---

### 4. Performance Benchmark Tests (ci-performance.yml)

#### Goals

Monitor performance metrics to detect performance degradation in a timely manner.

#### Trigger Condition

```yaml
on:
  push:
    branches: [ develop ]
  workflow_dispatch:
```

#### Test Scenarios

| Scenario | Concurrency | Duration | Target |
|-----|-------|---------|------|
| Low concurrency | 10 | 30s | Baseline performance |
| Medium concurrency | 50 | 30s | Regular load |
| High concurrency | 100 | 30s | Peak performance |
| Extreme concurrency | 500 | 30s | Stress testing |
| Sustained load | 100 | 60s | Stability |

#### Test Tools

```bash
# Install wrk
sudo apt-get install wrk

# Run the test
wrk -t4 -c100 -d30s http://localhost:18081/
```

#### Performance Metrics

| Metric | Target Value | Description |
|-----|-------|------|
| RPS (low concurrency) | > 30,000 | 10 connections |
| RPS (medium concurrency) | > 30,000 | 50 connections |
| RPS (high concurrency) | > 30,000 | 100 connections |
| RPS (extreme concurrency) | > 25,000 | 500 connections |
| Error rate | < 0.1% | all scenarios |
| P50 latency | < 5ms | median |
| P95 latency | < 20ms | 95% of requests |
| P99 latency | < 50ms | 99% of requests |

#### Execution Flow

```
1. Start the test server
   ├─ Compile benchmark_unified
   ├─ Start the server (port 18081)
   └─ Wait for the server to be ready (3s)

2. Run the performance tests
   ├─ Test 1: low concurrency (10 connections, 30s)
   ├─ Test 2: medium concurrency (50 connections, 30s)
   ├─ Test 3: high concurrency (100 connections, 30s)
   ├─ Test 4: extreme concurrency (500 connections, 30s)
   └─ Test 5: sustained load (100 connections, 60s)

3. Parse the test results
   ├─ Extract the RPS metrics
   ├─ Extract the latency metrics
   └─ Extract the error rate

4. Generate the report
   ├─ JSON-format results
   ├─ Markdown report
   └─ Performance trend chart

5. Compare against the baseline
   ├─ Read the baseline data
   ├─ Calculate the performance change
   └─ Flag degradation (>5%)
```

#### Output Artifacts

- Performance results: `performance-results.json`
- Test logs: `performance.log`
- Performance report: `performance-report.md`
- Trend chart: `performance-trend.png`

#### Performance Degradation Detection

```python
# Compare against the baseline
baseline_rps = 23000
current_rps = 21000
degradation = (baseline_rps - current_rps) / baseline_rps * 100

if degradation > 5:
    print(f"Performance degradation: {degradation:.2f}%")
```

#### Baseline Management

- **Baseline location**: `config/performance-baseline.yml`
- **Update timing**: after performance optimization or architecture changes
- **Validation process**: run 3 times and take the average

#### Execution Time

- **Total time**: 15-20 minutes
- **Server startup**: 1 minute
- **Test execution**: 10-15 minutes
- **Report generation**: 2 minutes

---

### 5. TLS Performance Tests (ci-performance-tls.yml)

#### Goals

Validate the impact of TLS encryption on performance, ensuring that TLS performance meets requirements.

#### Trigger Condition

```yaml
on:
  push:
    branches: [ develop ]
  workflow_dispatch:
```

#### Test Scenarios

| Scenario | Encryption Method | Concurrency | Duration |
|-----|---------|-------|---------|
| HTTPS (TLS 1.2) | AES-256-GCM | 100 | 30s |
| HTTPS (TLS 1.3) | AES-256-GCM | 100 | 30s |
| HTTPS (TLS 1.3) | ChaCha20-Poly1305 | 100 | 30s |

#### Performance Metrics

| Metric | HTTPS Target | HTTP Target | Degradation Limit |
|-----|-----------|----------|---------|
| RPS | > 20,000 | > 30,000 | < 35% |
| P50 latency | < 10ms | < 5ms | < 2x |
| P95 latency | < 40ms | < 20ms | < 2x |

#### Output Artifacts

- HTTPS performance results: `https-performance-results.json`
- HTTPS performance report: `https-performance-report.md`
- HTTP vs HTTPS comparison: `https-comparison.md`

---

### 6. Daily Deep Testing (ci-nightly.yml)

#### Goals

Run deep tests daily, generating coverage reports, memory leak detection, stress tests, and performance trend analysis.

#### Trigger Condition

```yaml
on:
  schedule:
    - cron: '0 0 * * *'  # Daily UTC 0:00
  workflow_dispatch:
```

#### Task List

```
Phase 1: Parallel build and scanning
├─ ubuntu-build-all: Debug + Coverage build
├─ code-quality-full: full code quality check
└─ security-scan-full: full security scan

Phase 2: Full testing
├─ ubuntu-test-full: full test suite
└─ test-coverage: coverage report generation

Phase 3: Deep testing
├─ coverage-report: coverage HTML report
├─ test-stress: stress test (5 minutes)
├─ performance-full: full performance benchmark
└─ test-memory: memory leak detection (ASan)

Phase 4: Generate summary
└─ generate-summary: generate the daily report
```

#### Execution Time

- **Total time**: 60-90 minutes
- **Build**: 25 minutes
- **Testing**: 15 minutes
- **Coverage**: 20 minutes
- **Stress testing**: 35 minutes
- **Performance testing**: 90 minutes
- **Memory testing**: 25 minutes

#### Test Details

##### 1. Coverage Report

```bash
# Generate the coverage data
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/deps/*' --output-file coverage.info
lcov --remove coverage.info '*/test/*' --output-file coverage.info

# Generate the HTML report
genhtml coverage.info --output-directory coverage-report
```

Coverage targets:

| Module | Target Coverage | Current Coverage |
|-----|-----------|-----------|
| Overall | 80% | 42.9% |
| uvhttp_static.c | 80% | 17.2% |
| uvhttp_router.c | 80% | 32.3% |
| uvhttp_connection.c | 80% | 32.2% |

##### 2. Stress Test

```bash
# Sustained high load test (5 minutes)
wrk -t8 -c500 -d300s --timeout 10s http://localhost:18081/
```

Validation content:

- Error rate < 0.1%
- No memory leaks
- No crashes
- Stable response times

##### 3. Memory Leak Detection

```bash
# Compile with AddressSanitizer
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

# Run the tests
ctest --output-on-failure
```

Validation content:

- No memory leaks
- No use-after-free
- No double free
- No buffer overflows

##### 4. Performance Trend Analysis

Run 8 performance test scenarios:

1. Low concurrency (10 connections, 30s)
2. Medium concurrency (50 connections, 30s)
3. High concurrency (100 connections, 30s)
4. Extreme concurrency (500 connections, 30s)
5. Ultra-high concurrency (1000 connections, 30s)
6. Sustained load (100 connections, 60s)
7. Small file transfer
8. Large file transfer

Generate a performance trend chart and compare against historical data.

#### Output Artifacts

- Coverage report: `coverage-report-nightly-{RUN_NUMBER}.zip`
- Performance results: `performance-full-nightly-{RUN_NUMBER}.zip`
- Stress test results: `stress-test-results-nightly-{RUN_NUMBER}.zip`
- Memory test results: `memory-test-results-nightly-{RUN_NUMBER}.zip`
- Security scan results: `security-results-nightly-{RUN_NUMBER}.zip`
- Code quality results: `code-quality-results-nightly-{RUN_NUMBER}.zip`

#### Nightly Release

Create a pre-release nightly version that includes all test artifacts:

```yaml
- name: Create nightly release
  uses: softprops/action-gh-release@v1
  with:
    tag_name: nightly-${{ github.run_number }}
    name: Nightly Build - ${{ github.event.head_commit.timestamp }}
    body_path: performance-trend.md
    draft: false
    prerelease: true
```

---

### 7. Release Process (ci-release.yml)

#### Goals

Automate the release process, fully automating the flow from code to Release.

#### Trigger Condition

```yaml
on:
  push:
    tags:
      - 'v*'  # e.g. v1.5.0, v2.0.0
  workflow_dispatch:
```

#### Task List

```
Phase 1: Linux Release build
└─ ubuntu-release-build: build in Release mode

Phase 2: Release tests
└─ release-test: full test suite

Phase 3: Create the Release
└─ create-release: create the GitHub Release
```

#### Execution Time

- **Total time**: 30-40 minutes
- **Build**: 15 minutes
- **Testing**: 20 minutes
- **Packaging**: 2 minutes
- **Release creation**: 3 minutes

#### Release Flow

```
1. Parse the version number
   ├─ Extract the version number from the tag (e.g. v2.3.0 → 2.3.0)
   └─ Validate the version number format

2. Build the release version
   ├─ CMake Release mode
   ├─ Compile all modules
   └─ Run the full test suite

3. Package the release artifacts
   ├─ Copy the library file (libuvhttp.a)
   ├─ Copy the header files (include/)
   ├─ Create an archive (tar.gz)
   └─ Calculate the checksum (SHA256)

4. Create the GitHub Release
   ├─ Use softprops/action-gh-release
   ├─ Upload the archive file
   ├─ Automatically generate Release Notes
   └─ Mark as an official release
```

#### Release Artifacts

- Linux x86_64: `uvhttp-{VERSION}-linux-x86_64.tar.gz`
- Checksum: `uvhttp-{VERSION}-SHA256.txt`
- Release Notes: automatically generated

#### Version Number Convention

Follows Semantic Versioning (SemVer):

```
MAJOR.MINOR.PATCH

MAJOR: incompatible API changes
MINOR: backward-compatible feature additions
PATCH: backward-compatible bug fixes
```

Examples:

- `v2.3.0`: 2.3.0 official release
- `v2.3.1`: patch version of 2.3.0
- `v3.0.0`: 3.0.0 major version upgrade

#### Release Checklist

Before a release, the following must be ensured:

- [ ] All CI/CD tests pass
- [ ] Code coverage meets the target (80%+)
- [ ] Performance tests pass
- [ ] Security scan shows no high-severity vulnerabilities
- [ ] Documentation has been updated
- [ ] CHANGELOG has been updated
- [ ] Version number has been updated
- [ ] Git tag has been created

---

## Reusable Actions

### 1. setup-build

#### Path

`.github/actions/setup-build`

#### Function

Sets up the build environment, including:

- Installing CMake
- Installing the compiler
- Configuring compilation options

#### Usage Example

```yaml
- name: Setup build environment
  uses: ./.github/actions/setup-build
  with:
    os: ubuntu-latest
```

#### Parameters

| Parameter | Type | Required | Description |
|-----|------|------|------|
| os | string | Yes | Operating system (ubuntu-latest, macos-latest, windows-latest) |

---

### 2. cache-deps

#### Path

`.github/actions/cache-deps`

#### Function

Caches dependency libraries to reduce repeated compilation time.

#### Usage Example

```yaml
- name: Cache dependencies
  uses: ./.github/actions/cache-deps
  with:
    cache-key: pr-${{ github.event.pull_request.number }}
    build-type: Release
```

#### Parameters

| Parameter | Type | Required | Description |
|-----|------|------|------|
| cache-key | string | Yes | Cache key name |
| build-type | string | Yes | Build type (Debug, Release) |

#### Cache Strategy

```yaml
- uses: actions/cache@v3
  with:
    path: |
      deps/*/build
      build/CMakeCache.txt
      build/CMakeFiles
    key: ${{ inputs.cache-key }}-${{ runner.os }}-${{ inputs.build-type }}-${{ hashFiles('CMakeLists.txt', 'cmake/*.cmake') }}
    restore-keys: |
      ${{ inputs.cache-key }}-${{ runner.os }}-${{ inputs.build-type }}-
      ${{ inputs.cache-key }}-${{ runner.os }}-
```

---

### 3. run-tests

#### Path

`.github/actions/run-tests`

#### Function

Runs the test suite, supporting different test types.

#### Usage Example

```yaml
- name: Run fast tests
  uses: ./.github/actions/run-tests
  with:
    build-dir: build
    test-type: fast
    timeout: 60
    parallel: $(nproc)
```

#### Parameters

| Parameter | Type | Required | Description |
|-----|------|------|------|
| build-dir | string | Yes | Build directory |
| test-type | string | Yes | Test type (fast, all, coverage, memory) |
| timeout | number | No | Timeout in seconds, default 60 |
| parallel | number | No | Number of parallel jobs, default 1 |

#### Test Types

| Type | Description | Purpose |
|-----|------|------|
| fast | Fast test suite | PR validation |
| all | Full test suite | Push validation |
| coverage | Coverage tests | Nightly testing |
| memory | Memory tests (ASan) | Nightly testing |

---

## Configuration Matrix

### Build Matrix Summary

| Configuration | WebSocket | mimalloc | HTTPS | Debug | Coverage | Examples | Purpose |
|-----|-----------|----------|-------|-------|----------|----------|------|
| Minimal | OFF | OFF | OFF | OFF | OFF | OFF | Minimal dependency validation |
| Full Features | ON | ON | ON | OFF | OFF | OFF | Full-feature validation |
| WebSocket Only | ON | OFF | OFF | OFF | OFF | OFF | WebSocket standalone testing |
| HTTPS Only | OFF | OFF | ON | OFF | OFF | OFF | HTTPS standalone testing |
| mimalloc Only | OFF | ON | OFF | OFF | OFF | OFF | mimalloc standalone testing |
| Debug Mode | ON | ON | ON | ON | OFF | OFF | Debug mode validation |
| Coverage Mode | ON | ON | ON | OFF | ON | OFF | Coverage testing |
| WebSocket + HTTPS | ON | OFF | ON | OFF | OFF | OFF | Combination validation |
| WebSocket + mimalloc | ON | ON | OFF | OFF | OFF | OFF | Combination validation |
| HTTPS + mimalloc | OFF | ON | ON | OFF | OFF | OFF | Combination validation |
| WebSocket + HTTPS + mimalloc | ON | ON | ON | OFF | OFF | OFF | Full combination |
| Debug + Coverage | ON | ON | ON | ON | ON | OFF | Debug + coverage |
| Minimal + Debug | OFF | OFF | OFF | ON | OFF | OFF | Minimal + Debug |
| Full + Examples | ON | ON | ON | OFF | OFF | ON | Full + Examples |

### Matrix Validation Strategy

1. **Independent validation**: each configuration runs independently without affecting others
2. **Failures do not stop**: `fail-fast: false` to discover all issues
3. **Parallel execution**: leverage the GitHub Actions parallel capability
4. **Result aggregation**: uniformly generate a test summary

---

## Trigger Conditions

### Trigger Types

| Trigger Type | Trigger Condition | Workflow |
|---------|---------|-------|
| PR | PR to main/develop | ci-pr.yml |
| Push | Push to main/develop/pre-release | ci-build-matrix.yml |
| Push | Push to main/develop/feature/* | ci-32bit.yml |
| Push | Push to develop | ci-performance.yml |
| Push | Push to main | deploy-docs.yml |
| Tag | Push tag (v*) | ci-release.yml |
| Schedule | Daily UTC 0:00 | ci-nightly.yml |
| Manual | Workflow Dispatch | all workflows |

### Branch Strategy

```
main (production branch)
├─ Triggers: ci-pr.yml, ci-build-matrix.yml, ci-32bit.yml
├─ Merge source: pre-release
└─ Protection rule: CI/CD must pass

develop (development branch)
├─ Triggers: ci-pr.yml, ci-build-matrix.yml, ci-32bit.yml, ci-performance.yml, ci-performance-tls.yml
├─ Merge source: feature/*
└─ Protection rule: CI/CD must pass

pre-release (pre-release branch)
├─ Triggers: ci-build-matrix.yml
├─ Merge source: main
└─ Protection rule: CI/CD must pass

feature/* (feature branches)
├─ Triggers: ci-32bit.yml
├─ Merge source: develop
└─ Protection rule: none
```

---

## Concurrency Control

### Concurrency Strategy

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

### Workflow Concurrency Groups

| Workflow | Concurrency Group | Cancellation Policy |
|-------|-------|---------|
| ci-pr.yml | ci-pr-{PR_NUMBER} | Cancel old runs |
| ci-build-matrix.yml | ci-build-matrix-{BRANCH} | Cancel old runs |
| ci-32bit.yml | ci-32bit-{BRANCH} | Cancel old runs |
| ci-performance.yml | ci-performance-{SHA} | Do not cancel |
| ci-nightly.yml | ci-nightly-{CRON} | Do not cancel |
| ci-release.yml | ci-release-{TAG} | Do not cancel |

### Cancellation Policy Notes

- **PR workflows**: new commits to the same PR cancel old runs to save resources
- **Push workflows**: new pushes to the same branch cancel old runs
- **Scheduled tasks**: not cancelled, ensuring the daily tests always execute
- **Release workflows**: not cancelled, ensuring the complete release process

---

## Build Configuration

### Compiler Versions

| Platform | Compiler | Version |
|-----|-------|------|
| Linux | gcc | 12.x |
| Linux | g++ | 12.x |
| macOS | clang | Apple LLVM 14+ |
| Windows | MSVC | Visual Studio 2022 |

### CMake Versions

- **Minimum version**: 3.10
- **Recommended version**: 3.25+
- **GitHub Actions**: 3.25.1

### Compilation Options

#### Release Mode

```cmake
-DCMAKE_BUILD_TYPE=Release
-DCMAKE_C_FLAGS="-O3 -DNDEBUG"
-DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"
```

#### Debug Mode

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-O0 -g -DDEBUG"
-DCMAKE_CXX_FLAGS="-O0 -g -DDEBUG"
```

#### 32-bit Mode

```cmake
-DCMAKE_C_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"
-DCMAKE_CXX_FLAGS="-m32 -march=i686 -D_GNU_SOURCE"
-DEXE_LINKER_FLAGS="-m32"
-DSHARED_LINKER_FLAGS="-m32"
```

#### Coverage Mode

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-O0 -g --coverage"
-DCMAKE_CXX_FLAGS="-O0 -g --coverage"
```

#### ASan Mode

```cmake
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
```

---

## Testing Strategy

### Test Classification

#### 1. Unit Tests

- **Purpose**: test individual functions and modules
- **Tool**: Google Test
- **Execution time**: 1-2 minutes
- **Coverage target**: 80%+

#### 2. Integration Tests

- **Purpose**: test interactions between modules
- **Tool**: wrk, ab
- **Execution time**: 3-5 minutes
- **Test scenarios**:
  - HTTP request/response
  - WebSocket connections
  - Static file serving
  - Route matching

#### 3. Performance Tests

- **Purpose**: monitor performance metrics
- **Tool**: wrk, ab
- **Execution time**: 10-15 minutes
- **Test scenarios**:
  - Low/medium/high concurrency
  - Sustained load
  - TLS performance

#### 4. Stress Tests

- **Purpose**: validate stability
- **Tool**: wrk
- **Execution time**: 35 minutes
- **Test scenarios**:
  - 500 concurrent connections for 5 minutes
  - 1000 concurrent connections for 30 seconds

#### 5. Memory Tests

- **Purpose**: detect memory issues
- **Tool**: AddressSanitizer
- **Execution time**: 20-25 minutes
- **Detection content**:
  - Memory leaks
  - Use-after-free
  - Double free
  - Buffer overflows

### Test Execution

#### Fast Tests (PR validation)

```bash
ctest --output-on-failure --timeout 60 -j$(nproc) \
  --label-regex "fast"
```

#### Full Tests (Push validation)

```bash
ctest --output-on-failure --timeout 180 -j$(nproc)
```

#### Coverage Tests (Nightly)

```bash
ctest --output-on-failure --timeout 120 -j$(nproc)
lcov --capture --directory . --output-file coverage.info
```

#### Memory Tests (Nightly)

```bash
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0 \
  ctest --output-on-failure --timeout 120 -j1
```

### Test Result Handling

#### Exit Code Handling

```bash
ctest --output-on-failure || {
  EXIT_CODE=$?
  if [ $EXIT_CODE -eq 8 ]; then
    echo "Some tests were skipped (expected when optional features are disabled)"
    exit 0
  else
    echo "Tests failed with exit code: $EXIT_CODE"
    exit $EXIT_CODE
  fi
}
```

Exit code descriptions:

| Exit Code | Meaning |
|-------|------|
| 0 | All tests passed |
| 1-7 | Test failures |
| 8 | Tests skipped (expected behavior) |

---

## Coverage Management

### Coverage Targets

| Module | Target Coverage | Current Coverage | Status |
|-----|-----------|-----------|------|
| Overall | 80% | 42.9% | needs improvement |
| uvhttp_static.c | 80% | 17.2% | severely insufficient |
| uvhttp_router.c | 80% | 32.3% | needs improvement |
| uvhttp_connection.c | 80% | 32.2% | needs improvement |
| uvhttp_server.c | 80% | 38.4% | needs improvement |
| uvhttp_request.c | 80% | 40.5% | needs improvement |
| uvhttp_tls.c | 80% | 38.3% | needs improvement |

### Coverage Report Generation

```bash
# Generate the coverage data
lcov --capture --directory . --output-file coverage.info

# Filter out unnecessary files
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info '*/deps/*' --output-file coverage.info
lcov --remove coverage.info '*/test/*' --output-file coverage.info

# Generate the HTML report
genhtml coverage.info --output-directory coverage-report

# View the coverage
lcov --list coverage.info
```

### Coverage Upload

```yaml
- name: Upload coverage to Codecov
  uses: codecov/codecov-action@v4
  with:
    files: ./build/coverage.info
    flags: nightly
    name: nightly-coverage
    fail_ci_if_error: false
```

### Coverage Improvement Strategy

1. **Identify low-coverage modules**
   - Use `lcov --list coverage.info` to view
   - Prioritize improving the modules with the lowest coverage

2. **Add test cases**
   - Add tests for uncovered code paths
   - Use linker wrap to implement mocks

3. **Regular review**
   - Nightly tests generate the coverage report
   - Review coverage changes on a weekly basis

4. **Quality gate**
   - New code coverage must be no lower than 80%
   - Overall coverage target of 80%

---

## Performance Monitoring

### Performance Baseline

#### Baseline Configuration

```yaml
# config/performance-baseline.yml
baseline:
  low_concurrent:
    rps: 30000
    latency_p50: "2ms"
    latency_p95: "5ms"
    latency_p99: "10ms"
  
  medium_concurrent:
    rps: 30000
    latency_p50: "3ms"
    latency_p95: "8ms"
    latency_p99: "15ms"
  
  high_concurrent:
    rps: 30000
    latency_p50: "4ms"
    latency_p95: "12ms"
    latency_p99: "25ms"
  
  extreme_concurrent:
    rps: 25000
    latency_p50: "5ms"
    latency_p95: "20ms"
    latency_p99: "50ms"
```

### Performance Degradation Detection

#### Detection Rules

```python
def check_performance_degradation(current, baseline, threshold=0.05):
    """
    Check for performance degradation
    
    Args:
        current: current performance metric
        baseline: baseline performance metric
        threshold: degradation threshold (default 5%)
    
    Returns:
        bool: whether degradation occurred
    """
    degradation = (baseline - current) / baseline
    return degradation > threshold
```

#### Alert Rules

| Metric | Degradation Threshold | Alert Level |
|-----|---------|---------|
| RPS | > 5% | Warning |
| RPS | > 10% | Error |
| P95 latency | > 20% | Warning |
| P95 latency | > 50% | Error |
| Error rate | > 0.1% | Error |

### Performance Trend Analysis

#### Trend Chart Generation

```python
import matplotlib.pyplot as plt

def plot_performance_trend(data):
    """
    Generate a performance trend chart
    
    Args:
        data: historical performance data
    """
    dates = [d['date'] for d in data]
    rps_values = [d['rps'] for d in data]
    
    plt.figure(figsize=(12, 6))
    plt.plot(dates, rps_values, marker='o')
    plt.title('Performance Trend (RPS)')
    plt.xlabel('Date')
    plt.ylabel('RPS')
    plt.xticks(rotation=45)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig('performance-trend.png')
```

#### Performance Report

```markdown
# Performance Report

## Summary

- **Date**: 2026-02-23
- **Commit**: 7fe7b27
- **Test**: #148

## Results

| Scenario | Baseline RPS | Current RPS | Change | Status |
|----------|--------------|-------------|--------|--------|
| Low Concurrent (10) | 30,000 | 31,151 | +3.8% | |
| Medium Concurrent (50) | 30,000 | 30,487 | +1.6% | |
| High Concurrent (100) | 30,000 | 31,409 | +4.7% | |
| Extreme Concurrent (500) | 25,000 | 28,234 | +12.9% | |

## Conclusion

No performance degradation detected. All metrics within acceptable range.
```

---

## Security Scanning

### CodeQL Scanning

#### Scan Configuration

```yaml
- name: Run CodeQL analysis
  uses: github/codeql-action/analyze@v3
  with:
    languages: cpp
    queries: security-extended,security-and-quality
```

#### Query Types

| Query Type | Description |
|---------|------|
| security-extended | Extended security queries |
| security-and-quality | Security and quality queries |

### Dependency Scanning

#### Dependency Check

```bash
# Check dependency library versions
check_deps() {
  echo "Checking dependencies..."
  
  # libuv
  libuv_version=$(cat deps/libuv/VERSION)
  echo "libuv: $libuv_version"
  
  # mbedtls
  mbedtls_version=$(cat deps/mbedtls/VERSION)
  echo "mbedtls: $mbedtls_version"
  
  # llhttp
  llhttp_version=$(cat deps/cllhttp/VERSION)
  echo "llhttp: $llhttp_version"
}
```

#### Vulnerability Scan

```yaml
- name: Run dependency check
  run: |
    # Use Dependency Check
    dependency-check --scan . --format JSON --out dependency-check-results.json
```

### Automatic Security Issue Creation

#### Workflow

```yaml
name: Security Issue Creator

on:
  security:
    types: [dependabot_alert]

jobs:
  create-issue:
    runs-on: ubuntu-latest
    permissions:
      issues: write
    
    steps:
      - name: Create security issue
        uses: actions/github-script@v6
        with:
          script: |
            const alert = context.payload.alert;
            const issue = await github.rest.issues.create({
              owner: context.repo.owner,
              repo: context.repo.repo,
              title: `Security: ${alert.security_vulnerability.package.name}`,
              body: `Security alert: ${alert.html_url}`,
              labels: ['security', 'dependabot']
            });
```

---

## Release Process

### Branch Strategy

```
feature/* → develop → main → pre-release → release
```

### Merge Rules

| Target Branch | Source Branch | CI/CD Requirement | Review Requirement |
|---------|---------|-----------|---------|
| develop | feature/* | ci-pr.yml passed | 1 reviewer |
| main | develop | ci-build-matrix.yml passed | 1 reviewer |
| pre-release | main | ci-build-matrix.yml passed | 1 reviewer |
| release | pre-release | all CI/CD passed | 1 reviewer |

### Release Steps

#### 1. develop → main

```bash
# Create a PR
gh pr create --base main --head develop --title "Release $(date +%Y.%m.%d)"

# Wait for CI/CD to pass

# Merge the PR
gh pr merge --merge
```

#### 2. main → pre-release

```bash
# Create a PR
gh pr create --base pre-release --head main --title "Pre-release $(date +%Y.%m.%d)"

# Wait for CI/CD to pass (full test suite)

# Merge the PR
gh pr merge --merge
```

#### 3. pre-release → release

```bash
# Create a PR
gh pr create --base release --head pre-release --title "Release $(cat VERSION)"

# Wait for CI/CD to pass

# Merge the PR
gh pr merge --merge

# Create the Git tag
git tag -a v$(cat VERSION) -m "Release v$(cat VERSION)"
git push origin v$(cat VERSION)

# Automatically triggers ci-release.yml
```

#### 4. GitHub Release

```bash
# ci-release.yml automatically creates the Release
# Includes:
# - Source archive
# - Build artifacts
# - Release Notes
```

### Release Checklist

Before a release, the following must be ensured:

- [ ] All CI/CD tests pass
- [ ] Code coverage meets the target (80%+)
- [ ] Performance tests pass
- [ ] Security scan shows no high-severity vulnerabilities
- [ ] Documentation has been updated (API changes)
- [ ] CHANGELOG has been updated
- [ ] Version number has been updated (VERSION file)
- [ ] Git tag has been created

---

## Monitoring and Alerts

### Monitoring Metrics

| Metric | Tool | Threshold | Alert |
|-----|------|------|------|
| CI/CD failure rate | GitHub Actions | > 5% | Email |
| Test failure rate | Google Test | > 0% | Email |
| Performance degradation | wrk | > 5% | Email |
| Coverage decline | lcov | > 1% | Email |
| Security vulnerabilities | CodeQL | high severity | Issue |
| Dependency vulnerabilities | Dependabot | high severity | Issue |

### Alert Channels

| Channel | Type | Purpose |
|-----|------|------|
| Email | Notification | CI/CD failures, performance degradation |
| GitHub Issue | Tracking | security vulnerabilities, bug reports |
| Slack | Real time | CI/CD status updates |
| GitHub Summary | Visualization | PR test results |

### Alert Rules

#### CI/CD Failure Alerts

```yaml
- name: Notify on failure
  if: failure()
  uses: actions/github-script@v6
  with:
    script: |
      await github.rest.repos.createDispatchEvent({
        owner: context.repo.owner,
        repo: context.repo.repo,
        event_type: 'ci-failure',
        client_payload: {
          workflow: context.workflow,
          run_number: context.runNumber,
          run_id: context.runId
        }
      });
```

#### Performance Degradation Alerts

```yaml
- name: Check performance degradation
  run: |
    python3 << 'EOF'
    import json
    
    with open('performance-results.json', 'r') as f:
        data = json.load(f)
    
    baseline_rps = 30000
    current_rps = data['results']['test_3']['rps']
    
    degradation = (baseline_rps - current_rps) / baseline_rps
    
    if degradation > 0.05:
        print(f"Performance degradation: {degradation:.2%}")
        exit(1)
    EOF
```

---

## Best Practices

### 1. Workflow Design

#### Recommended

```yaml
# Use concurrency control
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

# Use the matrix strategy
strategy:
  fail-fast: false
  matrix:
    config: [...]
```

#### Avoid

```yaml
# Do not hardcode version numbers
- uses: actions/checkout@v4.1.1

# Do not ignore errors
- run: ./run_tests.sh || true
```

### 2. Cache Strategy

#### Recommended

```yaml
- uses: actions/cache@v3
  with:
    path: |
      deps/*/build
      build/CMakeCache.txt
    key: ${{ hashFiles('CMakeLists.txt') }}
```

#### Avoid

```yaml
# Do not cache everything
- uses: actions/cache@v3
  with:
    path: build/
```

### 3. Error Handling

#### Recommended

```yaml
# Check the test exit code
- run: ctest --output-on-failure || {
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 8 ]; then
      echo "Tests skipped (expected)"
      exit 0
    else
      exit $EXIT_CODE
    fi
  }
```

#### Avoid

```yaml
# Do not ignore all errors
- run: ctest || true
```

### 4. Security Practices

#### Recommended

```yaml
# Use least privilege
permissions:
  contents: read
  pull-requests: write

# Do not leak secrets
- run: echo ${{ secrets.MY_SECRET }}
```

#### Avoid

```yaml
# Do not grant excessive permissions
permissions:
  contents: write
  issues: write
  pull-requests: write
  deployments: write
```

### 5. Documentation and Comments

#### Recommended

```yaml
# Add detailed comments
- name: Build 32-bit version (core library only)
  run: |
    # Install 32-bit toolchain
    sudo apt-get install gcc-multilib g++-multilib
    
    # Configure with 32-bit flags
    cmake -B build-32bit -DCMAKE_C_FLAGS="-m32"
```

#### Avoid

```yaml
# Do not use complex commands without comments
- run: cmake -B build-32bit -DCMAKE_C_FLAGS="-m32" && cd build-32bit && make build && file dist/lib/libuvhttp.a
```

---

## Appendix

### A. Environment Variables

| Variable | Description | Example |
|-----|------|------|
| `GITHUB_SHA` | Commit SHA | `7fe7b27e29d41c06a0b539b6d742692ac055240d` |
| `GITHUB_REF` | Branch reference | `refs/heads/develop` |
| `GITHUB_RUN_ID` | Run ID | `22299433830` |
| `GITHUB_RUN_NUMBER` | Run number | `148` |
| `GITHUB_TOKEN` | GitHub Token | `***` |

### B. Common Commands

#### GitHub CLI

```bash
# View workflow runs
gh run list --workflow=ci-32bit.yml

# View workflow details
gh run view 22299433830

# Monitor a workflow run
gh run watch 22299433830

# Download workflow artifacts
gh run download 22299433830
```

#### CMake

```bash
# Configure Debug mode
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Configure Release mode
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Configure 32-bit mode
cmake -B build-32bit -DCMAKE_C_FLAGS="-m32"

# Build the project
cmake --build build -j$(nproc)

# Run the tests
cd build && ctest --output-on-failure
```

#### Coverage

```bash
# Generate the coverage data
lcov --capture --directory build --output-file coverage.info

# Filter the coverage data
lcov --remove coverage.info '/usr/*' --output-file coverage.info

# Generate the HTML report
genhtml coverage.info --output-directory coverage-report

# View the coverage
lcov --list coverage.info
```

### C. Troubleshooting

#### Compilation Failures

1. Check the compiler version
2. Check the CMake version
3. Check that the dependency libraries compiled correctly
4. View the full logs

#### Test Failures

1. Download the test logs
2. View the detailed information for the failed tests
3. Reproduce the issue locally
4. Add debug information

#### Performance Degradation

1. Compare against the baseline data
2. Check the recent code changes
3. Use performance analysis tools (perf)
4. Optimize the hot code

#### Cache Issues

1. Clear the cache
2. Update the cache key
3. Check the cache paths

### D. References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [CMake Documentation](https://cmake.org/documentation/)
- [Google Test Documentation](https://google.github.io/googletest/)
- [lcov Documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [wrk Documentation](https://github.com/wg/wrk)
- [CodeQL Documentation](https://codeql.github.com/docs/)

---

## Change History

| Date | Version | Changes | Author |
|-----|------|---------|------|
| 2026-02-23 | 1.0 | Initial version, complete CI/CD solution design | iFlow |

---

## Contact

If you have questions or suggestions, please contact us via:

- **GitHub Issues**: https://github.com/adam-ikari/uvhttp/issues
- **Email**: [To be added]
- **Slack**: [To be added]

---

**End of Document**
