/*
 * Asynchronous error observer.
 *
 * Parse errors and failed write completions have no return path to the
 * caller. Before the observer seam they were logged (often into a no-op
 * Release build) and dropped; an embedder had no way to count or report them.
 * This test uses a real socket and an invalid request line so the parse error
 * is deterministic.
 */

#include <gtest/gtest.h>
#include "uvhttp_response_internal.h"

#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

namespace {

int g_error_calls = 0;
uvhttp_error_t g_last_error = UVHTTP_OK;

void OnError(uvhttp_server_t* server, uvhttp_error_t error,
             uvhttp_connection_t* conn, void* user_data) {
    (void)server;
    (void)conn;
    (void)user_data;
    ++g_error_calls;
    g_last_error = error;
}

int OkHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_response_set_body(resp, "ok", 2);
    return uvhttp_response_send(resp);
}

int ConnectTo(int port) {
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

void Pump(uv_loop_t* loop) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, [](uv_timer_t* t) { uv_stop((uv_loop_t*)t->data); },
                   10, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

class ServerErrorObserverTest : public ::testing::Test {
   protected:
    void SetUp() override {
        g_error_calls = 0;
        g_last_error = UVHTTP_OK;
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        ASSERT_EQ(uvhttp_server_set_error_callback(server_, OnError, nullptr),
                  UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/ok", OkHandler), UVHTTP_OK);
        uvhttp_server_take_router(server_, router);
        ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
        struct sockaddr_in addr;
        int namelen = sizeof(addr);
        memset(&addr, 0, sizeof(addr));
        ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                     (struct sockaddr*)&addr, &namelen),
                  0);
        port_ = ntohs(addr.sin_port);
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
    int port_ = 0;
};

}  // namespace

TEST_F(ServerErrorObserverTest, ParseErrorReachesObserver) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);
    const char bad[] = "NOT-A-METHOD /\r\nHost: x\r\n\r\n";
    ASSERT_EQ(send(fd, bad, strlen(bad), 0), (ssize_t)strlen(bad));
    for (int i = 0; i < 20 && g_error_calls == 0; ++i) {
        Pump(loop_);
    }
    close(fd);
    EXPECT_GE(g_error_calls, 1);
    /* Must be a real library error code, not a cast llhttp errno. */
    EXPECT_EQ(g_last_error, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServerErrorObserverTest, ValidRequestDoesNotFireObserver) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);
    const char good[] = "GET /ok HTTP/1.1\r\nHost: x\r\n\r\n";
    ASSERT_EQ(send(fd, good, strlen(good), 0), (ssize_t)strlen(good));
    for (int i = 0; i < 10; ++i) {
        Pump(loop_);
    }
    close(fd);
    EXPECT_EQ(g_error_calls, 0);
}

TEST_F(ServerErrorObserverTest, SetterRejectsNullServer) {
    EXPECT_EQ(uvhttp_server_set_error_callback(nullptr, OnError, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}
