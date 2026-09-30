# UVHTTP Fuzz Targets

libFuzzer harnesses for finding memory-safety bugs that the fixed unit-test
inputs do not reach. Each harness links the UVHTTP library under
AddressSanitizer and feeds it arbitrary bytes via libFuzzer.

## Targets

| Harness | Fuzzes | What it catches |
|---------|--------|-----------------|
| `fuzz_router.c` | `uvhttp_router_find_handler` / `uvhttp_router_match` / `uvhttp_parse_path_params` over arbitrary path strings | router trie traversal, parameter extraction, path-parsing overflows/UAF |
| `fuzz_request.c` | the llhttp → uvhttp request-parse callback chain (`on_url` / `on_header_field` / `on_header_value` / `on_body`) over arbitrary byte sequences | URL accumulation, header extraction, body buffering — chunk-boundary and overflow edge cases |
| `fuzz_websocket.c` | `uvhttp_ws_parse_frame_header` (RFC 6455 frame-header decode) + `uvhttp_ws_apply_mask` (mask XOR) over arbitrary byte sequences | frame-header bitfield packing, 16/64-bit extended length, RFC 6455 §5.2 top-bit rejection, mask index-wrap |


## Build & run

```bash
# 1. Build the UVHTTP library with ASan instrumentation (fuzzer-no-link so the
#    harness supplies the fuzzer main).
cmake -B build_fuzz -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON \
  -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
cmake --build build_fuzz -j$(nproc) --target uvhttp

# 2. Link each harness with libFuzzer + ASan (same library + deps).
clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
  -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
  -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
  test/fuzz/fuzz_router.c build_fuzz/dist/lib/libuvhttp.a \
  -Wl,--start-group \
  deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
  deps/mbedtls/build/library/libmbedtls.a deps/mbedtls/build/library/libmbedx509.a \
  deps/mbedtls/build/library/libmbedcrypto.a deps/libuv/build/libuv.a \
  -Wl,--end-group -lpthread -lm -ldl -o build_fuzz/fuzz_router

clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
  -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
  -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
  test/fuzz/fuzz_request.c build_fuzz/dist/lib/libuvhttp.a \
  -Wl,--start-group \
  deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
  deps/mbedtls/build/library/libmbedtls.a deps/mbedtls/build/library/libmbedx509.a \
  deps/mbedtls/build/library/libmbedcrypto.a deps/libuv/build/libuv.a \
  -Wl,--end-group -lpthread -lm -ldl -o build_fuzz/fuzz_request

clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
  -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
  -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
  test/fuzz/fuzz_websocket.c build_fuzz/dist/lib/libuvhttp.a \
  -Wl,--start-group \
  deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
  deps/mbedtls/build/library/libmbedtls.a deps/mbedtls/build/library/libmbedx509.a \
  deps/mbedtls/build/library/libmbedcrypto.a deps/libuv/build/libuv.a \
  -Wl,--end-group -lpthread -lm -ldl -o build_fuzz/fuzz_websocket

# 3. Run (60s by default).
./build_fuzz/fuzz_router -max_total_time=60 -max_len=256
./build_fuzz/fuzz_request -max_total_time=60 -max_len=4096
./build_fuzz/fuzz_websocket -max_total_time=60 -max_len=256

A clean run exits 0 with no `ERROR:` line. libFuzzer writes any crashing input
to `crash-*`; reproduce with `./fuzz_router crash-<hash>`.

## CI

Fuzzing runs nightly in `.github/workflows/ci-fuzz.yml` (clang on
`ubuntu-latest`, 60s per target, crash artifacts uploaded on failure).

## Adding a new target

1. Write `test/fuzz/fuzz_<area>.c` with an `int LLVMFuzzerTestOneInput(const
   uint8_t* data, size_t size)` entry point that null-terminates the input and
   feeds it to a UVHTTP API that takes strings/buffers (not raw sockets).
2. Add a build+run step to `.github/workflows/ci-fuzz.yml`.

Guidance: fuzz parsing/matching surfaces (router, request parsing, WebSocket
frame decode) — anything that consumes attacker-controlled bytes.
