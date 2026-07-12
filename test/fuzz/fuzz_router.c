/*
 * libFuzzer harness for UVHTTP router path matching.
 *
 * Fuzzes uvhttp_router_find_handler / uvhttp_router_match over arbitrary path
 * strings — exercises route trie traversal, parameter extraction, and path
 * parsing. Catches memory-safety bugs in the router that the unit tests'
 * fixed inputs do not reach.
 *
 * Build (clang + libFuzzer + ASan):
 *   clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
 *     -Iinclude -Ideps/llhttp/include \
 *     test/fuzz/fuzz_router.c src/uvhttp_router.c src/uvhttp_router_cache.c \
 *     src/uvhttp_utils.c src/uvhttp_error.c deps/xxhash/xxhash.c \
 *     -o fuzz_router
 *
 * Run:
 *   ./fuzz_router -max_total_time=60 -max_len=256
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "uvhttp_router.h"
#include "uvhttp_request.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"

/* A handler that does nothing; we only care that lookup returns without
 * corrupting memory. */
static int noop_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    (void)res;
    return 0;
}

/* Build a router with a representative set of routes (static, param, nested,
 * wildcard) so the fuzzed path exercises multiple trie branches. */
static uvhttp_router_t* build_router(void) {
    uvhttp_router_t* r = NULL;
    if (uvhttp_router_new(&r) != UVHTTP_OK || !r) {
        return NULL;
    }
    uvhttp_router_add_route(r, "/", noop_handler);
    uvhttp_router_add_route(r, "/users", noop_handler);
    uvhttp_router_add_route(r, "/users/:id", noop_handler);
    uvhttp_router_add_route(r, "/users/:id/posts", noop_handler);
    uvhttp_router_add_route(r, "/users/:id/posts/:post_id", noop_handler);
    uvhttp_router_add_route(r, "/api/v1/health", noop_handler);
    uvhttp_router_add_route(r, "/api/v1/items/:item_id", noop_handler);
    uvhttp_router_add_route(r, "/static/*", noop_handler);
    uvhttp_router_add_route(r, "/a/b/c/d/e/f/g", noop_handler);
    uvhttp_router_add_route(r, "/:category/:subcategory", noop_handler);
    return r;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    /* Null-terminate the fuzz input so it can be treated as a path string. */
    char* path = (char*)malloc(size + 1);
    if (!path) {
        return 0;
    }
    memcpy(path, data, size);
    path[size] = '\0';

    static uvhttp_router_t* router = NULL;
    if (!router) {
        router = build_router();
        if (!router) {
            free(path);
            return 0;
        }
    }

    /* Exercise the lookup + match paths. These must not read past the
     * null terminator, leak, or corrupt memory for any input. */
    (void)uvhttp_router_find_handler(router, path, "GET");

    uvhttp_route_match_t match;
    (void)uvhttp_router_match(router, path, "GET", &match);

    uvhttp_param_t params[16];
    size_t param_count = 0;
    (void)uvhttp_parse_path_params(path, params, &param_count);

    free(path);
    return 0;
}
