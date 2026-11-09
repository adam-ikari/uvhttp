# Test Coverage Spec

## Overview

This spec defines the test coverage targets, test categories, and quality
criteria for the UVHTTP test suite.

## Test Suite

The suite uses Google Test (C++) for unit tests, with some C integration
tests. It holds 119 unit tests, 20 standalone C integration programs, and 1
fuzz harness for the router.

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

### Unit Tests (119 files)
Unit tests call one module in isolation under Google Test, with the libuv mock
standing in for the event loop. Each file corresponds to one source module and
covers normal paths, error paths, and boundary conditions.

### Integration Tests (20 files)
Integration tests start a real server on real libuv and exercise it end to
end. The standalone C programs cover HTTP methods, TLS, WebSocket, the
request filter, and static files. CTest does not register them; run them by
hand.

### Fuzz Tests (1 harness)
The libFuzzer harness targets router path matching and runs under ASan. CI
gives each target 60 seconds and caps input at 256 byte.

## Test Quality Criteria

1. Every public API function has at least one test.
2. Every public function has its NULL parameter paths tested.
3. Tests cover error conditions as well as success paths.
4. Tests cover boundary conditions at the minimum and maximum values.
5. Tests share no mutable state and run independently.
6. Tests leak no memory; ASan verifies this.
7. Tests make no timing-dependent assertions, so results stay deterministic.

## Test Naming

Test files follow `test_<module>_<functionality>.cpp`. Test cases use
`TEST(<Module>Test, <TestCaseName>)` or `TEST_F(<Module>Test, <TestCaseName>)`,
and the case name describes the scenario: `CreateServer`,
`ListenWithNullHost`, `AddRouteWithNullPath`.

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

PR CI (ci-pr.yml) compiles the default configuration, runs cppcheck, checks
test validity and docs, and builds the feature matrix. It does not run the
test suite or ASan. The full ASan suite runs in the pre-release gate, and the
nightly workflow (ci-nightly.yml) runs UBSan, coverage, and stress. Fuzzing
runs in ci-fuzz.yml. Coverage is generated nightly and published to the
coverage report.
