# UVHTTP Build Mode Specification

## Overview

This document defines the build mode specification for the UVHTTP project, clarifying which programs must be compiled in Release mode, which may use Debug mode, and the corresponding build options and best practices.

## Build Mode Classification

### 1. Release Mode (Production)

**Use cases**:
- All performance test programs
- Production deployment
- Performance benchmarking
- Release builds

**Programs that must use Release mode**:
- `benchmark_unified` - comprehensive performance test server (includes all test scenarios)
- `performance_allocator` - allocator performance tests
- `performance_allocator_compare` - allocator comparison tests
- `test_bitfield` - bitfield performance tests
- All example programs (examples/)

**Build options**:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

**Optimization level**: `-O2` (default)
- Code-size optimization
- Performance optimization
- No debug symbols
- `NDEBUG` macro enabled

**Performance characteristics**:
- Maximum performance
- Minimum memory footprint
- Minimum binary size

### 2. Debug Mode (Development)

**Use cases**:
- Unit tests
- Integration tests
- Code coverage analysis
- Debugging and problem troubleshooting
- Development phase

**Programs that may use Debug mode**:
- `uvhttp_unit_tests` - unit tests
- All test programs (test/)

**Build options**:
```bash
cmake -DENABLE_DEBUG=ON ..
```

or

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

**Optimization level**: `-O0`
- No optimization
- Full debug symbols
- `NDEBUG` macro disabled
- Assertions enabled

**Debugging characteristics**:
- Complete debug information
- Supports GDB/LLDB debugging
- Supports Valgrind memory checking
- Supports AddressSanitizer

### 3. Coverage Mode (Code Coverage)

**Use cases**:
- Code coverage analysis
- Test quality assessment

**Build options**:
```bash
cmake -DENABLE_COVERAGE=ON ..
```

**Optimization level**: `-O0`
- No optimization
- gcov coverage collection enabled
- Generates `.gcda` and `.gcno` files

**Coverage characteristics**:
- Supports line coverage
- Supports branch coverage
- Supports function coverage

## Build Mode Comparison

| Feature | Release | Debug | Coverage |
|---------|---------|-------|----------|
| Optimization level | `-O2` | `-O0` | `-O0` |
| Debug symbols | None | Full | Full |
| NDEBUG | Enabled | Disabled | Disabled |
| Assertions | Disabled | Enabled | Enabled |
| Performance | Highest | Lowest | Lowest |
| Debugging capability | Limited | Full | Full |
| Binary size | Smallest | Largest | Largest |
| Use case | Production, performance testing | Development, debugging | Coverage analysis |

## Build Option Details

### Release Mode Options

```cmake
# Defined in CMakeLists.txt
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
```

**Description**:
- `-O2`: enables advanced optimizations, balancing performance and compilation time
- `-DNDEBUG`: disables assertions to improve performance
- `-ffunction-sections -fdata-sections`: function and data section placement
- `-Wl,--gc-sections -s`: removes unused sections at link time, strips the symbol table

### Debug Mode Options

```cmake
# Defined in CMakeLists.txt
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()
```

**Description**:
- `-g`: generates debug information
- `-O0`: disables all optimizations
- Retains all assertions and debug code

### Coverage Mode Options

```cmake
# Defined in CMakeLists.txt
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

**Description**:
- `--coverage`: enables code coverage collection
- `-fprofile-arcs`: generates the program flow graph
- `-ftest-coverage`: generates coverage data

## Build Best Practices

### 1. Performance Tests Must Use Release Mode

**Reason**:
- Performance data from Debug mode is not representative
- Debug mode disables all optimizations; performance can be 10-100x lower
- Performance tests must run under production configuration

**Example**:
```bash
# Incorrect: running performance tests in Debug mode
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make benchmark_rps
./dist/bin/benchmark_rps 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# Result: ~10,000 RPS (inaccurate)

# Correct: running performance tests in Release mode
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make benchmark_unified
./dist/bin/benchmark_unified 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# Result: ~20,000+ RPS (accurate)
```

### 2. Use Debug Mode for Development and Debugging

**Reason**:
- Complete debug information
- Assertions enabled to catch problems early
- Convenient GDB/LLDB debugging

**Example**:
```bash
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
```

### 3. Use Coverage Mode for Code Coverage

**Reason**:
- Accurate coverage data
- Supports generating detailed coverage reports

**Example**:
```bash
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
gcovr --html --html-details -o coverage.html
```

### 4. Separate Build Directories

**Recommended practice**:
```bash
# Debug build
mkdir build-debug && cd build-debug
cmake -DENABLE_DEBUG=ON ..
make

