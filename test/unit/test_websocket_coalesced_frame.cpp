/*
 * A client is allowed to put its first WebSocket frame in the same TCP write
 * as the HTTP upgrade request. llhttp stops at the upgrade boundary and leaves
 * those bytes unconsumed; the old on_read cleared read_buffer_used
 * unconditionally, so the frame was silently dropped and the application saw
 * nothing until the client sent another frame.
 *
 * This drives a real socket and asserts the application's on_message callback
 * fires for a frame that was coalesced with the handshake.
 */

#include "uvhttp_features.h"

#if UVHTTP_FEATURE_WEBSOCKET

#    include "uvhttp_allocator.h"
#    include "uvhttp_error.h"
#    include "uvhttp_server.h"
#    include "uvhttp_websocket.h"

#    include <arpa/inet.h>
#    include <gtest/gtest.h>
#    include <netinet/in.h>
#    include <poll.h>
#    include <stdio.h>
#    include <string.h>
#    include <string>
#    include <sys/socket.h>
#    include <unistd.h>
#    include <uv.h>

namespace {

int g_message_calls = 0;
std::string g_message_payload;

int OnMessage(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len,
              int opcode) {
    (void)ws_conn;
    (void)opcode;
    ++g_message_calls;
    g_message_payload.assign(data, len);
    return 0;
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

void Pump(uv_loop_t* loop) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, StopLoop, 5, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

}  // namespace

class WebSocketCoalescedFrameTest : public ::testing::Test {
   protected:
    void SetUp() override {
        g_message_calls = 0;
        g_message_payload.clear();
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        /* server_new already registers it; calling again is idempotent-ish and
         * reports ALREADY_EXISTS, which is the state we want. */
        uvhttp_error_t upgrade_rc =
            uvhttp_server_register_websocket_upgrade(server_);
        ASSERT_TRUE(upgrade_rc == UVHTTP_OK ||
                    upgrade_rc == UVHTTP_ERROR_ALREADY_EXISTS);

        uvhttp_ws_handler_t handler;
        memset(&handler, 0, sizeof(handler));
        handler.on_message = OnMessage;
        ASSERT_EQ(uvhttp_server_register_ws_handler(server_, "/ws", &handler),
                  UVHTTP_OK);

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

TEST_F(WebSocketCoalescedFrameTest, FirstFrameInHandshakeWriteIsDelivered) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);

    static const char kHandshake[] =
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    /* Masked TEXT frame, payload "HELLO". Zero mask keeps the payload plain. */
    static const unsigned char kFrame[] = {
        0x81, 0x85, 0x00, 0x00, 0x00, 0x00,
        'H',  'E',  'L',  'L',  'O'};

    std::string wire(kHandshake, sizeof(kHandshake) - 1);
    wire.append((const char*)kFrame, sizeof(kFrame));
    ASSERT_EQ(send(fd, wire.data(), wire.size(), 0), (ssize_t)wire.size());

    /* Drain the loop; the server must parse the handshake, switch to WS mode,
     * and process the trailing frame in the same read buffer. */
    char buf[4096];
    for (int i = 0; i < 100 && g_message_calls == 0; ++i) {
        Pump(loop_);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 10) > 0) {
            (void)recv(fd, buf, sizeof(buf), 0);
        }
    }
    close(fd);

    EXPECT_EQ(g_message_calls, 1);
    EXPECT_EQ(g_message_payload, "HELLO");
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */
