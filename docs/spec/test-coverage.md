# Test Coverage Spec

## Overview

This spec defines the test coverage targets, test categories, and quality
criteria for the UVHTTP test suite.

## Test Suite

- **Framework**: Google Test (C++), with some C integration tests
- **Total tests**: 91 unit tests
- **Integration tests**: 20 standalone C programs
- **Fuzz harness**: 1 (router)

## Coverage Targets

| Module | Current | Target | Priority |
|--------|---------|--------|----------|
| uvhttp_version.c | 98.3% | 95% | Low |
| uvhttp_error.c | 98.8% | 95% | Low |
| uvhttp_utils.c | 100.0% | 95% | Low |
| uvhttp_router.c | 62.9% | 80% | High |
| uvhttp_connection.c | 42.8% | 70% | High |
| uvhttp_server.c | ~50% | 70% | High |
| uvhttp_request.c | ~60% | 70% | High |
| uvhttp_response.c | ~60% | 70% | High |
| uvhttp_static.c | ~40% | 60% | Medium |
| uvhttp_websocket.c | ~50% | 60% | Medium |
| uvhttp_tls.c | ~40% | 60% | Medium |
| **Overall** | **~50%** | **80%** | **High** |

## Test Categories

### Unit Tests (91 files)
- Test individual functions in isolation
- Use Google Test framework
- Use libuv mock for dependency isolation
- Cover normal paths, error paths, and boundary conditions
- Each test file corresponds to one source module

### Integration Tests (20 files)
- Test end-to-end scenarios with real libuv
- Standalone C programs that start a server and test it
- Cover HTTP methods, TLS, WebSocket, rate limiting, static files
- Not automatically registered with CTest (manual execution)

### Fuzz Tests (1 harness)
- libFuzzer harness for router path matching
- Runs with ASan for memory safety
- 60 seconds per target in CI
- 256 byte max input length

## Test Quality Criteria

1. **Every public API function must have at least one test**
2. **NULL parameter paths must be tested for every public function**
3. **Error conditions must be tested (not just success paths)**
4. **Boundary conditions must be tested (min/max values)**
5. **Tests must be independent** — no shared mutable state between tests
6. **Tests must not leak memory** — verified by ASan
7. **Tests must be deterministic** — no timing-dependent assertions

## Test Naming

- Test files: `test_<module>_<functionality>.cpp`
- Test cases: `TEST(<Module>Test, <TestCaseName>)` or `TEST_F(<Module>Test, <TestCaseName>)`
- Test case names describe the scenario: `CreateServer`, `ListenWithNullHost`, `AddRouteWithNullPath`

## Running Tests

```bash
# All tests
make test

# Specific test (via ctest)
cd build && ctest -R "test_router"

# Memory safety
make verify-memory-safety

# Coverage
make coverage
```

## Continuous Verification

- **PR gate**: ASan + unit tests must pass (ci-pr.yml)
- **Nightly**: Full test suite, ASan, UBSan, fuzzing (ci-nightly.yml, ci-fuzz.yml)
- **Coverage**: Generated nightly, published to coverage report