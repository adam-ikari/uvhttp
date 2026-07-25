/**
 * @file test_protocol_upgrade_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_protocol_upgrade module
 *
 * Tests protocol registration/unregistration, null checks, and error paths.
 * Connection-level functions (transfer_ownership, get_fd, get_peer_address)
 * require real libuv handles so only null checks are tested for those.
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_protocol_upgrade.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
}

#include <string.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>
#include <arpa/inet.h>

// Dummy detector and handler for testing
static int test_detector(uvhttp_request_t* request, char* protocol_name,
                         size_t protocol_name_len, const char* upgrade_header,
                         const char* connection_header) {
    (void)request;
    (void)upgrade_header;
    (void)connection_header;
    if (protocol_name && protocol_name_len > 0) {
        strncpy(protocol_name, "test-proto", protocol_name_len - 1);
        protocol_name[protocol_name_len - 1] = '\0';
    }
    return 1;
}

static uvhttp_error_t test_handler(uvhttp_connection_t* conn,
                                   const char* protocol_name, void* user_data) {
    (void)conn;
    (void)protocol_name;
    (void)user_data;
    return UVHTTP_OK;
}

class ProtocolUpgradeTest : public ::testing::Test {
protected:
    uv_loop_t loop;
    uvhttp_server_t* server = nullptr;

    void SetUp() override {
        int err = uv_loop_init(&loop);
        ASSERT_EQ(err, 0);
        uvhttp_error_t serr = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(serr, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            // Clean up protocol registry before freeing server
            if (server->protocol_registry) {
                uvhttp_protocol_registry_t* reg =
                    (uvhttp_protocol_registry_t*)server->protocol_registry;
                uvhttp_protocol_info_t* p = reg->protocols;
                while (p) {
                    uvhttp_protocol_info_t* next = p->next;
                    uvhttp_free(p);
                    p = next;
                }
                uvhttp_free(reg);
                server->protocol_registry = nullptr;
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// ========== uvhttp_server_register_protocol_upgrade ==========

TEST_F(ProtocolUpgradeTest, Register_NullServer_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  nullptr, "test", "websocket", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, nullptr, "websocket", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullDetector_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", "websocket", nullptr, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullHandler_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", "websocket", test_detector, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NameTooLong_ReturnsError) {
    char long_name[64];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, long_name, nullptr, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_UpgradeHeaderTooLong_ReturnsError) {
    char long_header[128];
    memset(long_header, 'b', sizeof(long_header) - 1);
    long_header[sizeof(long_header) - 1] = '\0';
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", long_header, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_Valid_CreatesRegistry) {
    // Server pre-registers websocket, so use a different name
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "custom-proto", "custom-upgrade", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_NE(server->protocol_registry, nullptr);
}

TEST_F(ProtocolUpgradeTest, Register_NullUpgradeHeader_Success) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "custom", nullptr, test_detector, test_handler, nullptr),
              UVHTTP_OK);
}

TEST_F(ProtocolUpgradeTest, Register_DuplicateName_ReturnsAlreadyExists) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "myproto", "myupgrade", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "myproto", "myupgrade", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_ALREADY_EXISTS);
}

TEST_F(ProtocolUpgradeTest, Register_NameNormalizedToLowercase) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "MyCustomProto", "MyUpgradeHeader", test_detector, test_handler, nullptr),
              UVHTTP_OK);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    uvhttp_protocol_info_t* info = reg->protocols;
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->name, "mycustomproto");
    EXPECT_STREQ(info->upgrade_header, "myupgradeheader");
}

TEST_F(ProtocolUpgradeTest, Register_MultipleProtocols_InsertedAtHead) {
    // Server already has websocket registered, so count starts at 1
    size_t initial_count = 0;
    {
        uvhttp_protocol_registry_t* reg =
            (uvhttp_protocol_registry_t*)server->protocol_registry;
        if (reg) initial_count = reg->protocol_count;
    }

    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto1", "upgrade1", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto2", "upgrade2", test_detector, test_handler, (void*)0x42),
              UVHTTP_OK);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    EXPECT_EQ(reg->protocol_count, initial_count + 2);

    // Newest protocol is at head
    EXPECT_STREQ(reg->protocols->name, "proto2");
    EXPECT_EQ(reg->protocols->user_data, (void*)0x42);
}

TEST_F(ProtocolUpgradeTest, Register_MaxProtocols_ReturnsError) {
    // Server already has websocket registered (1), register 9 more to reach 10
    for (int i = 0; i < 9; i++) {
        char name[32];
        snprintf(name, sizeof(name), "proto%d", i);
        EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                      server, name, nullptr, test_detector, test_handler, nullptr),
                  UVHTTP_OK);
    }
    // 11th total (10 + 1 existing) should fail
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto_extra", nullptr, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_server_unregister_protocol_upgrade ==========

TEST_F(ProtocolUpgradeTest, Unregister_NullServer_ReturnsError) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(nullptr, "test"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Unregister_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Unregister_NoRegistry_ReturnsNotFound) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "test"),
              UVHTTP_ERROR_NOT_FOUND);
}

TEST_F(ProtocolUpgradeTest, Unregister_NotFound_ReturnsNotFound) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "nonexistent"),
              UVHTTP_ERROR_NOT_FOUND);
}

TEST_F(ProtocolUpgradeTest, Unregister_Valid_RemovesProtocol) {
    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t initial = reg->protocol_count;

    uvhttp_server_register_protocol_upgrade(
        server, "tempproto", "tempupgrade", test_detector, test_handler, nullptr);
    EXPECT_EQ(reg->protocol_count, initial + 1);

    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "tempproto"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, initial);
}

TEST_F(ProtocolUpgradeTest, Unregister_MiddleOfList_Works) {
    uvhttp_server_register_protocol_upgrade(
        server, "aaa", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "bbb", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "ccc", nullptr, test_detector, test_handler, nullptr);

    // List is: ccc -> bbb -> aaa -> (existing protocols)
    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t count_before = reg->protocol_count;

    // Remove middle
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "bbb"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, count_before - 1);
    EXPECT_STREQ(reg->protocols->name, "ccc");
}

TEST_F(ProtocolUpgradeTest, Unregister_HeadOfList_Works) {
    uvhttp_server_register_protocol_upgrade(
        server, "first", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "second", nullptr, test_detector, test_handler, nullptr);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t count_before = reg->protocol_count;

    // Remove head (second, which was inserted last)
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "second"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, count_before - 1);
    EXPECT_STREQ(reg->protocols->name, "first");
}

// ========== uvhttp_connection_set_lifecycle ==========

TEST_F(ProtocolUpgradeTest, SetLifecycle_NullConn_ReturnsError) {
    uvhttp_connection_lifecycle_t lifecycle = {nullptr, nullptr};
    EXPECT_EQ(uvhttp_connection_set_lifecycle(nullptr, &lifecycle),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, SetLifecycle_NullLifecycle_ReturnsError) {
    // Need a non-null conn pointer - use stack address as dummy
    // This will crash if the function tries to access conn fields
    // so we only test the null lifecycle path
    // Actually, the function checks !conn first, so we need a valid conn
    // Just test null lifecycle
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_set_lifecycle(&dummy_conn, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_get_fd ==========

TEST_F(ProtocolUpgradeTest, GetFd_NullConn_ReturnsError) {
    int fd = 0;
    EXPECT_EQ(uvhttp_connection_get_fd(nullptr, &fd), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetFd_NullFd_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_get_fd(&dummy_conn, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_get_peer_address ==========

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullConn_ReturnsError) {
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    EXPECT_EQ(uvhttp_connection_get_peer_address(nullptr, &addr, &len),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullAddr_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    socklen_t len = sizeof(struct sockaddr_storage);
    EXPECT_EQ(uvhttp_connection_get_peer_address(&dummy_conn, nullptr, &len),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullLen_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    struct sockaddr_storage addr;
    EXPECT_EQ(uvhttp_connection_get_peer_address(&dummy_conn, &addr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_transfer_ownership ==========

TEST_F(ProtocolUpgradeTest, TransferOwnership_NullConn_ReturnsError) {
    EXPECT_EQ(uvhttp_connection_transfer_ownership(nullptr, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, TransferOwnership_NullCallback_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_transfer_ownership(&dummy_conn, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_transfer_ownership wrong state ==========

TEST_F(ProtocolUpgradeTest, TransferOwnership_WrongState_ReturnsError) {
    // Create a real connection via server
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    // Connection starts in UVHTTP_CONN_STATE_NEW (not HTTP_PROCESSING)
    // transfer_ownership should reject this state
    auto dummy_callback = [](uv_tcp_t*, int, void*) {};
    result = uvhttp_connection_transfer_ownership(
        conn, (uvhttp_connection_ownership_callback_t)dummy_callback, nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_INIT);

    uvhttp_connection_free(conn);
}

// ========== uvhttp_connection_set_lifecycle overwrite ==========

static void lifecycle_on_close_1(void* user_data) {
    if (user_data) {
        (*(int*)user_data)++;
    }
}

static void lifecycle_on_close_2(void* user_data) {
    if (user_data) {
        (*(int*)user_data) += 100;
    }
}

TEST_F(ProtocolUpgradeTest, SetLifecycle_Overwrite_Works) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    // Set first lifecycle
    uvhttp_connection_lifecycle_t lifecycle1 = {nullptr, lifecycle_on_close_1};
    result = uvhttp_connection_set_lifecycle(conn, &lifecycle1);
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn->lifecycle, nullptr);
    EXPECT_EQ(((uvhttp_connection_lifecycle_t*)conn->lifecycle)->on_close,
              lifecycle_on_close_1);

    // Overwrite with second lifecycle
    uvhttp_connection_lifecycle_t lifecycle2 = {nullptr, lifecycle_on_close_2};
    result = uvhttp_connection_set_lifecycle(conn, &lifecycle2);
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn->lifecycle, nullptr);
    EXPECT_EQ(((uvhttp_connection_lifecycle_t*)conn->lifecycle)->on_close,
              lifecycle_on_close_2);

    uvhttp_connection_free(conn);
}

// ========== uvhttp_connection_get_fd and get_peer_address with accepted connection ==========

// Helper: connect callback for protocol upgrade tests
static void proto_upgrade_on_connect(uv_connect_t* req, int status) {
    if (status < 0) return;
    bool* flag = (bool*)req->data;
    *flag = true;
}

// Helper: alloc callback for protocol upgrade read
static void proto_upgrade_alloc_cb(uv_handle_t* handle, size_t suggested_size,
                                    uv_buf_t* buf) {
    (void)handle;
    (void)suggested_size;
    static char slab[4096];
    buf->base = slab;
    buf->len = sizeof(slab);
}

// Helper: close callback
static void proto_upgrade_on_close(uv_handle_t* handle) {
    (void)handle;
}

// Helper: walk callback to close all open handles
static void proto_upgrade_close_walk_cb(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, proto_upgrade_on_close);
    }
}

// Helper: pump the event loop for a bounded time
static void proto_upgrade_pump_loop(uv_loop_t* loop, int timeout_ms) {
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

// Global results from the handler
static int g_handler_fd_result = -1;
static int g_handler_fd = -1;
static int g_handler_peer_result = -1;
static struct sockaddr_storage g_handler_peer_addr;
static socklen_t g_handler_peer_addr_len = 0;

// Handler that exercises get_fd and get_peer_address on the connection
static int fd_peeraddr_handler(uvhttp_request_t* request,
                                uvhttp_response_t* response) {
    (void)response;

    // Get the connection from request->client using offsetof
    // request->client == &conn->tcp_handle
    uvhttp_connection_t* conn =
        (uvhttp_connection_t*)((char*)request->client -
                               offsetof(uvhttp_connection_t, tcp_handle));

    // Test get_fd
    int fd = -1;
    g_handler_fd_result = uvhttp_connection_get_fd(conn, &fd);
    g_handler_fd = fd;

    // Test get_peer_address
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    g_handler_peer_result =
        uvhttp_connection_get_peer_address(conn, &addr, &addr_len);
    if (g_handler_peer_result == 0) {
        g_handler_peer_addr = addr;
        g_handler_peer_addr_len = addr_len;
    }

    // Send a minimal response
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_body(response, "ok", 2);
    uvhttp_response_send(response);
    return 0;
}

class ConnectionHelperTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    uvhttp_router_t* router = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        g_handler_fd_result = -1;
        g_handler_fd = -1;
        g_handler_peer_result = -1;
        memset(&g_handler_peer_addr, 0, sizeof(g_handler_peer_addr));
        g_handler_peer_addr_len = 0;
    }

    void TearDown() override {
        uv_walk(&loop, proto_upgrade_close_walk_cb, nullptr);
        for (int i = 0; i < 20; i++) {
            if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
        }
        if (router) {
            server->router = nullptr;
            uvhttp_router_free(router);
            router = nullptr;
        }
        if (server) {
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(ConnectionHelperTest, GetFd_WithAcceptedConnection_ReturnsValidFd) {
    // Set up router with handler that calls get_fd
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    rerr = uvhttp_router_add_route_method(router, "/test", UVHTTP_GET,
                                            fd_peeraddr_handler);
    ASSERT_EQ(rerr, UVHTTP_OK);
    uvhttp_server_set_router(server, router);

    // Listen on port 0
    uvhttp_error_t serr = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Get bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                  (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);

    // Connect client
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);
    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);
    uv_connect_t connect_req;
    bool connected = false;
    connect_req.data = &connected;
    ret = uv_tcp_connect(&connect_req, &client,
                          (const struct sockaddr*)&connect_addr,
                          proto_upgrade_on_connect);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send HTTP request
    const char* http_request =
        "GET /test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    uv_buf_t write_buf = uv_buf_init((char*)http_request, strlen(http_request));
    uv_write_t write_req;
    bool write_done = false;
    write_req.data = &write_done;
    auto on_write = [](uv_write_t* req, int status) {
        (void)status;
        *(bool*)req->data = true;
    };
    ret = uv_write(&write_req, (uv_stream_t*)&client, &write_buf, 1, on_write);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Start reading to detect response (handler will have run)
    static char read_buf[4096];
    bool read_done = false;
    client.data = &read_done;
    auto on_read = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        (void)buf;
        if (nread > 0 || nread < 0) {
            *(bool*)stream->data = true;
            uv_read_stop(stream);
        }
    };
    ret = uv_read_start((uv_stream_t*)&client, proto_upgrade_alloc_cb, on_read);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 500);
    uv_read_stop((uv_stream_t*)&client);

    // Verify get_fd was called successfully
    EXPECT_EQ(g_handler_fd_result, UVHTTP_OK)
        << "uvhttp_connection_get_fd should succeed on accepted connection";
    EXPECT_GE(g_handler_fd, 0)
        << "File descriptor should be non-negative";

    uv_close((uv_handle_t*)&client, proto_upgrade_on_close);
    proto_upgrade_pump_loop(&loop, 100);
}

TEST_F(ConnectionHelperTest, GetPeerAddress_WithAcceptedConnection_ReturnsAddr) {
    // Set up router with handler that calls get_peer_address
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    rerr = uvhttp_router_add_route_method(router, "/test", UVHTTP_GET,
                                            fd_peeraddr_handler);
    ASSERT_EQ(rerr, UVHTTP_OK);
    uvhttp_server_set_router(server, router);

    // Listen on port 0
    uvhttp_error_t serr = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Get bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                  (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);

    // Connect client
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);
    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);
    uv_connect_t connect_req;
    bool connected = false;
    connect_req.data = &connected;
    ret = uv_tcp_connect(&connect_req, &client,
                          (const struct sockaddr*)&connect_addr,
                          proto_upgrade_on_connect);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send HTTP request
    const char* http_request =
        "GET /test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    uv_buf_t write_buf = uv_buf_init((char*)http_request, strlen(http_request));
    uv_write_t write_req;
    bool write_done = false;
    write_req.data = &write_done;
    auto on_write = [](uv_write_t* req, int status) {
        (void)status;
        *(bool*)req->data = true;
    };
    ret = uv_write(&write_req, (uv_stream_t*)&client, &write_buf, 1, on_write);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Start reading to trigger handler
    bool read_done = false;
    client.data = &read_done;
    auto on_read = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        (void)buf;
        if (nread > 0 || nread < 0) {
            *(bool*)stream->data = true;
            uv_read_stop(stream);
        }
    };
    ret = uv_read_start((uv_stream_t*)&client, proto_upgrade_alloc_cb, on_read);
    ASSERT_EQ(ret, 0);
    proto_upgrade_pump_loop(&loop, 500);
    uv_read_stop((uv_stream_t*)&client);

    // Verify get_peer_address was called successfully
    EXPECT_EQ(g_handler_peer_result, UVHTTP_OK)
        << "uvhttp_connection_get_peer_address should succeed on accepted connection";
    EXPECT_GT(g_handler_peer_addr_len, (socklen_t)0)
        << "Address length should be positive";

    // Verify the peer address is 127.0.0.1
    if (g_handler_peer_addr.ss_family == AF_INET) {
        struct sockaddr_in* peer_in = (struct sockaddr_in*)&g_handler_peer_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_in->sin_addr, ip_str, sizeof(ip_str));
        EXPECT_STREQ(ip_str, "127.0.0.1");
    } else if (g_handler_peer_addr.ss_family == AF_INET6) {
        // Could be IPv6 loopback
        struct sockaddr_in6* peer_in6 =
            (struct sockaddr_in6*)&g_handler_peer_addr;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &peer_in6->sin6_addr, ip_str, sizeof(ip_str));
        EXPECT_STREQ(ip_str, "::1");
    } else {
        FAIL() << "Unexpected address family: " << g_handler_peer_addr.ss_family;
    }

    uv_close((uv_handle_t*)&client, proto_upgrade_on_close);
    proto_upgrade_pump_loop(&loop, 100);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
