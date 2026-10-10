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

#include <arpa/inet.h>
#include <sys/socket.h>

namespace {

int ok_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return 0;
}

/* Distinct handlers, so "the route landed" can be checked per path instead
 * of just "some handler is somewhere in the table". */
int health_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return 0;
}

int submit_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
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

/* A listening server is not evidence that its routes landed: a regression
 * that swallowed add_route results would still produce a listening server
 * here whose every path 404s, and every assertion above would still pass.
 * Same bug class as the router migration that silently dropped every
 * registered route - pin the route table itself. */
TEST(UvhttpServerAtomicConstruct, RoutesAreActuallyInstalledInTheRouter) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/health", UVHTTP_GET, health_handler},
        {"/submit", UVHTTP_POST, submit_handler},
    };

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_listen_routes(loop, routes, 2, "127.0.0.1", 0,
                                          &server),
              UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    ASSERT_NE(server->router, nullptr);

    EXPECT_EQ(uvhttp_router_find_handler(server->router, "/health", "GET"),
              health_handler)
        << "the route that was just registered must resolve to its handler";
    EXPECT_EQ(uvhttp_router_find_handler(server->router, "/submit", "POST"),
              submit_handler);
    /* A method mismatch must not resolve - guards against routes that match
     * every method by accident. */
    EXPECT_EQ(uvhttp_router_find_handler(server->router, "/health", "POST"),
              nullptr);

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

/* An out-of-range method registers a route that can never match: every
 * lookup is an equality test against the method enum. That is a silently
 * dead route, so it must be rejected instead of built into the table. */
TEST(UvhttpServerAtomicConstruct, OutOfRangeMethodIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    const uvhttp_route_t routes[] = {
        {"/", (uvhttp_method_t)99, ok_handler},
    };

    uvhttp_server_t* server = nullptr;
    EXPECT_EQ(uvhttp_server_listen_routes(loop, routes, 1, "127.0.0.1", 0,
                                          &server),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(server, nullptr);
}

/* Deterministic bind failure: occupy an ephemeral port with our own
 * listening socket first, then hand the same port to listen_routes - it must
 * fail with EADDRINUSE no matter who runs the test. (The previous version
 * bound privileged port 1 instead, which succeeds under root: the test then
 * GTEST_SKIPs and this exact path - the one the embedding example got wrong -
 * goes unverified.) The server already owned the router at this point, so the
 * cleanup must be server_free ALONE; under ASan a double free here is the
 * regression signal. */
TEST(UvhttpServerAtomicConstruct, ListenFailureLeavesNothingBehind) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uv_tcp_t blocker;
    ASSERT_EQ(uv_tcp_init(loop, &blocker), 0);
    struct sockaddr_in bind_addr;
    ASSERT_EQ(uv_ip4_addr("127.0.0.1", 0, &bind_addr), 0);
    ASSERT_EQ(uv_tcp_bind(&blocker, (const struct sockaddr*)&bind_addr, 0), 0);
    ASSERT_EQ(uv_listen((uv_stream_t*)&blocker, 1,
                        [](uv_stream_t*, int) {}),
              0);

    struct sockaddr_in bound_addr;
    int addr_len = sizeof(bound_addr);
    ASSERT_EQ(uv_tcp_getsockname(&blocker, (struct sockaddr*)&bound_addr,
                                 &addr_len),
              0);
    const int occupied_port = ntohs(bound_addr.sin_port);
    ASSERT_GT(occupied_port, 0);

    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, ok_handler},
    };

    uvhttp_server_t* server = nullptr;
    const uvhttp_error_t err = uvhttp_server_listen_routes(
        loop, routes, 1, "127.0.0.1", occupied_port, &server);

    EXPECT_NE(err, UVHTTP_OK) << "port " << occupied_port
                              << " is occupied; listen must fail";
    EXPECT_EQ(server, nullptr)
        << "out param must be NULL after a failed bind; the router the "
           "server had already taken ownership of must not be freed twice";

    uv_close((uv_handle_t*)&blocker, nullptr);
    uv_run(loop, UV_RUN_NOWAIT);
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
    EXPECT_EQ(uvhttp_server_take_router(server, router), UVHTTP_OK)
        << "re-taking the same router is a no-op, not an error";
    EXPECT_EQ(server->router, router)
        << "the no-op must leave the owned router in place";

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

/* ---------- ownership: NULL is not a detach ----------
 *
 * Before v2.10, take_router / take_context were "set_*" and passing NULL was
 * documented as clearing the field. That only ever worked when the server
 * held nothing: once an object is taken, the reject-if-different guard turns
 * NULL into an error too. There is no detach operation - a taken object
 * stays owned by the server until uvhttp_server_free. These tests pin that
 * (the misinterpretation is what made callers free a router the server still
 * owns). */

TEST(UvhttpServerOwnership, NullRouterAfterTakeIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_router_t* router = nullptr;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_take_router(server, router), UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_take_router(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM)
        << "there is no detach: a taken router stays owned by the server";
    EXPECT_EQ(server->router, router)
        << "the rejected NULL must leave the owned router in place";

    uvhttp_server_free(server);
}

TEST(UvhttpServerOwnership, NullRouterWithoutRouterIsNoOp) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_take_router(server, nullptr), UVHTTP_OK)
        << "NULL is only a no-op when the server holds no router";
    EXPECT_EQ(server->router, nullptr);

    uvhttp_server_free(server);
}

TEST(UvhttpServerOwnership, NullContextAfterTakeIsRejected) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_context_t* context = nullptr;
    ASSERT_EQ(uvhttp_context_create(loop, &context), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_take_context(server, context), UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_take_context(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM)
        << "there is no detach: a taken context stays owned by the server";
    EXPECT_EQ(server->context, context);

    uvhttp_server_free(server);
}

TEST(UvhttpServerOwnership, NullContextWithoutContextIsNoOp) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_take_context(server, nullptr), UVHTTP_OK);
    EXPECT_EQ(server->context, nullptr);

    uvhttp_server_free(server);
}
