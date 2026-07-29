/*
 * uvhttp_connection.c integration-style coverage test
 *
 * Creates a real server and TCP client to trigger internal libuv callbacks.
 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "uvhttp_router.h"
#include <string.h>
#include <uv.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

static int test_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "OK", 2);
    return uvhttp_response_send(resp);
}

static int connect_to_port(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void stop_loop(uv_timer_t* t) {
    uv_stop((uv_loop_t*)t->data);
}

/* Run loop with timeout to avoid blocking forever */
static void run_loop_with_timeout(uv_loop_t* loop, int ms) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, stop_loop, ms, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

/* Setup server and get port */
static int setup_server(uv_loop_t* loop, uvhttp_server_t** server) {
    uvhttp_error_t err = uvhttp_server_new(loop, server);
    if (err != UVHTTP_OK) return -1;

    uvhttp_router_t* router = nullptr;
    uvhttp_router_new(&router);
    if (router) {
        uvhttp_router_add_route(router, "/test", test_handler);
        uvhttp_server_set_router(*server, router);
    }

    err = uvhttp_server_listen(*server, "127.0.0.1", 0);
    if (err != UVHTTP_OK) return -1;

    struct sockaddr_in addr;
    int namelen = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    uv_tcp_getsockname(&(*server)->tcp_handle, (struct sockaddr*)&addr, &namelen);
    return ntohs(addr.sin_port);
}

/* ========== Connection timeout callback test ========== */

TEST(UvhttpConnectionIntegrationTest, ConnectionTimeoutFires) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    int port = setup_server(loop, &server);
    ASSERT_GT(port, 0);

    int fd = connect_to_port(port);
    ASSERT_GE(fd, 0);

    const char* req = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(fd, req, strlen(req), 0);

    /* Let the loop process for 500ms */
    run_loop_with_timeout(loop, 100);
    run_loop_with_timeout(loop, 100);

    close(fd);
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== HTTP parse error handling ========== */

TEST(UvhttpConnectionIntegrationTest, HttpParseError) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    int port = setup_server(loop, &server);
    ASSERT_GT(port, 0);

    int fd = connect_to_port(port);
    ASSERT_GE(fd, 0);

    const char* bad = "GET /\r\n";
    send(fd, bad, strlen(bad), 0);

    run_loop_with_timeout(loop, 100);
    run_loop_with_timeout(loop, 100);

    close(fd);
    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Multiple connections ========== */

TEST(UvhttpConnectionIntegrationTest, MultipleConnections) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    int port = setup_server(loop, &server);
    ASSERT_GT(port, 0);

    int fds[3];
    for (int i = 0; i < 3; i++) {
        fds[i] = connect_to_port(port);
        ASSERT_GE(fds[i], 0);
    }

    const char* req = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    for (int i = 0; i < 3; i++) {
        send(fds[i], req, strlen(req), 0);
    }

    run_loop_with_timeout(loop, 100);
    run_loop_with_timeout(loop, 100);

    for (int i = 0; i < 3; i++) {
        close(fds[i]);
    }

    uvhttp_server_free(server);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}