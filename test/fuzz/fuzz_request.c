/*
 * libFuzzer harness for UVHTTP HTTP request parsing.
 *
 * Fuzzes uvhttp_request_parse over arbitrary byte sequences — exercises
 * llhttp parser, header extraction, method/path/version parsing, and
 * connection state management. Catches memory-safety bugs in request
 * parsing that the unit tests' fixed inputs do not reach.
 *
 * Build (clang + libFuzzer + ASan):
 *   clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
 *     -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
 *     test/fuzz/fuzz_request.c src/uvhttp_request.c src/uvhttp_error.c \
 *     src/uvhttp_utils.c deps/llhttp/build/libllhttp.a \
 *     -o fuzz_request
 *
 * Run:
 *   ./fuzz_request -max_total_time=60 -max_len=4096
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "uvhttp_request.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    /* Null-terminate the fuzz input so it can be treated as raw HTTP. */
    char* input = (char*)malloc(size + 1);
    if (!input) {
        return 0;
    }
    memcpy(input, data, size);
    input[size] = '\0';

    /* Create a request object and feed the fuzzed input through the parser.
     * The parser must not read past the null terminator, leak, or corrupt
     * memory for any input. */
    uvhttp_request_t* req = NULL;

    /* We need a connection object for the parser. Create one on the stack. */
    uvhttp_connection_t conn;
    memset(&conn, 0, sizeof(conn));
    conn.read_buffer = input;
    conn.read_buffer_size = size;
    conn.read_buffer_used = size;

    /* Allocate a request object */
    req = (uvhttp_request_t*)malloc(sizeof(uvhttp_request_t));
    if (!req) {
        free(input);
        return 0;
    }
    memset(req, 0, sizeof(uvhttp_request_t));
    req->conn = &conn;

    /* Feed the data through the parsers multiple times with different offsets
     * to exercise incremental parsing paths. */
    size_t offset = 0;
    while (offset < size) {
        size_t chunk = (size - offset > 64) ? 64 : (size - offset);
        uvhttp_request_parse(req, input + offset, chunk);
        offset += chunk;
        /* After each chunk, check if parsing is complete */
        if (req->state >= UVHTTP_REQUEST_STATE_DONE) {
            break;
        }
    }

    /* Cleanup */
    if (req) {
        free(req);
    }
    free(input);

    return 0;
}