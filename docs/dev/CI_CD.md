# CI/CD Workflow Guide

This document details the configuration and purpose of the UVHTTP project's CI/CD workflows.

## Workflow Overview

UVHTTP uses GitHub Actions for automated CI/CD, focused on the Linux platform, and includes the following workflows:

| Workflow Name | Trigger | Purpose | Runtime |
|---------|---------|------|---------|
| CI/CD - PR Quick Validation | PR to main/develop | Quickly validate PRs | ~15 min |
| CI/CD - Push Full Validation | Push to main/develop/feature/* | Fully validate code changes | ~35 min |
| CI/CD - Nightly Deep Test | Daily schedule | Deep-test code quality | ~100 min |
| CI/CD - Release Build | Push tag v* | Build and publish official releases | ~60 min |
| CI/CD - Deploy Documentation | Push to main/release/* + docs/** | Deploy documentation to GitHub Pages | ~15 min |
| CI/CD - Notification Service | Other workflows complete | Send CI/CD result notifications | ~5 min |
| Security Issue Creator | Security scan completes | Automatically create security issues | ~10 min |

## Workflow Details

### 1. CI/CD - PR Quick Validation

**File**: `.github/workflows/ci-pr.yml`

**Trigger**:
- Pull Request opened, synchronized, or reopened against the `main` or `develop` branch

**Jobs included**:

#### ubuntu-build
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Purpose**:
  - Check out code (including submodules)
  - Configure CMake (Release mode, examples enabled)
  - Build the project
  - Check for compiler warnings (zero-warning policy)
  - Upload build artifacts (retained for 7 days)

#### code-quality-check
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Purpose**:
  - Install cppcheck and clang-tidy
  - Run static code analysis
  - Upload analysis results (retained for 7 days)

#### dependency-scan
- **Platform**: Ubuntu Latest
- **Timeout**: 5 min
- **Purpose**:
  - Check dependencies in .gitmodules
  - Scan for dependency vulnerabilities

#### ubuntu-test-fast
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Needs**: ubuntu-build
- **Purpose**:
  - Download build artifacts
  - Run quick tests using the `run-tests` action
  - Upload test logs (retained for 7 days)

#### performance-regression-check
- **Platform**: Ubuntu Latest
- **Timeout**: 15 min
- **Needs**: ubuntu-build
- **Purpose**:
  - Download build artifacts
  - Install wrk
  - Run quick performance tests (3 scenarios, 5 seconds each)
  - Compare performance metrics against baseline
  - Detect performance regressions
  - Upload performance results (retained for 7 days)

#### generate-pr-summary
- **Platform**: Ubuntu Latest
- **Timeout**: 2 min
- **Needs**: all other jobs
- **Purpose**:
  - Generate a PR summary
  - Add a comment to the PR

**Concurrency control**: Cancel previous runs on the same PR

---

### 2. CI/CD - Push Full Validation

**File**: `.github/workflows/ci-push.yml`

**Trigger**:
- Push to `main`, `develop`, `feature/*` branches
- Manual trigger

**Jobs included**:

#### ubuntu-build
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Purpose**:
  - Check out code (including submodules)
  - Configure CMake (Release mode, WebSocket and mimalloc enabled)
  - Build the project
  - Upload build artifacts (retained for 7 days)

#### code-quality-check
- **Platform**: Ubuntu Latest
- **Timeout**: 15 min
- **Purpose**:
  - Install cppcheck
  - Run static code analysis
  - Upload analysis results (retained for 7 days)

#### security-scan
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Purpose**:
  - Run CodeQL security analysis
  - Upload analysis results

#### ubuntu-test-full
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Needs**: ubuntu-build
- **Purpose**:
  - Download build artifacts
  - Set executable permissions
  - Run full tests using the `run-tests` action
  - Upload test logs (retained for 7 days)

#### performance-benchmark
- **Platform**: Ubuntu Latest
- **Timeout**: 35 min
- **Needs**: ubuntu-test-full
- **Purpose**:
  - Download build artifacts
  - Install wrk
  - Run performance tests (8 scenarios)
  - Parse performance results
  - Generate a performance report
  - Update the performance baseline history
  - Upload performance results (retained for 30 days)

#### generate-summary
- **Platform**: Ubuntu Latest
- **Timeout**: 5 min
- **Needs**: ubuntu-test-full, performance-benchmark
- **Purpose**:
  - Generate a CI/CD summary
  - Update the GitHub Step Summary

**Concurrency control**: Cancel previous runs on the same branch

---

### 3. CI/CD - Nightly Deep Test

**File**: `.github/workflows/ci-nightly.yml`

**Trigger**:
- Scheduled run daily at UTC 0:00
- Manual trigger

**Jobs included**:

#### ubuntu-build-all
- **Platform**: Ubuntu Latest
- **Timeout**: 25 min
- **Purpose**:
  - Check out code (including submodules)
  - Configure CMake (Debug mode, coverage enabled)
  - Build the project
  - Upload build artifacts (retained for 7 days)

#### code-quality-full
- **Platform**: Ubuntu Latest
- **Timeout**: 15 min
- **Purpose**:
  - Run cppcheck
  - Run clang-tidy
  - Run clang-format
  - Upload analysis results (retained for 30 days)

#### security-scan-full
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Purpose**:
  - Run CodeQL security analysis
  - Run cppcheck security checks
  - Upload analysis results (retained for 30 days)

#### test-coverage
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Needs**: ubuntu-build-all
- **Purpose**:
  - Download build artifacts
  - Run coverage tests using the `run-tests` action
  - Generate a coverage report
  - Upload the coverage report (retained for 30 days)
  - Upload to Codecov

#### test-memory
- **Platform**: Ubuntu Latest
- **Timeout**: 20 min
- **Needs**: ubuntu-build-all
- **Purpose**:
  - Configure CMake (AddressSanitizer enabled)
  - Build the project
  - Run memory tests using the `run-tests` action
  - Upload memory test results (retained for 30 days)

#### test-stress
- **Platform**: Ubuntu Latest
- **Timeout**: 35 min
- **Needs**: ubuntu-build-all
- **Purpose**:
  - Download build artifacts
  - Run stress tests
  - Upload stress test results (retained for 30 days)

#### performance-full
- **Platform**: Ubuntu Latest
- **Timeout**: 35 min
- **Needs**: ubuntu-build-all
- **Purpose**:
  - Download build artifacts
  - Run full performance tests (8 scenarios)
  - Upload performance results (retained for 30 days)

#### generate-nightly-report
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Needs**: all test jobs
- **Purpose**:
  - Generate a daily test report
  - Update the GitHub Step Summary

---

### 4. CI/CD - Release Build

**File**: `.github/workflows/ci-release.yml`

**Trigger**:
- Push tag matching `v*` (e.g., v1.5.0)
- Manual trigger

**Jobs included**:

#### ubuntu-release-build
- **Platform**: Ubuntu Latest
- **Timeout**: 25 min
- **Purpose**:
  - Check out code (including submodules)
  - Retrieve the version number
  - Configure CMake (Release mode, WebSocket and mimalloc enabled)
  - Build the project
  - Upload build artifacts (retained for 30 days)

#### release-test
- **Platform**: Ubuntu Latest
- **Timeout**: 25 min
- **Needs**: ubuntu-release-build
- **Purpose**:
  - Download build artifacts
  - Run release tests
  - Upload test logs (retained for 30 days)

#### create-release
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Needs**: release-test
- **Purpose**:
  - Download build artifacts
  - Generate Release Notes
  - Create a GitHub Release

#### update-baseline
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Needs**: create-release
- **Purpose**:
  - Download build artifacts
  - Run performance benchmark tests
  - Update the performance baseline
  - Create a PR to update the baseline

---

### 5. CI/CD - Deploy Documentation

**File**: `.github/workflows/deploy-docs.yml`

**Trigger**:
- Push to `main` or `release/*` branches
- Changes under the `docs/**` path
- Manual trigger

**Jobs included**:

#### build-and-deploy
- **Platform**: Ubuntu Latest
- **Timeout**: 15 min
- **Purpose**:
  - Check out code (including submodules)
  - Set up the Node.js environment
  - Set up pnpm
  - Install dependencies
  - Build the VitePress documentation
  - Deploy to GitHub Pages

---

### 6. CI/CD - Notification Service

**File**: `.github/workflows/notify.yml`

**Trigger**:
- Listens for the completion of other workflows

**Jobs included**:

#### notify
- **Platform**: Ubuntu Latest
- **Timeout**: 5 min
- **Purpose**:
  - Download workflow artifacts
  - Parse workflow results
  - Send notifications (PR comments, issue creation)

---

### 7. Security Issue Creator

**File**: `.github/workflows/security-issue-creator.yml`

**Trigger**:
- Listens for the completion of security scan workflows

**Jobs included**:

#### create-security-issue
- **Platform**: Ubuntu Latest
- **Timeout**: 10 min
- **Purpose**:
  - Download security scan results
  - Detect security issues
  - Create security issues
  - Update issue status

---

## Reusable Actions

### setup-build

**File**: `.github/actions/setup-build/action.yml`

**Purpose**: Set up the build environment and install dependencies

**Inputs**:
- `os`: Operating system (ubuntu-latest)

**Output**: None

---

### cache-deps

**File**: `.github/actions/cache-deps/action.yml`

**Purpose**: Cache CMake dependencies

**Inputs**:
- `cache-key`: Cache key (for identification)
- `build-type`: Build type (Release, Debug)

**Cache key format**: `deps-{RUNNER_OS}-{BUILD_TYPE}-{HASH}`

---

### run-tests

**File**: `.github/actions/run-tests/action.yml`

**Purpose**: Run tests and collect results

**Inputs**:
- `build-dir`: Build directory
- `test-type`: Test type (fast, slow, all, coverage, stress, memory)
- `timeout`: Test timeout (seconds)
- `parallel`: Number of parallel tests

**Outputs**:
- `status`: Test status (success, failed)
- `total`: Total number of tests
- `passed`: Number of passed tests
- `failed`: Number of failed tests
- `duration`: Test duration (seconds)

---

## Performance Benchmarking

### Test Scenarios

#### PR Quick Test (ci-pr.yml)
- **Purpose**: Quickly verify whether a PR introduces a performance regression
- **Test duration**: 5 seconds/test
- **Number of runs**: 1
- **Scenarios included**:
  1. Low concurrency (2 threads, 10 connections)
  2. Medium concurrency (4 threads, 50 connections)
  3. High concurrency (8 threads, 200 connections)

#### Push Full Test (ci-push.yml)
- **Purpose**: Verify performance after code changes
- **Test duration**: 10 seconds/test
- **Number of runs**: 1
- **Scenarios included**:
  1. Low concurrency (2 threads, 10 connections)
  2. Medium concurrency (4 threads, 50 connections)
  3. High concurrency (8 threads, 200 connections)
  4. Extreme concurrency (16 threads, 500 connections)
  5. Home page (small response)
  6. API route (medium response)
  7. Static files (large response)
  8. WebSocket connection management

#### Nightly Full Test (ci-nightly.yml)
- **Purpose**: Deep performance testing and trend analysis
- **Test duration**: 30 seconds/test
- **Number of runs**: 3 (average taken)
- **Scenarios included**: Same as Push Full Test

### Performance Metrics

- **RPS (Requests Per Second)**: Requests per second
- **Latency**: Average latency
- **Transfer/sec**: Transfer amount per second
- **Requests/sec**: Total requests per second

### Performance Regression Detection

- **Baseline**: Read from `docs/performance/baseline.json`
- **Threshold**: A performance drop exceeding 5% is considered a regression
- **Action**: When a regression is detected, block the workflow and create an issue

---

## Artifact Retention Policy

| Artifact Type | Retention Period | Description |
|---------|---------|------|
| PR build artifacts | 7 days | Quick validation, short-term retention |
| Push build artifacts | 7 days | Code validation, short-term retention |
| Nightly test results | 30 days | Deep testing, medium-term retention |
| Release build artifacts | 30 days | Release versions, medium-term retention |
| Performance test results | 30 days | Performance analysis, medium-term retention |
| Code quality analysis results | 7-30 days | Quality tracking, retained by importance |

---

## Caching Strategy

### Dependency Caching

**Used by**: cache-deps action

**Cache paths**:
- `deps/libuv/build`
- `deps/mbedtls/build`
- `deps/xxhash/libxxhash.a`
- `deps/cllhttp/build`
- `deps/mimalloc/build`
- `deps/googletest/build`
- `deps/cjson/build`

**Cache key**: `deps-{RUNNER_OS}-{BUILD_TYPE}-{HASH}`

**Restore key**: `deps-{RUNNER_OS}-{BUILD_TYPE}-`

---

## Notification Mechanism

### PR Comments

**Trigger**: ci-pr.yml completes

**Content**:
- Build status
- Test results
- Code quality
- Performance regression detection
- Dependency vulnerabilities

### Issue Creation

**Trigger**: Severe failure or security issue

**Content**:
- Failure details
- Error logs
- Fix suggestions

### GitHub Step Summary

**Trigger**: All workflows complete

**Content**:
- CI/CD summary
- Test results
- Performance metrics
- Code quality

---

## Security Scanning

### CodeQL

**Runs in**: ci-push.yml, ci-nightly.yml

**Language**: C++

**Queries**: security-extended, security-and-quality

### cppcheck

**Runs in**: All workflows

**Enables**: warning, performance, portability

**Suppressions**:
- missingIncludeSystem
- unusedFunction
- constParameter
- unusedStructMember

### Dependency Scanning

**Runs in**: dependency-scan job

**Checks**: Dependencies in .gitmodules

---

## Performance Optimization

### Build Optimization

- Parallel compilation: `-j$(nproc)`
- Zero-warning policy: compiler warnings cause failure
- Incremental builds: dependency caching

### Test Optimization

- Parallel tests: adjusted by test type
- Test timeouts: set by test type
- Quick tests: exclude slow, stress, and memory tests

### Cache Optimization

- Dependency caching: cache compile dependencies
- Smart restore: restore cache based on file hash

---

## Troubleshooting

### Common Issues

#### 1. Build Failure

**Check**:
- Compiler warnings
- Dependency installation
- Submodule updates

**Resolution**:
- Fix compiler warnings
- Check CMakeLists.txt
- Update submodules

#### 2. Test Failure

**Check**:
- Test logs
- Build artifacts
- Test timeouts

**Resolution**:
- Review the test logs
- Check the test code
- Adjust the test timeout

#### 3. Performance Regression

**Check**:
- Performance baseline
- Test scenarios
- Performance report

**Resolution**:
- Compare against the baseline
- Analyze performance bottlenecks
- Optimize the code

#### 4. Security Scan Failure

**Check**:
- cppcheck results
- CodeQL results
- Dependency vulnerabilities

**Resolution**:
- Fix security issues
- Update dependencies
- Add suppression rules

---

## Best Practices

### 1. Pre-commit Checks

- Run local tests
- Check for compiler warnings
- Run performance tests
- Update documentation

### 2. PR Guidelines

- Ensure all tests pass
- No performance regressions
- No security issues
- Update related documentation

### 3. Release Process

- Update the version number
- Run full tests
- Create Release Notes
- Update documentation

### 4. Performance Optimization

- Use performance analysis tools
- Optimize hot-path code
- Update the performance baseline
- Monitor performance trends

---

## Reference Resources

- [GitHub Actions documentation](https://docs.github.com/en/actions)
- [CMake documentation](https://cmake.org/documentation/)
- [ctest documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [CodeQL documentation](https://codeql.github.com/docs/)
- [cppcheck documentation](https://cppcheck.sourceforge.io/manual.html)
- [wrk documentation](https://github.com/wg/wrk)

---

## Changelog

### v2.2.0 (2026-01-29)

**Refactoring**:
- Removed non-Linux platform support (macOS, Windows)
- Removed redundant workflows (ci.yml, deploy.yml)
- Full reuse of reusable actions (run-tests)
- Unified caching strategy
- Optimized notification service trigger conditions

**Optimization**:
- PR validation time reduced by 25%
- Push validation time reduced by 22%
- Nightly test time reduced by 17%
- CI run count reduced by 20%
- Artifact storage reduced by 50%

**New**:
- Performance regression detection (ci-pr.yml)
- Unified test output format
- Improved notification mechanism

---

## Contribution Guidelines

To modify the CI/CD configuration:

1. Understand the existing workflows and dependencies
2. Test the modified workflows
3. Update this document
4. Submit a PR and describe the changes

---

**Last updated**: 2026-01-29
**Maintainer**: UVHTTP development team
**License**: MIT License
