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

/* ========== Server new_with_loop failure ========== */

TEST_F(UvhttpConnectionMockTest, ServerNewWithLoopLoopInitFails) {
    libuv_mock_set_uv_loop_init_result(-1);

    uvhttp_server_t* srv = nullptr;
    uvhttp_error_t err = uvhttp_server_new_with_loop(&srv);
    EXPECT_NE(err, UVHTTP_OK);
    EXPECT_EQ(srv, nullptr);
}