#include <string>
/*
 * A client may send the first bytes of a custom protocol in the same TCP
 * write as the upgrade request. Those bytes were already read into
 * conn->read_buffer when transfer_ownership handed the fd to the new owner;
 * on_read then cleared the buffer unconditionally, so the owner never saw
 * them. The ownership callback now receives the buffered remainder.
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_protocol_upgrade.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

namespace {

struct captured {
    std::string buffered;
    int calls = 0;
};

void capture(uv_tcp_t*, int, const char* buf, size_t len, void* ud) {
    captured* c = (captured*)ud;
    ++c->calls;
    c->buffered.assign(buf, len);
}

/* Give the connection a real fd via socketpair so uv_fileno succeeds. */
bool GiveRealFd(uv_loop_t* loop, uvhttp_server_t*, uvhttp_connection_t* conn) {
    int fds[2];
#ifdef _WIN32
    return false; /* socketpair not available; test is POSIX-only */
#else
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return false;
    uv_close((uv_handle_t*)&conn->tcp_handle, nullptr);
    uv_run(loop, UV_RUN_NOWAIT);
    if (uv_tcp_init(loop, &conn->tcp_handle) != 0) {
        close(fds[0]); close(fds[1]); return false;
    }
    if (uv_tcp_open(&conn->tcp_handle, fds[0]) != 0) {
        close(fds[0]); close(fds[1]); return false;
    }
    conn->tcp_handle.data = conn;
    close(fds[1]);
    return true;
#endif
}

}  // namespace

TEST(UpgradeBufferedTest, OwnershipCallbackReceivesBytesAfterHeaders) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);
    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    ASSERT_EQ(uvhttp_connection_new(server, &conn), UVHTTP_OK);
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;
    ASSERT_TRUE(GiveRealFd(loop, server, conn));

    static const char kWire[] =
        "GET / HTTP/1.1\r\nUpgrade: h2c\r\nConnection: Upgrade\r\n\r\nFRAME";
    memcpy(conn->read_buffer, kWire, sizeof(kWire) - 1);
    conn->read_buffer_used = sizeof(kWire) - 1;

    captured out;
    ASSERT_EQ(uvhttp_connection_transfer_ownership(conn, capture, &out),
              UVHTTP_OK);
    EXPECT_EQ(out.calls, 1);
    EXPECT_EQ(out.buffered, "FRAME");

    /* connection_new owns the struct; transfer_ownership did not free it. */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

TEST(UpgradeBufferedTest, NoRemainderYieldsEmptyBuffer) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);
    uvhttp_server_t* server = nullptr;
    ASSERT_EQ(uvhttp_server_new(loop, &server), UVHTTP_OK);
    uvhttp_connection_t* conn = nullptr;
    ASSERT_EQ(uvhttp_connection_new(server, &conn), UVHTTP_OK);
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;
    ASSERT_TRUE(GiveRealFd(loop, server, conn));

    static const char kWire[] = "GET / HTTP/1.1\r\n\r\n";
    memcpy(conn->read_buffer, kWire, sizeof(kWire) - 1);
    conn->read_buffer_used = sizeof(kWire) - 1;

    captured out;
    ASSERT_EQ(uvhttp_connection_transfer_ownership(conn, capture, &out),
              UVHTTP_OK);
    EXPECT_EQ(out.calls, 1);
    EXPECT_TRUE(out.buffered.empty());

    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}
