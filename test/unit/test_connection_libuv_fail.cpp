/*
 * Tests using libuv_mock (linker wrap) to cover error paths
 * in uvhttp_connection.c and uvhttp_server.c.
 *
 * By linking against the mock libuv, we can make libuv functions
 * return error codes that are nearly impossible to trigger with
 * the real libuv.
 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "libuv_mock.h"
#include <string.h>

class UvhttpConnectionMockTest : public ::testing::Test {
protected:
    uv_loop_t* loop;
    uvhttp_server_t* server;

    void SetUp() override {
        libuv_mock_reset();
        loop = uv_loop_new();
        ASSERT_NE(loop, nullptr);
        uvhttp_error_t err = uvhttp_server_new(loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
    }

    void TearDown() override {
        if (server) {
            uvhttp_server_free(server);
        }
        if (loop) {
            uv_run(loop, UV_RUN_NOWAIT);
            uv_loop_close(loop);
            uvhttp_free(loop);
        }
    }
};

/* ========== uvhttp_connection_new failure paths ========== */

TEST_F(UvhttpConnectionMockTest, NewIdleInitFails) {
    libuv_mock_set_uv_idle_init_result(-1);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t err = uvhttp_connection_new(server, &conn);
    EXPECT_NE(err, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}

TEST_F(UvhttpConnectionMockTest, NewTimerInitFails) {
    libuv_mock_set_uv_timer_init_result(-1);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t err = uvhttp_connection_new(server, &conn);
    EXPECT_NE(err, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}

TEST_F(UvhttpConnectionMockTest, NewTcpInitFails) {
    libuv_mock_set_uv_tcp_init_result(-1);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t err = uvhttp_connection_new(server, &conn);
    EXPECT_NE(err, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}

/* ========== Connection start failure ========== */

TEST_F(UvhttpConnectionMockTest, StartReadStartFails) {
    libuv_mock_set_uv_read_start_result(-1);

    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t err = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    err = uvhttp_connection_start(conn);
    EXPECT_NE(err, UVHTTP_OK);
    /* connection_start failure already called uvhttp_connection_close internally,
     * which triggered the close callback (via mock). Do not call free again. */
}

/* ========== Server listen failure ========== */

TEST_F(UvhttpConnectionMockTest, ServerListenBindFails) {
    libuv_mock_set_uv_tcp_bind_result(-1);

    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 8080);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST_F(UvhttpConnectionMockTest, ServerListenListenFails) {
    libuv_mock_set_uv_listen_result(-1);

    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 8080);
    EXPECT_NE(err, UVHTTP_OK);
}

/* ========== on_connection 503 path: uv_accept failure ========== */

/*
 * Regression test: when the connection limit is reached and uv_accept fails
 * on the temporary 503 client, the temp_client must be released through
 * uv_close (so libuv drops it from the loop's handle queue and then frees it),
 * not freed directly. Directly freeing an initialized libuv handle leaves a
 * dangling pointer in the loop's handle queue -> use-after-free on the next
 * uv_run / uv_loop_close.
 */
TEST_F(UvhttpConnectionMockTest, ConnectionLimitAcceptFailClosesTempClient) {
    /* Force the 503 path: max_connections = 0 so a single new connection
     * already exceeds the limit. The config is owned by the server (freed by
     * uvhttp_server_free in TearDown). */
    uvhttp_config_t* config = nullptr;
    uvhttp_error_t cerr = uvhttp_config_new(&config);
    ASSERT_EQ(cerr, UVHTTP_OK);
    ASSERT_NE(config, nullptr);
    config->max_connections = 0;
    server->config = config;

    /* Listen so uv_listen registers on_connection as the connection callback */
    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);

    /* Make uv_accept fail on the temporary 503 client */
    libuv_mock_set_uv_accept_result(-1);

    size_t close_before = 0;
    libuv_mock_get_call_count("uv_close", &close_before);

    /* Trigger on_connection via the stored connection_cb */
    libuv_mock_trigger_connection_cb((uv_stream_t*)&server->tcp_handle, 0);

    size_t close_after = 0;
    libuv_mock_get_call_count("uv_close", &close_after);
    /* The temp_client must be closed through uv_close, not freed directly. */
    EXPECT_GT(close_after, close_before);
}

/*
 * Regression test: server->max_connections (public struct field) must be
 * honored by on_connection when no config is attached, instead of a hardcoded
 * default. Previously the field was initialized to UVHTTP_MAX_CONNECTIONS_MAX
 * and never read, so setting it to 0 here had no effect and the 503 path was
 * never entered.
 */
TEST_F(UvhttpConnectionMockTest, ServerMaxConnectionsFieldIsAuthoritative) {
    /* No server->config: on_connection must fall back to server->max_connections.
     * Setting it to 0 forces the 503 path (a single new connection exceeds it). */
    server->max_connections = 0;
    /* Fail uv_accept so the 503 temp client is closed (uv_close) instead of
     * starting a write we'd have to drain. */
    libuv_mock_set_uv_accept_result(-1);

    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);

    size_t close_before = 0;
    libuv_mock_get_call_count("uv_close", &close_before);

    libuv_mock_trigger_connection_cb((uv_stream_t*)&server->tcp_handle, 0);

    size_t close_after = 0;
    libuv_mock_get_call_count("uv_close", &close_after);
    /* 503 path entered => temp client created + closed via uv_close. */
    EXPECT_GT(close_after, close_before);
}

/* ========== Server new_with_loop failure ========== */

TEST_F(UvhttpConnectionMockTest, ServerNewWithLoopLoopInitFails) {
    libuv_mock_set_uv_loop_init_result(-1);

    uvhttp_server_t* srv = nullptr;
    uvhttp_error_t err = uvhttp_server_new_with_loop(&srv);
    EXPECT_NE(err, UVHTTP_OK);
    EXPECT_EQ(srv, nullptr);
}