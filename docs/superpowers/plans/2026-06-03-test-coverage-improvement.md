# Test Coverage Improvement Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Raise uvhttp test coverage from 62.7% to ~80% by adding targeted unit tests for uncovered code paths.

**Architecture:** Focus on testable modules first (request, response, router, protocol_upgrade) that don't require libuv event loop mocking. Connection, server, and websocket modules have heavy libuv callback dependencies — those will be addressed with the existing libuv_mock infrastructure where feasible.

**Tech Stack:** C11, GoogleTest (C++), CMake

---

## Coverage Baseline (62.7% lines, 82.8% functions)

| File | Lines | Coverage | Target | Strategy |
|------|-------|----------|--------|----------|
| `uvhttp_request.c` | 358 | 52% | 80%+ | Test getter/setter functions, null checks, edge cases |
| `uvhttp_response.c` | 282 | 66% | 85%+ | Test header/body setters, status codes, edge cases |
| `uvhttp_router.c` | 356 | 61% | 80%+ | Test trie matching, parameter extraction, edge cases |
| `uvhttp_protocol_upgrade.c` | 91 | 73% | 90%+ | Test WebSocket upgrade validation |
| `uvhttp_context.c` | 109 | 79% | 90%+ | Test init/cleanup edge cases |
| `uvhttp_error.c` | 351 | 98% | 99%+ | Cover remaining edge cases |

---

### Task 1: Boost uvhttp_request.c coverage (52% → 80%+)

**Files:**
- Create: `test/unit/test_request_boost_coverage.cpp`

**Strategy:** The uncovered lines are primarily in:
- `uvhttp_request_get_header()` edge cases (null request, missing header)
- `uvhttp_request_get_body()` with various states
- `uvhttp_request_get_method()` / `uvhttp_request_get_path()` edge cases
- Internal parsing functions called during request processing

Since request functions are tightly coupled to llhttp parsing, the best approach is to test through the public API with mock request objects. Many getter functions can be tested by directly setting struct fields.

- [ ] **Step 1: Read uncovered lines and identify testable functions**

Read `src/uvhttp_request.c` and identify all public functions that can be tested without a full connection. Focus on functions that take `uvhttp_request_t*` and return values.

- [ ] **Step 2: Write tests for request getter/setter functions**

Create `test/unit/test_request_boost_coverage.cpp` with tests for:
- `uvhttp_request_get_method()` with null request
- `uvhttp_request_get_path()` with null request
- `uvhttp_request_get_header()` with null request, missing header, valid header
- `uvhttp_request_get_body()` with null request, empty body, valid body
- `uvhttp_request_get_content_length()` edge cases
- Any other getter functions

- [ ] **Step 3: Build and run**

```bash
make build && cd build && ./dist/bin/test_request_boost_coverage
```

- [ ] **Step 4: Commit**

```bash
git add test/unit/test_request_boost_coverage.cpp
git commit -m "test(request): add coverage boost tests for request module"
```

---

### Task 2: Boost uvhttp_response.c coverage (66% → 85%+)

**Files:**
- Create: `test/unit/test_response_boost_coverage.cpp`

**Strategy:** Response functions are more testable since they work with response struct directly. Focus on:
- `uvhttp_response_set_status()` edge cases
- `uvhttp_response_set_header()` with various inputs
- `uvhttp_response_set_body()` edge cases
- Response formatting/sending paths (may need mock uv_write_t)

- [ ] **Step 1: Read uncovered lines**

Read `src/uvhttp_response.c` lines: 111-112, 160, 167, 170, 337-338, 418-419, 428, 430, 436, 444, 446, 448, 455, 457, 459, 461, 463, 465-466, 470, 475, 477, 488-491, 493-498, 500, 504, 512, 514, 609-610, 613-614, 620-621, 631-632, 679, 684, 687-688, 693-695, 698-699, 701, 703, 706-708, 712-714, 718-719, 721-726, 730-731, 735, 738, 740-741, 746, 749-753, 800, 1033-1034, 1037, 1061, 1064, 1068-1071

- [ ] **Step 2: Write tests for response functions**

Create `test/unit/test_response_boost_coverage.cpp` with tests for all public response API functions, focusing on null inputs, boundary values, and error paths.

- [ ] **Step 3: Build and run**

```bash
make build && cd build && ./dist/bin/test_response_boost_coverage
```

- [ ] **Step 4: Commit**

```bash
git add test/unit/test_response_boost_coverage.cpp
git commit -m "test(response): add coverage boost tests for response module"
```

---

### Task 3: Boost uvhttp_router.c coverage (61% → 80%+)

**Files:**
- Create: `test/unit/test_router_boost_coverage.cpp`

**Strategy:** Router has both trie and array routing modes. Focus on:
- Trie node creation, matching, parameter extraction
- Array routing fallback
- Edge cases: empty routes, max routes, invalid paths
- Route matching with parameters (`:id`, `*wildcard`)

