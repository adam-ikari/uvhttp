# Build Configuration Matrix

This document details the various build configuration combinations supported by UVHTTP and their purposes.

## Overview

UVHTTP supports flexible combination of different functional modules through CMake configuration options to meet various usage scenario requirements.

This document primarily covers common core configuration options. For advanced configuration options (such as buffer sizes, connection limits, cache parameters, etc.), please refer to [Advanced Build Options](ADVANCED_BUILD_OPTIONS.md).

## Feature Options

These options control which functional modules are compiled into the library.

### BUILD_WITH_WEBSOCKET
- **Type**: BOOL
- **Default**: ON
- **Description**: Enable WebSocket support (both ws:// and wss://)
- **Impact**:
  - Compile `src/uvhttp_websocket.c`
  - Link WebSocket-related dependencies
  - Provide WebSocket API
  - Support both ws:// (WebSocket) and wss:// (WebSocket Secure)
- **Dependencies**: mbedtls library (required for base64, SHA1, random number generation, and TLS encryption)
- **Notes**:
  - Both ws:// and wss:// require TLS library (mbedtls)
  - ws:// uses mbedtls for frame encoding and security features
  - wss:// uses mbedtls for TLS encryption layer
  - mbedtls is compiled if either BUILD_WITH_HTTPS or BUILD_WITH_WEBSOCKET is ON

### BUILD_WITH_HTTPS
- **Type**: BOOL
- **Default**: ON
- **Description**: Enable HTTPS support
- **Impact**:
  - Compile `src/uvhttp_tls.c`
  - Link mbedtls library
  - Provide TLS context and encryption functionality
  - Support HTTPS and WSS (WebSocket Secure)
  - Set UVHTTP_FEATURE_TLS=1 macro
- **Dependencies**: mbedtls library
- **Notes**:
  - mbedtls is compiled if either BUILD_WITH_HTTPS or BUILD_WITH_WEBSOCKET is ON
  - If both BUILD_WITH_HTTPS and BUILD_WITH_WEBSOCKET are OFF, mbedtls is not compiled

### BUILD_WITH_MIMALLOC
- **Type**: BOOL
- **Default**: Based on `UVHTTP_ALLOCATOR_TYPE` (OFF for system/custom, ON for mimalloc)
- **Description**: Use mimalloc memory allocator
- **Impact**:
  - Link mimalloc library
  - Provide faster memory allocation performance (30-50% improvement)
  - Reduce memory fragmentation
- **Dependencies**: None
- **Notes**:
  - Automatically enabled when `UVHTTP_ALLOCATOR_TYPE=1`
  - Can be manually enabled regardless of allocator type
  - Disabled by default for `UVHTTP_ALLOCATOR_TYPE=0` (system) and `UVHTTP_ALLOCATOR_TYPE=2` (custom)

### ENABLE_DEBUG
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable debug mode
- **Impact**:
  - Disable compiler optimizations (-O0)
  - Enable debug symbols (-g)
  - Enable log output (even if UVHTTP_FEATURE_LOGGING=0)
- **Dependencies**: None

### ENABLE_COVERAGE
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable code coverage
- **Impact**:
  - Add coverage compilation flags
  - Generate coverage reports
- **Dependencies**: `ENABLE_DEBUG=ON` (recommended)

### BUILD_EXAMPLES
- **Type**: BOOL
- **Default**: OFF
- **Description**: Build example programs
- **Impact**:
  - Compile all examples in examples/ directory
  - Generate executables to build/dist/bin/
- **Dependencies**: None

## Build Mode Options

These options control the build mode (debug, release, coverage).

### BUILD_BENCHMARKS
- **Type**: BOOL
- **Default**: OFF
- **Description**: Build performance benchmark programs
- **Impact**:
  - Compile all performance benchmark programs in benchmark/ directory
  - Generate executables to build/dist/bin/
- **Dependencies**: None

### ENABLE_LTO
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable Link Time Optimization (LTO)
- **Impact**:
  - Cross-compilation unit optimization
  - Potential performance improvement of 5-15%
  - Increased compilation time (50-100%)
- **Dependencies**: `CMAKE_BUILD_TYPE=Release` (recommended)
- **Conflicts**: `ENABLE_DEBUG=ON`, `ENABLE_ASAN=ON`, `ENABLE_TSAN=ON`

### ENABLE_PGO
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable Profile Guided Optimization (PGO)
- **Impact**:
  - Optimize based on actual runtime data
  - Potential performance improvement of 10-20%
  - Requires two-step compilation process
- **Dependencies**: `CMAKE_BUILD_TYPE=Release`
- **Conflicts**: `ENABLE_DEBUG=ON`

### ENABLE_FASTER_MATH
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable faster math optimizations
- **Impact**:
  - Use -ffast-math compilation option
  - Potential improvement in floating-point operation performance
  - May affect numerical precision
- **Dependencies**: None

## Debugging Tools

These options enable runtime error detection tools (Sanitizers).

### ENABLE_ASAN
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable AddressSanitizer (memory error detection)
- **Impact**:
  - Detect memory out-of-bounds, use-after-free, and other errors
  - Performance degradation of 50-70%
  - Memory usage increase of 2-3x
- **Dependencies**: `ENABLE_DEBUG=ON` (recommended)
- **Conflicts**: `ENABLE_LTO=ON`, `ENABLE_TSAN=ON`

### ENABLE_UBSAN
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable UndefinedBehaviorSanitizer (undefined behavior detection)
- **Impact**:
  - Detect undefined behaviors
  - Performance degradation of 20-30%
- **Dependencies**: `ENABLE_DEBUG=ON` (recommended)
- **Conflicts**: `ENABLE_LTO=ON`

### ENABLE_TSAN
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable ThreadSanitizer (thread safety detection)
- **Impact**:
  - Detect data races
  - Performance degradation of 50-70%
  - Memory usage increase of 5-10x
- **Dependencies**: `ENABLE_DEBUG=ON` (recommended)
- **Conflicts**: `ENABLE_LTO=ON`, `ENABLE_ASAN=ON`

### ENABLE_VALGRIND
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable Valgrind support
- **Impact**:
  - Add Valgrind compatibility code
  - Facilitate memory leak detection
- **Dependencies**: None

### ENABLE_DEV_MODE
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable development mode
- **Impact**:
  - Enable logging system (UVHTTP_FEATURE_LOGGING=1)
  - Add development debugging aids
  - Should be disabled in production for optimal performance
- **Dependencies**: None

## Logging Options

These options control logging verbosity.

### ENABLE_LOG_DEBUG
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable debug log level
- **Impact**:
  - Output detailed debug information
  - May affect performance
- **Dependencies**: None

### ENABLE_LOG_TRACE
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable trace log level
- **Impact**:
  - Output most detailed trace information
  - Significantly affects performance
- **Dependencies**: None

### ENABLE_LOG_PERFORMANCE
- **Type**: BOOL
- **Default**: OFF
- **Description**: Enable performance logging
- **Impact**:
  - Record performance metrics
  - Minor performance impact
- **Dependencies**: None

## Security Options

These options enable security hardening features.

### ENABLE_HARDENING
- **Type**: BOOL
- **Default**: ON
- **Description**: Enable security hardening
- **Impact**:
  - Stack protection, RELRO, and other security features
  - Minor performance impact
- **Dependencies**: None

### ENABLE_STACK_PROTECTION
- **Type**: BOOL
- **Default**: ON
- **Description**: Enable stack protection
- **Impact**:
  - Prevent stack overflow attacks
  - Minor performance impact
- **Dependencies**: None

### ENABLE_FORTIFY_SOURCE
- **Type**: BOOL
- **Default**: ON
- **Description**: Enable _FORTIFY_SOURCE
- **Impact**:
  - Enhanced buffer overflow detection
  - Minor performance impact
- **Dependencies**: None

## Advanced Configuration Options

Advanced configuration options (such as buffer sizes, connection limits, cache parameters) have been moved to a separate document:

**[Advanced Build Options](ADVANCED_BUILD_OPTIONS.md)**

Advanced configuration options are primarily used for:
- Performance tuning
- Memory-constrained environments
- Specific scenario optimization
- Resource-constrained environments

Most users do not need to modify these options. Default values have been optimized for general scenarios.

## Configuration Dependency Graph

```
BUILD_WITH_WEBSOCKET (ON)
    └─> mbedtls library [required - provides base64, SHA1, RNG, and encryption]

BUILD_WITH_HTTPS (ON)
    └─> mbedtls library [required - provides TLS for HTTPS and WSS]

mbedtls library
    └─> Compiled if BUILD_WITH_HTTPS=ON or BUILD_WITH_WEBSOCKET=ON

BUILD_WITH_MIMALLOC (ON)
    └─> No dependencies

ENABLE_DEBUG (OFF)
    └─> No dependencies

ENABLE_COVERAGE (OFF)
    └─> ENABLE_DEBUG=ON [recommended]

ENABLE_LTO (OFF)
    └─> CMAKE_BUILD_TYPE=Release [recommended]
    └─> !ENABLE_DEBUG [conflict]
    └─> !ENABLE_ASAN [conflict]
    └─> !ENABLE_TSAN [conflict]

ENABLE_PGO (OFF)
    └─> CMAKE_BUILD_TYPE=Release [required]
    └─> !ENABLE_DEBUG [conflict]

ENABLE_ASAN (OFF)
    └─> ENABLE_DEBUG=ON [recommended]
    └─> !ENABLE_LTO [conflict]
    └─> !ENABLE_TSAN [conflict]

ENABLE_UBSAN (OFF)
    └─> ENABLE_DEBUG=ON [recommended]
    └─> !ENABLE_LTO [conflict]

ENABLE_TSAN (OFF)
    └─> ENABLE_DEBUG=ON [recommended]
    └─> !ENABLE_LTO [conflict]
    └─> !ENABLE_ASAN [conflict]

ENABLE_LOG_DEBUG (OFF)
    └─> No dependencies

ENABLE_LOG_TRACE (OFF)
    └─> No dependencies

ENABLE_LOG_PERFORMANCE (OFF)
    └─> No dependencies

ENABLE_HARDENING (ON)
    └─> No dependencies

ENABLE_STACK_PROTECTION (ON)
    └─> No dependencies

ENABLE_FORTIFY_SOURCE (ON)
    └─> No dependencies

ENABLE_FASTER_MATH (OFF)
    └─> No dependencies

ENABLE_VALGRIND (OFF)
    └─> No dependencies

ENABLE_DEV_MODE (OFF)
    └─> No dependencies
```

## Configuration Combination Suggestions

### Development Environment
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_LOG_DEBUG=ON \
      -DENABLE_LOG_TRACE=ON \
      -DBUILD_EXAMPLES=ON
```

### Testing Environment
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_ASAN=ON \
      -DBUILD_EXAMPLES=OFF
```

### Production Environment (Security Hardened)
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_HARDENING=ON \
      -DENABLE_STACK_PROTECTION=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```

### Production Environment (Performance Optimized)
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_LTO=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```

### Performance Testing Environment
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DBUILD_BENCHMARKS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```

### Minimal Deployment
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```

## Notes

### Dependencies
- WebSocket depends on HTTPS (mbedtls library)
- If BUILD_WITH_HTTPS=OFF, WebSocket must also be disabled
- Sanitizers (ASAN, UBSAN, TSAN) should be used with ENABLE_DEBUG=ON

### Performance Impact
- **mimalloc**: 30-50% faster memory allocation
- **WebSocket**: Adds ~50KB to binary size
- **HTTPS**: Adds ~200KB to binary size
- **WebSocket + HTTPS**: Adds ~250KB to binary size
- **Debug mode**: 50-80% performance degradation
- **LTO**: 5-15% performance improvement, 50-100% longer compilation time
- **ASAN/TSAN**: 50-70% performance degradation, 2-10x memory usage increase

## Memory Allocator Configuration

### UVHTTP_ALLOCATOR_TYPE
- **Type**: INTEGER
- **Default**: 0
- **Description**: Memory allocator type selection
- **Options**:
  - `0`: System allocator (malloc/free)
  - `1`: mimalloc allocator (auto-enables `BUILD_WITH_MIMALLOC=ON`)
  - `2`: Custom allocator (application layer implementation)
- **Impact**:
  - System allocator: Standard performance, no dependencies
  - mimalloc: 30-50% faster memory allocation, reduced fragmentation
  - Custom: Full control over memory management
- **Notes**:
  - Option `1` automatically enables `BUILD_WITH_MIMALLOC=ON` for convenience
  - Option `2` requires application layer to implement custom allocator functions
  - For custom allocator implementation details, see [Advanced Build Options](ADVANCED_BUILD_OPTIONS.md)

### BUILD_WITH_MIMALLOC
- **Type**: BOOL
- **Default**: Based on `UVHTTP_ALLOCATOR_TYPE` (OFF for system/custom, ON for mimalloc)
- **Description**: Use mimalloc memory allocator
- **Impact**:
  - Link mimalloc library
  - Provide faster memory allocation performance (30-50% improvement)
  - Reduce memory fragmentation
- **Dependencies**: None
- **Notes**:
  - Automatically enabled when `UVHTTP_ALLOCATOR_TYPE=1`
  - Can be manually enabled regardless of allocator type
  - Disabled by default for `UVHTTP_ALLOCATOR_TYPE=0` (system) and `UVHTTP_ALLOCATOR_TYPE=2` (custom)

## Build Types

### CMAKE_BUILD_TYPE
- **Default**: Release
- **Options**:
  - `Release`: Optimized version, no debug symbols
  - `Debug`: Debug version, no optimization
  - `RelWithDebInfo`: Optimized version with debug symbols
  - `MinSizeRel`: Minimized size version

## Recommended Configuration Matrix

### 1. Minimal Configuration (No Optional Features)
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Minimal deployment, core HTTP functionality only
- **Size**: Minimal
- **Performance**: Basic performance

### 2. Full Features (All Optional Features)
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Production environment, full features
- **Size**: Medium
- **Performance**: Optimal performance

### 3. Debug Mode
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Development and debugging
- **Size**: Larger
- **Performance**: No optimization, easy to debug

### 4. Coverage Mode
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=ON \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Test coverage analysis
- **Size**: Maximum
- **Performance**: No optimization, with coverage detection

### 5. System Allocator
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Use system memory allocator (default)
- **Size**: Medium
- **Performance**: Standard malloc/free performance

### 6. Mimalloc Allocator
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Use mimalloc memory allocator
- **Size**: Medium
- **Performance**: Faster memory allocation (30-50% improvement)

**Note**: For more memory allocator configuration options (custom allocator, etc.), please refer to [Advanced Build Options](ADVANCED_BUILD_OPTIONS.md).

### 7. WebSocket Only
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: WebSocket applications
- **Size**: Medium
- **Performance**: Optimal performance

### 8. HTTPS Only
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: HTTPS applications
- **Size**: Medium
- **Performance**: Optimal performance

### 9. WebSocket + HTTPS
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: WebSocket and HTTPS applications
- **Size**: Medium
- **Performance**: Optimal performance

### 10. Example Programs
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DBUILD_EXAMPLES=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF
```
- **Purpose**: Build example programs
- **Size**: Large
- **Performance**: Optimal performance

### 11. 32-bit Compatibility Check
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Verify 32-bit system compatibility
- **Size**: Large
- **Performance**: No optimization

### 12. Static Analysis Preparation
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Prepare for static analysis tools
- **Size**: Large
- **Performance**: No optimization

### 13. Minimal Build
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=OFF \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Minimal build size
- **Size**: Minimal
- **Performance**: Basic performance

### 14. Performance Benchmark Programs
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DBUILD_BENCHMARKS=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Compile performance benchmark programs
- **Size**: Medium + ~500KB
- **Performance**: Optimal performance

### 15. Performance Benchmark + Example Programs
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DBUILD_BENCHMARKS=ON \
      -DBUILD_EXAMPLES=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF
```
- **Purpose**: Compile performance benchmarks and examples
- **Size**: Large
- **Performance**: Optimal performance

### 16. LTO Optimization
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_LTO=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Enable Link Time Optimization
- **Size**: Medium
- **Performance**: 5-15% performance improvement

### 17. AddressSanitizer
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_ASAN=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Memory error detection
- **Size**: Large
- **Performance**: 50-70% performance degradation

### 18. ThreadSanitizer
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_TSAN=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Thread safety detection
- **Size**: Large
- **Performance**: 50-70% performance degradation

### 19. Debug Logging
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_DEBUG=ON \
      -DENABLE_LOG_DEBUG=ON \
      -DENABLE_LOG_TRACE=ON \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Enable detailed debug logging
- **Size**: Large
- **Performance**: No optimization, significant logging overhead

### 20. Security Hardening
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_HTTPS=ON \
      -DENABLE_HARDENING=ON \
      -DENABLE_STACK_PROTECTION=ON \
      -DENABLE_DEBUG=OFF \
      -DENABLE_COVERAGE=OFF \
      -DBUILD_EXAMPLES=OFF
```
- **Purpose**: Production environment security hardening
- **Size**: Medium
- **Performance**: Minor impact
- **Security**: Stack protection, RELRO, etc.

## Configuration Validation

Use the provided test script to validate all configuration combinations:

```bash
./test_cmake_configs.sh
```

This script will test the above 20 configuration combinations and report pass/fail status.

### Compile Performance Benchmark Programs

Compile all performance benchmark programs:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON ..
make -j$(nproc)
```

Compile specific performance benchmark programs:

```bash
make benchmark_rps
make benchmark_latency
make benchmark_comprehensive
```

### Run Performance Tests

Run RPS test:

```bash
./build/dist/bin/benchmark_rps 18081
```

Run comprehensive performance test:

```bash
./build/dist/bin/benchmark_comprehensive 18082
```

Use wrk for stress testing:

```bash
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
```

## Notes

### TLS Disable Limitations
When `BUILD_WITH_HTTPS=OFF`, TLS-related code in the source files needs to be wrapped with `#ifdef UVHTTP_TLS_ENABLED`. In the current version, the following files contain unprotected TLS code:

- `src/uvhttp_connection.c` - TLS handshake and cleanup functions
- `src/uvhttp_server.c` - TLS context management
- `src/uvhttp_context.c` - TLS cleanup functions
- `src/uvhttp_websocket.c` - TLS-related WebSocket functionality

**Recommendation**: If you need to disable TLS, you should first modify these source files to add conditional compilation directives.

### Performance Impact
- **mimalloc**: 30-50% faster memory allocation
- **WebSocket**: Adds ~50KB to binary size
- **HTTPS**: Adds ~200KB to binary size
- **WebSocket + HTTPS**: Adds ~250KB to binary size
- **Debug mode**: 50-80% performance degradation
- **LTO**: 5-15% performance improvement, 50-100% longer compilation time
- **ASAN/TSAN**: 50-70% performance degradation, 2-10x memory usage increase

### Compilation Time
- **Minimal configuration**: ~30 seconds
- **Full features**: ~60 seconds
- **Coverage mode**: ~90 seconds
- **Performance benchmark programs**: ~120 seconds

## Related Documentation

- [Advanced Build Options](ADVANCED_BUILD_OPTIONS.md)
- [Developer Guide](guide/DEVELOPER_GUIDE.md)
- [Build Modes Details](zh/dev/BUILD_MODES.md)
- [CMake Target Linking Guide](dev/CMAKE_TARGET_LINKING_GUIDE.md)
- [Performance Benchmark](dev/PERFORMANCE_BENCHMARK.md)
- [Performance Testing Guide](benchmark/README.md)
- [Performance Testing Standard](dev/PERFORMANCE_TESTING_STANDARD.md)