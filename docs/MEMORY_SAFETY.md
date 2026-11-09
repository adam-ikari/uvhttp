---
title: Memory Safety — ASan & UBSan Verified
description: "UVHTTP's memory-safety guarantee: the full 119-test suite is verified clean under AddressSanitizer and UndefinedBehaviorSanitizer, nightly in CI, reproducible with one command. Bug classes fixed, defense in depth, and why it matters for long-running and embedded deployments."
---

# Memory Safety

UVHTTP does not win on peak single-host throughput; nginx and h2o are faster there. It wins on memory safety in a small, embeddable, 32-bit-capable C library. Production services run for months and embedded devices cannot restart to clear a leak. A slow leak or a rare use-after-free kills both. Most lightweight C HTTP libraries do not test for these bugs at all.

This page is the evidence, not the claim.

## The guarantee

The full 119-test suite is verified clean under **both**:

| Sanitizer | What it catches | Result |
|-----------|-----------------|--------|
| **AddressSanitizer** (with leak detection) | heap use-after-free, heap-buffer-overflow, stack-buffer-overflow, memory leaks | **119/119 pass, zero findings** |
| **UndefinedBehaviorSanitizer** | signed integer overflow, invalid shifts, null dereference, misalignment, out-of-bounds | **119/119 pass, zero findings** |

ASan and UBSan cannot run in the same build, so they are two separate configurations. Both have to pass.

## Reproduce it yourself (one command)

```bash
make verify-memory-safety
```

This configures and builds the suite under ASan and UBSan, runs the full
119-test suite under each, and exits non-zero if any test fails or any sanitizer
reports a finding. A clean run prints:

```
==> PASS: full suite clean under ASan and UBSan (119/119 each).
```

The equivalent manual commands:

```bash
# AddressSanitizer (leaks, use-after-free, overflows)
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
(cd build_asan && ctest --output-on-failure)

# UndefinedBehaviorSanitizer
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc)
(cd build_ubsan && ctest --output-on-failure)
```

Sanitizer builds keep debug symbols and never pass `-s`, so a finding produces a stack trace with file and line.

## Continuous verification

Memory safety is an invariant that has to keep holding, so the checks sit in the release and nightly pipelines:

> **Coverage**: 86% line / 99% function coverage on project code (excl. deps/).

- **PR CI** (`.github/workflows/ci-pr.yml`) builds and lints every change bound for `main`, but it does not run the test suite.
- **Pre-release gate** (ci-release-gate.yml): the `asan-gate` job builds with AddressSanitizer and runs the full suite. A finding blocks the release.
- **Nightly CI** (`.github/workflows/ci-nightly.yml`): the `test-ubsan` job runs the full suite under UBSan every night. `test-memory` was removed because it duplicated the ASan job in the release gate.
- **Nightly fuzzing** (`.github/workflows/ci-fuzz.yml`): a libFuzzer harness
  (`test/fuzz/fuzz_router.c`) drives the router with millions of arbitrary
  inputs under ASan. It reaches states the fixed test inputs never do, and it
  has already found one bug (below).
- **Build-system guard**: sanitizer and Debug builds are never stripped, so CI
  findings stay debuggable.

## Fuzzing

Sanitizers cover the paths the test suite exercises. Fuzzing covers the rest. UVHTTP ships a libFuzzer harness for the router
(`test/fuzz/fuzz_router.c`), run nightly in CI.

```bash
# Build (clang + libFuzzer + ASan)
cmake -B build_fuzz -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON \
  -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
cmake --build build_fuzz -j$(nproc) --target uvhttp
clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
  -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
  -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
  test/fuzz/fuzz_router.c build_fuzz/dist/lib/libuvhttp.a \
  -Wl,--start-group \
  deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
  deps/mbedtls/build/library/libmbedtls.a deps/mbedtls/build/library/libmbedx509.a \
  deps/mbedtls/build/library/libmbedcrypto.a deps/libuv/build/libuv.a \
  -Wl,--end-group -lpthread -lm -ldl -o fuzz_router

# Run
./fuzz_router -max_total_time=60 -max_len=256
```

