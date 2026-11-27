/*
 * Wire-level HTTP response semantics: HEAD and Connection: close.
 *
 * Both are RFC MUSTs that a handler-level unit test cannot see: the decision
 * is made while the response is built and while the write-completion callback
 * decides whether to restart reads or close the socket. These tests drive a
 * real server over a real socket and inspect the exact bytes and EOF.
 */

#include "uvhttp_error.h"
#include "uvhttp_response_internal.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

namespace {

const char kBody[] = "BODY-BYTES-1234567890";

int EchoHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, kBody, strlen(kBody));
    return uvhttp_response_send(resp);
}

const char kMultiBody[] = "multi";

int MultiHeaderHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_header_view_t headers[] = {
        {"Content-Type", "application/json"},
        {"X-Request-Id", "abc-123"},
    };
    return uvhttp_respond_with_headers(resp, 200, headers, 2, kMultiBody,
                                       strlen(kMultiBody));
}

const char kAtomicBody[] = "{\"ok\":1}";

int AtomicHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    return uvhttp_respond(resp, 201, "application/json", kAtomicBody,
                          strlen(kAtomicBody));
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

void StopLoop(uv_timer_t* t) {
    uv_stop((uv_loop_t*)t->data);
}

void Pump(uv_loop_t* loop, int ms) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, StopLoop, ms, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

struct RawResponse {
    std::string headers;
    std::string body;
    size_t declared_length = 0;
    bool closed = false;
};

/* Send a caller-supplied request, pump the loop until the header block is
 * complete, then collect exactly Content-Length body bytes. `expect_body`
 * controls whether the body is read: HEAD must declare a length but must not
 * put matching bytes on the wire. */
bool Exchange(uv_loop_t* loop, int port, const std::string& request,
              bool expect_body, RawResponse* out) {
    int fd = ConnectTo(port);
    if (fd < 0) {
        return false;
    }
    if (send(fd, request.data(), request.size(), 0) != (ssize_t)request.size()) {
        close(fd);
        return false;
    }

    std::string raw;
    char buf[4096];
    bool have_headers = false;
    for (int i = 0; i < 200 && !have_headers; ++i) {
        Pump(loop, 5);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 20) <= 0) {
            continue;
        }
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n < 0) {
            close(fd);
            return false;
        }
        if (n == 0) {
            out->closed = true;
            break;
        }
        raw.append(buf, (size_t)n);
        size_t sep = raw.find("\r\n\r\n");
        if (sep != std::string::npos) {
            out->headers = raw.substr(0, sep);
            const std::string key = "Content-Length: ";
            size_t at = out->headers.find(key);
            if (at == std::string::npos) {
                close(fd);
                return false;
            }
            out->declared_length =
                (size_t)atol(out->headers.c_str() + at + key.size());
            have_headers = true;
        }
    }
    if (!have_headers) {
        close(fd);
        return false;
    }

    size_t body_start = raw.find("\r\n\r\n") + 4;
    if (expect_body) {
        while (raw.size() < body_start + out->declared_length) {
            Pump(loop, 5);
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLIN;
            int ready = poll(&pfd, 1, 50);
            if (ready < 0) {
                break;
            }
            if (ready == 0) {
                continue;
            }
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n <= 0) {
                if (n == 0) {
                    out->closed = true;
                }
                break;
            }
            raw.append(buf, (size_t)n);
        }
        out->body = raw.substr(body_start);
        /* Distinguish "server closed after the response" from "socket left
         * open for another request" without relying on a timeout alone. */
        for (int i = 0; i < 20 && !out->closed; ++i) {
            Pump(loop, 5);
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLIN;
            int ready = poll(&pfd, 1, 20);
            if (ready <= 0) {
                continue;
            }
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n == 0) {
                out->closed = true;
            }
        }
    } else {
        /* HEAD: give any misplaced body bytes a chance to arrive before we
         * conclude none were sent. */
        for (int i = 0; i < 10; ++i) {
            Pump(loop, 5);
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
            raw.append(buf, (size_t)n);
        }
        out->body = raw.substr(body_start);
    }
    close(fd);
    return true;
}

}  // namespace

class HttpResponseSemanticsTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/echo", EchoHandler),
                  UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/atomic", AtomicHandler),
                  UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/multi", MultiHeaderHandler),
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
        uv_run(loop_, UV_RUN_NOWAIT);
        uv_loop_close(loop_);
        uvhttp_free(loop_);
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
    int port_ = 0;
};

TEST_F(HttpResponseSemanticsTest, HeadDeclaresLengthButSendsNoBody) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "HEAD /echo HTTP/1.1\r\nHost: localhost\r\n\r\n",
                         /*expect_body=*/false, &resp));
    EXPECT_EQ(resp.declared_length, strlen(kBody));
    EXPECT_TRUE(resp.body.empty())
        << "HEAD response leaked " << resp.body.size() << " body bytes";
}

TEST_F(HttpResponseSemanticsTest, ConnectionCloseClosesAfterResponse) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /echo HTTP/1.1\r\nHost: localhost\r\n"
                         "Connection: close\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("Connection: close"), std::string::npos);
    EXPECT_EQ(resp.body, kBody);
    /* The socket must be closed by the server, not merely left idle. */
    EXPECT_TRUE(resp.closed);
}

TEST_F(HttpResponseSemanticsTest, Http10DefaultsToClose) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /echo HTTP/1.0\r\nHost: localhost\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("Connection: close"), std::string::npos);
    EXPECT_TRUE(resp.closed);
}

TEST_F(HttpResponseSemanticsTest, Http10KeepAliveTokenStaysOpen) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /echo HTTP/1.0\r\nHost: localhost\r\n"
                         "Connection: keep-alive\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("Connection: keep-alive"), std::string::npos);
    EXPECT_FALSE(resp.closed);
}

TEST_F(HttpResponseSemanticsTest, AtomicRespondSendsStatusTypeAndBody) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /atomic HTTP/1.1\r\nHost: localhost\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("HTTP/1.1 201"), std::string::npos);
    EXPECT_NE(resp.headers.find("Content-Type: application/json"),
              std::string::npos);
    EXPECT_EQ(resp.body, kAtomicBody);
    EXPECT_EQ(resp.declared_length, strlen(kAtomicBody));
}

TEST_F(HttpResponseSemanticsTest, AtomicRespondWithSendsAllHeaders) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /multi HTTP/1.1\r\nHost: localhost\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("Content-Type: application/json"),
              std::string::npos);
    EXPECT_NE(resp.headers.find("X-Request-Id: abc-123"), std::string::npos);
    EXPECT_EQ(resp.body, kMultiBody);
}

TEST_F(HttpResponseSemanticsTest, Http11DefaultStaysKeepAlive) {
    RawResponse resp;
    ASSERT_TRUE(Exchange(loop_, port_,
                         "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n",
                         /*expect_body=*/true, &resp));
    EXPECT_NE(resp.headers.find("Connection: keep-alive"), std::string::npos);
    EXPECT_FALSE(resp.closed);
}
