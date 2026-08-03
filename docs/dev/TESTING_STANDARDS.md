# UVHTTP Testing Standards

## Overview

This document defines the testing standards for the UVHTTP project, including standards for organizing, writing, naming, and maintaining test code.

## Table of Contents

1. [Test Types](#test-types)
2. [Test Frameworks](#test-frameworks)
3. [Test File Organization](#test-file-organization)
4. [Test Naming Conventions](#test-naming-conventions)
5. [Test Writing Standards](#test-writing-standards)
6. [Assertion Usage Standards](#assertion-usage-standards)
7. [Test Coverage Requirements](#test-coverage-requirements)
8. [Test Documentation Requirements](#test-documentation-requirements)
9. [CI/CD Integration](#cicd-integration)
10. [Test Maintenance](#test-maintenance)

## Test Types

### Unit Tests

- **Location**: `test/unit/`
- **Purpose**: Test the functionality of individual functions, modules, or classes
- **Characteristics**:
  - Fast execution (millisecond-scale)
  - Independence: no dependency on external resources
  - Repeatable: consistent results across runs
  - Automated: can be integrated into CI/CD

### Integration Tests

- **Location**: `test/integration/`
- **Purpose**: Test the interaction between multiple modules
- **Characteristics**:
  - Moderate execution time (second-scale)
  - May depend on external resources (files, network)
  - Test module cooperation in realistic scenarios
  - Can be automated

### Performance Tests

- **Location**: `test/performance/`
- **Purpose**: Test system performance metrics
- **Characteristics**:
  - Longer execution time (second to minute scale)
  - Test throughput, latency, and resource usage
  - Require specialized testing tools
  - Usually not run on every build

## Test Frameworks

### C++ Tests (Recommended)

**Framework**: Google Test (gtest)

**Advantages**:
- Feature-rich with comprehensive assertions
- Cross-platform support
- Active community and complete documentation
- Well integrated with CMake

**Use Cases**:
- Newly written tests
- Tests requiring complex assertions
- Tests requiring test suite organization

### C Tests (Compatibility)

**Framework**: Standard assert macros

**Advantages**:
- No external dependencies
- Simple and straightforward
- Compatible with existing code

**Use Cases**:
- Existing test code
- Simple test scenarios
- Tests that do not require complex assertions

## Test File Organization

### Directory Structure

```
test/
├── unit/                   # Unit tests
│   ├── test_*.cpp          # C++ tests (using GTest)
│   ├── test_*.c            # C tests (using assert)
│   └── simple_test.cpp     # GTest example test
├── integration/            # Integration tests
│   ├── test_*.c            # C tests
│   └── websocket_test.html # WebSocket test page
├── performance/            # Performance tests
│   ├── test_*.c            # C tests
│   └── performance_*.c     # Performance tests
├── config/                 # Configuration files
│   ├── config_*.conf
│   └── *.md
├── scripts/                # Test scripts
│   └── *.sh
├── results/                # Test results (not committed)
└── CMakeLists.txt          # Test CMake configuration
```

### File Naming Conventions

**Unit tests**:
- C++ tests: `test_<module>.cpp`
- C tests: `test_<module>.c`
- Examples: `test_allocator.cpp`, `test_utils.c`

**Integration tests**:
- Format: `test_<feature>.c`
- Examples: `test_simple.c`, `test_route.c`

**Performance tests**:
- Format: `performance_<feature>.c`
- Examples: `performance_allocator.c`

## Test Naming Conventions

### Test Suite Naming

**Format**: `<Module>Test`

**Examples**:
- `UvhttpUtilsTest`
- `UvhttpAllocatorTest`
- `UvhttpValidationTest`

### Test Case Naming

**Format**: `CamelCase`, descriptive

**Examples**:
- `SafeStrncpyNormal`
- `SafeStrncpyOverflow`
- `ValidateUrlValid`
- `NullFree`

### Test Function Naming (C tests)

**Format**: `test_<feature>_<scenario>`

**Examples**:
- `test_allocator_basic`
- `test_allocator_calloc`
- `test_manager_create_normal`

## Test Writing Standards

### C++ Tests (GTest)

```cpp
#include <gtest/gtest.h>
#include "uvhttp_utils.h"

// Test suite
TEST(UvhttpUtilsTest, SafeStrncpyNormal) {
    // Arrange
    char dest[10];
    
    // Act
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    
    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "hello");
}
```

### C Tests (assert)

```c
#include "uvhttp_utils.h"
#include <stdio.h>
#include <assert.h>

void test_safe_strncpy_normal(void) {
    // Arrange
    char dest[10];
    
    // Act
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    
    // Assert
    assert(result == 0);
    assert(strcmp(dest, "hello") == 0);
}
```

### AAA Pattern

All tests should follow the **Arrange-Act-Assert** pattern:

1. **Arrange**: Set up the test environment and initialize variables
2. **Act**: Call the function under test
3. **Assert**: Verify the result

## Assertion Usage Standards

### GTest Assertions

| Assertion Type | Description | Failure Behavior |
|----------------|-------------|------------------|
| `EXPECT_EQ(val1, val2)` | Expect equality | Continue testing |
| `ASSERT_EQ(val1, val2)` | Assert equality | Stop testing |
| `EXPECT_NE(val1, val2)` | Expect inequality | Continue testing |
| `ASSERT_NE(val1, val2)` | Assert inequality | Stop testing |
| `EXPECT_STREQ(str1, str2)` | Expect string equality | Continue testing |
| `ASSERT_STREQ(str1, str2)` | Assert string equality | Stop testing |
| `EXPECT_GT(val1, val2)` | Expect greater than | Continue testing |
| `EXPECT_LT(val1, val2)` | Expect less than | Continue testing |
| `EXPECT_TRUE(condition)` | Expect true | Continue testing |
| `EXPECT_FALSE(condition)` | Expect false | Continue testing |
| `EXPECT_PTR_EQ(ptr1, ptr2)` | Expect pointer equality | Continue testing |
| `EXPECT_PTR_NE(ptr1, ptr2)` | Expect pointer inequality | Continue testing |
| `EXPECT_NULL(ptr)` | Expect NULL | Continue testing |
| `EXPECT_NOTNULL(ptr)` | Expect not NULL | Continue testing |

### Standard assert

```c
assert(condition);           // Stops the program on failure
assert(ptr != NULL);        // Check that the pointer is not NULL
assert(a == b);             // Check equality
assert(strcmp(s1, s2) == 0); // Check string equality
```

### Assertion Selection Principles

1. **Use EXPECT**:
   - Non-critical verification
   - Can continue running subsequent tests
   - Need to collect multiple failure messages

2. **Use ASSERT**:
   - Critical preconditions
   - Testing cannot continue after failure
   - Prevent NULL pointer dereferencing

3. **Standard assert**:
   - C test code
   - Simple verification scenarios
   - When complex assertions are not needed

## Test Coverage Requirements

### Coverage Targets

- **Overall coverage**: ≥ 80%
- **Core module coverage**: ≥ 90%
- **Critical path coverage**: 100%

### Coverage Types

- **Line coverage**: each line of code executed at least once
- **Branch coverage**: each if/else branch executed at least once
- **Function coverage**: each function called at least once

### Coverage Checks

```bash
# Generate a coverage report
cmake -DENABLE_COVERAGE=ON ..
make
./run_tests.sh --detailed

# View the coverage report
lcov --capture --directory build --output-file coverage.info
lcov --report coverage.info
```

## Test Documentation Requirements

### Test File Header Comments

Each test file should contain:

```c
/**
 * @file test_allocator.c
 * @brief UVHTTP unified memory allocator tests
 * 
 * Test content:
 * - Basic allocation and deallocation
 * - calloc allocation
 * - realloc reallocation
 * - Boundary condition handling
 * - Large memory allocation
 * 
 * Test coverage target: 95%
 * Dependencies: uvhttp_allocator.h, test_memory_helpers.c
 */
```

### Test Function Comments

Complex test functions should include comments:

```c
/**
 * @brief Test medium file chunked transfer
 * 
 * Creates a 5MB test file and verifies the file size and content
 * 
 * @note This test creates a temporary file that is automatically deleted when the test completes
 */
static void test_medium_file_chunked_transfer(void) {
    // ...
}
```

## CI/CD Integration

### Automated Testing

All tests must be integrated into the CI/CD system:

```yaml
# .github/workflows/ci.yml
- name: Run unit tests
  run: make test

- name: Run integration tests
  run: ctest -R integration

- name: Generate coverage report
  run: ./run_tests.sh --detailed
```

### Test Timeout Configuration

The UVHTTP project uses the following test timeout configuration:

```yaml
# .github/workflows/ci.yml
- name: Run tests
  run: |
    cd build
    ctest --output-on-failure -j$(nproc) --timeout 600 \
      --exclude-regex "test_deps_full_coverage|test_lru_cache_full_coverage|test_server_full_coverage|test_server_rate_limit_coverage|test_server_simple_api_coverage"
```

**Timeout configuration notes:**

1. **Global timeout (600 seconds)**:
   - Reason: some tests include time-related operations (such as `sleep(2)` to test cache expiration)
   - `test_lru_cache_full_coverage` contains 3 `sleep(2)` calls, totaling 6 seconds
   - Other tests may include network operations or file I/O that require more time
   - 600 seconds ensures all normal tests complete while avoiding infinite waits

2. **Skipped tests**:
   - `test_deps_full_coverage`: contains long-running dependency initialization operations
   - `test_lru_cache_full_coverage`: contains `sleep(2)` calls to test cache expiration
   - `test_server_full_coverage`: server-related tests that may require more time
   - `test_server_rate_limit_coverage`: rate limiting feature tests that may require waiting
   - `test_server_simple_api_coverage`: simple API tests that may have timeout issues

3. **Future optimization directions**:
   - Optimize test design to reduce reliance on `sleep()`
   - Use mocks or stubs instead of real time-related operations
   - Separate slow tests into their own CI/CD job
   - Use parallel testing to accelerate execution

### Test Failure Handling

- Unit test failure: blocks merge
- Integration test failure: blocks merge
- Performance test failure: issues a warning, does not block merge

## Test Maintenance

### Test Update Principles

1. **When adding new features**:
   - Add corresponding tests at the same time
   - Ensure test coverage does not decline

2. **When fixing bugs**:
   - First add a failing test case
   - Fix the bug so the test passes
   - Prevent regression

3. **When refactoring code**:
   - Update related tests
   - Ensure tests still pass
   - Maintain test coverage

### Test Cleanup

- Remove tests for deprecated functionality
- Update outdated test cases
- Optimize slow tests

## Best Practices

### 1. Test Independence

Each test should run independently without depending on the execution order or state of other tests.

### 2. Test Repeatability

Test results should be repeatable and not depend on the external environment or time.

### 3. Test Speed

Unit tests should execute quickly (< 1 second); integration tests should be as fast as possible (< 10 seconds).

### 4. Test Clarity

Test names and assertions should clearly express the test intent.

### 5. Test Completeness

Tests should cover normal cases, boundary cases, and error cases.

## Prohibited Practices

1. Do not use global variables in tests
2. Do not use hardcoded paths in tests
3. Do not use sleep waits in tests
4. Do not depend on network resources in tests
5. Do not modify source code in tests
6. Do not ignore compilation warnings in tests

## Examples

### GTest Example

```cpp
#include <gtest/gtest.h>
#include "uvhttp_utils.h"

TEST(UvhttpUtilsTest, SafeStrncpyNormal) {
    char dest[10];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
}

TEST(UvhttpUtilsTest, SafeStrncpyOverflow) {
    char dest[5];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "123456789", sizeof(dest)), 0);
    EXPECT_LT(strlen(dest), sizeof(dest));
}
```

### C Test Example

```c
#include "uvhttp_utils.h"
#include <stdio.h>
#include <assert.h>

void test_safe_strncpy_normal(void) {
    char dest[10];
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    assert(result == 0);
    assert(strcmp(dest, "hello") == 0);
}

void test_safe_strncpy_overflow(void) {
    char dest[5];
    int result = uvhttp_safe_strncpy(dest, "123456789", sizeof(dest));
    assert(result == 0);
    assert(strlen(dest) < sizeof(dest));
}

int main(void) {
    test_safe_strncpy_normal();
    printf("✓ test_safe_strncpy_normal: PASSED\n");
    
    test_safe_strncpy_overflow();
    printf("✓ test_safe_strncpy_overflow: PASSED\n");
    
    return 0;
}
```

## Reference Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [UVHTTP Architecture Documentation](./ARCHITECTURE.md)
- [UVHTTP Developer Guide](../guide/DEVELOPER_GUIDE.md)

## Version History

- v1.0 (2026-01-15): Initial version, defining basic testing standards
