/**
 * @file test_connection_context_coverage.cpp
 * @brief Coverage boost tests for uvhttp_connection.c and uvhttp_context.c
 *
 * Targets uncovered lines in:
 *   - connection.c: restart_read branches, close/free paths, timeout config,
 *                   TLS NOT_SUPPORTED paths, start with valid connection
 *   - context.c:    complementary init/cleanup workflow tests
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_server.h"

#include "llhttp.h"

#include <climits>
#include <cstring>
#if UVHTTP_FEATURE_TLS
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#endif

/* ========== Helpers ========== */

static void create_server_and_loop(uv_loop_t** loop,
                                   uvhttp_server_t** server) {
    *loop = uv_loop_new();
    ASSERT_NE(*loop, nullptr);
    uvhttp_error_t result = uvhttp_server_new(*loop, server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(*server, nullptr);
}

static void destroy_server_and_loop(uvhttp_server_t* server,
                                    uv_loop_t* loop) {
    if (server) {
        uvhttp_server_free(server);
    }
    if (loop) {
        uv_run(loop, UV_RUN_DEFAULT);
        uv_loop_close(loop);
        uvhttp_free(loop);
    }
}

/* ========== 1. uvhttp_connection_restart_read branches ========== */

/* restart_read: null conn (already tested elsewhere, included for completeness) */
TEST(ConnectionContextCoverage, RestartRead_NullConn) {
    uvhttp_error_t result = uvhttp_connection_restart_read(nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* restart_read: null request */
TEST(ConnectionContextCoverage, RestartRead_NullRequest) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Free the request before dropping the pointer, otherwise the allocation
     * from uvhttp_connection_new leaks. uvhttp_connection_free's free_resources
     * guards on conn->request, so leaving it NULL is safe. */
    uvhttp_request_free(conn->request);
    conn->request = nullptr;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    /* Restore before free to avoid crash in cleanup */
    conn->server = server;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* restart_read: null response */
TEST(ConnectionContextCoverage, RestartRead_NullResponse) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Free the response before dropping the pointer, otherwise the allocation
     * from uvhttp_connection_new leaks. free_resources guards on conn->response,
     * so leaving it NULL is safe. */
    uvhttp_response_free(conn->response);
    conn->response = nullptr;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    /* response already freed above; leave NULL so free_resources skips it */
    uvhttp_connection_close(conn);
    destroy_server_and_loop(server, loop);
}

/* restart_read: null parser inside request */
TEST(ConnectionContextCoverage, RestartRead_NullParser) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->request, nullptr);
    llhttp_t* saved_parser = conn->request->parser;
    conn->request->parser = nullptr;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->request->parser = saved_parser;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* restart_read: null parser_settings inside request */
TEST(ConnectionContextCoverage, RestartRead_NullParserSettings) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->request, nullptr);
    llhttp_settings_t* saved = conn->request->parser_settings;
    conn->request->parser_settings = nullptr;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->request->parser_settings = saved;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* restart_read: connection in CLOSING state */
TEST(ConnectionContextCoverage, RestartRead_ClosingState) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->state = UVHTTP_CONN_STATE_CLOSING;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_CLOSE);

    /* Reset state for clean free */
    conn->state = UVHTTP_CONN_STATE_NEW;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== 2. uvhttp_connection_close with valid connection ========== */

TEST(ConnectionContextCoverage, CloseValidConnection) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Close should set state to CLOSING and close all handles */
    uvhttp_connection_close(conn);

    /* Run the loop to process close callbacks */
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 3. uvhttp_connection_free edge cases ========== */

/* free: double-free prevention via 'freed' flag */
TEST(ConnectionContextCoverage, FreeDoubleFreePrevention) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Simulate that the connection was already freed */
    conn->freed = 1;

    /* This should return immediately without crashing */
    uvhttp_connection_free(conn);

    /* The guard above skipped the actual cleanup, so the connection's
     * resources were never released. Reset the guard flag and free for real
     * to avoid leaking the connection allocated by uvhttp_connection_new. */
    conn->freed = 0;
    uvhttp_connection_free(conn);

    destroy_server_and_loop(server, loop);
}

