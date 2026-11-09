# Build System Spec

## Overview

UVHTTP builds with CMake. This spec covers the build configuration, C
standard, compiler flags, and the CI build matrix.

## C Standard

The project compiles as C11. CMake sets `CMAKE_C_STANDARD 11` and
`CMAKE_C_STANDARD_REQUIRED ON`, so a compiler without support for that
standard fails at configure time. Supported compilers are GCC 4.8+ and
Clang 3.4+.

## Build Types

| Type | CMake Preset | Flags | Use Case |
|------|-------------|-------|----------|
| Debug | `-DCMAKE_BUILD_TYPE=Debug` | `-g -O0` | Development, ASan |
| Release | `-DCMAKE_BUILD_TYPE=Release` | `-O2 -DNDEBUG` | Production, benchmarks |
| Coverage | `-DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON` | `--coverage` | Test coverage analysis |
| ASan | `-DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON` | `-fsanitize=address` | Memory safety |
| UBSan | `-DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON` | `-fsanitize=undefined` | UB detection |
| Benchmark | `-DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON` | `-O2 -DNDEBUG` | Performance testing |

## Compiler Flags (All Builds)

Every build passes these flags:

```cmake
-Wall -Wextra -Wformat=2 -Wformat-security
-fstack-protector-strong
-fno-common
-Werror
-Werror=implicit-function-declaration
-Werror=format-security
-Werror=return-type
-D_FORTIFY_SOURCE=2
```

## Linker Flags (All Builds)

Every build links with these flags:

```cmake
-Wl,-z,relro -Wl,-z,now
```

## Feature Flags

| Flag | Default | Description |
|------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | WebSocket support |
| `BUILD_WITH_HTTPS` | ON | TLS support (mbedtls) |
| `BUILD_WITH_MIMALLOC` | OFF | Use mimalloc allocator |
| `BUILD_WITH_COMPRESSION` | ON | Compression support (zlib) |
| `BUILD_TESTING` | ON | Build test suite |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `BUILD_BENCHMARKS` | OFF | Build benchmark server |
| `UVHTTP_FEATURE_STATIC_FILES` | ON | Static file serving |
| `UVHTTP_FEATURE_LRU_CACHE` | ON | LRU cache |
| `UVHTTP_FEATURE_MIDDLEWARE` | ON | Middleware support |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | 0=system, 1=mimalloc, 2=custom |

## Allocator

`UVHTTP_ALLOCATOR_TYPE` selects the allocator at compile time. Value 0 uses
the system allocator (`malloc`/`free`). Value 1 selects mimalloc and requires
`BUILD_WITH_MIMALLOC=ON`. Value 2 uses an allocator the application provides.

## CI Build Matrix

| Workflow | Trigger | Scope |
|----------|---------|-------|
| `ci-pr.yml` | Every PR | Compile, cppcheck, test-validity, doc checks, build matrix |
| `ci-nightly.yml` | Nightly | Full build matrix, coverage, stress, UBSan |
| `ci-fuzz.yml` | Nightly | libFuzzer + ASan (60s per target) |
| `deploy-docs.yml` | Push to main | Build and deploy VitePress docs |

PR CI does not run the unit tests or ASan. It checks that the code compiles,
that cppcheck reports nothing, that test registration is valid, and that the
docs match the code, and it builds the feature matrix. The full ASan suite
runs in the pre-release gate (ci-release-gate.yml, job asan-gate).

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make build` | Debug build |
| `make build-release` | Release build |
| `make test` | Run tests |
| `make verify-memory-safety` | ASan + UBSan gate |
| `make docs` | Build all documentation |
| `make docs-clean` | Clean generated docs |
| `make bench` | Benchmark build |

## Test Registration

CTest registers 119 unit test files, discovered with
`file(GLOB_RECURSE ...)`; each file becomes its own CTest test. Integration
tests are standalone server programs and fuzz tests are libFuzzer harnesses.
Neither of the latter two is registered with CTest.

## Directory Structure

```
uvhttp/
├── src/           # Source files (17 .c files)
├── include/       # Public headers (28 .h files)
├── test/          # Test files
│   ├── unit/      # Google Test unit tests
│   ├── integration/  # Integration test servers
│   ├── mock/      # libuv mock library
│   └── fuzz/      # libFuzzer harnesses
├── deps/          # Vendored dependencies (git submodules)
├── examples/      # Example programs
├── benchmark/     # Benchmark server
├── docs/          # Documentation (VitePress)
└── scripts/       # Build and utility scripts
```
