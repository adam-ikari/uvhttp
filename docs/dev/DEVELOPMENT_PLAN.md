# UVHTTP Development Plan

**Created**: 2026-01-13
**Current Version**: v1.4.0
**Planning Period**: 2026-01-13 - 2026-02-28 (6 weeks)
**Owner**: Development Team

---

## Executive Summary

This development plan targets the current state of the UVHTTP project (code coverage 42.7%, target 80%) and is organized into 3 priority levels:

- **P0 (High Priority)**: Complete in 1-2 weeks - fix build issues, set up CI/CD, raise coverage to 60%
- **P1 (Medium Priority)**: Complete in 1 month - raise coverage to 80%, performance optimization, cloud-native features
- **P2 (Low Priority)**: Complete later - documentation, examples

---

## Project Status Assessment

### Current Metrics

| Metric | Current Value | Target Value | Gap |
|------|--------|--------|------|
| Code coverage | 42.7% | 80% | -37.3% |
| Test pass rate | 100% (69/69) | 100% | |
| Compile warnings | 30+ | 0 | |
| Compile errors | Examples fail to build | 0 | |
| CI/CD | None | Complete | |
| Performance (RPS) | 12,026 | 20,000+ | -40% |

### Low-Coverage Modules

| Module | Current Coverage | Target | Priority |
|------|-----------|------|--------|
| uvhttp_websocket_wrapper.c | 5.1% | 80% | P0 |
| uvhttp_server.c | 16.7% | 80% | P0 |
| uvhttp_request.c | 28.3% | 80% | P1 |
| uvhttp_tls_openssl.c | 28.1% | 80% | P1 |
| uvhttp_connection.c | 0% | 80% | P0 |
| uvhttp_static.c | 11.7% | 80% | P0 |

### Build Issues

1. **Dependency libraries not built**: libuv, mbedtls, llhttp and other dependencies are not compiled
2. **Example linking errors**: `cannot find -luv`
3. **Compile warnings**: 30+ unused variable warnings

---

## P0 Tasks (High Priority - 1-2 weeks)

### Task 1: Fix Dependency Build Issues (2-3 days)

**Status**: Blocked
**Priority**: P0
**Owner**: Development Team
**Estimated Effort**: 2-3 days

#### Problem Description

Examples fail to build because dependency libraries (libuv, mbedtls, llhttp) are not built.

#### Current State

```bash
# Error message
/usr/bin/ld: cannot find -luv
collect2: error: ld returned 1 exit status
```

#### Solution

1. **Build libuv**
   ```bash
   cd deps/libuv
   mkdir build && cd build
   cmake .. -DBUILD_TESTING=OFF
   make build
   ```

2. **Build mbedtls**
   ```bash
   cd deps/mbedtls
   mkdir build && cd build
   make build
   ```

3. **Build llhttp**
   ```bash
   cd deps/llhttp
   mkdir build && cd build
   make build
   ```

4. **Build xxhash**
   ```bash
   cd deps/xxhash
   make
   ```

5. **Build mimalloc**
   ```bash
   cd deps/mimalloc
   mkdir build && cd build
   make build
   ```

6. **Update CMakeLists.txt**
   - Add dependency discovery logic
   - Ensure library paths are correct
   - Add build dependency checks

#### Acceptance Criteria

- [ ] All dependency libraries compile successfully
- [ ] Examples compile successfully
- [ ] No linker errors
- [ ] Examples run without errors

#### Risks

- **Dependency version compatibility**: CMake configuration may need adjustment
- **Build time**: Building dependencies may take a while

---

### Task 2: Fix Compile Warnings (1-2 days)

**Status**: Not started
**Priority**: P0
**Owner**: Development Team
**Estimated Effort**: 1-2 days

#### Problem Description

There are currently 30+ compile warnings, mainly unused variable warnings.

#### Warning List

| File | Warning Type | Count |
|------|---------|------|
| test_static_full_coverage.c | unused-but-set-variable | 9 |
| test_server_full_coverage.c | unused-but-set-variable | 3 |
| test_context_simple.c | unused-but-set-variable | 1 |
| test_websocket_full_coverage.c | unused-but-set-variable | 17 |