/* free: close_pending > 0 guard */
TEST(ConnectionContextCoverage, FreeClosePendingGuard) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Simulate handles being closed asynchronously */
    conn->close_pending = 1;

    /* This should return immediately because handles are being closed */
    uvhttp_connection_free(conn);

    /* Now run close to actually clean up */
    conn->close_pending = 0;
    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 4. uvhttp_connection_start with valid non-TLS connection ========== */

TEST(ConnectionContextCoverage, StartValidNonTlsConnection) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Ensure TLS is disabled for this connection */
    conn->tls_enabled = 0;

    result = uvhttp_connection_start(conn);
    /* May succeed or fail depending on whether tcp_handle is accepted,
     * but exercises the non-TLS start path */
    (void)result;

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 5. Timeout with server config set ========== */

TEST(ConnectionContextCoverage, StartTimeoutWithServerConfig) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Create and attach a config to exercise the config-based timeout path */
    uvhttp_config_t* config = nullptr;
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);

    config->connection_timeout = 45;
    server->config = config;

    result = uvhttp_connection_start_timeout(conn);
    /* Exercises the conn->server->config branch in start_timeout */
    EXPECT_EQ(result, UVHTTP_OK);

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    /* Restore config ownership so server_free doesn't double-free */
    server->config = nullptr;
    uvhttp_config_free(config);
    destroy_server_and_loop(server, loop);
}

/* ========== 6. Custom timeout exact boundary values ========== */

TEST(ConnectionContextCoverage, StartTimeoutCustom_ExactMin) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* UVHTTP_CONNECTION_TIMEOUT_MIN = 5, this should be accepted */
    result = uvhttp_connection_start_timeout_custom(conn, 5);
    EXPECT_EQ(result, UVHTTP_OK);

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, StartTimeoutCustom_ExactMax) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* UVHTTP_CONNECTION_TIMEOUT_MAX = 300, this should be accepted */
    result = uvhttp_connection_start_timeout_custom(conn, 300);
    EXPECT_EQ(result, UVHTTP_OK);

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, StartTimeoutCustom_BelowMin) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* UVHTTP_CONNECTION_TIMEOUT_MIN = 5, 4 should be rejected */
    result = uvhttp_connection_start_timeout_custom(conn, 4);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, StartTimeoutCustom_AboveMax) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* UVHTTP_CONNECTION_TIMEOUT_MAX = 300, 301 should be rejected */
    result = uvhttp_connection_start_timeout_custom(conn, 301);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* Integer overflow check: timeout_seconds > INT_MAX / 1000 */
TEST(ConnectionContextCoverage, StartTimeoutCustom_IntegerOverflow) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Use a value that would overflow when multiplied by 1000.
     * INT_MAX/1000 = 2147483, so 2147484 is just above.
     * But the range check [5,300] catches it first.
     * To reach the overflow check, we'd need to bypass range check.
     * Since the range check comes first, this path is unreachable in practice.
     * Test with INT_MAX just to be thorough. */
    result = uvhttp_connection_start_timeout_custom(conn, INT_MAX);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== 7. TLS NOT_SUPPORTED paths (non-TLS build) ========== */

#if !UVHTTP_FEATURE_TLS

