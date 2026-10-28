# UVHTTP Contributor Guide

## Overview

This document is intended for contributors to the UVHTTP library. It describes how to participate in library development, coding standards, best practices, and more.

## Development Environment

### System Requirements

- **Operating System**: Linux, macOS, Windows
- **Compiler**: GCC or Clang with C11 support
- **CMake**: 3.13+
- **Dependencies** (vendored as submodules under `deps/`):
  - libuv 1.x
  - llhttp
  - mbedtls (for TLS)
  - googletest (tests)
  - mimalloc (optional, high-performance memory allocator)

### Installing Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential cmake
```

#### CentOS/RHEL

```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake
```

#### macOS

```bash
brew install cmake
```

## Building the Project

### Build Mode Specification

The UVHTTP project defines three build modes, each suited to different scenarios:

| Build Mode | Use Case | Compile Flags | Applicable Programs |
|----------|------|----------|----------|
| **Release** | Production, performance testing | `-O2 -DNDEBUG` | All benchmark programs, example programs |
| **Debug** | Development, debugging, unit testing | `-O0 -g` | Unit tests, test programs |
| **Coverage** | Code coverage analysis | `-O0 --coverage` | Coverage testing |

**Important notes**:
- Performance testing must use Release mode, otherwise the data is inaccurate (Debug mode can be 10-100 times slower)
- For detailed build mode specifications, refer to [BUILD_MODES.md](../dev/BUILD_MODES.md)

### Basic Build

Use the Makefile targets; they create separate build directories and pass the correct flags.

```bash
# Debug build (default): build/
make build

# Release build: build_release/
make build-release

# Coverage build: build_coverage/
make build-coverage
```

Equivalent raw CMake commands:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
```

Release is required for benchmarks and production. The project also supports
`ENABLE_DEBUG=ON` as an explicit switch; `CMAKE_BUILD_TYPE=Debug` is enforced to
`-O0` without `NDEBUG`, so either form gives a real debug build.

### Compilation Options

Pass options at configure time instead of editing `CMakeLists.txt`:

```bash
# Feature switches (defaults shown)
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_WITH_WEBSOCKET=ON \
  -DBUILD_WITH_HTTPS=ON \
  -DBUILD_WITH_STATIC_FILES=OFF \
  -DBUILD_WITH_ROUTER_CACHE=OFF \
  -DBUILD_WITH_COMPRESSION=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_BENCHMARKS=ON
```

The Makefile forwards extra flags through `CMAKE_FLAGS`:

```bash
make build CMAKE_FLAGS="-DBUILD_WITH_STATIC_FILES=ON"
```

`make cmake-options` lists the currently supported options, and
[Build Configuration Matrix](BUILD_CONFIGURATION_MATRIX.md) documents each one.

### Selecting the Memory Allocator

`UVHTTP_ALLOCATOR_TYPE` selects the allocator. `1` requires the mimalloc
submodule and the matching flag; `2` means a user-provided allocator.

```bash
# System allocator (default)
cmake -B build -DUVHTTP_ALLOCATOR_TYPE=0

# mimalloc allocator
cmake -B build -DUVHTTP_ALLOCATOR_TYPE=1 -DBUILD_WITH_MIMALLOC=ON
```

## Coding Standards

### Naming Conventions

- **Functions**: `uvhttp_module_action` (e.g., `uvhttp_server_new`)
- **Types**: `uvhttp_name_t` (e.g., `uvhttp_server_t`)
- **Constants**: `UVHTTP_UPPER_CASE` (e.g., `UVHTTP_MAX_HEADERS`)
- **Macros**: `UVHTTP_UPPER_CASE` (e.g., `UVHTTP_MALLOC`)

### Code Style

- **Standard**: C11
- **Indentation**: 4 spaces
- **Braces**: K&R style
- **Line length**: 120 characters maximum

### Comment Standards

```c
/**
 * @brief Brief description of the function
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return Description of the return value
 */
uvhttp_error_t uvhttp_function(int param1, const char* param2);
```

## Memory Management

### Unified Allocator

UVHTTP provides a unified memory management interface, selecting the allocator type at compile time.

#### Basic Operations

