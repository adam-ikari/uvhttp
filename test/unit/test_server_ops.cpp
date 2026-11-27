/*
 * Production-operation primitives: readable stats and a stop that actually
 * quiets the loop it owns.
 *
 * Before this, the server maintained counters that no public API could read,
 * and uvhttp_server_stop closed only the listening handle. Enabling WS
 * keepalive left two timers on the loop, so a stopped server kept
 * uv_run(UV_RUN_DEFAULT) alive forever.
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <uv.h>

namespace {

class ServerOpsTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
    }
    void TearDown() override {
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        if (loop_) {
            uv_run(loop_, UV_RUN_NOWAIT);
            uv_loop_close(loop_);
            uvhttp_free(loop_);
        }
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
};

}  // namespace

TEST_F(ServerOpsTest, GetStatsRejectsNullArguments) {
    uvhttp_server_stats_t stats;
    EXPECT_EQ(uvhttp_server_get_stats(nullptr, &stats),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_server_get_stats(server_, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServerOpsTest, GetStatsReadsCountersAlreadyMaintained) {
    uvhttp_context_t* context = nullptr;
    ASSERT_EQ(uvhttp_context_create(loop_, &context), UVHTTP_OK);
    context->total_requests = 17;
    context->total_connections = 5;
    context->active_connections = 2;
    ASSERT_EQ(uvhttp_server_take_context(server_, context), UVHTTP_OK);
    server_->active_connections = 3; /* authoritative for the limit */

    uvhttp_server_stats_t stats;
    ASSERT_EQ(uvhttp_server_get_stats(server_, &stats), UVHTTP_OK);
    EXPECT_EQ(stats.active_connections, 3u);
    EXPECT_EQ(stats.total_requests, 17u);
    EXPECT_EQ(stats.total_connections, 5u);
}

#if UVHTTP_FEATURE_ASYNC_STOP
TEST_F(ServerOpsTest, AsyncStopFromForeignThreadStopsOnLoopThread) {
    ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
    ASSERT_EQ(server_->is_listening, 1);

    std::atomic<bool> posted{false};
    std::thread stopper([this, &posted]() {
        posted.store(true);
        EXPECT_EQ(uvhttp_server_async_stop(server_), UVHTTP_OK);
    });

    /* Pump until the post is visible and the async callback has run; never
     * rely on a fixed sleep that can be delayed under ASan/load. */
    for (int i = 0; i < 100 && !posted.load(); ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
        std::this_thread::yield();
    }
    for (int i = 0; i < 20; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }

    stopper.join();

    EXPECT_EQ(server_->is_listening, 0);
}

TEST_F(ServerOpsTest, AsyncStopRejectsNullServer) {
    EXPECT_EQ(uvhttp_server_async_stop(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}
#endif

#if UVHTTP_FEATURE_WEBSOCKET
TEST_F(ServerOpsTest, StopQuietsWebsocketKeepaliveTimers) {
    ASSERT_EQ(uvhttp_server_ws_enable_connection_management(server_, 10, 5),
              UVHTTP_OK);
    EXPECT_EQ(uv_loop_alive(loop_), 1);

    ASSERT_EQ(uvhttp_server_stop(server_), UVHTTP_OK);
    for (int i = 0; i < 10; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }
    EXPECT_EQ(uv_loop_alive(loop_), 0);

    /* idempotent, including after the timers are gone */
    EXPECT_EQ(uvhttp_server_stop(server_), UVHTTP_OK);
}
#endif

namespace {

int ConnectPort(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

}  // namespace

TEST_F(ServerOpsTest, StopClosesAcceptedConnections) {
    ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
    struct sockaddr_in addr;
    int namelen = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                 (struct sockaddr*)&addr, &namelen),
              0);
    int port = ntohs(addr.sin_port);

    int a = ConnectPort(port);
    int b = ConnectPort(port);
    ASSERT_GE(a, 0);
    ASSERT_GE(b, 0);
    /* Let accept() complete and register both connections. */
    for (int i = 0; i < 10; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }
    ASSERT_NE(server_->connections, nullptr);

    ASSERT_EQ(uvhttp_server_stop(server_), UVHTTP_OK);
    for (int i = 0; i < 30; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }

    for (int fd : {a, b}) {
        char buf[16];
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        bool eof = false;
        for (int i = 0; i < 20 && !eof; ++i) {
            uv_run(loop_, UV_RUN_NOWAIT);
            if (poll(&pfd, 1, 5) > 0 && recv(fd, buf, sizeof(buf), 0) == 0) {
                eof = true;
            }
        }
        EXPECT_TRUE(eof) << "accepted connection survived server_stop";
        close(fd);
    }
}

TEST_F(ServerOpsTest, FreeWithoutStopClosesAcceptedConnections) {
    /* Regression: server_free used to release the server while keep-alive
     * connections still held conn->server, so their callbacks (or the next
     * request on the socket) dereferenced freed memory. */
    ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
    struct sockaddr_in addr;
    int namelen = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                 (struct sockaddr*)&addr, &namelen),
              0);
    int port = ntohs(addr.sin_port);
    int fd = ConnectPort(port);
    ASSERT_GE(fd, 0);
    for (int i = 0; i < 10; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }
    ASSERT_NE(server_->connections, nullptr);
    ASSERT_EQ(server_->active_connections, 1u);

    /* Free without stop: the server itself must close tracked connections. */
    uvhttp_server_free(server_);
    server_ = nullptr;
    for (int i = 0; i < 20; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }

    char buf[16];
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    bool eof = false;
    for (int i = 0; i < 20 && !eof; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
        if (poll(&pfd, 1, 5) > 0 && recv(fd, buf, sizeof(buf), 0) == 0) {
            eof = true;
        }
    }
    EXPECT_TRUE(eof) << "connection survived server_free without stop";
    close(fd);
}

TEST_F(ServerOpsTest, DirectConnectionNewDoesNotUndercountActive) {
    /* uvhttp_connection_new is a public API; connections created through it
     * never incremented active_connections, so their close callback must not
     * decrement it either. */
    uvhttp_connection_t* conn = nullptr;
    ASSERT_EQ(uvhttp_connection_new(server_, &conn), UVHTTP_OK);
    ASSERT_EQ(server_->active_connections, 0u);

    uvhttp_connection_close(conn);
    /* All three handles must fire on_handle_close before the counter is
     * touched; pump until the connection is actually freed. */
    for (int i = 0; i < 50; ++i) {
        uv_run(loop_, UV_RUN_NOWAIT);
    }
    EXPECT_EQ(server_->active_connections, 0u);
}
