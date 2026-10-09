/* Atomicity tests for uvhttp_server_listen_routes and the ownership
 * contract of uvhttp_server_take_router / uvhttp_server_take_context.
 *
 * Background: before v2.10, starting a server took five separate calls
 * (server_new -> router_new -> add_route xN -> set_router -> listen) and the
 * caller had to hand-write the rollback for each failure. The official
 * embedding example got that rollback wrong: on listen failure it called
 * uvhttp_router_free(router) and then uvhttp_server_free(server), which frees
 * the router again - a double free (reproduced under ASan).
 *
 * These tests pin down the properties that fix is supposed to guarantee:
 *   - all-or-nothing construction (no partial state on any failure path)
 *   - *server is NULL on every failure
 *   - ownership transfer is total and cannot be double-applied
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp.h"
#include "uvhttp_context.h"
}

namespace {

int ok_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return 0;
}

}  // namespace

/* ---------- success path ---------- */

TEST(UvhttpServerAtomicConstruct, BuildsListeningServerWithRoutes) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, ok_handler},
        {"/health", UVHTTP_GET, ok_handler},
        {"/submit", UVHTTP_POST, ok_handler},
    };

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_listen_routes(loop, routes, 3, "127.0.0.1", 0,
                                          &server),
              UVHTTP_OK)
        << "atomic construction should succeed";
    ASSERT_NE(server, nullptr);

    uvhttp_server_free(server);
}

/* A route table of zero entries is legal: a server that serves via a handler
 * or a router handed over later. */
TEST(UvhttpServerAtomicConstruct, ZeroRoutesIsAllowed) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_listen_routes(loop, nullptr, 0, "127.0.0.1", 0,
                                          &server),
              UVHTTP_OK);
    ASSERT_NE(server, nullptr);

    uvhttp_server_free(server);
}

/* ---------- failure paths: *server must be NULL, nothing allocated ---------- */

TEST(UvhttpServerAtomicConstruct, NullOutParamIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    EXPECT_EQ(uvhttp_server_listen_routes(loop, nullptr, 0, "127.0.0.1", 0,
                                          nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpServerAtomicConstruct, NullLoopIsRejected) {
    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(nullptr, nullptr, 0, "127.0.0.1", 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr) << "out param must stay NULL on failure";
}

TEST(UvhttpServerAtomicConstruct, NullHostIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(loop, nullptr, 0, nullptr, 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr);
}

/* count > 0 with a NULL table is a caller bug; must not dereference. */
TEST(UvhttpServerAtomicConstruct, NullRoutesWithNonZeroCountIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(loop, nullptr, 3, "127.0.0.1", 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr);
}

/* A malformed entry (NULL path or NULL handler) must abort construction
 * cleanly - this is the case the five-step version made the caller handle. */
TEST(UvhttpServerAtomicConstruct, NullPathInRouteTableIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, ok_handler},
        {nullptr, UVHTTP_ANY, ok_handler},
    };

    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(loop, routes, 2, "127.0.0.1", 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr)
        << "a bad entry must abort the whole construction, not half-apply it";
}

TEST(UvhttpServerAtomicConstruct, NullHandlerInRouteTableIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, nullptr},
    };

    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(loop, routes, 1, "127.0.0.1", 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr);
}

/* Port 1 is privileged, so bind fails. This is the exact path the embedding
 * example got wrong: the server already owned the router at this point, so
 * the cleanup must be server_free ALONE. Under ASan/valgrind a double free
 * here is the regression signal. */
TEST(UvhttpServerAtomicConstruct, ListenFailureLeavesNothingBehind) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, ok_handler},
    };

    uvhttp_server_t* server = nullptr;
    const uvhttp_error_t err =
        uvhttp_server_listen_routes(loop, routes, 1, "127.0.0.1", 1, &server);

    /* Binding a privileged port must fail. If it somehow succeeds, the test
     * would leak the server, so free it and skip rather than assert. */
    if (err == UVHTTP_OK) {
        uvhttp_server_free(server);
        GTEST_SKIP() << "running as root: privileged port bind succeeded";
    }

    EXPECT_EQ(server, nullptr)
        << "out param must be NULL after a failed bind; the router the "
           "server had already taken ownership of must not be freed twice";
}

/* ---------- ownership: take_router ---------- */

TEST(UvhttpServerOwnership, TakeRouterRejectsNullServer) {
    EXPECT_EQ(uvhttp_server_take_router(nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpServerOwnership, TakeContextRejectsNullServer) {
    EXPECT_EQ(uvhttp_server_take_context(nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

/* Handing over a second, different router would leak the first one - the old
 * implementation silently overwrote the pointer. Now it is rejected. */
TEST(UvhttpServerOwnership, SecondRouterIsRejectedNotSilentlyDropped) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_router_t* first = nullptr;
    ASSERT_EQ(uvhttp_router_new(&first), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_take_router(server, first), UVHTTP_OK);

    uvhttp_router_t* second = nullptr;
    ASSERT_EQ(uvhttp_router_new(&second), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_take_router(server, second),
              UVHTTP_ERROR_INVALID_PARAM)
        << "replacing an owned router must be refused, not silently leak it";

    /* The rejected router is still ours to free; the server frees its own. */
    uvhttp_router_free(second);
    uvhttp_server_free(server);
}

/* Re-taking the SAME router is a no-op, not an error: it keeps idempotent
 * teardown paths (e.g. a helper that hands over a router it already gave
 * away) working. */
TEST(UvhttpServerOwnership, RetakingSameRouterIsIdempotent) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_router_t* router = nullptr;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_EQ(uvhttp_router_add_route(router, "/", ok_handler), UVHTTP_OK);

    ASSERT_EQ(uvhttp_server_take_router(server, router), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_take_router(server, router), UVHTTP_OK);

    uvhttp_server_free(server);
}

TEST(UvhttpServerOwnership, SecondContextIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_context_t* first = nullptr;
    ASSERT_EQ(uvhttp_context_create(loop, &first), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_take_context(server, first), UVHTTP_OK);

    uvhttp_context_t* second = nullptr;
    ASSERT_EQ(uvhttp_context_create(loop, &second), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_take_context(server, second),
              UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_context_destroy(second);
    uvhttp_server_free(server);
}