#### Solution

1. **Remove unused variables**
   ```c
   // Before
   int result = some_function();
   // result is not used

   // After
   some_function();
   ```

2. **Use (void) to mark intentionally unused variables**
   ```c
   int result = some_function();
   (void)result;  // explicitly mark as intentionally ignored
   ```

3. **Add assertions to ensure variables are used**
   ```c
   int result = some_function();
   assert(result == 0);  // use the variable for verification
   ```

#### Acceptance Criteria

- [ ] Compile warning count reduced to 0
- [ ] All tests still pass
- [ ] Code logic unchanged

#### Risks

- **Test logic changes**: Modifying variable usage may affect test logic

---

### Task 3: Set Up GitHub Actions CI/CD (3-5 days)

**Status**: Not started
**Priority**: P0
**Owner**: DevOps
**Estimated Effort**: 3-5 days

#### Problem Description

The project lacks a CI/CD pipeline and cannot automate testing and deployment.

#### CI/CD Requirements

1. **Multi-platform support**
   - Linux (Ubuntu 20.04, 22.04)
   - macOS (12, 13)
   - Windows (2019, 2022)

2. **Test workflow**
   - Build check
   - Unit tests
   - Coverage report
   - Performance benchmarks

3. **Code quality**
   - Static analysis (clang-tidy)
   - Memory leak detection (valgrind)
   - Compile warning checks

#### Implementation

Create `.github/workflows/ci.yml`:

```yaml
name: CI/CD Pipeline

on:
  push:
    branches: [ main, develop, release/* ]
  pull_request:
    branches: [ main, develop ]

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-20.04, ubuntu-22.04, macos-12, macos-13, windows-2019, windows-2022]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Install dependencies (Linux)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake build-essential valgrind

    - name: Install dependencies (macOS)
      if: runner.os == 'macOS'
      run: |
        brew install cmake valgrind

    - name: Install dependencies (Windows)
      if: runner.os == 'Windows'
      run: |
        choco install cmake

    - name: Build dependencies
      run: |
        ./scripts/build_deps.sh

    - name: Configure CMake
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

    - name: Build
      run: |
        cmake --build build --config ${{ matrix.build_type }} -j$(nproc)

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure

    - name: Generate coverage report
      if: matrix.build_type == 'Debug' && runner.os == 'Linux'
      run: |
        cd build
        make coverage
        bash <(curl -s https://codecov.io/bash)

    - name: Memory leak check
      if: matrix.build_type == 'Debug' && runner.os == 'Linux'
      run: |
        cd build
        valgrind --leak-check=full ./dist/bin/uvhttp_unit_tests

  performance-test:
    runs-on: ubuntu-22.04
    needs: build-and-test

    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Build project
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build --config Release -j$(nproc)

    - name: Run performance tests
      run: |
        cd build
        ./scripts/run_performance_tests.sh

    - name: Upload performance results
      uses: actions/upload-artifact@v3
      with:
        name: performance-results
        path: build/performance_results/
```

#### Acceptance Criteria

- [ ] CI pipeline runs successfully
- [ ] Tests pass on all platforms
- [ ] Coverage report generated automatically
- [ ] Performance test results uploaded automatically

#### Risks

- **Build time**: Multi-platform testing may take a while
- **Resource limits**: GitHub Actions has time limits

---

### Task 4: Raise Code Coverage to 60% (1 week)

**Status**: Not started
**Priority**: P0
**Owner**: Development Team
**Estimated Effort**: 1 week

#### Problem Description

Current code coverage is 42.7%, need to raise it to 60%.

#### Low-Coverage Module Analysis

**uvhttp_websocket_wrapper.c (5.1%)**

Features to test:
- WebSocket connection establishment
- Message send/receive
- Connection close
- Error handling

**uvhttp_server.c (16.7%)**

Features to test:
- Server start/stop
- Connection management
- Routing
- Error handling

**uvhttp_connection.c (0%)**

Features to test:
- Connection establishment
- Connection close
- Timeout handling
- Error handling

**uvhttp_static.c (11.7%)**

Features to test:
- Static file serving
- Cache management
- sendfile optimization
- Error handling

#### Implementation

