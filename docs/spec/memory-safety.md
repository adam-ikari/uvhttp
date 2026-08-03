# Memory Safety Spec

## Overview

UVHTTP guarantees memory safety for long-running production services and
embedded devices. This spec defines the memory safety criteria that all code
must satisfy before being merged.

## Sanitizer Gates

### AddressSanitizer (ASan)
- **Requirement**: The full test suite (101 tests) must pass with zero findings under ASan with leak detection enabled.
- **Build**: `cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON`
- **Run**: `cd build_asan && ctest --output-on-failure`
- **CI gate**: `.github/workflows/ci-pr.yml` — every PR must pass ASan

### UndefinedBehaviorSanitizer (UBSan)
- **Requirement**: The full test suite must pass with zero findings under UBSan.
- **Build**: `cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON`
- **Run**: `cd build_ubsan && ctest --output-on-failure`
- **CI gate**: `.github/workflows/ci-nightly.yml` — nightly run

### Why separate builds
ASan and UBSan cannot be combined in a single build. Both must be green.

## Bug Classes — Zero Tolerance

The following bug classes are explicitly prohibited. The CHANGELOG documents
all fixes applied to reach zero findings.

| Bug Class | Detection | Prevention |
|-----------|-----------|------------|
| Heap use-after-free | ASan | NULL pointers after free, freed flag |
| Heap buffer overflow | ASan | Bounds checking, snprintf, safe string ops |
| Stack buffer overflow | ASan | Fixed-size buffers, snprintf |
| Stack buffer underflow | ASan | Array index validation |
| Memory leak | ASan (leak detection) | Alloc/free pairing, cleanup functions |
| Double free | ASan | freed flag, NULL checks |
| Signed integer overflow | UBSan | Bounds checking |
| Invalid shift | UBSan | Platform-specific shift handling (32-bit) |
| Null pointer dereference | UBSan | NULL checks on all public API params |
| Misalignment | UBSan | Static assertions on struct layout |

## Reproduce Command

```bash
make verify-memory-safety
```

This builds and runs both ASan and UBSan configurations. A clean run prints:
```
==> PASS: full suite clean under ASan and UBSan (101/101 each).
```

## Defense in Depth

Beyond sanitizers, the codebase applies:

1. **Compile-time hardening**:
   - `-Werror` with `-Wall -Wextra -Wformat=2 -Wformat-security`
   - `-fstack-protector-strong`, `-fno-common`
   - Full RELRO (`-Wl,-z,relro,now`)
   - `_FORTIFY_SOURCE=2`

2. **Safe string operations**: No `strcpy` or `sprintf`. All string operations
   use `snprintf` or `uvhttp_safe_strcpy` (which wraps snprintf).

3. **Input validation**: URL length, path traversal, header size/format, body
   size limits are checked before any buffer operation.

4. **Response splitting guards**: All header values are checked for control
   characters before emission.

5. **Idempotent cleanup**: All `*_cleanup` / `*_free_resources` paths NULL
   pointers after freeing and guard on a `freed` flag.

## Fuzzing

The router module is fuzzed nightly with libFuzzer + ASan:

```bash
./fuzz_router -max_total_time=60 -max_len=256
```

The fuzzer already found one real bug (heap-buffer-overflow in
`add_route_method`) that the unit tests never hit.

## Leak Policy

- **Zero tolerance**: Any leak detected by ASan is a release blocker
- **False positives**: ASan false positives (e.g., stack frame reuse) must be
  documented with a `no_sanitize("address")` attribute and a comment
  explaining the false positive
- **Leak detection**: ASan is configured with `detect_leaks=1`