# Release build
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Coverage build
mkdir build-coverage && cd build-coverage
cmake -DENABLE_COVERAGE=ON ..
make
```

## CMake Configuration Specification

### Performance Test Program Configuration

All performance test programs should be configured in `benchmark/benchmark.cmake` and must ensure Release mode optimization.

```cmake
# benchmark/benchmark.cmake

# Performance test executables
add_executable(benchmark_rps
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_rps.c
)
target_link_libraries(benchmark_rps
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

# Ensure Release optimization
set_target_properties(benchmark_unified PROPERTIES
    CMAKE_BUILD_TYPE Release
)
```

### Unit Test Program Configuration

Unit test programs should be configured in `test/CMakeLists.txt` and may use Debug mode.

```cmake
# test/CMakeLists.txt

# Unit tests
add_executable(uvhttp_unit_tests
    ${TEST_SOURCES}
)
target_link_libraries(uvhttp_unit_tests
    uvhttp
    ${UVHTTP_TEST_DEPS}
)

# May use Debug mode
if(ENABLE_DEBUG)
    set_target_properties(uvhttp_unit_tests PROPERTIES
        CMAKE_BUILD_TYPE Debug
    )
endif()
```

## Verifying the Build Mode

### Checking the Current Build Mode

```bash
# Method 1: inspect the CMake cache
cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE

# Method 2: use the cmake command
cd build
cmake -L | grep CMAKE_BUILD_TYPE

# Method 3: check the binary
file build/dist/bin/benchmark_rps
```

### Verifying the Optimization Level

```bash
# Check build options
make VERBOSE=1 | grep -E "\-O[0-3]"

# Check the symbol table
nm build/dist/bin/benchmark_unified | wc -l
# Release mode: small symbol table
# Debug mode: large symbol table
```

## Frequently Asked Questions

### Q1: Why are performance test results unstable?

**A**: Possible causes:
1. Compiled in Debug mode
2. System load too high
3. Network bottleneck
4. Port in use

**Solution**:
```bash
# Ensure Release mode is used
cmake -DCMAKE_BUILD_TYPE=Release ..
make clean && make

# Use a free port
./dist/bin/benchmark_unified 18082

# Run the test multiple times and average the results
for i in {1..5}; do
    wrk -t4 -c100 -d10s http://127.0.0.1:18082/
done
```

### Q2: How to debug in Release mode?

**A**: You can add debug information in Release mode:

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      ..
```

### Q3: When should `-O3` optimization be used?

**A**: Using `-O3` is generally not recommended because:
1. It may increase code size
2. It may introduce unstable optimizations
3. `-O2` already provides a good performance balance

If maximum performance is required, you can try:
```bash
cmake -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" \
      ..
```

## Performance Benchmarks

### Expected Performance Metrics (Release mode)

| Test type | Expected RPS | Expected latency |
|-----------|--------------|------------------|
| Simple GET | 20,000+ | < 5ms |
| JSON response | 15,000+ | < 10ms |
| Static files | 10,000+ | < 20ms |
| Route lookup | 600,000+ | < 2μs |

### Debug Mode Performance Degradation

Debug mode performance is typically **10-100x lower** than Release mode, therefore:

- Do not use Debug mode performance data as a baseline
- Do not base optimization decisions on Debug mode performance
- Performance tests must use Release mode
- Performance comparisons must be made in the same build mode

## Summary

**Key principles**:
1. **Performance tests must use Release mode** - otherwise the data is inaccurate
2. **Use Debug mode for development and debugging** - for easier troubleshooting
3. **Use Coverage mode for coverage analysis** - for accurate coverage data
4. **Separate build directories** - to avoid build mode confusion
5. **Verify the build mode** - to ensure the correct configuration is used

**Quick command reference**:
```bash
# Release build (performance testing)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build (development and debugging)
cmake -DENABLE_DEBUG=ON ..

# Coverage build (coverage analysis)
cmake -DENABLE_COVERAGE=ON ..

# Check the build mode
cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE
```

## References

- [CMake build mode documentation](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [Contributor guide](../guide/DEVELOPER_GUIDE.md)
- [Performance testing standard](PERFORMANCE_TESTING_STANDARD.md)