1. **Create test files**
   - `test/unit/test_websocket_wrapper_coverage.c`
   - `test/unit/test_server_coverage.c`
   - `test/unit/test_connection_coverage.c`
   - `test/unit/test_static_coverage.c`

2. **Test case design**

**WebSocket test cases**:
```c
// Connection establishment test
TEST(WebSocketWrapperTest, ConnectionEstablishment) {
    // Test normal connection establishment
    // Test connection failure
    // Test timeout
}

// Message handling test
TEST(WebSocketWrapperTest, MessageHandling) {
    // Test sending a message
    // Test receiving a message
    // Test large messages
    // Test binary messages
}

// Connection close test
TEST(WebSocketWrapperTest, ConnectionClose) {
    // Test normal close
    // Test abnormal close
    // Test timeout close
}
```

**Server test cases**:
```c
// Server start test
TEST(ServerTest, ServerStartStop) {
    // Test normal start
    // Test port conflict
    // Test repeated start
}

// Connection management test
TEST(ServerTest, ConnectionManagement) {
    // Test maximum connections
    // Test connection timeout
    // Test connection cleanup
}

// Routing test
TEST(ServerTest, Routing) {
    // Test exact match
    // Test wildcard match
    // Test route priority
}
```

3. **Coverage target breakdown**

| Module | Current Coverage | Target Coverage | Tests to Add |
|------|-----------|-----------|---------------|
| uvhttp_websocket_wrapper.c | 5.1% | 60% | 15+ test cases |
| uvhttp_server.c | 16.7% | 60% | 20+ test cases |
| uvhttp_connection.c | 0% | 60% | 25+ test cases |
| uvhttp_static.c | 11.7% | 60% | 20+ test cases |

#### Acceptance Criteria

- [ ] Overall code coverage ≥ 60%
- [ ] All new tests pass
- [ ] No regressions
- [ ] Coverage report generated successfully

#### Risks

- **Test writing time**: May need more time
- **Complex test scenarios**: Some scenarios are hard to simulate

---

## P1 Tasks (Medium Priority - 1 month)

### Task 5: Raise Code Coverage to 80% (2-3 weeks)

**Status**: Not started
**Priority**: P1
**Owner**: Development Team
**Estimated Effort**: 2-3 weeks

#### Problem Description

After completing the P0 tasks, continue to raise code coverage to 80%.

#### Remaining Low-Coverage Modules

| Module | Current Coverage | Target Coverage | Priority |
|------|-----------|-----------|--------|
| uvhttp_request.c | 28.3% | 80% | P1 |
| uvhttp_tls_openssl.c | 28.1% | 80% | P1 |
| uvhttp_response.c | 40% | 80% | P1 |
| uvhttp_router.c | 45% | 80% | P1 |

#### Implementation

1. **Request module tests**
   - Request parsing
   - Request header handling
   - Request body handling
   - Error handling

2. **TLS module tests**
   - TLS handshake
   - Certificate verification
   - Encryption/decryption
   - Error handling

3. **Response module tests**
   - Response construction
   - Response header setting
   - Response body sending
   - Error responses

4. **Router module tests**
   - Route registration
   - Route matching
   - Route parameters
   - Route caching

#### Acceptance Criteria

- [ ] Overall code coverage ≥ 80%
- [ ] All modules coverage ≥ 70%
- [ ] Core modules coverage ≥ 85%
- [ ] All tests pass

---

### Task 6: Performance Optimization (2 weeks)

**Status**: Not started
**Priority**: P1
**Owner**: Development Team
**Estimated Effort**: 2 weeks

#### Problem Description

Current performance is 12,026 RPS, target is 20,000+ RPS.

#### Performance Bottleneck Analysis

1. **Memory allocation**
   - Frequent memory allocation/deallocation
   - Missing object pools

2. **Lock contention**
   - Lock contention in connection management
   - Lock contention in route cache

3. **I/O operations**
   - Non-optimal buffer sizes
   - Missing batch processing

#### Optimization Plan

