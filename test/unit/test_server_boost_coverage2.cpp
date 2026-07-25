/**
 * @file test_server_boost_coverage2.cpp
 * @brief Additional coverage boost tests for uvhttp_server module
 *
 * Targets uncovered areas in src/uvhttp_server.c including:
 * - uvhttp_server_listen with context config (lines 467, 470-472)
 * - uvhttp_server_ws_broadcast with OPEN connections (lines 1474-1475)
 * - uvhttp_server_ws_close_all with non-null ws_conn (line 1512)
 * - ws_timeout_timer_callback (lines 1174-1208)
 * - ws_heartbeat_timer_callback (lines 1221-1255)
 * - uvhttp_server_ws_enable_connection_management success path (lines 1307-1349)
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_config.h"
}

#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

// ============================================================================
// Helper: dummy request handler
// ============================================================================
static int boost2_dummy_handler(uvhttp_request_t* request,
                                uvhttp_response_t* response) {
    (void)request;
    (void)response;
    return 0;
}

// ============================================================================
// Helper: pump the event loop for a bounded time using UV_RUN_NOWAIT
// ============================================================================
static void pump_loop(uv_loop_t* loop, int timeout_ms) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        uv_run(loop, UV_RUN_NOWAIT);
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                          (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= timeout_ms) break;
        usleep(1000);  // 1ms
    }
}

// ============================================================================
// Helper: close callback for libuv handles
// ============================================================================
static void boost2_on_close(uv_handle_t* handle) {
    (void)handle;
}

// Helper: walk callback to close all open handles
static void boost2_close_walk_cb(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, boost2_on_close);
    }
}

// ============================================================================
// uvhttp_server_listen with context config (lines 467, 470-472)
// ============================================================================
class ListenWithContextConfigTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    uvhttp_context_t* context = nullptr;
    uvhttp_config_t* config = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        // Walk and close any remaining handles
        uv_walk(&loop, boost2_close_walk_cb, nullptr);
        for (int i = 0; i < 20; i++) {
            if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
        }

        if (server) {
            // Null out context to prevent server_free from destroying it
            // (we manage context/config lifetime separately)
            server->context = nullptr;
            uvhttp_server_free(server);
            server = nullptr;
        }
        if (context) {
            // Clear the config reference in context before destroying,
            // since we own config separately
            uvhttp_config_set_current(context, nullptr);
            uvhttp_context_destroy(context);
            context = nullptr;
        }
        if (config) {
            uvhttp_config_free(config);
            config = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(ListenWithContextConfigTest, Listen_WithContextConfigBacklog) {
    // Create a context
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(context, nullptr);

    // Create a config with backlog > 0
    err = uvhttp_config_new(&config);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(config, nullptr);
    config->backlog = 256;

    // Set the config as current on the context
    uvhttp_config_set_current(context, config);

    // Set the context on the server
    err = uvhttp_server_set_context(server, context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Set a handler
    err = uvhttp_server_set_handler(server, boost2_dummy_handler);
    ASSERT_EQ(err, UVHTTP_OK);

    // Listen - this should use the context's config backlog (lines 467, 470-472)
    err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_EQ(server->is_listening, 1);

    // Verify the server is actually listening by getting the bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Stop the server
    uvhttp_server_stop(server);
    EXPECT_EQ(server->is_listening, 0);
}

TEST_F(ListenWithContextConfigTest, Listen_WithContextNullConfig) {
    // Create a context but don't set a config on it
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Set the context on the server
    err = uvhttp_server_set_context(server, context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Listen - context exists but config is NULL, uses default backlog
    err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_EQ(server->is_listening, 1);

    // Stop the server
    uvhttp_server_stop(server);
}

TEST_F(ListenWithContextConfigTest, Listen_WithContextConfigBacklogZero) {
    // Create a context
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Create a config with backlog = 0 (should use default)
    err = uvhttp_config_new(&config);
    ASSERT_EQ(err, UVHTTP_OK);
    config->backlog = 0;

    // Set the config as current on the context
    uvhttp_config_set_current(context, config);

    // Set the context on the server
    err = uvhttp_server_set_context(server, context);
    ASSERT_EQ(err, UVHTTP_OK);

    // Listen - config->backlog is 0, so default backlog is used
    err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_EQ(server->is_listening, 1);

    // Stop the server
    uvhttp_server_stop(server);
}

// ============================================================================
// WebSocket tests (gated by UVHTTP_FEATURE_WEBSOCKET)
// ============================================================================
#if UVHTTP_FEATURE_WEBSOCKET

// ============================================================================
// uvhttp_server_ws_broadcast with OPEN connections (lines 1474-1475)
// ============================================================================
class WsBroadcastOpenTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    int sock_fds[2]{-1, -1};

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        if (sock_fds[0] >= 0) { close(sock_fds[0]); sock_fds[0] = -1; }
        if (sock_fds[1] >= 0) { close(sock_fds[1]); sock_fds[1] = -1; }
        uv_loop_close(&loop);
    }
};

TEST_F(WsBroadcastOpenTest, Broadcast_OpenConnection) {
    // Create a socketpair so send() has a valid fd
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fds), 0);

    // Enable connection management
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    ASSERT_EQ(err, UVHTTP_OK);

    // Create a ws_connection in OPEN state with a valid fd
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = sock_fds[0];
    ws_conn.is_server = 1;

    // Add the connection to the manager
    uvhttp_server_ws_add_connection(server, &ws_conn, "/chat");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);

    // Broadcast to the path - exercises lines 1474-1475
    // (uvhttp_ws_send_text is called, then sent_count++)
    err = uvhttp_server_ws_broadcast(server, "/chat", "hello", 5);
    EXPECT_EQ(err, UVHTTP_OK);

    // Read the sent data from the other end of the socketpair to prevent buffer fill
    char buf[64];
    ssize_t n = recv(sock_fds[1], buf, sizeof(buf), 0);
    EXPECT_GT(n, 0);  // Should have received the WebSocket frame
}

TEST_F(WsBroadcastOpenTest, Broadcast_MultipleOpenConnections) {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fds), 0);

    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    ASSERT_EQ(err, UVHTTP_OK);

    // Add multiple connections in OPEN state
    uvhttp_ws_connection_t ws1{}, ws2{}, ws3{};
    int fds_a[2], fds_b[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds_a), 0);
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds_b), 0);

    ws1.state = UVHTTP_WS_STATE_OPEN;
    ws1.fd = sock_fds[0];
    ws1.is_server = 1;

    ws2.state = UVHTTP_WS_STATE_OPEN;
    ws2.fd = fds_a[0];
    ws2.is_server = 1;

    ws3.state = UVHTTP_WS_STATE_OPEN;
    ws3.fd = fds_b[0];
    ws3.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws1, "/chat");
    uvhttp_server_ws_add_connection(server, &ws2, "/chat");
    uvhttp_server_ws_add_connection(server, &ws3, "/other");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 3);

    // Broadcast to /chat - should send to ws1 and ws2 (both OPEN)
    err = uvhttp_server_ws_broadcast(server, "/chat", "hi", 2);
    EXPECT_EQ(err, UVHTTP_OK);

    // Broadcast with NULL path - should send to all OPEN connections
    err = uvhttp_server_ws_broadcast(server, nullptr, "all", 3);
    EXPECT_EQ(err, UVHTTP_OK);

    // Drain sockets
    char buf[64];
    while (recv(sock_fds[1], buf, sizeof(buf), MSG_DONTWAIT) > 0) {}
    while (recv(fds_a[1], buf, sizeof(buf), MSG_DONTWAIT) > 0) {}
    while (recv(fds_b[1], buf, sizeof(buf), MSG_DONTWAIT) > 0) {}

    close(fds_a[0]); close(fds_a[1]);
    close(fds_b[0]); close(fds_b[1]);
}

// ============================================================================
// uvhttp_server_ws_close_all with non-null ws_conn (line 1512)
// ============================================================================
class WsCloseAllWithConnTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    int sock_fds[2]{-1, -1};

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        if (sock_fds[0] >= 0) { close(sock_fds[0]); sock_fds[0] = -1; }
        if (sock_fds[1] >= 0) { close(sock_fds[1]); sock_fds[1] = -1; }
        uv_loop_close(&loop);
    }
};

TEST_F(WsCloseAllWithConnTest, CloseAll_WithNonNullWsConn) {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fds), 0);

    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    ASSERT_EQ(err, UVHTTP_OK);

    // Create a connection with non-null ws_conn so line 1512 is executed
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = sock_fds[0];
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/chat");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);

    // close_all with a matching path - exercises line 1512
    // uvhttp_ws_close(NULL, ws_conn, 1000, "Server closed connection") is called
    err = uvhttp_server_ws_close_all(server, "/chat");
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
}

TEST_F(WsCloseAllWithConnTest, CloseAll_NullPathClosesAll_WithWsConn) {
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fds), 0);
    int fds2[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds2), 0);

    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    ASSERT_EQ(err, UVHTTP_OK);

    uvhttp_ws_connection_t ws1{}, ws2{};
    ws1.state = UVHTTP_WS_STATE_OPEN;
    ws1.fd = sock_fds[0];
    ws1.is_server = 1;

    ws2.state = UVHTTP_WS_STATE_CLOSING;
    ws2.fd = fds2[0];
    ws2.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws1, "/chat");
    uvhttp_server_ws_add_connection(server, &ws2, "/other");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 2);

    // NULL path closes all connections - exercises line 1512 for both nodes
    err = uvhttp_server_ws_close_all(server, nullptr);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);

    close(fds2[0]); close(fds2[1]);
}

// ============================================================================
// ws_timeout_timer_callback (lines 1174-1208)
// ============================================================================
class WsTimeoutTimerTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(WsTimeoutTimerTest, TimeoutTimer_ClosesTimedOutConnection) {
    // Enable with minimum timeout (10s) and heartbeat (5s)
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 10, 300);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add a connection with a very old last_activity
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = -1;  // Will fail send, but timeout just needs to detect staleness
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/ws");
    EXPECT_EQ(mgr->connection_count, 1);

    // Set last_activity to 0 so current_time - 0 > timeout_ms immediately
    mgr->connections->last_activity = 0;

    // Restart the timeout timer with a very short initial delay (10ms)
    // Access the callback stored in the timer handle's private fields
    uv_timer_cb timeout_cb = mgr->timeout_timer.timer_cb;
    ASSERT_NE(timeout_cb, nullptr);
    uv_timer_stop(&mgr->timeout_timer);
    int ret = uv_timer_start(&mgr->timeout_timer, timeout_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    // Pump the loop so the timer fires and the callback executes
    pump_loop(&loop, 200);

    // The timeout callback should have closed and removed the connection
    EXPECT_EQ(mgr->connection_count, 0);
}

TEST_F(WsTimeoutTimerTest, TimeoutTimer_KeepsActiveConnection) {
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 10, 300);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add a connection with a recent last_activity (will be set by add_connection)
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = -1;
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/ws");
    EXPECT_EQ(mgr->connection_count, 1);
    // last_activity was set to current time by add_connection

    // Restart the timeout timer with a short delay
    uv_timer_cb timeout_cb = mgr->timeout_timer.timer_cb;
    ASSERT_NE(timeout_cb, nullptr);
    uv_timer_stop(&mgr->timeout_timer);
    int ret = uv_timer_start(&mgr->timeout_timer, timeout_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    // Pump the loop so the timer fires
    pump_loop(&loop, 200);

    // The connection should NOT have been removed (it's still active)
    EXPECT_EQ(mgr->connection_count, 1);
}

TEST_F(WsTimeoutTimerTest, TimeoutTimer_MultipleConnections) {
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 10, 300);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add 3 connections: 2 stale, 1 active
    uvhttp_ws_connection_t ws1{}, ws2{}, ws3{};
    ws1.state = UVHTTP_WS_STATE_OPEN; ws1.fd = -1; ws1.is_server = 1;
    ws2.state = UVHTTP_WS_STATE_OPEN; ws2.fd = -1; ws2.is_server = 1;
    ws3.state = UVHTTP_WS_STATE_OPEN; ws3.fd = -1; ws3.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws1, "/ws");
    uvhttp_server_ws_add_connection(server, &ws2, "/ws");
    uvhttp_server_ws_add_connection(server, &ws3, "/ws");
    EXPECT_EQ(mgr->connection_count, 3);

    // Set first two connections' last_activity to 0 (stale)
    // List is: ws3 -> ws2 -> ws1 (add prepends)
    ws_connection_node_t* node = mgr->connections;
    ASSERT_NE(node, nullptr);
    // ws3 (head) - keep active
    node = node->next;
    ASSERT_NE(node, nullptr);
    node->last_activity = 0;  // ws2 - stale
    node = node->next;
    ASSERT_NE(node, nullptr);
    node->last_activity = 0;  // ws1 - stale

    // Restart timeout timer with short delay
    uv_timer_cb timeout_cb = mgr->timeout_timer.timer_cb;
    ASSERT_NE(timeout_cb, nullptr);
    uv_timer_stop(&mgr->timeout_timer);
    int ret = uv_timer_start(&mgr->timeout_timer, timeout_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    pump_loop(&loop, 200);

    // Only the active connection should remain
    EXPECT_EQ(mgr->connection_count, 1);
}

// ============================================================================
// ws_heartbeat_timer_callback (lines 1221-1255)
// ============================================================================
class WsHeartbeatTimerTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    int sock_fds[2]{-1, -1};

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fds), 0);
    }

    void TearDown() override {
        if (server) {
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        if (sock_fds[0] >= 0) { close(sock_fds[0]); sock_fds[0] = -1; }
        if (sock_fds[1] >= 0) { close(sock_fds[1]); sock_fds[1] = -1; }
        uv_loop_close(&loop);
    }
};

TEST_F(WsHeartbeatTimerTest, Heartbeat_SendsPingToOpenConnection) {
    // Enable with long timeout (so timeout doesn't fire) and short heartbeat
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 3600, 5);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add a connection in OPEN state with a valid fd for send()
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = sock_fds[0];
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/ws");
    EXPECT_EQ(mgr->connection_count, 1);
    EXPECT_EQ(mgr->connections->ping_pending, 0);

    // Restart the heartbeat timer with a short delay (10ms)
    uv_timer_cb heartbeat_cb = mgr->heartbeat_timer.timer_cb;
    ASSERT_NE(heartbeat_cb, nullptr);
    uv_timer_stop(&mgr->heartbeat_timer);
    int ret = uv_timer_start(&mgr->heartbeat_timer, heartbeat_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    // Pump the loop so the heartbeat fires
    pump_loop(&loop, 200);

    // The heartbeat should have sent a ping and set ping_pending = 1
    EXPECT_EQ(mgr->connections->ping_pending, 1);

    // Drain any data sent to the socketpair
    char buf[64];
    while (recv(sock_fds[1], buf, sizeof(buf), MSG_DONTWAIT) > 0) {}
}

TEST_F(WsHeartbeatTimerTest, Heartbeat_PingTimeoutClosesConnection) {
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 3600, 5);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add a connection in OPEN state
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_OPEN;
    ws_conn.fd = sock_fds[0];
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/ws");
    EXPECT_EQ(mgr->connection_count, 1);

    // Simulate that a ping was already sent and has timed out
    mgr->connections->ping_pending = 1;
    mgr->connections->last_ping_sent = 0;  // very old timestamp
    mgr->ping_timeout_ms = 0;              // timeout immediately

    // Restart the heartbeat timer with a short delay
    uv_timer_cb heartbeat_cb = mgr->heartbeat_timer.timer_cb;
    ASSERT_NE(heartbeat_cb, nullptr);
    uv_timer_stop(&mgr->heartbeat_timer);
    int ret = uv_timer_start(&mgr->heartbeat_timer, heartbeat_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    // Pump the loop so the heartbeat fires
    pump_loop(&loop, 200);

    // The heartbeat should have detected the ping timeout and closed the connection
    // (sets state to CLOSING, but node remains in list)
    EXPECT_EQ(ws_conn.state, UVHTTP_WS_STATE_CLOSING);
}

TEST_F(WsHeartbeatTimerTest, Heartbeat_SkipsNonOpenConnection) {
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 3600, 5);
    ASSERT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Add a connection in CLOSING state (not OPEN)
    uvhttp_ws_connection_t ws_conn{};
    ws_conn.state = UVHTTP_WS_STATE_CLOSING;
    ws_conn.fd = sock_fds[0];
    ws_conn.is_server = 1;

    uvhttp_server_ws_add_connection(server, &ws_conn, "/ws");
    EXPECT_EQ(mgr->connection_count, 1);
    EXPECT_EQ(mgr->connections->ping_pending, 0);

    // Restart heartbeat timer
    uv_timer_cb heartbeat_cb = mgr->heartbeat_timer.timer_cb;
    ASSERT_NE(heartbeat_cb, nullptr);
    uv_timer_stop(&mgr->heartbeat_timer);
    int ret = uv_timer_start(&mgr->heartbeat_timer, heartbeat_cb, 10, 60000);
    ASSERT_EQ(ret, 0);

    pump_loop(&loop, 200);

    // ping_pending should remain 0 since connection is not OPEN
    EXPECT_EQ(mgr->connections->ping_pending, 0);
}

// ============================================================================
// uvhttp_server_ws_enable_connection_management success path (lines 1307-1349)
// ============================================================================
class WsEnableConnMgmtSuccessTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(WsEnableConnMgmtSuccessTest, Enable_SuccessPath) {
    // Call with valid parameters - exercises lines 1307-1349
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(err, UVHTTP_OK);

    // Verify the manager was set up correctly
    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(mgr->timeout_seconds, 60);
    EXPECT_EQ(mgr->heartbeat_interval, 30);
    EXPECT_EQ(mgr->ping_timeout_ms, (uint64_t)10000);
    EXPECT_EQ(mgr->enabled, 1);
    EXPECT_EQ(mgr->connection_count, 0);
    EXPECT_EQ(mgr->connections, nullptr);
}

TEST_F(WsEnableConnMgmtSuccessTest, Enable_MinBoundaries) {
    // Minimum valid values: timeout=10, heartbeat=5
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 10, 5);
    EXPECT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(mgr->timeout_seconds, 10);
    EXPECT_EQ(mgr->heartbeat_interval, 5);
}

TEST_F(WsEnableConnMgmtSuccessTest, Enable_MaxBoundaries) {
    // Maximum valid values: timeout=3600, heartbeat=300
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 3600, 300);
    EXPECT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(mgr->timeout_seconds, 3600);
    EXPECT_EQ(mgr->heartbeat_interval, 300);
}

TEST_F(WsEnableConnMgmtSuccessTest, Enable_TimersAreInitialized) {
    uvhttp_error_t err =
        uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(err, UVHTTP_OK);

    ws_connection_manager_t* mgr = server->ws_connection_manager;
    ASSERT_NE(mgr, nullptr);

    // Verify timers are active by checking they have valid handles
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&mgr->timeout_timer));
    EXPECT_FALSE(uv_is_closing((uv_handle_t*)&mgr->heartbeat_timer));

    // Verify timer callbacks are set
    EXPECT_NE(mgr->timeout_timer.timer_cb, nullptr);
    EXPECT_NE(mgr->heartbeat_timer.timer_cb, nullptr);
}

#endif  // UVHTTP_FEATURE_WEBSOCKET
