# Fix Double Free Bugs & Compression Test Build

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 3 double-free memory safety bugs in unit tests and restore compression test compilation.

**Architecture:** Each fix is isolated to a single test file (or CMake config). No production code changes needed — the bugs are in test code that misuses API contracts (freeing stack memory, double-freeing context, or double-freeing config owned by context).

**Tech Stack:** C11, CMake, GoogleTest

---

## Root Cause Summary

| Test | Symptom | Root Cause |
|------|---------|------------|
| `test_config_full_coverage` | `free(): double free` | `uvhttp_config_set_current` makes context own config; TearDown frees config twice (once directly, once via context cleanup) |
| `test_context_full_coverage` | `free(): double free` | Test calls `uvhttp_context_destroy(context)` twice on same pointer without nulling it |
| `test_error_helpers_basic_coverage` | `double free or corruption` | Test passes stack-allocated `uv_write_t` to `uvhttp_handle_write_error` which calls `uvhttp_free(req)` — freeing stack memory |
| `test_compression_helpers` | `Not Run` | `BUILD_WITH_COMPRESSION=OFF` by default, tests are skipped at CMake configure time |

---

### Task 1: Fix test_config_full_coverage double free

**Files:**
- Modify: `test/unit/test_config_full_coverage.cpp:297-301`

**Root Cause:** `uvhttp_config_set_current(context, config)` sets `context->current_config = config`. TearDown first calls `uvhttp_config_free(config)`, then `uvhttp_context_destroy(context)` → `uvhttp_context_cleanup_config` → `uvhttp_config_free(context->current_config)` = double free.

- [ ] **Step 1: Detach config from context before TearDown**

In `ConfigGetCurrentValid`, after the assertion, detach config from context so TearDown's cleanup won't double-free:

```cpp
TEST_F(UvhttpConfigTest, ConfigGetCurrentValid) {
    uvhttp_config_set_current(context, config);
    const uvhttp_config_t* result = uvhttp_config_get_current(context);
    EXPECT_EQ(result, config);
    // Detach: context cleanup must not free config (TearDown frees it directly)
    context->current_config = nullptr;
}
```

- [ ] **Step 2: Build and run the test**

```bash
cd /home/gem/project/uvhttp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
./dist/bin/test_config_full_coverage
```

Expected: All tests PASS, no double free.

- [ ] **Step 3: Commit**

```bash
git add test/unit/test_config_full_coverage.cpp
git commit -m "fix(test): detach config from context before TearDown to prevent double free"
```

---

### Task 2: Fix test_context_full_coverage double free

**Files:**
- Modify: `test/unit/test_context_full_coverage.cpp:397-410`

**Root Cause:** Test calls `uvhttp_context_destroy(context)` twice. First call frees memory via `uvhttp_free(context)`, second call operates on dangling pointer.

- [ ] **Step 1: Null the pointer after first destroy**

```cpp
TEST_F(UvhttpContextTest, ContextMultipleDestroy) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);

    err = uvhttp_context_init(context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Destroy once
    uvhttp_context_destroy(context);
    context = nullptr;

    // Second destroy with NULL should be safe (no-op)
    uvhttp_context_destroy(context);
}
```

- [ ] **Step 2: Build and run the test**

```bash
cd /home/gem/project/uvhttp
make build && make test
```

Expected: `test_context_full_coverage` PASS, no double free.

- [ ] **Step 3: Commit**

```bash
git add test/unit/test_context_full_coverage.cpp
git commit -m "fix(test): null context pointer after first destroy to prevent double free"
```

---

### Task 3: Fix test_error_helpers_basic_coverage memory corruption

**Files:**
- Modify: `test/unit/test_error_helpers_basic_coverage.cpp:62-68`

**Root Cause:** `uvhttp_handle_write_error` calls `uvhttp_free(req)` at line 72 of `src/uvhttp_error_helpers.c`. Test passes stack-allocated `uv_write_t` — freeing stack memory causes corruption.

- [ ] **Step 1: Heap-allocate the uv_write_t**