1. **Memory pool optimization**
   ```c
   // Implement connection object pool
   typedef struct {
       uvhttp_connection_t* pool;
       size_t pool_size;
       size_t used_count;
   } connection_pool_t;

   uvhttp_connection_t* connection_pool_acquire(connection_pool_t* pool);
   void connection_pool_release(connection_pool_t* pool, uvhttp_connection_t* conn);
   ```

2. **Lock-free data structures**
   - Use atomic operations
   - Use RCU (Read-Copy-Update)

3. **Batch processing**
   - Batch send responses
   - Batch process requests

4. **Zero-copy optimization**
   - Use sendfile
   - Use splice

#### Acceptance Criteria

- [ ] Performance ≥ 20,000 RPS
- [ ] Memory usage reduced by 20%
- [ ] CPU usage reduced by 15%
- [ ] No performance regressions

---

### Task 7: Implement Cloud-Native Features (1-2 weeks)

**Status**: Not started
**Priority**: P1
**Owner**: Development Team
**Estimated Effort**: 1-2 weeks

#### Problem Description

Implement the core features required for cloud-native scenarios.

#### Feature List

1. **Health check endpoint**
   ```c
   // GET /health
   // Returns: {"status": "healthy", "timestamp": "..."}
   ```

2. **Prometheus metrics export**
   ```c
   // GET /metrics
   // Returns metrics in Prometheus format
   ```

3. **Graceful shutdown**
   ```c
   uvhttp_error_t uvhttp_server_graceful_shutdown(
       uvhttp_server_t* server,
       int timeout_seconds
   );
   ```

4. **Environment variable configuration**
   ```c
   uvhttp_error_t uvhttp_config_load_from_env(
       uvhttp_config_t* config
   );
   ```

#### Implementation

1. **Health check**
   - Check server status
   - Check connection count
   - Check memory usage

2. **Prometheus metrics**
   - Request counter
   - Response time histogram
   - Error rate

3. **Graceful shutdown**
   - Stop accepting new connections
   - Wait for existing requests to complete
   - Force close after timeout

4. **Environment variable configuration**
   - Support common configuration options
   - Provide default values
   - Validate configuration validity

#### Acceptance Criteria

- [ ] Health check endpoint works correctly
- [ ] Prometheus metrics exported correctly
- [ ] Graceful shutdown with no data loss
- [ ] Environment variable configuration takes effect

---

### Task 8: Implement Backlog Items (1 week)

**Status**: Not started
**Priority**: P1
**Owner**: Development Team
**Estimated Effort**: 1 week

#### Backlog List

1. **HTTP/1.1 streaming** (Planned)
2. **WebSocket compression** (permessage-deflate)
3. **Request rate limiting enhancement** (sliding window)
4. **Connection reuse optimization**
5. **Server-Sent Events (SSE)** (Research phase)

#### Implementation

1. **HTTP/1.1 streaming**
   - Implement chunked transfer encoding
   - Streaming upload API
   - Streaming download API
   - Progress callbacks

2. **WebSocket compression**
   - Implement permessage-deflate
   - Add compression level configuration
   - Performance tests

3. **Request rate limiting enhancement**
   - Implement sliding window algorithm
   - Support distributed rate limiting
   - Add rate limit statistics

3. **Connection reuse optimization**
   - Improve Keep-Alive policies
   - Optimize connection pool management
   - Add connection warm-up

#### Acceptance Criteria

- [ ] WebSocket compression works correctly
- [ ] Sliding window rate limiting is correct
- [ ] Connection reuse performance improves

---

## P2 Tasks (Low Priority - Complete Later)

### Task 9: Documentation Improvements (2 weeks)

**Status**: Not started
**Priority**: P2
**Owner**: Documentation Team
**Estimated Effort**: 2 weeks

#### Documents to Improve

1. **API documentation**
   - WebSocket authentication API
   - Connection management API
   - Error code reference API

2. **Tutorials**
   - WebSocket tutorial
   - Performance optimization tutorial
   - Cloud-native deployment tutorial

3. **Examples**
   - WebSocket authentication example
   - Connection management example
   - Broadcast example

#### Acceptance Criteria

- [ ] All APIs documented
- [ ] All features have tutorials
- [ ] All features have examples

---

### Task 10: Test Enhancement (1 week)

