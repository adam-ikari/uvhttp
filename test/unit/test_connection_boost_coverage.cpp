/**
 * @file test_connection_boost_coverage.cpp
 * @brief Boost coverage tests for uvhttp_connection.c
 *
 * Focuses on uncovered code paths in uvhttp_connection.c.
 * Uses consistent cleanup: close + run loop to let on_handle_close free resources.
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

/*
 * Safely tear down a connection: close via the library path, run the loop to
 * let on_handle_close free resources, then destroy server/loop.
 * After this call the connection pointer is invalid.
 */
static void close_and_drain(uvhttp_connection_t* conn, uv_loop_t* loop) {
    /* Call free which internally handles close + drain + resource cleanup */
    uvhttp_connection_free(conn);
    /* Run the loop to process any pending close callbacks */
    uv_run(loop, UV_RUN_DEFAULT);
}

/* ========== 1. restart_read with non-NULL request body ========== */

TEST(ConnectionBoostCoverage, RestartRead_WithRequestBody) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->request, nullptr);
    /* Free the body allocated by uvhttp_request_init before replacing it */
    if (conn->request->body) {
        uvhttp_free(conn->request->body);
    }
    conn->request->body = (char*)uvhttp_alloc(128);
    ASSERT_NE(conn->request->body, nullptr);
    conn->request->body_length = 128;
    conn->request->body_capacity = 128;

    /* This will free the body, reset it, then attempt restart read */
    result = uvhttp_connection_restart_read(conn);

    EXPECT_EQ(conn->request->body, nullptr);
    EXPECT_EQ(conn->request->body_length, 0u);
    EXPECT_EQ(conn->request->body_capacity, 0u);

    /* The body was already freed by restart_read, no need to free again */
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 2. restart_read with non-NULL response body ========== */

TEST(ConnectionBoostCoverage, RestartRead_WithResponseBody) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->response, nullptr);
    /* Free the body allocated by uvhttp_request_init before replacing it */
    if (conn->response->body) {
        uvhttp_free(conn->response->body);
    }
    conn->response->body = (char*)uvhttp_alloc(256);
    ASSERT_NE(conn->response->body, nullptr);

    result = uvhttp_connection_restart_read(conn);

    EXPECT_EQ(conn->response->body, nullptr);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 3. close with already-closing handles ========== */

TEST(ConnectionBoostCoverage, Close_AlreadyClosingHandles) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Close once - starts async close */
    uvhttp_connection_close(conn);
    /* Close again - state is CLOSING, close_pending > 0, guard returns early */
    uvhttp_connection_close(conn);
    /* Third time */
    uvhttp_connection_close(conn);

    uv_run(loop, UV_RUN_DEFAULT);
    destroy_server_and_loop(server, loop);
}

/* ========== 4. close synchronous free path ========== */

TEST(ConnectionBoostCoverage, Close_SynchronousFree) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 5. schedule_restart_read success path ========== */

TEST(ConnectionBoostCoverage, ScheduleRestartRead_Success) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Schedule restart read - starts the idle handle */
    result = uvhttp_connection_schedule_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Stop the idle handle immediately so it doesn't fire on the unconnected
     * TCP handle. This exercises the schedule path without triggering the
     * idle callback which would call uv_read_start on an unconnected handle. */
    uv_idle_stop(&conn->idle_handle);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 6. start_timeout success ========== */

TEST(ConnectionBoostCoverage, StartTimeout_Success) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Start timeout - uses default 60 second timeout, won't fire during test */
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 7. start_timeout_custom with timer stop + restart ========== */

TEST(ConnectionBoostCoverage, StartTimeoutCustom_StopAndRestart) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, 30);
    EXPECT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, 60);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 8. timeout callback set ========== */

static int g_timeout_callback_called = 0;
static void test_timeout_callback(uvhttp_server_t* srv,
                                   uvhttp_connection_t* c,
                                   uint64_t timeout_ms,
                                   void* user_data) {
    (void)srv;
    (void)c;
    (void)timeout_ms;
    (void)user_data;
    g_timeout_callback_called = 1;
}

TEST(ConnectionBoostCoverage, TimeoutCallback_WithCallbackSet) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    server->timeout_callback = test_timeout_callback;
    server->timeout_callback_user_data = nullptr;

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    g_timeout_callback_called = 0;
    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Restore callback before teardown */
    server->timeout_callback = nullptr;
    server->timeout_callback_user_data = nullptr;

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 9. tls_handshake_func with null server ========== */

