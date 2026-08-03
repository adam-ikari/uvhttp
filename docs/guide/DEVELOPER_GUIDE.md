# UVHTTP Contributor Guide

## Overview

This document is intended for contributors to the UVHTTP library. It describes how to participate in library development, coding standards, best practices, and more.

## Development Environment

### System Requirements

- **Operating System**: Linux, macOS, Windows
- **Compiler**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **CMake**: 3.10+
- **Dependencies**:
  - libuv 1.x
  - mbedtls 2.x (optional, for TLS)
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

#### Release Mode (recommended for performance testing)

```bash
make build
```

#### Debug Mode (for development and debugging)

```bash
make build
```

#### Coverage Mode (for coverage analysis)

```bash
make build
```

### Compilation Options

Edit the `option()` defaults in `CMakeLists.txt`, then run `make build`:

```bash
# Enable WebSocket support — set BUILD_WITH_WEBSOCKET to ON in CMakeLists.txt
# Enable the mimalloc allocator — set BUILD_WITH_MIMALLOC to ON in CMakeLists.txt
# Enable TLS support — set BUILD_WITH_HTTPS to ON in CMakeLists.txt
# Debug mode — set ENABLE_DEBUG to ON in CMakeLists.txt
# Enable code coverage — set ENABLE_COVERAGE to ON in CMakeLists.txt
# Enable example programs — set BUILD_EXAMPLES to ON in CMakeLists.txt
make build
```

### Selecting the Memory Allocator

Edit `CMakeLists.txt`, modify the value of `UVHTTP_ALLOCATOR_TYPE`, then run `make build`:

```bash
# System allocator (default) — UVHTTP_ALLOCATOR_TYPE=0
# mimalloc allocator — UVHTTP_ALLOCATOR_TYPE=1
make build
```

## Coding Standards

### Naming Conventions

- **Functions**: `uvhttp_module_action` (e.g., `uvhttp_server_new`)
- **Types**: `uvhttp_name_t` (e.g., `uvhttp_server_t`)
- **Constants**: `UVHTTP_UPPER_CASE` (e.g., `UVHTTP_MAX_HEADERS`)
- **Macros**: `UVHTTP_UPPER_CASE` (e.g., `UVHTTP_MALLOC`)

### Code Style

- **Standard**: C99
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

UVHTTP supports two allocator types:

1. **System allocator** (default)
   - Stable and reliable, no extra dependencies
   - Zero abstraction overhead
   - Suitable for most scenarios

2. **mimalloc allocator**
   - Modern allocator
   - Built-in small-object optimization
   - Multithreaded scalability
   - Reduced memory fragmentation

#### Compile-Time Configuration

Edit `CMakeLists.txt`, modify the value of `UVHTTP_ALLOCATOR_TYPE`, then run `make build`:

```bash
# System allocator (default) — UVHTTP_ALLOCATOR_TYPE=0
# mimalloc allocator — UVHTTP_ALLOCATOR_TYPE=1
make build
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

Use Valgrind to detect memory leaks:

```bash
# Build the Debug version — set ENABLE_DEBUG to ON in CMakeLists.txt, then run:
make build

# Run Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./dist/bin/helloworld
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
# Run all tests
./run_tests.sh

# Run a specific test
./dist/bin/test_router_full_coverage

# Generate a coverage report
./run_tests.sh --detailed
```

### Writing Tests

```c
#include <gtest/gtest.h>

TEST(UvhttpTest, BasicFunctionality) {
    // Test code
    EXPECT_EQ(result, UVHTTP_OK);
}
```

### Test Coverage

- **Unit tests**: test individual functions
- **Integration tests**: test module interactions
- **Performance tests**: test performance metrics

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
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_new(1024);
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
# Set ENABLE_DEBUG to ON in CMakeLists.txt, then run:
make build
```

### Using GDB

```bash
gdb ./dist/bin/helloworld
(gdb) run
(gdb) backtrace
(gdb) print variable
```

### Log Output

```c
#include "uvhttp_logging.h"

// Set the log level
uvhttp_log_set_level(UVHTTP_LOG_LEVEL_DEBUG);

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

- [Architecture Design](../dev/ARCHITECTURE.md)
- [API Reference](../../api/API_REFERENCE.md)
- [Tutorial](../guide/TUTORIAL.md)
- [libuv Documentation](https://docs.libuv.org/)
- [HTTP/1.1 Specification](https://tools.ietf.org/html/rfc7230)

## License

MIT License - see the LICENSE file for details
