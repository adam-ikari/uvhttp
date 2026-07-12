# Memory Safety

UVHTTP's headline differentiator is not peak single-host throughput (nginx and
h2o win there) — it is **verified memory safety in a lightweight, embeddable,
32-bit-capable C library**. For long-running production services and embedded
devices that cannot tolerate a slow memory leak or a use-after-free after weeks
of uptime, this is the property that matters, and it is the one most comparable
lightweight C HTTP libraries do not provide.

This document is the engineering evidence.

## The guarantee

The full 91-test suite is verified clean under **both**:

| Sanitizer | What it catches | Result |
|-----------|-----------------|--------|
| **AddressSanitizer** (with leak detection) | heap use-after-free, heap-buffer-overflow, stack-buffer-overflow, memory leaks | **91/91 pass, zero findings** |
| **UndefinedBehaviorSanitizer** | signed integer overflow, invalid shifts, null dereference, misalignment, out-of-bounds | **91/91 pass, zero findings** |

ASan and UBSan cannot be combined in a single build, so they run as two separate
configurations. Both must be green.

## Reproduce it yourself (one command)

```bash
make verify-memory-safety
```

This configures and builds the suite under ASan and UBSan, runs the full
91-test suite under each, and exits non-zero if any test fails or any sanitizer
reports a finding. A clean run prints:

```
==> PASS: full suite clean under ASan and UBSan (91/91 each).
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

Sanitizer builds keep debug symbols (no `-s` strip) so any finding produces a
resolvable, source-level stack trace.

## Continuous verification

Memory safety is a **non-regressing invariant**, not a one-time check:

- **Nightly CI** (`.github/workflows/ci-nightly.yml`): the `test-memory` (ASan
  with leak detection) and `test-ubsan` jobs run the full suite under each
  sanitizer every night.
- **Build-system guard**: sanitizer/Debug builds are never stripped, so CI
  findings are always debuggable.

## Bug classes fixed to reach this guarantee

The suite did not start clean. Reaching zero sanitizer findings required fixing
genuine memory-safety defects in production library code — bugs that passed the
normal test suite but corrupted memory under sanitizers. Representative fixes
(full list in the [Changelog](./guide/CHANGELOG.md)):

| Bug class | Location | Detail |
|-----------|----------|--------|
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

Each was root-caused (not masked) and fixed at the source.

## Why this matters (and why most lightweight C libraries lack it)

A C HTTP library that "passes its tests" can still leak a few kilobytes per
connection, dereference freed memory on a rare close race, or build an
unterminated string into a response. None of these fail a functional test. They
manifest as:

- **Slow RSS growth** over days/weeks of uptime (embedded devices cannot restart).
- **Intermittent crashes** under production load patterns the test suite never hit.
- **Security-reachable corruption** (buffer overflows in header/body parsing).

Sanitizers are how you find these *before* deployment. The reason most
lightweight C HTTP libraries do not advertise sanitizer-clean status is not that
they are clean — it is that they have not run the check. UVHTTP runs it on every
nightly, and treats any finding as a release blocker.

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
  pointers after freeing and guard on a `freed` flag to be robust against
  double-free at the API boundary.

## When a sanitizer finding is reported

1. Reproduce with `make verify-memory-safety` (or the specific sanitizer build).
2. Root-cause from the source-level stack trace (sanitizer builds are unstripped).
3. Fix at the source — do not suppress.
4. Confirm both ASan and UBSan are green before merging.

Report security-relevant findings via the process in
[SECURITY.md](./guide/SECURITY.md).