```c
// Allocate memory
void* ptr = uvhttp_alloc(size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// Reallocate
ptr = uvhttp_realloc(ptr, new_size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

// Free memory
uvhttp_free(ptr);

// Allocate and initialize
ptr = uvhttp_calloc(count, size);
if (!ptr) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}
```

#### Allocator Types

`UVHTTP_ALLOCATOR_TYPE` supports three values:

1. **System allocator** (`0`, default)
   - Stable and reliable, no extra dependencies
   - Zero abstraction overhead
   - Suitable for most scenarios

2. **mimalloc allocator** (`1`)
   - Modern allocator
   - Built-in small-object optimization
   - Multithreaded scalability
   - Reduced memory fragmentation

3. **Custom allocator** (`2`) — supplied by the embedder

#### Compile-Time Configuration

```bash
# System allocator (default)
cmake -B build -DUVHTTP_ALLOCATOR_TYPE=0

# mimalloc allocator
cmake -B build -DUVHTTP_ALLOCATOR_TYPE=1 -DBUILD_WITH_MIMALLOC=ON
```

#### Performance Characteristics

- **Zero runtime overhead**: all functions are inline
- **Compile-time optimization**: the compiler can fully optimize
- **Type safety**: compile-time type checking
- **Predictability**: no dynamic dispatch

#### Best Practices

1. **Uniform use**: always use `uvhttp_alloc/uvhttp_free`, do not mix with `malloc/free`
2. **Paired allocation**: every allocation has a corresponding free
3. **Check return values**: check whether the allocation succeeded
4. **Avoid leaks**: ensure all paths free memory
5. **Avoid double free**: set the pointer to `NULL` after freeing

#### Complete Example

```c
#include "uvhttp_allocator.h"

void example_memory_usage(void) {
    // Allocate memory
    char* buffer = uvhttp_alloc(1024);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }

    // Use the memory
    strcpy(buffer, "Hello, World!");

    // Reallocate
    buffer = uvhttp_realloc(buffer, 2048);
    if (!buffer) {
        fprintf(stderr, "Failed to reallocate memory\n");
        return;
    }

    // Free the memory
    uvhttp_free(buffer);
    buffer = NULL;  // avoid dangling pointer
}
```

### Memory Leak Detection

The sanitizer targets are the primary leak check; Valgrind is optional.

```bash
# Debug build with ASan + UBSan, runs the full registered suite
make verify-memory-safety

# Optional: Valgrind on a debug binary or test executable
make build
valgrind --leak-check=full --show-leak-kinds=all build/dist/bin/<binary>
```

## Error Handling

### Error Codes

All error codes are negative, and `UVHTTP_OK (0)` indicates success.

```c
typedef enum {
    UVHTTP_OK = 0,
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_IO = -3,
    // ... more error codes
} uvhttp_error_t;
```

### Error Handling Patterns

```c
// Basic error handling
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "Description: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "Suggestion: %s\n", uvhttp_error_suggestion(result));
    return 1;
}

// Error recovery
if (result != UVHTTP_OK && uvhttp_error_is_recoverable(result)) {
    // Attempt a recovery operation
    result = uvhttp_server_listen(server, "0.0.0.0", 8081);
}
```

### Error Handling Principles

1. **Check all function calls that could fail**
2. **Use a unified error type**
3. **Provide meaningful error messages**
4. **Support error recovery**

## Testing

### Running Tests

```bash
# Build (Debug) and run every ctest-registered test
make build
make test

# Or invoke ctest directly
ctest --test-dir build --output-on-failure

# Run one test binary by name
ctest --test-dir build -R test_router --output-on-failure
build/dist/bin/test_router_full_coverage

# ASan + UBSan over the full suite
make verify-memory-safety

# Coverage report (build_coverage/coverage_html/index.html)
make build-coverage
make coverage
```

### Writing Tests

Tests live in `test/unit/test_*.cpp`. CMake globs them, builds each file into
its own binary, and registers it with `add_test`; there is no `test/CMakeLists.txt`
to edit. Files under `manual/` are compiled but deliberately not registered in
ctest — do not put assertion tests there.

```cpp
#include <gtest/gtest.h>

TEST(UvhttpTest, BasicFunctionality) {
    EXPECT_EQ(result, UVHTTP_OK);
}
```

