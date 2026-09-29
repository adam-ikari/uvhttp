/*
 * libFuzzer harness for UVHTTP HTTP request parsing.
 *
 * Feeds arbitrary byte sequences through the full llhttp → uvhttp callback
 * chain (on_url, on_header_field, on_header_value, on_body,
 * on_message_complete) that the server uses to parse incoming requests.
 * These callbacks own the URL accumulation, header extraction, and body
 * buffering logic — the unit tests' fixed inputs do not reach the
 * chunk-boundary and overflow edge cases that a fuzzer explores.
 *
 * The harness builds a minimal request + connection in-process (no socket,
 * no event loop run) so each fuzzer iteration is a single llhttp_execute
 * call, keeping throughput high.
 *
 * Build (clang + libFuzzer + ASan):
 *   clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
 *     -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
 *     -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
 *     test/fuzz/fuzz_request.c \
 *     build_fuzz/dist/lib/libuvhttp.a build_fuzz/dist/lib/libminiz.a \
 *     deps/xxhash/libxxhash.a \
 *     -Wl,--start-group \
 *     deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
 *     deps/mbedtls/build/library/libmbedtls.a \
 *     deps/mbedtls/build/library/libmbedx509.a \
 *     deps/mbedtls/build/library/libmbedcrypto.a \
 *     deps/libuv/build/libuv.a \
 *     -Wl,--end-group -lpthread -lm -ldl -o fuzz_request
 *
 * Run:
 *   ./fuzz_request -max_total_time=60 -max_len=4096
 */
#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_request.h"
#include "uvhttp_server.h"

#include <llhttp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

/* A loop + bare TCP handle that uvhttp_request_init stores as the "client"
 * pointer. The callbacks do not dereference it (they go through conn->request
 * via parser->data), so a never-listened, never-connected socket is enough.
 * Allocated once and reused across iterations to avoid per-input overhead. */
static uv_loop_t* g_loop = NULL;
static uv_tcp_t g_tcp;

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (!g_loop) {
        g_loop = uv_loop_new();
        if (!g_loop) {
            return 0;
        }
        uv_tcp_init(g_loop, &g_tcp);
    }

    uvhttp_request_t* req =
        (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    if (!req) {
        return 0;
    }

    /* uvhttp_request_init zeroes the struct, allocates the llhttp parser +
     * settings, and wires up every callback (on_url, on_header_field, etc). */
    if (uvhttp_request_init(req, &g_tcp) != UVHTTP_OK) {
        uvhttp_free(req);
        return 0;
    }

    /* The callbacks resolve the request via parser->data → conn →
     * conn->request. A zeroed connection on the stack is enough: the callbacks
     * guard NULL and early-return -1, which llhttp treats as a parse error
     * (safe). */
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    conn.request = req;

    req->parser->data = &conn;

    /* Feed the fuzzed bytes through the parser. llhttp_execute invokes the
     * uvhttp callbacks for each URL segment, header field/value, and body
     * chunk it encounters. Any memory-safety bug in those callbacks surfaces
     * under ASan. */
    llhttp_execute(req->parser, (const char*)data, size);

    uvhttp_request_free(req);
    return 0;
}