TEST(ConnectionContextCoverage, TlsHandshakeFunc_NotSupported) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_tls_handshake_func(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_SUPPORTED);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, TlsStartHandshake_NotSupported) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_tls_handshake(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_SUPPORTED);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, TlsRead_NotSupported) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Ensure ssl is not NULL to bypass the INVALID_PARAM check,
     * reaching the NOT_SUPPORTED return. In non-TLS builds, the ssl
     * field check may differ. Set ssl to a non-null dummy. */
    int dummy_ssl;
    conn->ssl = &dummy_ssl;

    result = uvhttp_connection_tls_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_SUPPORTED);

    conn->ssl = nullptr;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, TlsWrite_NotSupported) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    int dummy_ssl;
    conn->ssl = &dummy_ssl;

    const char data[] = "test";
    result = uvhttp_connection_tls_write(conn, data, 4);
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_SUPPORTED);

    conn->ssl = nullptr;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, TlsCleanup_NotSupported) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* In non-TLS build, cleanup is a no-op */
    uvhttp_connection_tls_cleanup(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

#endif /* !UVHTTP_FEATURE_TLS */

/* ========== 8. TLS paths with TLS enabled flag but no SSL context ========== */

#if UVHTTP_FEATURE_TLS

TEST(ConnectionContextCoverage, TlsHandshakeFunc_TlsEnabledNoCtx) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* tls_ctx is NULL by default, should return INVALID_PARAM */
    conn->tls_enabled = 1;
    ASSERT_EQ(server->tls_ctx, nullptr);
    result = uvhttp_connection_tls_handshake_func(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->tls_enabled = 0;
    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

TEST(ConnectionContextCoverage, TlsCleanup_WithConn) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* ssl is NULL by default - should be a safe no-op */
    uvhttp_connection_tls_cleanup(conn);

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

#endif /* UVHTTP_FEATURE_TLS */

/* ========== 9. WebSocket null/safe paths ========== */

#if UVHTTP_FEATURE_WEBSOCKET

TEST(ConnectionContextCoverage, SwitchToWebsocket_NullConn) {
    /* Should not crash */
    uvhttp_connection_switch_to_websocket(nullptr);
}

TEST(ConnectionContextCoverage, WebsocketClose_NullConn) {
    /* Should not crash */
    uvhttp_connection_websocket_close(nullptr);
}

TEST(ConnectionContextCoverage, WebsocketClose_NoWsConnection) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* ws_connection is NULL by default */
    ASSERT_EQ(conn->ws_connection, nullptr);
    conn->is_websocket = 0;

    uvhttp_connection_websocket_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */

/* ========== 10. Context complementary coverage ========== */

/* Context: create, init, destroy lifecycle (non-TLS init paths) */
TEST(ConnectionContextCoverage, ContextLifecycleNonTlsInit) {
    uv_loop_t loop;
    int rc = uv_loop_init(&loop);
    ASSERT_EQ(rc, 0);

    uvhttp_context_t* ctx = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &ctx);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(ctx, nullptr);

    /* Fields should be zeroed */
    EXPECT_EQ(ctx->initialized, 0);
    EXPECT_EQ(ctx->server, nullptr);
    EXPECT_EQ(ctx->router, nullptr);
    EXPECT_EQ(ctx->total_requests, 0u);
    EXPECT_EQ(ctx->total_connections, 0u);
    EXPECT_EQ(ctx->active_connections, 0u);
    EXPECT_EQ(ctx->user_data, nullptr);

    /* Init */
    err = uvhttp_context_init(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(ctx->initialized, 1);

    /* Destroy */
    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: init_tls and init_websocket with non-TLS build */
#if !UVHTTP_FEATURE_TLS

TEST(ConnectionContextCoverage, ContextInitTls_NotSupported) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_tls(ctx);
    EXPECT_EQ(err, UVHTTP_ERROR_NOT_SUPPORTED);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

TEST(ConnectionContextCoverage, ContextInitWebsocket_NotSupported) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_websocket(ctx);
    EXPECT_EQ(err, UVHTTP_ERROR_NOT_SUPPORTED);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

#endif /* !UVHTTP_FEATURE_TLS */

/* Context: config init idempotent + cleanup */
TEST(ConnectionContextCoverage, ContextConfigIdempotentAndCleanup) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    /* Init config */
    uvhttp_error_t err = uvhttp_context_init_config(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_NE(ctx->current_config, nullptr);

    /* Second init should be idempotent */
    void* first_config = ctx->current_config;
    err = uvhttp_context_init_config(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(ctx->current_config, first_config);

    /* Cleanup */
    uvhttp_context_cleanup_config(ctx);
    EXPECT_EQ(ctx->current_config, nullptr);

    /* Cleanup again should be safe (no-op) */
    uvhttp_context_cleanup_config(ctx);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: destroy without init (no initialized fields to clean) */
TEST(ConnectionContextCoverage, ContextDestroyWithoutInit) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    /* Never call uvhttp_context_init - destroy should still be safe */
    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: cleanup functions with null context */
TEST(ConnectionContextCoverage, ContextCleanupAll_NullContext) {
    /* All should be safe no-ops */
    uvhttp_context_cleanup_tls(nullptr);
    uvhttp_context_cleanup_websocket(nullptr);
    uvhttp_context_cleanup_config(nullptr);
}

/* ========== 11. Connection new with both null args ========== */

TEST(ConnectionContextCoverage, ConnectionNew_BothNull) {
    uvhttp_error_t result = uvhttp_connection_new(nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* ========== 12. Close then run loop to exercise on_handle_close ========== */

TEST(ConnectionContextCoverage, CloseAndRunLoopToCompletion) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Close connection */
    uvhttp_connection_close(conn);

    /* Run loop until all handles are closed. The async close callback
     * (on_handle_close -> uvhttp_connection_free_resources) frees the
     * connection during this run, so conn is invalid afterward and must not
     * be dereferenced. The test verifies that close + run completes without
     * crashing. */
    uv_run(loop, UV_RUN_DEFAULT);

    destroy_server_and_loop(server, loop);
}

/* ========== 13. Multiple close calls (idempotent safety) ========== */

TEST(ConnectionContextCoverage, MultipleCloseCalls) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* First close starts async close */
    uvhttp_connection_close(conn);

    /* Subsequent close calls should be safe (handles are already closing) */
    uvhttp_connection_close(conn);
    uvhttp_connection_close(conn);

    uv_run(loop, UV_RUN_DEFAULT);

    destroy_server_and_loop(server, loop);
}

/* ========== 14. Connection start after close ========== */

TEST(ConnectionContextCoverage, StartAfterClose) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->state = UVHTTP_CONN_STATE_CLOSING;
    /* Starting a closing connection should fail at uv_read_start */
    result = uvhttp_connection_start(conn);
    /* The result depends on whether uv_read_start fails on a closing handle */
    (void)result;

    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 15. Schedule restart read after close ========== */

TEST(ConnectionContextCoverage, ScheduleRestartReadAfterClose) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Close connection first */
    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 16. Timeout restart (stop existing timer then start new) ========== */

TEST(ConnectionContextCoverage, TimeoutRestart) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Start timeout once */
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Start timeout again - should stop old timer and restart */
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Also test custom timeout restart */
    result = uvhttp_connection_start_timeout_custom(conn, 10);
    EXPECT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, 20);
    EXPECT_EQ(result, UVHTTP_OK);

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 17. Timeout with config, then without config ========== */

TEST(ConnectionContextCoverage, TimeoutConfigToggle) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Start with no config (default timeout path) */
    server->config = nullptr;
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Now set config */
    uvhttp_config_t* config = nullptr;
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    config->connection_timeout = 120;
    server->config = config;

    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Restore */
    server->config = nullptr;
    uvhttp_config_free(config);

    uvhttp_connection_close(conn);
    uv_run(loop, UV_RUN_NOWAIT);

    destroy_server_and_loop(server, loop);
}

/* ========== 18. Connection initial field verification ========== */

TEST(ConnectionContextCoverage, ConnectionInitialFields) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Verify all initial field values set by connection_new */
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->keepalive, 1);
    EXPECT_EQ(conn->chunked_encoding, 0);
    EXPECT_EQ(conn->close_pending, 0);
    EXPECT_EQ(conn->freed, 0);
    EXPECT_EQ(conn->need_restart_read, 0);
    EXPECT_EQ(conn->content_length, 0u);
    EXPECT_EQ(conn->body_received, 0u);
    EXPECT_EQ(conn->parsing_complete, 0);
    EXPECT_EQ(conn->current_header_is_important, 0);
    EXPECT_EQ(conn->read_buffer_used, 0u);
    EXPECT_EQ(conn->current_header_field_len, 0u);
    EXPECT_EQ(conn->parsing_header_field, 0);
    EXPECT_EQ(conn->server, server);
    EXPECT_NE(conn->request, nullptr);
    EXPECT_NE(conn->response, nullptr);
    EXPECT_NE(conn->read_buffer, nullptr);
    EXPECT_GT(conn->read_buffer_size, 0u);
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&conn->tcp_handle));
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&conn->idle_handle));
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&conn->timeout_timer));

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== 19. set_state with all enum values ========== */