TEST(ConnectionBoostCoverage, TlsHandshakeFunc_NullServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    struct uvhttp_server* saved_server = conn->server;
    conn->server = nullptr;

    result = uvhttp_connection_tls_handshake_func(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->server = saved_server;
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 10. free_resources with lifecycle set ========== */

TEST(ConnectionBoostCoverage, FreeResources_WithLifecycle) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->lifecycle = uvhttp_alloc(64);
    ASSERT_NE(conn->lifecycle, nullptr);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 11. start with TLS enabled (no TLS ctx) ========== */

TEST(ConnectionBoostCoverage, Start_TlsEnabledNoCtx) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->tls_enabled = 1;
#if UVHTTP_FEATURE_TLS
    ASSERT_EQ(server->tls_ctx, nullptr);
#else
    /* When TLS is disabled, tls_ctx doesn't exist; just verify start fails */
#endif

    result = uvhttp_connection_start(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_START);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 12. connection_new initial field verification ========== */

TEST(ConnectionBoostCoverage, ConnectionNew_InitialFields) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    EXPECT_EQ(conn->keepalive, 1);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->chunked_encoding, 0);
    EXPECT_EQ(conn->close_pending, 0);
    EXPECT_EQ(conn->freed, 0);
    EXPECT_EQ(conn->need_restart_read, 0);
    EXPECT_EQ(conn->parsing_complete, 0);
    EXPECT_EQ(conn->current_header_is_important, 0);
    EXPECT_EQ(conn->parsing_header_field, 0);
    EXPECT_EQ(conn->read_buffer_used, 0u);
    EXPECT_EQ(conn->content_length, 0u);
    EXPECT_EQ(conn->body_received, 0u);
    EXPECT_NE(conn->read_buffer, nullptr);
    EXPECT_GT(conn->read_buffer_size, 0u);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 13. close with null server ========== */

TEST(ConnectionBoostCoverage, Close_NullServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    struct uvhttp_server* saved_server = conn->server;
    conn->server = nullptr;

    uvhttp_connection_close(conn);

    conn->server = saved_server;
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 14. set_state with all states on null ========== */

TEST(ConnectionBoostCoverage, SetState_NullWithAllStates) {
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
        uvhttp_connection_set_state(nullptr, s);
    }
}

/* ========== 15. restart_read with CLOSING state ========== */

TEST(ConnectionBoostCoverage, RestartRead_ClosingState) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->state = UVHTTP_CONN_STATE_CLOSING;
    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_CLOSE);

    conn->state = UVHTTP_CONN_STATE_NEW;
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 16. start_timeout with config set ========== */

TEST(ConnectionBoostCoverage, StartTimeout_WithConfig) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_config_t* config = nullptr;
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    config->connection_timeout = 30;
    server->config = config;

    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    server->config = nullptr;
    uvhttp_config_free(config);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 17. start_timeout with no config ========== */

TEST(ConnectionBoostCoverage, StartTimeout_NoConfig) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    server->config = nullptr;

    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 18. start_timeout_custom with null server ========== */

TEST(ConnectionBoostCoverage, StartTimeoutCustom_NullServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    struct uvhttp_server* saved_server = conn->server;
    conn->server = nullptr;

    result = uvhttp_connection_start_timeout_custom(conn, 30);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->server = saved_server;
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 19. Connection free with close_pending = 0 ========== */

TEST(ConnectionBoostCoverage, Free_ClosePendingZero) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    EXPECT_EQ(conn->close_pending, 0);
    EXPECT_EQ(conn->freed, 0);

    /* Free calls close which starts async close, on_handle_close frees */
    uvhttp_connection_free(conn);

    uv_run(loop, UV_RUN_DEFAULT);
    destroy_server_and_loop(server, loop);
}

/* ========== 20. tls_cleanup with ssl set (no-op on non-TLS) ========== */

TEST(ConnectionBoostCoverage, TlsCleanup_WithSsl) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->ssl = nullptr;
    uvhttp_connection_tls_cleanup(conn);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 21. start_tls_handshake wrapper ========== */