See [Testing Guide](TESTING_GUIDE.md) for the mock framework, naming rules, and
sanitizer coverage workflow.

## Performance Optimization

### Zero-Copy Optimization

```c
// Use libuv buffers
uv_buf_t buf = uv_buf_init(data, len);
uv_write(req, stream, &buf, 1, callback);

// Use sendfile
uvhttp_static_sendfile("/path/to/file", response);
```

### Caching Strategy

```c
// Prewarm the cache
uvhttp_static_prewarm_cache(ctx, "/static/index.html");

// Use an LRU cache
cache_manager_t* cache = NULL;
uvhttp_lru_cache_create(1024 * 1024, 256, 0, &cache);
```

### Memory Optimization

```c
// Use inline functions
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}

// Avoid unnecessary copies
const char* data = uvhttp_request_get_body(request, &len);
```

## Debugging Tips

### Enabling Debug Output

```bash
# Debug build (assertions on, -O0 -g)
make build

# Also enable development-mode logging
make build CMAKE_FLAGS="-DENABLE_DEV_MODE=ON"
```

### Using GDB

```bash
gdb build/dist/bin/<binary>
(gdb) run
(gdb) backtrace
(gdb) print variable
```

### Log Output

```c
#include "uvhttp_logging.h"

// Log output goes to stderr in debug builds
// (controlled by NDEBUG / UVHTTP_FEATURE_LOGGING at compile time)

// Output logs
UVHTTP_LOG_DEBUG("Debug message: %s", message);
UVHTTP_LOG_INFO("Info message: %s", message);
UVHTTP_LOG_ERROR("Error message: %s", message);
```

## Committing Code

### Commit Conventions

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat`: new feature
- `fix`: bug fix
- `docs`: documentation update
- `style`: code formatting changes
- `refactor`: refactoring
- `perf`: performance optimization
- `test`: testing related
- `chore`: build/tooling related

### Example

```
feat(server): add WebSocket support

Implement WebSocket protocol support for real-time
communication.

- Add WebSocket handshake handling
- Add frame parsing and generation
- Add connection management

Closes #123
```

## Code Review

### Review Focus Points

- **Memory management**: correct use of `uvhttp_alloc/uvhttp_free`
- **Error handling**: check all functions that could fail
- **Naming conventions**: follow the naming conventions
- **Code style**: conform to the coding standards
- **Performance**: avoid unnecessary copies and allocations
- **Security**: input validation and bounds checking

### Review Process

1. Create a Pull Request
2. CI/CD tests run automatically
3. Code review
4. Make changes and re-review
5. Merge into the main branch

## FAQ

### Q: How do I choose a memory allocator?

A:
- **System allocator**: suitable for most scenarios, stable and reliable
- **mimalloc**: suitable for high-concurrency, multithreaded scenarios with better performance

```bash
# Edit CMakeLists.txt, set UVHTTP_ALLOCATOR_TYPE to 1, then run:
make build
```

### Q: How do I handle memory leaks?

A:
1. Use Valgrind to detect leaks
2. Ensure every `uvhttp_alloc` has a corresponding `uvhttp_free`
3. Use RAII patterns to manage resources

### Q: How do I optimize performance?

A:
1. Use zero-copy techniques
2. Enable caching
3. Use the mimalloc allocator
4. Avoid unnecessary memory allocations

### Q: How do I debug network issues?

A:
1. Enable debug logging
2. Capture packets with tcpdump
3. Debug with GDB
4. Check error codes and error messages

## Reference Materials

- [Developer Documentation Hub](../dev/index.md)
- [Architecture Design](../dev/ARCHITECTURE.md)
- [Build Modes](../dev/BUILD_MODES.md)
- [Testing Guide](TESTING_GUIDE.md)
- [Testing Standards](../dev/TESTING_STANDARDS.md)
- [CI/CD Guide](../dev/CI_CD.md)
- [API Reference](../api/API_REFERENCE.md)
- [Tutorial](TUTORIAL.md)
- [libuv Documentation](https://docs.libuv.org/)
- [HTTP/1.1 Specification](https://tools.ietf.org/html/rfc7230)

## License

MIT License - see the LICENSE file for details