**Status**: Not started
**Priority**: P2
**Owner**: Test Team
**Estimated Effort**: 1 week

#### Test Enhancement

1. **Integration tests**
   - End-to-end tests
   - Stress tests
   - Compatibility tests

2. **Performance tests**
   - Benchmark tests
   - Regression tests
   - Comparative tests

3. **Security tests**
   - Fuzz testing
   - Penetration testing
   - Dependency scanning

#### Acceptance Criteria

- [ ] Integration test coverage 90%
- [ ] Performance tests automated
- [ ] Security tests pass

---

## Timeline

### Weeks 1-2 (P0 Tasks)

| Week | Task | Owner | Status |
|------|------|--------|------|
| Week 1 | Fix dependency builds | Development Team | Blocked |
| Week 1 | Fix compile warnings | Development Team | Not started |
| Week 1-2 | Set up GitHub Actions CI/CD | DevOps | Not started |
| Week 2 | Raise coverage to 60% | Development Team | Not started |

### Weeks 3-4 (P1 Tasks - Batch 1)

| Week | Task | Owner | Status |
|------|------|--------|------|
| Week 3-4 | Raise coverage to 80% | Development Team | Not started |
| Week 3-4 | Performance optimization | Development Team | Not started |

### Weeks 5-6 (P1 Tasks - Batch 2)

| Week | Task | Owner | Status |
|------|------|--------|------|
| Week 5 | Implement cloud-native features | Development Team | Not started |
| Week 6 | Implement backlog items | Development Team | Not started |

### Later (P2 Tasks)

| Week | Task | Owner | Status |
|------|------|--------|------|
| Week 7-8 | Documentation improvements | Documentation Team | Not started |
| Week 9 | Test enhancement | Test Team | Not started |

---

## Dependencies

```
Task 1 (dependency builds)
  └─> Task 2 (fix compile warnings)
  └─> Task 3 (set up CI/CD)
  └─> Task 4 (raise coverage to 60%)

Task 4 (coverage 60%)
  └─> Task 5 (coverage 80%)

Task 5 (coverage 80%)
  └─> Task 6 (performance optimization)
  └─> Task 7 (cloud-native features)
  └─> Task 8 (backlog items)

Tasks 6-8
  └─> Task 9 (documentation improvements)
  └─> Task 10 (test enhancement)
```

---

## Risk Management

### High-Risk Items

| Risk | Impact | Probability | Mitigation |
|------|------|------|---------|
| Dependency build failure | High | Medium | Test early, prepare fallbacks |
| Performance target not met | High | Medium | Early profiling, iterative optimization |
| CI/CD configuration complexity | Medium | High | Phased implementation, follow best practices |

### Medium-Risk Items

| Risk | Impact | Probability | Mitigation |
|------|------|------|---------|
| Test writing takes time | Medium | Medium | Prioritize core features |
| Documentation updates lag | Low | High | Automated documentation generation |
| Personnel changes | Medium | Low | Knowledge sharing, code review |

---

## Resource Requirements

### Human Resources

| Role | Headcount | Time Commitment |
|------|------|---------|
| Development engineers | 3-4 | Full-time |
| DevOps engineer | 1 | 50% |
| Test engineer | 1 | 50% |
| Documentation engineer | 1 | 30% |

### Hardware Resources

| Resource | Quantity | Purpose |
|------|------|------|
| Development machines | 4 | Daily development |
| Test servers | 2 | Performance testing |
| CI/CD server | 1 | Continuous integration |

### Software Resources

| Software | Purpose |
|------|------|
| GitHub Actions | CI/CD |
| Codecov | Coverage reports |
| SonarQube | Code quality |

---

## Acceptance Criteria

### Overall Goals

- [ ] Code coverage ≥ 80%
- [ ] Compile warnings = 0
- [ ] All tests pass
- [ ] Performance ≥ 20,000 RPS
- [ ] CI/CD fully operational
- [ ] Documentation complete

### Milestone Goals

#### P0 Complete (Week 2)

- [ ] All examples compile
- [ ] Compile warnings = 0
- [ ] CI/CD basic functionality running
- [ ] Code coverage ≥ 60%

#### P1 Complete (Week 6)

