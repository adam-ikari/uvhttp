/* uvhttp_connection.c 扩展覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <uv.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionTlsHandshakeFunc) {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(uvhttp_connection_tls_handshake_func(conn), -1);

    uvhttp_connection_free(conn);
    uvhttp_free(server);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionStateTransitions) {
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    uvhttp_connection_free(conn);
    uvhttp_free(server);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionRestartReadWithNull) {
    EXPECT_EQ(uvhttp_connection_restart_read(nullptr), -1);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionScheduleRestartReadWithNull) {
    EXPECT_EQ(uvhttp_connection_schedule_restart_read(nullptr), -1);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionStartWithNull) {
    EXPECT_EQ(uvhttp_connection_start(nullptr), -1);
}