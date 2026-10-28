# UVHTTP Testing Guide

This guide describes how tests are built, registered, and run. It applies to
UVHTTP 3.x.

## How Tests Are Discovered

The top-level `CMakeLists.txt` globs `test/unit/test_*.cpp`. Each file becomes a
standalone GoogleTest binary under `build/dist/bin/<test_name>` and is
registered with `add_test`, so `ctest` picks it up automatically.

There is no `test/CMakeLists.txt`. Do not add one, and do not maintain a manual
list of test files.

Files under `manual/` are compiled but deliberately **not** registered in ctest:
they run `uv_run(UV_RUN_DEFAULT)` and never return. Assertion tests belong in
`test/unit/`.

## Test Layout

```text
test/
├── unit/                 # gtest *.cpp, auto-globbed and registered
├── mock/                 # libuv_mock.c/.h used by link-wrapped tests
├── certs/                # TLS test certificates
├── scripts/              # certificate / helper scripts
├── coverage/             # coverage helpers
└── README.md
manual/                   # long-running manual servers, not in ctest
```

Naming has build effects:

- `test_*_mock*.cpp` and `test_*_libuv_fail*.cpp` link `libuv_mock` and receive
  the `-Wl,--wrap=uv_*` options automatically.
- Files matching `*websocket*`, `*tls*`, or `*compression*` are skipped when the
  corresponding feature is disabled in the build.

## Running Tests

```bash
# Debug build, then every registered test
make build
make test

# Equivalent direct invocation
ctest --test-dir build --output-on-failure

# One test by name (or run its binary directly)
ctest --test-dir build -R test_router --output-on-failure
build/dist/bin/test_router_full_coverage

# Memory-safety gate: ASan and UBSan over the full suite
make verify-memory-safety

# Coverage report at build_coverage/coverage_html/index.html
make build-coverage
make coverage
```

`make test` is the local equivalent of what the release gate runs. PR CI only
compiles and runs static analysis, so a green PR does not prove tests pass.

## Writing a Test

```cpp
#include <gtest/gtest.h>

#include "uvhttp_router.h"

TEST(RouterTest, CreateRouter) {
    uvhttp_router_t* router = nullptr;

    uvhttp_error_t result = uvhttp_router_new(&router);

    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    uvhttp_router_free(router);
}
```

Rules that matter in this repository:

1. **Include what you use.** Minimal builds disable features and drop
   transitive includes; a test must include every header it calls directly.
2. **Use the real API.** Do not invent functions or drive internals that normal
   embedding code cannot reach.
3. **Clean up every path.** Free connections/servers and close socket fds on
   every early return. ASan does not detect fd leaks.
4. **Assert something.** Tests with no assertions, commented-out assertions, or
   assertions hidden behind unconditional control flow are rejected by
   `scripts/ci/check_test_validity.py --strict`.
5. **Mutation-check new tests.** Break the implementation the test is supposed
   to protect and confirm the test turns red. A test that stays green when the
   bug is reintroduced provides no protection.

## Mocking libuv

`test/mock/libuv_mock.c` replaces libuv symbols at link time. CMake applies the
wrap options for you based on the filename; do not copy them into test files.

```cpp
#include "libuv_mock.h"

TEST(ServerTest, ListenFailure) {
    libuv_mock_reset();
    libuv_mock_set_enabled(true);
    libuv_mock_set_uv_listen_result(-1);

    /* ...drive the code under test... */

    libuv_mock_reset();
}
```

The mock records calls and can trigger callbacks manually:

```cpp
size_t calls = 0;
libuv_mock_get_call_count("uv_listen", &calls);
EXPECT_EQ(calls, 1);
```

See `test/mock/libuv_mock.h` for the complete control, return-value, and
callback-trigger interfaces.

## Sanitizers and Coverage

| Command | Build | What it checks |
|---|---|---|
| `make verify-memory-safety` | `build_asan`, `build_ubsan` | Leaks, use-after-free, overflows, undefined behavior |
| `make build-coverage && make coverage` | `build_coverage` | Line/function/branch coverage |

`make verify-memory-safety` builds Debug with ASan, then Debug with UBSan, and
runs the full registered suite in both trees. The CI release gate runs a
separate ASan job with static files and router cache enabled; nightly adds
coverage, stress, and UBSan jobs.

To inspect a single finding:

```bash
cd build_asan
ctest --output-on-failure -R <test_name>
```

## CI Responsibilities

| Stage | Tests run |
|---|---|
| PR (`ci-pr.yml`) | None — compile, cppcheck, doc sync, build matrix only |
| Prerelease (`ci-release-gate.yml`) | Fast suite, build matrix, ASan |
| Nightly (`ci-nightly.yml`) | Coverage, stress, UBSan |

Because PR CI does not execute tests, run `make test` locally before opening a
PR.

## Further Reading

- [Developer Guide](DEVELOPER_GUIDE.md)
- [Testing Standards](../dev/TESTING_STANDARDS.md)
- [Testability Guide](../dev/TESTABILITY_GUIDE.md)
- [CI/CD Guide](../dev/CI_CD.md)
- [Manual test tools](https://github.com/adam-ikari/uvhttp/blob/main/manual/README.md)