```cpp
TEST_F(UvhttpErrorHelpersBasicTest, HandleWriteErrorWithValidStatus) {
    // uvhttp_handle_write_error calls uvhttp_free(req), so req must be heap-allocated
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    ASSERT_NE(req, nullptr);
    // Should not crash with various status codes
    uvhttp_handle_write_error(req, UV_EPIPE, "test_context");

    uv_write_t* req2 = (uv_write_t*)malloc(sizeof(uv_write_t));
    ASSERT_NE(req2, nullptr);
    uvhttp_handle_write_error(req2, UV_ECONNRESET, "test_context");

    uv_write_t* req3 = (uv_write_t*)malloc(sizeof(uv_write_t));
    ASSERT_NE(req3, nullptr);
    uvhttp_handle_write_error(req3, UV_ECANCELED, "test_context");
}
```

- [ ] **Step 2: Build and run the test**

```bash
cd /home/gem/project/uvhttp
make build && make test
```

Expected: `test_error_helpers_basic_coverage` PASS, no corruption.

- [ ] **Step 3: Commit**

```bash
git add test/unit/test_error_helpers_basic_coverage.cpp
git commit -m "fix(test): heap-allocate uv_write_t in error helpers test to prevent stack free"
```

---

### Task 4: Fix compression test build

**Files:**
- Modify: `CMakeLists.txt:21` (change default) OR verify tests are excluded correctly

**Root Cause:** `BUILD_WITH_COMPRESSION` defaults to `OFF`. The CMake filter at line 607-609 excludes tests matching `.*compression.*` when the flag is off. The test binaries are never built, so CTest reports "Not Run".

- [ ] **Step 1: Verify compression test exclusion logic**

Check that the CMake filter correctly skips compression tests when disabled:

```bash
cd /home/gem/project/uvhttp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug 2>&1 | grep -i compression
```

Expected output: `Static files support: DISABLED` and the warning about skipping compression tests.

- [ ] **Step 2: Verify with compression enabled**

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_COMPRESSION=ON
cmake --build . -j$(nproc)
ctest --output-on-failure -R compression
```

Expected: compression tests compile and run (pass or fail based on zlib availability).

- [ ] **Step 3: Decide on default or exclude from CTest**

If compression tests are not meant to run by default (since the feature is off), they should be excluded from CTest when not built. The CMake filter at line 607-609 already does this. The issue is that CTest still lists them as "Not Run" rather than skipping them silently.

Fix: wrap the `add_test` in the CTest section with the same compression check:

In `CMakeLists.txt` around line 785-788, the CTest loop unconditionally adds all test files. Add the same filter:

```cmake
# Add unit tests to CTest
foreach(test_file ${GTEST_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)

    # Skip compression tests when compression is disabled
    if(${test_name} MATCHES ".*compression.*" AND NOT BUILD_WITH_COMPRESSION)
        continue()
    endif()

    add_test(NAME ${test_name} COMMAND ${CMAKE_BINARY_DIR}/dist/bin/${test_name})
endforeach()
```

- [ ] **Step 4: Rebuild and verify**

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make test
```

Expected: No "Not Run" compression tests in output. Total test count reduced by 2.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt
git commit -m "fix(build): exclude compression tests from CTest when BUILD_WITH_COMPRESSION is OFF"
```

---

### Task 5: Full test suite verification

- [ ] **Step 1: Run complete test suite**

```bash
cd /home/gem/project/uvhttp
make build && make test
```

Expected: **0 failures**, all tests pass. The 5 previously failing tests should now be resolved:
- `test_config_full_coverage` — PASS (Task 1)
- `test_context_full_coverage` — PASS (Task 2)
- `test_error_helpers_basic_coverage` — PASS (Task 3)
- `test_compression_helpers` — excluded from CTest when feature off (Task 4)
- `test_response_compression` — excluded from CTest when feature off (Task 4)

- [ ] **Step 2: Final commit (if any remaining changes)**

```bash
git status
# If there are uncommitted changes, commit them
```

---

## Summary

| Task | Fix | Files Changed |
|------|-----|---------------|
| 1 | Config double free | `test_config_full_coverage.cpp` |
| 2 | Context double free | `test_context_full_coverage.cpp` |
| 3 | Stack free corruption | `test_error_helpers_basic_coverage.cpp` |
| 4 | Compression test build | `CMakeLists.txt` |
| 5 | Full test verification | (none) |