TEST(ConnectionBoostCoverage, StartTlsHandshake_Delegates) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_tls_handshake(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 22. tls_read with null ssl ========== */

TEST(ConnectionBoostCoverage, TlsRead_NullSsl) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_EQ(conn->ssl, nullptr);
    result = uvhttp_connection_tls_read(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 23. tls_write with null ssl ========== */

TEST(ConnectionBoostCoverage, TlsWrite_NullSsl) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    const char data[] = "test data";
    result = uvhttp_connection_tls_write(conn, data, sizeof(data));
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 24. Connection new with both null ========== */

TEST(ConnectionBoostCoverage, ConnectionNew_BothNull) {
    uvhttp_error_t result = uvhttp_connection_new(nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* ========== 25. Close null multiple times ========== */

TEST(ConnectionBoostCoverage, Close_NullMultipleTimes) {
    uvhttp_connection_close(nullptr);
    uvhttp_connection_close(nullptr);
    uvhttp_connection_close(nullptr);
}

/* ========== 26. Free null multiple times ========== */

TEST(ConnectionBoostCoverage, Free_NullMultipleTimes) {
    uvhttp_connection_free(nullptr);
    uvhttp_connection_free(nullptr);
    uvhttp_connection_free(nullptr);
}

/* ========== 27. Stress: create and free many connections ========== */

TEST(ConnectionBoostCoverage, ConnectionCreateFreeStress) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    for (int i = 0; i < 100; i++) {
        uvhttp_connection_t* conn = nullptr;
        uvhttp_error_t result = uvhttp_connection_new(server, &conn);
        ASSERT_EQ(result, UVHTTP_OK);
        ASSERT_NE(conn, nullptr);
        close_and_drain(conn, loop);
    }

    destroy_server_and_loop(server, loop);
}

/* ========== 28. Connection start (non-TLS) ========== */

TEST(ConnectionBoostCoverage, Start_NonTls) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->tls_enabled = 0;
    result = uvhttp_connection_start(conn);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 29. start_timeout_custom valid values ========== */

TEST(ConnectionBoostCoverage, StartTimeoutCustom_Valid) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, 5);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 30. start_timeout with null server ========== */

TEST(ConnectionBoostCoverage, StartTimeout_NullServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    struct uvhttp_server* saved_server = conn->server;
    conn->server = nullptr;

    result = uvhttp_connection_start_timeout(conn);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    conn->server = saved_server;
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 31. start_timeout_custom overflow check ========== */

TEST(ConnectionBoostCoverage, StartTimeoutCustom_OverflowCheck) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, INT_MAX);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 32. Multiple close with run loop ========== */

TEST(ConnectionBoostCoverage, MultipleCloseWithRun) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Close once - starts async close */
    uvhttp_connection_close(conn);

    /* Run loop to completion - on_handle_close frees the connection.
     * After UV_RUN_DEFAULT, conn is freed and must not be accessed. */
    uv_run(loop, UV_RUN_DEFAULT);

    destroy_server_and_loop(server, loop);
}

/* ========== 33. schedule_restart_read with CLOSING state ========== */

TEST(ConnectionBoostCoverage, ScheduleRestartRead_ClosingState) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->state = UVHTTP_CONN_STATE_CLOSING;

    result = uvhttp_connection_schedule_restart_read(conn);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Stop the idle handle immediately to avoid it firing */
    uv_idle_stop(&conn->idle_handle);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 34. restart_read null parser ========== */

TEST(ConnectionBoostCoverage, RestartRead_NullParser) {
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
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 35. restart_read null parser_settings ========== */

TEST(ConnectionBoostCoverage, RestartRead_NullParserSettings) {
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
    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 36. tls_cleanup no ssl ========== */

TEST(ConnectionBoostCoverage, TlsCleanup_NoSsl) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_tls_cleanup(conn);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 37. Connection new read buffer ========== */

TEST(ConnectionBoostCoverage, ConnectionNew_ReadBuffer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    EXPECT_NE(conn->read_buffer, nullptr);
    EXPECT_GT(conn->read_buffer_size, 0u);
    EXPECT_EQ(conn->read_buffer_used, 0u);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 38. Connection new parser data ========== */

TEST(ConnectionBoostCoverage, ConnectionNew_ParserData) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->request, nullptr);
    ASSERT_NE(conn->request->parser, nullptr);
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    EXPECT_EQ(parser->data, conn);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 39. start_timeout_custom valid range ========== */

TEST(ConnectionBoostCoverage, StartTimeoutCustom_Range) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Exact minimum */
    result = uvhttp_connection_start_timeout_custom(conn, 5);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);

    /* Exact maximum */
    conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_connection_start_timeout_custom(conn, 300);
    EXPECT_EQ(result, UVHTTP_OK);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 40. restart_read response body free (duplicate of #2) ========== */

TEST(ConnectionBoostCoverage, RestartRead_ResponseBodyFree) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    ASSERT_NE(conn->response, nullptr);
    /* Free the body allocated by uvhttp_request_init before replacing it */
    if (conn->response->body) {
        uvhttp_free(conn->response->body);
    }
    conn->response->body = (char*)uvhttp_alloc(512);
    ASSERT_NE(conn->response->body, nullptr);

    result = uvhttp_connection_restart_read(conn);
    EXPECT_EQ(conn->response->body, nullptr);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

#if UVHTTP_FEATURE_WEBSOCKET

/* ========== 41. WebSocket switch_to with CLOSING state guard ========== */

TEST(ConnectionBoostCoverage, SwitchToWebsocket_ClosingState) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->state = UVHTTP_CONN_STATE_CLOSING;
    uvhttp_connection_switch_to_websocket(conn);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 42. WebSocket close with ws_connection NULL ========== */

TEST(ConnectionBoostCoverage, WebsocketClose_WithWsConnection) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    conn->is_websocket = 1;
    conn->ws_connection = nullptr;

    uvhttp_connection_websocket_close(conn);
    EXPECT_EQ(conn->is_websocket, 0);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

/* ========== 43. WebSocket handshake both null ========== */

TEST(ConnectionBoostCoverage, WebsocketHandshake_BothNull) {
    uvhttp_error_t result = uvhttp_connection_handle_websocket_handshake(
        nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* ========== 44. WebSocket close multiple times ========== */

TEST(ConnectionBoostCoverage, WebsocketClose_Multiple) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_websocket_close(conn);
    uvhttp_connection_websocket_close(conn);
    uvhttp_connection_websocket_close(conn);

    close_and_drain(conn, loop);
    destroy_server_and_loop(server, loop);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */

/* ========== Main ========== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}