- [ ] **Step 1: Read uncovered lines**

Read `src/uvhttp_router.c` lines: 113-116, 121-122, 125-126, 140-144, 184-187, 189, 191-195, 223-224, 235-237, 243-246, 266-267, 279-282, 285-286, 327, 330-333, 335-339, 342-346, 349, 353-355, 404, 424-430, 513, 533, 536-542, 546-553, 557, 560, 570, 573-577, 591-592, 595-597, 599-600, 602-605, 609-611, 613, 618, 620, 625-626, 641, 663-664, 666-667, 693-697, 731, 734, 739-740, 744-746, 749, 751-752, 754, 760, 762, 766-767, 769

- [ ] **Step 2: Write router tests**

Create `test/unit/test_router_boost_coverage.cpp` with tests for:
- Adding routes with various methods (GET, POST, PUT, DELETE, PATCH)
- Route matching with exact paths
- Route matching with parameters
- Route matching with wildcards
- Trie vs array mode switching
- Max routes boundary
- Invalid path handling

- [ ] **Step 3: Build and run**

```bash
make build && cd build && ./dist/bin/test_router_boost_coverage
```

- [ ] **Step 4: Commit**

```bash
git add test/unit/test_router_boost_coverage.cpp
git commit -m "test(router): add coverage boost tests for router module"
```

---

### Task 4: Boost uvhttp_protocol_upgrade.c coverage (73% → 90%+)

**Files:**
- Create: `test/unit/test_protocol_upgrade_boost_coverage.cpp`

**Strategy:** Protocol upgrade is about WebSocket upgrade validation. Most functions are pure logic (header checking, key generation). Highly testable.

- [ ] **Step 1: Read uncovered lines**

Read `src/uvhttp_protocol_upgrade.c` lines: 139, 171, 178, 184, 187-188, 192-194, 200, 206, 209, 211, 213, 246-249, 263-266, 271, 273

- [ ] **Step 2: Write tests**

Create `test/unit/test_protocol_upgrade_boost_coverage.cpp` with tests for:
- `uvhttp_is_websocket_upgrade()` with various headers
- WebSocket key validation
- Upgrade header parsing
- Edge cases: missing headers, invalid values

- [ ] **Step 3: Build and run**

```bash
make build && cd build && ./dist/bin/test_protocol_upgrade_boost_coverage
```

- [ ] **Step 4: Commit**

```bash
git add test/unit/test_protocol_upgrade_boost_coverage.cpp
git commit -m "test(protocol_upgrade): add coverage boost tests"
```

---

### Task 5: Boost uvhttp_context.c coverage (79% → 90%+)

**Files:**
- Create: `test/unit/test_context_boost_coverage.cpp`

**Strategy:** Context has a few uncovered cleanup paths. Test init/cleanup with various feature flag states.

- [ ] **Step 1: Read uncovered lines**

Read `src/uvhttp_context.c` lines: 132-135, 145-151, 213-216, 225-231

- [ ] **Step 2: Write tests**

Create `test/unit/test_context_boost_coverage.cpp` with tests for:
- TLS init/cleanup paths (conditional on UVHTTP_FEATURE_TLS)
- WebSocket init/cleanup paths (conditional on UVHTTP_FEATURE_WEBSOCKET)
- Config init/cleanup edge cases

- [ ] **Step 3: Build and run**

```bash
make build && cd build && ./dist/bin/test_context_boost_coverage
```

- [ ] **Step 4: Commit**

```bash
git add test/unit/test_context_boost_coverage.cpp
git commit -m "test(context): add coverage boost tests for context module"
```

---

### Task 6: Verify final coverage

- [ ] **Step 1: Rebuild with coverage and measure**

```bash
cd /home/gem/project/uvhttp
cmake -B build_coverage -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build_coverage -j$(nproc)
cd build_coverage && ctest --output-on-failure
gcovr . --root /home/gem/project/uvhttp --filter '../src/' --print-summary
```

Expected: Total coverage ≥ 75%

- [ ] **Step 2: Commit coverage report (if desired)**

```bash
git add -A && git commit -m "test: boost overall coverage from 62% to 75%+"
```

---

## Summary

| Task | Module | Current | Target | File |
|------|--------|---------|--------|------|
| 1 | request | 52% | 80%+ | `test_request_boost_coverage.cpp` |
| 2 | response | 66% | 85%+ | `test_response_boost_coverage.cpp` |
| 3 | router | 61% | 80%+ | `test_router_boost_coverage.cpp` |
| 4 | protocol_upgrade | 73% | 90%+ | `test_protocol_upgrade_boost_coverage.cpp` |
| 5 | context | 79% | 90%+ | `test_context_boost_coverage.cpp` |
| 6 | verification | 62.7% | 75%+ | (none) |