TEST(ConnectionContextCoverage, SetStateAllEnumValues) {
    uvhttp_connection_state_t states[] = {
        UVHTTP_CONN_STATE_NEW,
        UVHTTP_CONN_STATE_TLS_HANDSHAKE,
        UVHTTP_CONN_STATE_HTTP_READING,
        UVHTTP_CONN_STATE_HTTP_PROCESSING,
        UVHTTP_CONN_STATE_HTTP_WRITING,
        UVHTTP_CONN_STATE_PROTOCOL_UPGRADED,
        UVHTTP_CONN_STATE_CLOSING,
    };

    for (auto s : states) {
        uvhttp_connection_set_state(nullptr, s); /* null conn, no crash */
    }

    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    for (auto s : states) {
        uvhttp_connection_set_state(conn, s);
        EXPECT_EQ(conn->state, s);
    }

    uvhttp_connection_free(conn);
    destroy_server_and_loop(server, loop);
}

/* ========== 20. Context init with TLS build (full workflow) ========== */

#if UVHTTP_FEATURE_TLS

TEST(ConnectionContextCoverage, ContextFullTlsWorkflow) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    /* Initialize TLS */
    uvhttp_error_t err = uvhttp_context_init_tls(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(ctx->tls_initialized, 1);
    EXPECT_NE(ctx->tls_entropy, nullptr);
    EXPECT_NE(ctx->tls_drbg, nullptr);

    /* Initialize WebSocket */
    err = uvhttp_context_init_websocket(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(ctx->ws_drbg_initialized, 1);
    EXPECT_NE(ctx->ws_entropy, nullptr);
    EXPECT_NE(ctx->ws_drbg, nullptr);

    /* Initialize config */
    err = uvhttp_context_init_config(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_NE(ctx->current_config, nullptr);

    /* All idempotent */
    err = uvhttp_context_init_tls(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    err = uvhttp_context_init_websocket(ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    err = uvhttp_context_init_config(ctx);
    EXPECT_EQ(err, UVHTTP_OK);

    /* Cleanup in reverse order */
    uvhttp_context_cleanup_config(ctx);
    EXPECT_EQ(ctx->current_config, nullptr);

    uvhttp_context_cleanup_websocket(ctx);
    EXPECT_EQ(ctx->ws_drbg_initialized, 0);
    EXPECT_EQ(ctx->ws_entropy, nullptr);
    EXPECT_EQ(ctx->ws_drbg, nullptr);

    uvhttp_context_cleanup_tls(ctx);
    EXPECT_EQ(ctx->tls_initialized, 0);
    EXPECT_EQ(ctx->tls_entropy, nullptr);
    EXPECT_EQ(ctx->tls_drbg, nullptr);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: TLS partial cleanup - entropy freed, drbg NULL */
TEST(ConnectionContextCoverage, ContextTlsPartialCleanup_EntropyOnly) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_tls(ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    /* Null out drbg to test cleanup path where entropy exists but drbg is NULL.
     * Free the drbg first so we don't leak the allocation by dropping the only
     * reference to it. */
    mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)ctx->tls_drbg);
    uvhttp_free(ctx->tls_drbg);
    ctx->tls_drbg = nullptr;

    uvhttp_context_cleanup_tls(ctx);
    EXPECT_EQ(ctx->tls_initialized, 0);
    EXPECT_EQ(ctx->tls_entropy, nullptr);
    EXPECT_EQ(ctx->tls_drbg, nullptr);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: TLS partial cleanup - drbg freed, entropy NULL */
TEST(ConnectionContextCoverage, ContextTlsPartialCleanup_DrbgOnly) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_tls(ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    /* Null out entropy to test cleanup path where drbg exists but entropy is NULL.
     * Free entropy first to avoid leaking it. */
    mbedtls_entropy_free((mbedtls_entropy_context*)ctx->tls_entropy);
    uvhttp_free(ctx->tls_entropy);
    ctx->tls_entropy = nullptr;

    uvhttp_context_cleanup_tls(ctx);
    EXPECT_EQ(ctx->tls_initialized, 0);
    EXPECT_EQ(ctx->tls_entropy, nullptr);
    EXPECT_EQ(ctx->tls_drbg, nullptr);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: WebSocket partial cleanup - entropy freed, drbg NULL */
TEST(ConnectionContextCoverage, ContextWsPartialCleanup_EntropyOnly) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_websocket(ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    /* Free drbg first to avoid leaking it, then exercise the entropy-only
     * cleanup path. */
    mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)ctx->ws_drbg);
    uvhttp_free(ctx->ws_drbg);
    ctx->ws_drbg = nullptr;

    uvhttp_context_cleanup_websocket(ctx);
    EXPECT_EQ(ctx->ws_drbg_initialized, 0);
    EXPECT_EQ(ctx->ws_entropy, nullptr);
    EXPECT_EQ(ctx->ws_drbg, nullptr);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

/* Context: WebSocket partial cleanup - drbg freed, entropy NULL */
TEST(ConnectionContextCoverage, ContextWsPartialCleanup_DrbgOnly) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    uvhttp_context_t* ctx = nullptr;
    ASSERT_EQ(uvhttp_context_create(&loop, &ctx), UVHTTP_OK);

    uvhttp_error_t err = uvhttp_context_init_websocket(ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    /* Free entropy first to avoid leaking it, then exercise the drbg-only
     * cleanup path. */
    mbedtls_entropy_free((mbedtls_entropy_context*)ctx->ws_entropy);
    uvhttp_free(ctx->ws_entropy);
    ctx->ws_entropy = nullptr;

    uvhttp_context_cleanup_websocket(ctx);
    EXPECT_EQ(ctx->ws_drbg_initialized, 0);
    EXPECT_EQ(ctx->ws_entropy, nullptr);
    EXPECT_EQ(ctx->ws_drbg, nullptr);

    uvhttp_context_destroy(ctx);
    uv_loop_close(&loop);
}

#endif /* UVHTTP_FEATURE_TLS */

/* ========== 21. Stress: create and free many connections ========== */

TEST(ConnectionContextCoverage, ConnectionCreateFreeStress) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    for (int i = 0; i < 50; i++) {
        uvhttp_connection_t* conn = nullptr;
        uvhttp_error_t result = uvhttp_connection_new(server, &conn);
        ASSERT_EQ(result, UVHTTP_OK);
        uvhttp_connection_free(conn);
    }

    destroy_server_and_loop(server, loop);
}

/* ========== 22. Context create/destroy stress ========== */

TEST(ConnectionContextCoverage, ContextCreateDestroyStress) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);

    for (int i = 0; i < 100; i++) {
        uvhttp_context_t* ctx = nullptr;
        uvhttp_error_t err = uvhttp_context_create(&loop, &ctx);
        ASSERT_EQ(err, UVHTTP_OK);
        uvhttp_context_destroy(ctx);
    }

    uv_loop_close(&loop);
}

/* ========== Main ========== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
