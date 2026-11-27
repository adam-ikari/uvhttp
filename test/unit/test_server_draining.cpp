/*
 * Draining mode for rolling deploys: new requests are refused with 503 before
 * any user code runs; turning it off restores normal dispatch.
 */

#include <gtest/gtest.h>
#include "uvhttp_response_internal.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

namespace {

int g_handler_calls = 0;
uv_loop_t* g_loop = nullptr;

int CountingHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    ++g_handler_calls;
    uvhttp_response_set_body(resp, "ok", 2);
    return uvhttp_response_send(resp);
}

static void Pump(uv_loop_t* loop);

std::string Exchange(int port, const char* request) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return "";
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return "";
    }
    send(fd, request, strlen(request), 0);
    std::string out;
    char buf[4096];
    for (int i = 0; i < 200; ++i) {
        Pump(g_loop);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        int ready = poll(&pfd, 1, 10);
        if (ready <= 0) {
            continue;
        }
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        out.append(buf, (size_t)n);
        if (out.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    close(fd);
    return out;
}

static void Pump(uv_loop_t* loop) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, [](uv_timer_t* t) { uv_stop((uv_loop_t*)t->data); },
                   5, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

class ServerDrainingTest : public ::testing::Test {
   protected:
    void SetUp() override {
        g_handler_calls = 0;
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        g_loop = loop_;
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/", CountingHandler),
                  UVHTTP_OK);
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

TEST_F(ServerDrainingTest, DrainingRejectsBeforeHandler) {
    ASSERT_EQ(uvhttp_server_set_draining(server_, 1), UVHTTP_OK);
    /* Exchange pumps the loop and reads until the header block is complete,
     * so a split TCP segment cannot produce a false failure. */
    std::string raw = Exchange(port_, "GET / HTTP/1.1\r\nHost: x\r\n\r\n");
    EXPECT_NE(raw.find("503"), std::string::npos);
    EXPECT_NE(raw.find("Connection: close"), std::string::npos);
    EXPECT_EQ(g_handler_calls, 0);
}

TEST_F(ServerDrainingTest, LeavingDrainingRestoresDispatch) {
    ASSERT_EQ(uvhttp_server_set_draining(server_, 1), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_set_draining(server_, 0), UVHTTP_OK);
    std::string raw = Exchange(port_, "GET / HTTP/1.1\r\nHost: x\r\n\r\n");
    EXPECT_NE(raw.find("200"), std::string::npos);
    EXPECT_EQ(g_handler_calls, 1);
}

TEST_F(ServerDrainingTest, NullServerRejected) {
    EXPECT_EQ(uvhttp_server_set_draining(nullptr, 1),
              UVHTTP_ERROR_INVALID_PARAM);
}