A clean run exits 0 and prints no `ERROR:` line. The harness found the `add_route_method` heap-buffer-overflow in the table below: a post-migration non-parameter route registration that the unit tests never performed in that order. That is what fuzzing is for.

## Bug classes fixed to reach this guarantee

The suite did not start clean. Reaching zero findings meant fixing real defects in library code, bugs that passed the functional tests and then corrupted memory under a sanitizer. Representative fixes are below; the full list is in the [Changelog](./guide/CHANGELOG.md).

| Bug class | Location | Detail |
|-----------|----------|--------|
| heap-buffer-overflow | `uvhttp_router.c` `add_route_method` | **Found by libFuzzer.** After a parameter route triggered trie migration (resetting `array_capacity` to 0), a later non-parameter route fell through to `add_array_route`, whose `0*2` capacity computed a zero-size realloc that `strncpy` overflowed. The normal test suite never hit this route-registration order. |
| heap-use-after-free | `uvhttp_router.c` `find_or_create_child` | cached pointer into node pool dangled after `realloc` grew the pool |
| heap-use-after-free | `uvhttp_server.c` `uvhttp_server_free` | `freed` flag read from already-freed memory on double-free |
| heap-use-after-free | `uvhttp_connection.c` | post-close read; `close_pending` accounting broken on re-entry |
| heap-buffer-overflow | `uvhttp_response.c` `build_response_headers` | `snprintf` return accumulation let `pos` exceed the buffer, underflowing the bound |
| stack-buffer-underflow | `uvhttp_websocket.c` `uvhttp_ws_close` | write to out-of-scope stack connection |
| memory leak | `uvhttp_connection.c` `restart_read` | request body pointer nulled without freeing |
| memory leak | `uvhttp_server.c` `on_connection` 503 path | `temp_client` (`uv_tcp_t`) never closed/freed |
| memory leak | `uvhttp_server.c` `ws_disable_connection_management` | manager struct (with embedded `uv_timer_t`s) freed before close callbacks fired |
| memory leak | `uvhttp_error_helpers.c` | `uv_strerror` on non-libuv errno triggered an unfreeable libuv `uv__strdup` |
| undefined behavior | `uvhttp_connection.c` `switch_to_websocket` | re-entry clobbered CLOSING state, breaking close accounting (caught as a UBSan-time hang) |

Each one was traced to its cause and fixed there, not masked.

## Why this matters (and why most lightweight C libraries lack it)

A C HTTP library can pass its tests and still leak a few kilobytes per connection, dereference freed memory on a rare close race, or build an unterminated string into a response. No functional test fails. The damage shows up later as:

- **Slow RSS growth** over days or weeks, on devices that cannot restart.
- **Intermittent crashes** under load patterns the test suite never produced.
- **Memory corruption reachable from the network**, through buffer overflows in header or body parsing.

Sanitizers catch these before deployment. Most lightweight C HTTP libraries do not advertise sanitizer-clean status because they never ran the check, not because their code is clean. UVHTTP runs it in the release gate and every night, and treats a finding as a release blocker.

## Defense in depth

Beyond sanitizers, the codebase applies:

- **`-Werror`** with `-Wall -Wextra -Wformat=2 -Wformat-security`: zero
  warnings compile gate.
- **`-fstack-protector-strong`**, `-fno-common`, full RELRO (`-Wl,-z,relro,now`).
- **HTTP response-splitting guards** (`contains_control_chars`) on all header
  values before emission.
- **Input validation**: URL length/path-traversal, header size/format, body size
  limits.
- **Idempotent resource cleanup**: `*_cleanup`/`*_free_resources` paths null
  pointers after freeing and guard on a `freed` flag, so a double free at the
  API boundary does not read freed memory.

## When a sanitizer finding is reported

1. Reproduce with `make verify-memory-safety`, or with the specific sanitizer build.
2. Read the source-level stack trace; sanitizer builds are unstripped.
3. Fix the cause. Do not suppress.
4. Confirm both ASan and UBSan are green before merging.

Report security-relevant findings through the process in
[SECURITY.md](./guide/SECURITY.md).
