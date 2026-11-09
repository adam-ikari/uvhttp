# UVHTTP Build Mode Specification

## Overview

UVHTTP builds in one of three modes. Release mode is for production and benchmarks, Debug mode is for development, and Coverage mode is for coverage analysis. This page describes what each mode changes and when to use it.

## Build Mode Classification

### 1. Release Mode (Production)

Release mode serves production deployment, performance benchmarking, and release builds. Every performance test program and every example program builds in Release.

The programs that must use Release mode:

- `benchmark_unified` is the full performance test server and includes all test scenarios.
- `performance_allocator` measures the allocator.
- `performance_allocator_compare` compares allocators.
- `test_bitfield` measures bitfield access.
- Every example program under examples/.

Build options:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

The optimization level is `-O2` by default. The build optimizes for code size and speed, leaves out debug symbols, and defines `NDEBUG`.

Release mode gives the highest throughput, the smallest memory footprint, and the smallest binary.

### 2. Debug Mode (Development)

Debug mode serves unit tests, integration tests, coverage analysis, debugging, and day-to-day development.

The programs that may use Debug mode:

- `uvhttp_unit_tests`, the unit test binary.
- Every test program under test/.

Build options:

```bash
cmake -DENABLE_DEBUG=ON ..
```

or

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

The optimization level is `-O0`. Optimization is off, debug symbols are complete, `NDEBUG` is undefined, and assertions stay active.

Debug mode gives complete debug information and works with GDB/LLDB, Valgrind, and AddressSanitizer.

### 3. Coverage Mode (Code Coverage)

Coverage mode exists for coverage analysis and test quality assessment.

Build options:

```bash
cmake -DENABLE_COVERAGE=ON ..
```

The optimization level is `-O0`. Optimization is off, gcov collection is on, and the build writes `.gcda` and `.gcno` files.

Coverage mode reports line coverage, branch coverage, and function coverage.

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

`-O2` turns on the optimizations that pay for themselves without slowing compilation much. `-DNDEBUG` removes assertions. `-ffunction-sections -fdata-sections` place each function and data item in its own section, and `-Wl,--gc-sections -s` drops unused sections at link time and strips the symbol table.

### Debug Mode Options

```cmake
# Defined in CMakeLists.txt
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()
```

`-g` emits debug information, and `-O0` turns optimization off. Assertions and debug code stay in the binary.

### Coverage Mode Options

```cmake
# Defined in CMakeLists.txt
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

`--coverage` turns on coverage collection, `-fprofile-arcs` records the program flow graph, and `-ftest-coverage` writes the coverage data.

## Build Best Practices

### 1. Performance Tests Must Use Release Mode

Debug numbers mislead. With optimization off, throughput can drop by 10-100x, so a benchmark has to run the production configuration.

The example below runs the same test both ways:

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

Debug mode keeps complete debug information and leaves assertions on, so a bad state surfaces close to where it was created. GDB and LLDB both work.

```bash
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
```

### 3. Use Coverage Mode for Code Coverage

Coverage mode produces accurate coverage data and can generate a detailed report.

```bash
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
gcovr --html --html-details -o coverage.html
```

### 4. Separate Build Directories

Keep one build directory per mode:

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

Benchmark targets live in `benchmark/benchmark.cmake` and must keep Release optimization.

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

Unit test targets live in `test/CMakeLists.txt` and may build in Debug.

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

The build log shows which optimization flag the compiler received:

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

Four causes account for most of it:

1. The binary was compiled in Debug mode.
2. The machine is under heavy load.
3. The network is the bottleneck.
4. The port is already in use.

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

Release mode can carry debug information:

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      ..
```

### Q3: When should `-O3` optimization be used?

`-O3` is usually a bad trade:

1. It can grow the code size.
2. It can introduce unstable optimizations.
3. `-O2` already balances speed and size well.

If you need the last bit of performance, try:

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

Debug mode typically runs 10-100x slower than Release mode. Therefore:

- Do not use Debug mode performance data as a baseline.
- Do not base optimization decisions on Debug mode performance.
- Run performance tests in Release mode.
- Compare numbers only within the same build mode.

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