- [ ] Code coverage ≥ 80%
- [ ] Performance ≥ 20,000 RPS
- [ ] Cloud-native features complete
- [ ] All backlog items complete

#### P2 Complete (Week 9)

- [ ] Documentation complete
- [ ] Testing complete
- [ ] Project ready for release

---

## Communication Plan

### Meeting Schedule

| Meeting | Frequency | Participants | Purpose |
|------|------|--------|------|
| Daily standup | Daily | Development Team | Progress sync |
| Weekly meeting | Weekly | Everyone | Progress report |
| Technical review | On demand | Development Team | Technical decisions |
| Code review | Continuous | Development Team | Code quality |

### Reporting Mechanism

- **Daily progress**: Slack/Teams status updates
- **Weekly report**: Email sent to all stakeholders
- **Milestone report**: Detailed report at key milestones

---

## Success Metrics

### Quantitative Metrics

| Metric | Current Value | Target Value | Measurement |
|------|--------|--------|---------|
| Code coverage | 42.7% | 80% | gcov/lcov |
| Performance (RPS) | 12,026 | 20,000+ | wrk/ab |
| Compile warnings | 30+ | 0 | Build output |
| Test pass rate | 100% | 100% | ctest |
| Documentation completeness | 70% | 95% | Documentation review |

### Qualitative Metrics

- Improved code quality
- Improved team efficiency
- Improved project maintainability
- Improved user experience

---

## Appendix

### A. Dependency Build Script

```bash
#!/bin/bash
# scripts/build_deps.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Building dependencies..."

# Build libuv
echo "Building libuv..."
cd "$PROJECT_ROOT/deps/libuv"
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=OFF
make build

# Build mbedtls
echo "Building mbedtls..."
cd "$PROJECT_ROOT/deps/mbedtls"
mkdir -p build && cd build
make build

# Build llhttp
echo "Building llhttp..."
cd "$PROJECT_ROOT/deps/llhttp"
mkdir -p build && cd build
make build

# Build xxhash
echo "Building xxhash..."
cd "$PROJECT_ROOT/deps/xxhash"
make

# Build mimalloc
echo "Building mimalloc..."
cd "$PROJECT_ROOT/deps/mimalloc"
mkdir -p build && cd build
make build

echo "All dependencies built successfully!"
```

### B. CI/CD Configuration Reference

See the CI/CD configuration example in Task 3.

### C. Coverage Report Generation

```bash
# Generate coverage report
cd build
make coverage

# View report
lcov --list coverage.info
genhtml coverage.info --output-directory coverage_html

# Upload to Codecov
bash <(curl -s https://codecov.io/bash)
```

---

## Appendix D: Compile Optimization Design Principles

### Design Principles

1. **volatile is unnecessary in single-threaded environments**
   - All code executes on the same thread
   - Compiler optimizations do not break single-threaded program correctness
   - volatile does not solve any real problem

2. **Use -O2 instead of -O3**
   - -O3 may cause over-optimization and timing issues
   - -O2 provides a good balance of performance and stability
   - Avoid aggressive loop unrolling

3. **Do not restrict inline count**
   - Let the compiler decide the inlining strategy
   - Only hint at hot-path functions with the `inline` keyword
   - Avoid performance loss from manual restrictions

4. **Complete boundary checks**
   - Add null pointer checks
   - Add buffer overflow checks
   - Avoid undefined behavior

5. **Correct loop logic**
   - Use `UV_RUN_ONCE` instead of `UV_RUN_NOWAIT`
   - Ensure callbacks are executed
   - Run the loop regardless of `owns_loop` state

### Hot-Path Functions

The following functions use the `inline` keyword to hint the compiler to inline:

- `route_hash()` - route hash computation
- `fast_method_parse()` - fast method parsing
- `find_in_hot_routes()` - hot path lookup

### Test Configuration

Add serial run flags for slow tests to avoid concurrent contention:

```cmake
set_tests_properties(${test_name} PROPERTIES 
    TIMEOUT 300 
    LABELS "slow" 
    RUN_SERIAL TRUE)
```

---

**Document Version**: 1.2
**Last Updated**: 2026-01-24
**Next Review**: 2026-01-27
