/*
 * uvhttp_connection.c static callback coverage test
 *
 * Exercises internal static callbacks and error paths
 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "uvhttp_router.h"
#include <string.h>
#include <uv.h>

static int test_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "OK", 2);
    return uvhttp_response_send(resp);
}

/* ========== Connection new error paths ========== */

TEST(UvhttpConnectionCallbackTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t err = uvhttp_connection_new(nullptr, &conn);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionCallbackTest, ConnectionNewNullConn) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    err = uvhttp_connection_new(server, nullptr);
    EXPECT_NE(err, UVHTTP_OK);

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

TEST(UvhttpConnectionCallbackTest, ConnectionNewAndFree) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_free(conn);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Connection start timeout ========== */

TEST(UvhttpConnectionCallbackTest, ConnectionTimeoutNull) {
    uvhttp_error_t err = uvhttp_connection_start_timeout(nullptr);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionCallbackTest, ConnectionTimeoutCustomNull) {
    uvhttp_error_t err = uvhttp_connection_start_timeout_custom(nullptr, 60);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionCallbackTest, ConnectionStartTimeout) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_start_timeout(conn);
        uvhttp_connection_start_timeout_custom(conn, 5000);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Schedule restart read ========== */

TEST(UvhttpConnectionCallbackTest, ScheduleRestartReadNull) {
    uvhttp_connection_schedule_restart_read(nullptr);
}

TEST(UvhttpConnectionCallbackTest, ScheduleRestartRead) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_schedule_restart_read(conn);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== TLS handshake without TLS ========== */

TEST(UvhttpConnectionCallbackTest, TlsHandshakeNull) {
    uvhttp_error_t err = uvhttp_connection_tls_handshake_func(nullptr);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionCallbackTest, TlsHandshakeNoTls) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_error_t tls_err = uvhttp_connection_tls_handshake_func(conn);
        EXPECT_NE(tls_err, UVHTTP_OK);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== TLS read/write without TLS ========== */

TEST(UvhttpConnectionCallbackTest, TlsReadWriteNoTls) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_tls_read(conn);
        const char* data = "test";
        uvhttp_connection_tls_write(conn, data, 4);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Connection start ========== */

TEST(UvhttpConnectionCallbackTest, ConnectionStartNull) {
    uvhttp_error_t err = uvhttp_connection_start(nullptr);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionCallbackTest, ConnectionStart) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_start(conn);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Websocket close ========== */

TEST(UvhttpConnectionCallbackTest, WebsocketCloseNull) {
    uvhttp_connection_websocket_close(nullptr);
}

TEST(UvhttpConnectionCallbackTest, WebsocketClose) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_websocket_close(conn);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== TLS cleanup ========== */

TEST(UvhttpConnectionCallbackTest, TlsCleanupNull) {
    uvhttp_connection_tls_cleanup(nullptr);
}

TEST(UvhttpConnectionCallbackTest, TlsCleanup) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t err = uvhttp_server_new(loop, &server);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    err = uvhttp_connection_new(server, &conn);
    if (err == UVHTTP_OK && conn) {
        uvhttp_connection_tls_cleanup(conn);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Connection close null ========== */

TEST(UvhttpConnectionCallbackTest, ConnectionCloseNull) {
    uvhttp_connection_close(nullptr);
}