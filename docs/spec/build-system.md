# Build System Spec

## Overview

UVHTTP uses CMake as its build system. This spec defines the build
configuration, C standard, compiler flags, and CI build matrix.

## C Standard

- **Standard**: C99 (`CMAKE_C_STANDARD 99`)
- **Enforcement**: `CMAKE_C_STANDARD_REQUIRED ON`
- **Compiler support**: GCC 4.8+, Clang 3.4+

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

```cmake
-Wl,-z,relro -Wl,-z,now
```

## Feature Flags

| Flag | Default | Description |
|------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | WebSocket support |
| `BUILD_WITH_HTTPS` | ON | TLS support (mbedtls) |
| `BUILD_WITH_MIMALLOC` | OFF | Use mimalloc allocator |
| `BUILD_WITH_COMPRESSION` | OFF | Compression support (zlib) |
| `BUILD_TESTING` | ON | Build test suite |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `BUILD_BENCHMARKS` | OFF | Build benchmark server |
| `UVHTTP_FEATURE_STATIC_FILES` | ON | Static file serving |
| `UVHTTP_FEATURE_RATE_LIMIT` | ON | Rate limiting |
| `UVHTTP_FEATURE_LRU_CACHE` | ON | LRU cache |
| `UVHTTP_FEATURE_MIDDLEWARE` | ON | Middleware support |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | 0=system, 1=mimalloc, 2=custom |

## Allocator

The allocator is selected at compile time via `UVHTTP_ALLOCATOR_TYPE`:

- **0**: System allocator (`malloc`/`free`)
- **1**: mimalloc (when `BUILD_WITH_MIMALLOC=ON`)
- **2**: Custom allocator (user-provided)

## CI Build Matrix

| Workflow | Trigger | Scope |
|----------|---------|-------|
| `ci-pr.yml` | Every PR | Build, unit tests, ASan gate, code quality |
| `ci-nightly.yml` | Nightly | Full build matrix, coverage, stress, ASan+UBSan |
| `ci-fuzz.yml` | Nightly | libFuzzer + ASan (60s per target) |
| `deploy-docs.yml` | Push to main | Build and deploy VitePress docs |

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

- Unit tests: 91 test files, discovered via `file(GLOB_RECURSE ...)`
- Each test file becomes its own CTest test
- Integration tests: standalone server programs (not registered with CTest)
- Fuzz tests: libFuzzer harness (not registered with CTest)

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