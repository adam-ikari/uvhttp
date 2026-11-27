/*
 * Oversized request bodies used to return -1 from on_body -> llhttp error ->
 * the server closed the socket with no response. The client could not tell
 * a rejected body from a dropped connection. This drives a real socket with
 * a body above UVHTTP_MAX_BODY_SIZE and asserts a 413 status line.
 */

#include <gtest/gtest.h>

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
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

namespace {

int HandlerCalls = 0;

int EchoHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    ++HandlerCalls;
    return uvhttp_respond(resp, 200, "text/plain", "accepted", 8);
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

class RequestBodyLimitTest : public ::testing::Test {
   protected:
    void SetUp() override {
        HandlerCalls = 0;
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/upload", EchoHandler),
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
    void Pump() {
        uv_timer_t timer;
        uv_timer_init(loop_, &timer);
        timer.data = loop_;
        uv_timer_start(&timer, [](uv_timer_t* t) { uv_stop((uv_loop_t*)t->data); },
                       5, 0);
        uv_run(loop_, UV_RUN_DEFAULT);
        uv_close((uv_handle_t*)&timer, NULL);
        uv_run(loop_, UV_RUN_NOWAIT);
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
    int port_ = 0;
};

}  // namespace

TEST_F(RequestBodyLimitTest, OversizedBodyGets413) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);

    const size_t body_len = UVHTTP_MAX_BODY_SIZE + 1;
    std::string request = "POST /upload HTTP/1.1\r\nHost: localhost\r\n"
                          "Content-Length: " +
                          std::to_string(body_len) + "\r\n\r\n";
    request.append(body_len, 'x');
    ASSERT_EQ(send(fd, request.data(), request.size(), 0),
              (ssize_t)request.size());

    std::string raw;
    char buf[4096];
    for (int i = 0; i < 400 && raw.find("\r\n\r\n") == std::string::npos;
         ++i) {
        Pump();
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 10) <= 0) {
            continue;
        }
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        raw.append(buf, (size_t)n);
    }
    close(fd);

    EXPECT_NE(raw.find("413"), std::string::npos)
        << "no 413 status in response: [" << raw.substr(0, 80) << "]";
    EXPECT_EQ(HandlerCalls, 0) << "handler ran for a rejected body";
}
