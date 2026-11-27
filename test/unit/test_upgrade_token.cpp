/*
 * Upgrade detection must treat Connection as a token list, not a substring.
 * - "Connection: not-upgrade" must NOT trigger 101 (substring match did).
 * - a second "Connection: keep-alive" header before "Connection: Upgrade"
 *   must still trigger 101 (get_header returned only the first header).
 */

#include "uvhttp_features.h"

#if UVHTTP_FEATURE_WEBSOCKET

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_server.h"
#include "uvhttp_websocket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

namespace {

void Pump(uv_loop_t* loop, int ms);

std::string SendAndRead(int port, const char* request, uv_loop_t* loop) {
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
    for (int i = 0; i < 200 && out.find("\r\n\r\n") == std::string::npos;
         ++i) {
        Pump(loop, 5);
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
        out.append(buf, (size_t)n);
    }
    close(fd);
    return out;
}

void Pump(uv_loop_t* loop, int ms) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, [](uv_timer_t* t) { uv_stop((uv_loop_t*)t->data); },
                   ms, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

class UpgradeTokenTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_ws_handler_t handler;
        memset(&handler, 0, sizeof(handler));
        ASSERT_EQ(uvhttp_server_register_ws_handler(server_, "/ws", &handler),
                  UVHTTP_OK);
        ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
        struct sockaddr_in addr;
        int len = sizeof(addr);
        memset(&addr, 0, sizeof(addr));
        ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                     (struct sockaddr*)&addr, &len),
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

TEST_F(UpgradeTokenTest, SubstringConnectionValueDoesNotUpgrade) {
    const char req[] =
        "GET /ws HTTP/1.1\r\nHost: x\r\n"
        "Upgrade: websocket\r\n"
        "Connection: not-upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";
    std::string raw = SendAndRead(port_, req, loop_);
    EXPECT_EQ(raw.find("101"), std::string::npos);
}

TEST_F(UpgradeTokenTest, LaterConnectionHeaderWithUpgradeTokenUpgrades) {
    const char req[] =
        "GET /ws HTTP/1.1\r\nHost: x\r\n"
        "Upgrade: websocket\r\n"
        "Connection: keep-alive\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";
    std::string raw = SendAndRead(port_, req, loop_);
    EXPECT_NE(raw.find("101"), std::string::npos);
}

#endif
