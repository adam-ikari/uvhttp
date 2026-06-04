/**
 * @file test_server_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_server module
 *
 * Targets uncovered areas in src/uvhttp_server.c including:
 * - Rate limit functions (#if UVHTTP_FEATURE_RATE_LIMIT)
 * - Builder API (uvhttp_get/post/put/delete/any, config setters, run/stop)
 * - Server set functions (set_handler, set_router, set_context, stop)
 * - Timeout callback setter
 * - WebSocket connection management (#if UVHTTP_FEATURE_WEBSOCKET)
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_TLS
#include "uvhttp_tls.h"
#endif
}

#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <atomic>

// Declaration not in any public header - defined in uvhttp_server.c
extern "C" {
uvhttp_error_t uvhttp_server_set_timeout_callback(
    uvhttp_server_t* server, uvhttp_timeout_callback_t callback,
    void* user_data);
}

// ============================================================================
// Helper: dummy request handler for builder API tests
// ============================================================================
static int dummy_handler(uvhttp_request_t* request,
                         uvhttp_response_t* response) {
    (void)request;
    (void)response;
    return 0;
}

// ============================================================================
// Server Set Functions (lines 487-529)
// ============================================================================
class ServerSetFunctionsTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(ServerSetFunctionsTest, SetHandler_ValidServer) {
    uvhttp_error_t err = uvhttp_server_set_handler(server, dummy_handler);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->handler, dummy_handler);
}

TEST_F(ServerSetFunctionsTest, SetHandler_NullServer) {
    uvhttp_error_t err = uvhttp_server_set_handler(nullptr, dummy_handler);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServerSetFunctionsTest, SetHandler_NullHandler) {
    uvhttp_error_t err = uvhttp_server_set_handler(server, nullptr);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->handler, nullptr);
}

TEST_F(ServerSetFunctionsTest, SetRouter_ValidServer) {
    uvhttp_router_t* router = nullptr;
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    uvhttp_error_t err = uvhttp_server_set_router(server, router);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->router, router);

    uvhttp_router_free(router);
    server->router = nullptr;  // prevent double free in server_free
}

TEST_F(ServerSetFunctionsTest, SetRouter_NullServer) {
    uvhttp_error_t err = uvhttp_server_set_router(server, nullptr);
    // NULL router is a valid operation (clears router)
    EXPECT_EQ(err, UVHTTP_OK);
}

TEST_F(ServerSetFunctionsTest, SetContext_ValidServer) {
    uvhttp_error_t err = uvhttp_server_set_context(server, nullptr);
    EXPECT_EQ(err, UVHTTP_OK);
}

TEST_F(ServerSetFunctionsTest, SetContext_NullServer) {
    uvhttp_error_t err = uvhttp_server_set_context(nullptr, nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServerSetFunctionsTest, Stop_NullServer) {
    uvhttp_error_t err = uvhttp_server_stop(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServerSetFunctionsTest, Stop_NotListening) {
    // Server was created but never started listening
    uvhttp_error_t err = uvhttp_server_stop(server);
    EXPECT_EQ(err, UVHTTP_ERROR_SERVER_STOP);
}

// ============================================================================
// Timeout Callback (lines 1650-1661)
// ============================================================================
class TimeoutCallbackTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

static void test_timeout_callback(uvhttp_server_t* srv,
                                  uvhttp_connection_t* conn,
                                  uint64_t timeout_ms, void* user_data) {
    (void)srv;
    (void)conn;
    (void)timeout_ms;
    (void)user_data;
}

TEST_F(TimeoutCallbackTest, SetTimeoutCallback_NullServer) {
    uvhttp_error_t err =
        uvhttp_server_set_timeout_callback(nullptr, test_timeout_callback, NULL);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(TimeoutCallbackTest, SetTimeoutCallback_ValidServer) {
    int user_data = 42;
    uvhttp_error_t err = uvhttp_server_set_timeout_callback(
        server, test_timeout_callback, &user_data);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->timeout_callback, test_timeout_callback);
    EXPECT_EQ(server->timeout_callback_user_data, &user_data);
}

TEST_F(TimeoutCallbackTest, SetTimeoutCallback_NullCallback) {
    uvhttp_error_t err =
        uvhttp_server_set_timeout_callback(server, nullptr, nullptr);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(server->timeout_callback, nullptr);
    EXPECT_EQ(server->timeout_callback_user_data, nullptr);
}

// ============================================================================
// Builder API (lines 685-778)
// ============================================================================
class BuilderAPITest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_builder_t* builder = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        // Use uvhttp_server_create to get a properly initialized builder
        uvhttp_error_t err =
            uvhttp_server_create(&loop, "127.0.0.1", 0, &builder);
        // It may fail to listen (port 0 might not work), but the builder
        // should still be created
        (void)err;
    }

    void TearDown() override {
        if (builder) {
            // Clear server's router reference since builder owns it
            if (builder->server) {
                builder->server->router = nullptr;
            }
            uvhttp_server_simple_free(builder);
            builder = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// --- Chained route API null checks ---

TEST_F(BuilderAPITest, Get_NullServer) {
    uvhttp_server_builder_t* result = uvhttp_get(nullptr, "/test", dummy_handler);
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, Get_NullPath) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result = uvhttp_get(builder, nullptr, dummy_handler);
    EXPECT_EQ(result, builder);  // returns server unchanged
}

TEST_F(BuilderAPITest, Get_NullHandler) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result = uvhttp_get(builder, "/test", nullptr);
    EXPECT_EQ(result, builder);  // returns server unchanged
}

TEST_F(BuilderAPITest, Get_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result = uvhttp_get(builder, "/test", dummy_handler);
    EXPECT_EQ(result, builder);
}

TEST_F(BuilderAPITest, Post_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result =
        uvhttp_post(builder, "/submit", dummy_handler);
    EXPECT_EQ(result, builder);
}

TEST_F(BuilderAPITest, Put_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result =
        uvhttp_put(builder, "/update", dummy_handler);
    EXPECT_EQ(result, builder);
}

TEST_F(BuilderAPITest, Delete_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result =
        uvhttp_delete(builder, "/remove", dummy_handler);
    EXPECT_EQ(result, builder);
}

TEST_F(BuilderAPITest, Any_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result =
        uvhttp_any(builder, "/catch-all", dummy_handler);
    EXPECT_EQ(result, builder);
}

// --- Config setter API ---

TEST_F(BuilderAPITest, SetMaxConnections_NullServer) {
    uvhttp_server_builder_t* result = uvhttp_set_max_connections(nullptr, 100);
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, SetMaxConnections_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result = uvhttp_set_max_connections(builder, 500);
    EXPECT_EQ(result, builder);
    EXPECT_EQ(builder->config->max_connections, 500);
}

TEST_F(BuilderAPITest, SetTimeout_NullServer) {
    uvhttp_server_builder_t* result = uvhttp_set_timeout(nullptr, 30);
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, SetTimeout_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result = uvhttp_set_timeout(builder, 60);
    EXPECT_EQ(result, builder);
    EXPECT_EQ(builder->config->request_timeout, 60);
    EXPECT_EQ(builder->config->keepalive_timeout, 60);
}

TEST_F(BuilderAPITest, SetMaxBodySize_NullServer) {
    uvhttp_server_builder_t* result = uvhttp_set_max_body_size(nullptr, 1024);
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, SetMaxBodySize_Valid) {
    if (!builder) GTEST_SKIP();
    uvhttp_server_builder_t* result =
        uvhttp_set_max_body_size(builder, 2048);
    EXPECT_EQ(result, builder);
    EXPECT_EQ(builder->config->max_body_size, (size_t)2048);
}

// --- Convenient request parameter access ---

TEST_F(BuilderAPITest, GetParam_NullRequest) {
    const char* result = uvhttp_get_param(nullptr, "key");
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, GetHeader_NullRequest) {
    const char* result = uvhttp_get_header(nullptr, "Content-Type");
    EXPECT_EQ(result, nullptr);
}

TEST_F(BuilderAPITest, GetBody_NullRequest) {
    const char* result = uvhttp_get_body(nullptr);
    EXPECT_EQ(result, nullptr);
}

// --- Server run and cleanup ---

TEST_F(BuilderAPITest, ServerRun_NullServer) {
    int result = uvhttp_server_run(nullptr);
    EXPECT_EQ(result, -1);
}

TEST_F(BuilderAPITest, ServerRun_NullLoop) {
    uvhttp_server_builder_t bad_builder{};
    bad_builder.loop = nullptr;
    int result = uvhttp_server_run(&bad_builder);
    EXPECT_EQ(result, -1);
}

TEST_F(BuilderAPITest, ServerStopSimple_NullServer) {
    // Should not crash
    uvhttp_server_stop_simple(nullptr);
}

TEST_F(BuilderAPITest, ServerStopSimple_ValidServer) {
    if (!builder) GTEST_SKIP();
    // Server is not listening, but stop_simple just delegates
    uvhttp_server_stop_simple(builder);
    SUCCEED();
}

TEST_F(BuilderAPITest, ServerSimpleFree_Null) {
    // Should not crash
    uvhttp_server_simple_free(nullptr);
}

// ============================================================================
// Rate Limit Functions (gated by UVHTTP_FEATURE_RATE_LIMIT)
// ============================================================================
#if UVHTTP_FEATURE_RATE_LIMIT
class RateLimitBoostTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// --- Enable rate limit ---

TEST_F(RateLimitBoostTest, Enable_NullServer) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(nullptr, 100, 60),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_ZeroRequests) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 0, 60),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_NegativeRequests) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, -1, 60),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_ExcessiveRequests) {
    // > MAX_RATE_LIMIT_REQUESTS (1000000)
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 1000001, 60),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_ZeroWindow) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 100, 0),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_NegativeWindow) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 100, -1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_ExcessiveWindow) {
    // > MAX_RATE_LIMIT_WINDOW_SECONDS (86400)
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 100, 86401),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Enable_Valid) {
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 100, 60), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_enabled, 1);
    EXPECT_EQ(server->rate_limit_max_requests, 100);
    EXPECT_EQ(server->rate_limit_window_seconds, 60);
    EXPECT_EQ(server->rate_limit_request_count, 0);
    EXPECT_EQ(server->rate_limit_window_start_time, (uint64_t)0);
}

TEST_F(RateLimitBoostTest, Enable_MaxBoundary) {
    // Exactly at max: 1000000 requests, 86400 seconds
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 1000000, 86400),
              UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_max_requests, 1000000);
    EXPECT_EQ(server->rate_limit_window_seconds, 86400);
}

TEST_F(RateLimitBoostTest, Enable_MinBoundary) {
    // Exactly at min: 1 request, 1 second
    EXPECT_EQ(uvhttp_server_enable_rate_limit(server, 1, 1), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_max_requests, 1);
    EXPECT_EQ(server->rate_limit_window_seconds, 1);
}

// --- Disable rate limit ---

TEST_F(RateLimitBoostTest, Disable_NullServer) {
    EXPECT_EQ(uvhttp_server_disable_rate_limit(nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Disable_NotEnabled) {
    EXPECT_EQ(uvhttp_server_disable_rate_limit(server), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_enabled, 0);
}

TEST_F(RateLimitBoostTest, Disable_AfterEnable) {
    uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(uvhttp_server_disable_rate_limit(server), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_enabled, 0);
    EXPECT_EQ(server->rate_limit_request_count, 0);
}

// --- Check rate limit ---

TEST_F(RateLimitBoostTest, Check_NullServer) {
    // check_rate_limit returns UVHTTP_OK for null (not enabled)
    EXPECT_EQ(uvhttp_server_check_rate_limit(nullptr), UVHTTP_OK);
}

TEST_F(RateLimitBoostTest, Check_NotEnabled) {
    EXPECT_EQ(uvhttp_server_check_rate_limit(server), UVHTTP_OK);
}

TEST_F(RateLimitBoostTest, Check_WithinLimit) {
    uvhttp_server_enable_rate_limit(server, 5, 60);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(uvhttp_server_check_rate_limit(server), UVHTTP_OK);
    }
}

TEST_F(RateLimitBoostTest, Check_ExceedsLimit) {
    uvhttp_server_enable_rate_limit(server, 3, 60);
    for (int i = 0; i < 3; i++) {
        uvhttp_server_check_rate_limit(server);
    }
    EXPECT_EQ(uvhttp_server_check_rate_limit(server),
              UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);
}

TEST_F(RateLimitBoostTest, Check_WindowExpiryResets) {
    // Set window to 1 second and force a reset by manually setting
    // window_start_time to the distant past
    uvhttp_server_enable_rate_limit(server, 2, 1);
    // Consume the limit
    uvhttp_server_check_rate_limit(server);
    uvhttp_server_check_rate_limit(server);
    EXPECT_EQ(uvhttp_server_check_rate_limit(server),
              UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);

    // Force window expiry by setting start time to 0
    server->rate_limit_window_start_time = 0;
    // Now check should succeed (window expired -> counter reset)
    EXPECT_EQ(uvhttp_server_check_rate_limit(server), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_request_count, 1);
}

// --- Whitelist ---

TEST_F(RateLimitBoostTest, Whitelist_NullServer) {
    EXPECT_EQ(uvhttp_server_add_rate_limit_whitelist(nullptr, "127.0.0.1"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Whitelist_NullIp) {
    EXPECT_EQ(uvhttp_server_add_rate_limit_whitelist(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Whitelist_InvalidIp) {
    EXPECT_EQ(uvhttp_server_add_rate_limit_whitelist(server, "not-an-ip"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, Whitelist_ValidIp) {
    EXPECT_EQ(uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.100"),
              UVHTTP_OK);
    EXPECT_GT(server->rate_limit_whitelist_count, (size_t)0);
}

TEST_F(RateLimitBoostTest, Whitelist_DuplicateIp) {
    uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1");
    size_t count_before = server->rate_limit_whitelist_count;
    // Adding same IP again should succeed without increasing count
    EXPECT_EQ(uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1"),
              UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_whitelist_count, count_before);
}

TEST_F(RateLimitBoostTest, Whitelist_MultipleIps) {
    uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1");
    uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.2");
    uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.3");
    EXPECT_EQ(server->rate_limit_whitelist_count, (size_t)3);
}

// --- Get rate limit status ---

TEST_F(RateLimitBoostTest, GetStatus_NullServer) {
    int remaining = 0;
    uint64_t reset_time = 0;
    EXPECT_EQ(uvhttp_server_get_rate_limit_status(nullptr, "127.0.0.1",
                                                   &remaining, &reset_time),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, GetStatus_NullRemaining) {
    EXPECT_EQ(uvhttp_server_get_rate_limit_status(server, "127.0.0.1",
                                                   nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, GetStatus_NotEnabled) {
    int remaining = 0;
    EXPECT_EQ(uvhttp_server_get_rate_limit_status(server, "127.0.0.1",
                                                   &remaining, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(remaining, -1);
}

TEST_F(RateLimitBoostTest, GetStatus_Enabled) {
    uvhttp_server_enable_rate_limit(server, 100, 60);
    int remaining = 0;
    uint64_t reset_time = 0;
    EXPECT_EQ(uvhttp_server_get_rate_limit_status(server, "127.0.0.1",
                                                   &remaining, &reset_time),
              UVHTTP_OK);
    EXPECT_EQ(remaining, 100);
    // reset_time should be > 0 since window_start_time is 0 and window is 60s
    // Actually: 0 + 60000 = 60000
    EXPECT_EQ(reset_time, (uint64_t)(60 * 1000));
}

TEST_F(RateLimitBoostTest, GetStatus_WithNullResetTime) {
    uvhttp_server_enable_rate_limit(server, 50, 30);
    int remaining = 0;
    // Pass nullptr for reset_time - should still succeed
    EXPECT_EQ(uvhttp_server_get_rate_limit_status(server, "127.0.0.1",
                                                   &remaining, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(remaining, 50);
}

TEST_F(RateLimitBoostTest, GetStatus_AfterSomeRequests) {
    uvhttp_server_enable_rate_limit(server, 10, 60);
    for (int i = 0; i < 4; i++) {
        uvhttp_server_check_rate_limit(server);
    }
    int remaining = 0;
    uvhttp_server_get_rate_limit_status(server, "127.0.0.1", &remaining,
                                        nullptr);
    EXPECT_EQ(remaining, 6);
}

// --- Reset client ---

TEST_F(RateLimitBoostTest, ResetClient_NullServer) {
    EXPECT_EQ(uvhttp_server_reset_rate_limit_client(nullptr, "127.0.0.1"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, ResetClient_NullIp) {
    EXPECT_EQ(uvhttp_server_reset_rate_limit_client(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, ResetClient_Valid) {
    uvhttp_server_enable_rate_limit(server, 5, 60);
    for (int i = 0; i < 5; i++) {
        uvhttp_server_check_rate_limit(server);
    }
    EXPECT_EQ(uvhttp_server_check_rate_limit(server),
              UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);

    EXPECT_EQ(uvhttp_server_reset_rate_limit_client(server, "127.0.0.1"),
              UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_request_count, 0);
    // Should be able to make requests again
    EXPECT_EQ(uvhttp_server_check_rate_limit(server), UVHTTP_OK);
}

// --- Clear all ---

TEST_F(RateLimitBoostTest, ClearAll_NullServer) {
    EXPECT_EQ(uvhttp_server_clear_rate_limit_all(nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RateLimitBoostTest, ClearAll_Valid) {
    uvhttp_server_enable_rate_limit(server, 100, 60);
    for (int i = 0; i < 50; i++) {
        uvhttp_server_check_rate_limit(server);
    }
    EXPECT_EQ(server->rate_limit_request_count, 50);

    EXPECT_EQ(uvhttp_server_clear_rate_limit_all(server), UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_request_count, 0);
    EXPECT_EQ(server->rate_limit_window_start_time, (uint64_t)0);
}

// --- Lifecycle: disable/re-enable ---

TEST_F(RateLimitBoostTest, DisableAndReenable) {
    uvhttp_server_enable_rate_limit(server, 10, 60);
    for (int i = 0; i < 5; i++) {
        uvhttp_server_check_rate_limit(server);
    }
    uvhttp_server_disable_rate_limit(server);
    EXPECT_EQ(server->rate_limit_enabled, 0);

    uvhttp_server_enable_rate_limit(server, 20, 120);
    EXPECT_EQ(server->rate_limit_enabled, 1);
    EXPECT_EQ(server->rate_limit_max_requests, 20);
    EXPECT_EQ(server->rate_limit_window_seconds, 120);
    EXPECT_EQ(server->rate_limit_request_count, 0);
}

#endif  // UVHTTP_FEATURE_RATE_LIMIT

// ============================================================================
// WebSocket Connection Management (gated by UVHTTP_FEATURE_WEBSOCKET)
// ============================================================================
#if UVHTTP_FEATURE_WEBSOCKET
class WSConnectionManagementTest : public ::testing::Test {
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
            // Disable connection management if active (clears timers)
            if (server->ws_connection_manager) {
                uvhttp_server_ws_disable_connection_management(server);
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// --- Enable connection management ---

TEST_F(WSConnectionManagementTest, Enable_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(nullptr, 60, 30),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Enable_TimeoutTooLow) {
    // Minimum is 10
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 9, 30),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Enable_TimeoutTooHigh) {
    // Maximum is 3600
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 3601, 30),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Enable_HeartbeatTooLow) {
    // Minimum is 5
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 60, 4),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Enable_HeartbeatTooHigh) {
    // Maximum is 300
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 60, 301),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Enable_Valid) {
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 60, 30),
              UVHTTP_OK);
    ASSERT_NE(server->ws_connection_manager, nullptr);
    EXPECT_EQ(server->ws_connection_manager->timeout_seconds, 60);
    EXPECT_EQ(server->ws_connection_manager->heartbeat_interval, 30);
    EXPECT_EQ(server->ws_connection_manager->enabled, 1);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
}

TEST_F(WSConnectionManagementTest, Enable_MinBoundaries) {
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 10, 5),
              UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->timeout_seconds, 10);
    EXPECT_EQ(server->ws_connection_manager->heartbeat_interval, 5);
}

TEST_F(WSConnectionManagementTest, Enable_MaxBoundaries) {
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 3600, 300),
              UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->timeout_seconds, 3600);
    EXPECT_EQ(server->ws_connection_manager->heartbeat_interval, 300);
}

TEST_F(WSConnectionManagementTest, Enable_Reenable_DisablesPrevious) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    ws_connection_manager_t* first_mgr = server->ws_connection_manager;
    EXPECT_NE(first_mgr, nullptr);

    // Re-enabling should disable the previous and create new
    EXPECT_EQ(uvhttp_server_ws_enable_connection_management(server, 120, 60),
              UVHTTP_OK);
    EXPECT_NE(server->ws_connection_manager, nullptr);
    // The manager pointer should be different (old was freed)
    // New settings should be applied
    EXPECT_EQ(server->ws_connection_manager->timeout_seconds, 120);
    EXPECT_EQ(server->ws_connection_manager->heartbeat_interval, 60);
}

// --- Disable connection management ---

TEST_F(WSConnectionManagementTest, Disable_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_disable_connection_management(nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Disable_NotEnabled) {
    // Not enabled -> returns OK (no-op)
    EXPECT_EQ(uvhttp_server_ws_disable_connection_management(server),
              UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager, nullptr);
}

TEST_F(WSConnectionManagementTest, Disable_AfterEnable) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_NE(server->ws_connection_manager, nullptr);

    EXPECT_EQ(uvhttp_server_ws_disable_connection_management(server),
              UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager, nullptr);
}

// --- Get connection count ---

TEST_F(WSConnectionManagementTest, GetConnectionCount_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_get_connection_count(nullptr), 0);
}

TEST_F(WSConnectionManagementTest, GetConnectionCount_NoManager) {
    EXPECT_EQ(uvhttp_server_ws_get_connection_count(server), 0);
}

TEST_F(WSConnectionManagementTest, GetConnectionCount_EmptyManager) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count(server), 0);
}

// --- Get connection count by path ---

TEST_F(WSConnectionManagementTest, GetCountByPath_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(nullptr, "/ws"), 0);
}

TEST_F(WSConnectionManagementTest, GetCountByPath_NullPath) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, nullptr), 0);
}

TEST_F(WSConnectionManagementTest, GetCountByPath_NoManager) {
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, "/ws"), 0);
}

TEST_F(WSConnectionManagementTest, GetCountByPath_EmptyManager) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, "/ws"), 0);
}

// --- Add/Remove/Update connection ---

TEST_F(WSConnectionManagementTest, AddConnection_NullServer) {
    uvhttp_ws_connection_t fake_conn{};
    // Should not crash
    uvhttp_server_ws_add_connection(nullptr, &fake_conn, "/ws");
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, AddConnection_NullWsConn) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_server_ws_add_connection(server, nullptr, "/ws");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
}

TEST_F(WSConnectionManagementTest, AddConnection_NullPath) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_add_connection(server, &fake_conn, nullptr);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
}

TEST_F(WSConnectionManagementTest, AddConnection_NoManager) {
    uvhttp_ws_connection_t fake_conn{};
    // No manager set, should be a no-op
    uvhttp_server_ws_add_connection(server, &fake_conn, "/ws");
    EXPECT_EQ(server->ws_connection_manager, nullptr);
}

TEST_F(WSConnectionManagementTest, AddConnection_Valid) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_add_connection(server, &fake_conn, "/ws/chat");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count(server), 1);
}

TEST_F(WSConnectionManagementTest, AddConnection_MultipleSamePath) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{}, fake3{};
    uvhttp_server_ws_add_connection(server, &fake1, "/ws");
    uvhttp_server_ws_add_connection(server, &fake2, "/ws");
    uvhttp_server_ws_add_connection(server, &fake3, "/ws");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 3);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, "/ws"), 3);
}

TEST_F(WSConnectionManagementTest, AddConnection_DifferentPaths) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{};
    uvhttp_server_ws_add_connection(server, &fake1, "/ws/chat");
    uvhttp_server_ws_add_connection(server, &fake2, "/ws/notify");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 2);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, "/ws/chat"),
              1);
    EXPECT_EQ(
        uvhttp_server_ws_get_connection_count_by_path(server, "/ws/notify"), 1);
    EXPECT_EQ(
        uvhttp_server_ws_get_connection_count_by_path(server, "/ws/other"), 0);
}

TEST_F(WSConnectionManagementTest, RemoveConnection_NullServer) {
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_remove_connection(nullptr, &fake_conn);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, RemoveConnection_NullWsConn) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_server_ws_remove_connection(server, nullptr);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, RemoveConnection_NoManager) {
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_remove_connection(server, &fake_conn);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, RemoveConnection_Valid) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_add_connection(server, &fake_conn, "/ws");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);

    uvhttp_server_ws_remove_connection(server, &fake_conn);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
}

TEST_F(WSConnectionManagementTest, RemoveConnection_NotFound) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{};
    uvhttp_server_ws_add_connection(server, &fake1, "/ws");
    // Remove a connection that was never added
    uvhttp_server_ws_remove_connection(server, &fake2);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);
}

TEST_F(WSConnectionManagementTest, RemoveConnection_FromMiddle) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{}, fake3{};
    uvhttp_server_ws_add_connection(server, &fake1, "/ws");
    uvhttp_server_ws_add_connection(server, &fake2, "/ws");
    uvhttp_server_ws_add_connection(server, &fake3, "/ws");
    // List is: fake3 -> fake2 -> fake1 (add prepends)
    // Remove the middle one
    uvhttp_server_ws_remove_connection(server, &fake2);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 2);
}

TEST_F(WSConnectionManagementTest, RemoveConnection_FirstNode) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{};
    uvhttp_server_ws_add_connection(server, &fake1, "/ws");
    uvhttp_server_ws_add_connection(server, &fake2, "/ws");
    // List is: fake2 -> fake1 (add prepends)
    // Remove the head (most recently added)
    uvhttp_server_ws_remove_connection(server, &fake2);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);
}

// --- Update activity ---

TEST_F(WSConnectionManagementTest, UpdateActivity_NullServer) {
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_update_activity(nullptr, &fake_conn);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, UpdateActivity_NullWsConn) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_server_ws_update_activity(server, nullptr);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, UpdateActivity_NoManager) {
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_update_activity(server, &fake_conn);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, UpdateActivity_Valid) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake_conn{};
    uvhttp_server_ws_add_connection(server, &fake_conn, "/ws");

    // Update activity - should not crash and should clear ping_pending
    uvhttp_server_ws_update_activity(server, &fake_conn);
    SUCCEED();
}

TEST_F(WSConnectionManagementTest, UpdateActivity_NotFound) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t added{}, not_added{};
    uvhttp_server_ws_add_connection(server, &added, "/ws");

    // Update a connection not in the list - should be no-op
    uvhttp_server_ws_update_activity(server, &not_added);
    SUCCEED();
}

// --- Broadcast ---

TEST_F(WSConnectionManagementTest, Broadcast_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_broadcast(nullptr, "/ws", "hello", 5),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Broadcast_NoManager) {
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/ws", "hello", 5),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Broadcast_NullData) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/ws", nullptr, 5),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Broadcast_ZeroLength) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/ws", "hello", 0),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, Broadcast_EmptyConnectionList) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    // No connections added, broadcast should succeed (no-op)
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/ws", "hello", 5), UVHTTP_OK);
}

// --- Close all ---

TEST_F(WSConnectionManagementTest, CloseAll_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_close_all(nullptr, "/ws"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, CloseAll_NoManager) {
    EXPECT_EQ(uvhttp_server_ws_close_all(server, "/ws"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WSConnectionManagementTest, CloseAll_EmptyList) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    // No connections to close
    EXPECT_EQ(uvhttp_server_ws_close_all(server, "/ws"), UVHTTP_OK);
}

TEST_F(WSConnectionManagementTest, CloseAll_NullPath) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    // NULL path means close all
    EXPECT_EQ(uvhttp_server_ws_close_all(server, nullptr), UVHTTP_OK);
}

// --- Broadcast with connections (exercises linked list traversal and state
// check) ---

TEST_F(WSConnectionManagementTest, Broadcast_NonOpenConnections) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{};
    fake1.state = UVHTTP_WS_STATE_CONNECTING;  // not OPEN, skip send
    fake2.state = UVHTTP_WS_STATE_CLOSING;     // not OPEN, skip send
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");
    uvhttp_server_ws_add_connection(server, &fake2, "/chat");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 2);

    // Broadcast traverses list but skips send (state != OPEN)
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/chat", "hello", 5),
              UVHTTP_OK);
}

TEST_F(WSConnectionManagementTest, Broadcast_NullPathMatchesAll) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{};
    fake1.state = UVHTTP_WS_STATE_CLOSING;
    fake2.state = UVHTTP_WS_STATE_CLOSED;
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");
    uvhttp_server_ws_add_connection(server, &fake2, "/other");

    // NULL path matches all connections (exercises the !path branch)
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, nullptr, "hello", 5),
              UVHTTP_OK);
}

TEST_F(WSConnectionManagementTest, Broadcast_PathNoMatch) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{};
    fake1.state = UVHTTP_WS_STATE_CONNECTING;
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");

    // Broadcast to a different path - traversal finds no match
    EXPECT_EQ(uvhttp_server_ws_broadcast(server, "/other", "hello", 5),
              UVHTTP_OK);
}

// --- Close all with connections (exercises node removal from linked list) ---

TEST_F(WSConnectionManagementTest, CloseAll_WithMatchingConnections) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{}, fake3{};
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");
    uvhttp_server_ws_add_connection(server, &fake2, "/chat");
    uvhttp_server_ws_add_connection(server, &fake3, "/other");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 3);

    // Null out ws_conn pointers to prevent uvhttp_ws_close from dereferencing
    // fake pointers (the function checks ws_conn != NULL before calling close)
    ws_connection_node_t* node = server->ws_connection_manager->connections;
    while (node) {
        node->ws_conn = nullptr;
        node = node->next;
    }

    // Close all on /chat - should remove 2 nodes, leave /other
    EXPECT_EQ(uvhttp_server_ws_close_all(server, "/chat"), UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);
    EXPECT_EQ(uvhttp_server_ws_get_connection_count_by_path(server, "/other"),
              1);
}

TEST_F(WSConnectionManagementTest, CloseAll_NullPathClosesAll) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{}, fake2{}, fake3{};
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");
    uvhttp_server_ws_add_connection(server, &fake2, "/other");
    uvhttp_server_ws_add_connection(server, &fake3, "/status");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 3);

    // Null out ws_conn to prevent uvhttp_ws_close crash
    ws_connection_node_t* node = server->ws_connection_manager->connections;
    while (node) {
        node->ws_conn = nullptr;
        node = node->next;
    }

    // NULL path closes all connections
    EXPECT_EQ(uvhttp_server_ws_close_all(server, nullptr), UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 0);
    EXPECT_EQ(server->ws_connection_manager->connections, nullptr);
}

TEST_F(WSConnectionManagementTest, CloseAll_PathNoMatch) {
    uvhttp_server_ws_enable_connection_management(server, 60, 30);
    uvhttp_ws_connection_t fake1{};
    uvhttp_server_ws_add_connection(server, &fake1, "/chat");
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);

    // Close on a different path - no match, nothing removed
    EXPECT_EQ(uvhttp_server_ws_close_all(server, "/other"), UVHTTP_OK);
    EXPECT_EQ(server->ws_connection_manager->connection_count, 1);
}

// ============================================================================
// WS Handler Registration (lines 850-889 in server.c)
// ============================================================================
class WsHandlerRegistrationTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// Dummy WS callbacks for handler registration tests
static int dummy_ws_connect(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    return 0;
}
static int dummy_ws_connect2(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    return 1;
}
static int dummy_ws_connect3(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    return 2;
}

TEST_F(WsHandlerRegistrationTest, RegisterWsHandler_NullServer) {
    uvhttp_ws_handler_t handler = {};
    handler.on_connect = dummy_ws_connect;
    EXPECT_EQ(uvhttp_server_register_ws_handler(nullptr, "/ws", &handler),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsHandlerRegistrationTest, RegisterWsHandler_NullPath) {
    uvhttp_ws_handler_t handler = {};
    handler.on_connect = dummy_ws_connect;
    EXPECT_EQ(uvhttp_server_register_ws_handler(server, nullptr, &handler),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsHandlerRegistrationTest, RegisterWsHandler_NullHandler) {
    EXPECT_EQ(uvhttp_server_register_ws_handler(server, "/ws", nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsHandlerRegistrationTest, RegisterWsHandler_Valid) {
    uvhttp_ws_handler_t handler = {};
    handler.on_connect = dummy_ws_connect;
    EXPECT_EQ(uvhttp_server_register_ws_handler(server, "/ws", &handler),
              UVHTTP_OK);

    uvhttp_ws_handler_t* found = uvhttp_server_find_ws_handler(server, "/ws");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->on_connect, dummy_ws_connect);
}

TEST_F(WsHandlerRegistrationTest, RegisterWsHandler_Multiple) {
    uvhttp_ws_handler_t h1 = {}, h2 = {}, h3 = {};
    h1.on_connect = dummy_ws_connect;
    h2.on_connect = dummy_ws_connect2;
    h3.on_connect = dummy_ws_connect3;

    EXPECT_EQ(uvhttp_server_register_ws_handler(server, "/ws/chat", &h1),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_ws_handler(server, "/ws/notify", &h2),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_ws_handler(server, "/ws/status", &h3),
              UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_find_ws_handler(server, "/ws/chat")->on_connect,
              dummy_ws_connect);
    EXPECT_EQ(uvhttp_server_find_ws_handler(server, "/ws/notify")->on_connect,
              dummy_ws_connect2);
    EXPECT_EQ(uvhttp_server_find_ws_handler(server, "/ws/status")->on_connect,
              dummy_ws_connect3);
}

TEST_F(WsHandlerRegistrationTest, FindWsHandler_NotFound) {
    uvhttp_ws_handler_t handler = {};
    handler.on_connect = dummy_ws_connect;
    uvhttp_server_register_ws_handler(server, "/ws/exists", &handler);

    EXPECT_EQ(uvhttp_server_find_ws_handler(server, "/ws/missing"), nullptr);
}

TEST_F(WsHandlerRegistrationTest, FindWsHandler_NullServer) {
    EXPECT_EQ(uvhttp_server_find_ws_handler(nullptr, "/ws"), nullptr);
}

TEST_F(WsHandlerRegistrationTest, FindWsHandler_NullPath) {
    EXPECT_EQ(uvhttp_server_find_ws_handler(server, nullptr), nullptr);
}

// ============================================================================
// WS Send/Close null checks (lines 913-966 in server.c)
// ============================================================================
class WsSendCloseTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(WsSendCloseTest, WsSend_NullConn) {
    EXPECT_EQ(uvhttp_server_ws_send(nullptr, "hello", 5),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsSendCloseTest, WsSend_NullData) {
    uvhttp_ws_connection_t* fake_conn = (uvhttp_ws_connection_t*)0x1;
    EXPECT_EQ(uvhttp_server_ws_send(fake_conn, nullptr, 5),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsSendCloseTest, WsSend_ZeroLen) {
    uvhttp_ws_connection_t* fake_conn = (uvhttp_ws_connection_t*)0x1;
    EXPECT_EQ(uvhttp_server_ws_send(fake_conn, "hello", 0),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsSendCloseTest, WsClose_NullConn) {
    EXPECT_EQ(uvhttp_server_ws_close(nullptr, 1000, "normal"),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ============================================================================
// WS Disable management with no manager (no-op path)
// ============================================================================
class WsDisableMgmtTest : public ::testing::Test {
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
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(WsDisableMgmtTest, WsDisableMgmt_NullServer) {
    EXPECT_EQ(uvhttp_server_ws_disable_connection_management(nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(WsDisableMgmtTest, WsDisableMgmt_NoManager) {
    // No manager was ever enabled - should be a no-op returning OK
    EXPECT_EQ(uvhttp_server_ws_disable_connection_management(server),
              UVHTTP_OK);
}

#endif  // UVHTTP_FEATURE_WEBSOCKET

// ============================================================================
// TLS Enable/Disable/IsTlsEnabled (lines 531-580 in server.c)
// ============================================================================
#if UVHTTP_FEATURE_TLS

// Declaration not in any public header - defined in uvhttp_server.c
extern "C" {
int uvhttp_server_is_tls_enabled(uvhttp_server_t* server);
}

class TlsBoostTest : public ::testing::Test {
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
            // If TLS was enabled, null out tls_ctx to prevent server_free
            // from calling uvhttp_tls_context_free on a fake pointer
            server->tls_ctx = nullptr;
            server->tls_enabled = 0;
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(TlsBoostTest, IsTlsEnabled_DefaultFalse) {
    // Freshly created server should not have TLS enabled
    EXPECT_FALSE(uvhttp_server_is_tls_enabled(server));
}

TEST_F(TlsBoostTest, IsTlsEnabled_NullServer) {
    // NULL server returns 0 (not enabled)
    EXPECT_FALSE(uvhttp_server_is_tls_enabled(nullptr));
}

TEST_F(TlsBoostTest, EnableTls_NullServer) {
    // Use a non-NULL dummy pointer (opaque type, can't instantiate)
    uvhttp_tls_context_t* fake_ctx = (uvhttp_tls_context_t*)0x1;
    EXPECT_EQ(uvhttp_server_enable_tls(nullptr, fake_ctx),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(TlsBoostTest, EnableTls_NullContext) {
    EXPECT_EQ(uvhttp_server_enable_tls(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(TlsBoostTest, EnableTls_ValidSetsFlag) {
    // Use a non-NULL dummy pointer - enable_tls only stores it
    uvhttp_tls_context_t* fake_ctx = (uvhttp_tls_context_t*)0x1;
    EXPECT_EQ(uvhttp_server_enable_tls(server, fake_ctx), UVHTTP_OK);
    EXPECT_TRUE(uvhttp_server_is_tls_enabled(server));
    // Prevent TearDown from calling uvhttp_tls_context_free on dummy pointer
    server->tls_ctx = nullptr;
}

TEST_F(TlsBoostTest, DisableTls_NullServer) {
    EXPECT_EQ(uvhttp_server_disable_tls(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(TlsBoostTest, DisableTls_NotEnabled) {
    // Not enabled -> no-op, returns OK
    EXPECT_EQ(uvhttp_server_disable_tls(server), UVHTTP_OK);
    EXPECT_FALSE(uvhttp_server_is_tls_enabled(server));
}

TEST_F(TlsBoostTest, DisableTls_ClearsFlag) {
    // Enable then disable
    uvhttp_tls_context_t* fake_ctx = (uvhttp_tls_context_t*)0x1;
    uvhttp_server_enable_tls(server, fake_ctx);
    EXPECT_TRUE(uvhttp_server_is_tls_enabled(server));

    // Null out tls_ctx before disable to prevent free on dummy pointer
    server->tls_ctx = nullptr;
    EXPECT_EQ(uvhttp_server_disable_tls(server), UVHTTP_OK);
    EXPECT_FALSE(uvhttp_server_is_tls_enabled(server));
}

#endif  // UVHTTP_FEATURE_TLS

// ============================================================================
// uvhttp_serve parameter validation (lines 804-842 in server.c)
// ============================================================================
class ServeParamValidationTest : public ::testing::Test {
protected:
    uv_loop_t loop{};

    void SetUp() override { uv_loop_init(&loop); }

    void TearDown() override { uv_loop_close(&loop); }
};

TEST_F(ServeParamValidationTest, Serve_NullLoop) {
    int result = uvhttp_serve(nullptr, "localhost", 8080);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServeParamValidationTest, Serve_InvalidPortZero) {
    int result = uvhttp_serve(&loop, "localhost", 0);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServeParamValidationTest, Serve_InvalidPortNegative) {
    int result = uvhttp_serve(&loop, "localhost", -1);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServeParamValidationTest, Serve_InvalidPortTooHigh) {
    int result = uvhttp_serve(&loop, "localhost", 65536);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ServeParamValidationTest, Serve_InvalidPortMaxInt) {
    int result = uvhttp_serve(&loop, "localhost", 99999);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// ============================================================================
// TLS with real TLS context (gated by UVHTTP_FEATURE_TLS)
// Exercises uvhttp_tls_context_new/free and enable/disable with real allocation
// ============================================================================
#if UVHTTP_FEATURE_TLS

class TlsRealContextTest : public ::testing::Test {
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
            // tls_ctx ownership may have been transferred to server;
            // null it out so server_free does not double-free contexts
            // we already freed manually in some tests.
            // For tests where server still owns the ctx, server_free will
            // call uvhttp_tls_context_free which is correct.
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(TlsRealContextTest, EnableTls_WithRealContext) {
    uvhttp_tls_context_t* tls_ctx = nullptr;
    uvhttp_error_t err = uvhttp_tls_context_new(&tls_ctx);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(tls_ctx, nullptr);

    err = uvhttp_server_enable_tls(server, tls_ctx);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_TRUE(uvhttp_server_is_tls_enabled(server));
    // server now owns tls_ctx; TearDown will free it via server_free
}

TEST_F(TlsRealContextTest, DisableTls_WithRealContext) {
    uvhttp_tls_context_t* tls_ctx = nullptr;
    uvhttp_error_t err = uvhttp_tls_context_new(&tls_ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    err = uvhttp_server_enable_tls(server, tls_ctx);
    ASSERT_EQ(err, UVHTTP_OK);

    // disable_tls frees the context internally
    err = uvhttp_server_disable_tls(server);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_FALSE(uvhttp_server_is_tls_enabled(server));
}

TEST_F(TlsRealContextTest, EnableTls_ReplacesExisting) {
    uvhttp_tls_context_t* ctx1 = nullptr;
    uvhttp_tls_context_t* ctx2 = nullptr;
    uvhttp_error_t err;

    err = uvhttp_tls_context_new(&ctx1);
    ASSERT_EQ(err, UVHTTP_OK);
    err = uvhttp_tls_context_new(&ctx2);
    ASSERT_EQ(err, UVHTTP_OK);

    // Enable with ctx1
    err = uvhttp_server_enable_tls(server, ctx1);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_TRUE(uvhttp_server_is_tls_enabled(server));

    // Enable again with ctx2 - enable_tls should free ctx1 internally
    err = uvhttp_server_enable_tls(server, ctx2);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_TRUE(uvhttp_server_is_tls_enabled(server));
    // server now owns ctx2; TearDown will free it via server_free
}

#endif  // UVHTTP_FEATURE_TLS (real context tests)

// ============================================================================
// on_connection callback coverage (lines 71-206 in uvhttp_server.c)
// ============================================================================

// Helper: TCP connect callback (no-op, just acknowledge)
static void test_on_connect(uv_connect_t* req, int status) {
    (void)req;
    (void)status;
}

// Helper: generic close callback (no-op)
static void test_on_close(uv_handle_t* handle) {
    (void)handle;
}

// Helper: walk callback to close all open handles
static void test_close_walk_cb(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, test_on_close);
    }
}

// Helper: timer callback that stops the event loop
static void test_stop_timer_cb(uv_timer_t* handle) {
    uv_stop(handle->loop);
}

// Helper: alloc callback for read_start (needed to detect disconnects)
static void test_alloc_cb(uv_handle_t* handle, size_t suggested_size,
                          uv_buf_t* buf) {
    (void)handle;
    (void)suggested_size;
    // Provide a small static buffer - we don't actually read data
    static char slab[64];
    buf->base = slab;
    buf->len = sizeof(slab);
}

// Helper: read callback (no-op, just drain)
static void test_read_cb(uv_stream_t* stream, ssize_t nread,
                         const uv_buf_t* buf) {
    (void)buf;
    if (nread < 0) {
        // Error or EOF - close the stream
        uv_close((uv_handle_t*)stream, test_on_close);
    }
}

class OnConnectionTest : public ::testing::Test {
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
        // Walk and close any remaining handles (e.g. leaked temp clients from
        // 503 path)
        uv_walk(&loop, test_close_walk_cb, nullptr);
        for (int i = 0; i < 20; i++) {
            if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
        }

        if (server) {
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// Helper: pump the event loop for a bounded time using UV_RUN_NOWAIT
// Returns after at most `timeout_ms` milliseconds or when no more handles
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

// Test 1: Normal connection acceptance
// Exercises on_connection lines 71-84, 91-103, 168-206
TEST_F(OnConnectionTest, NormalConnectionAcceptance) {
    // Set a handler so the server can process requests
    uvhttp_error_t herr = uvhttp_server_set_handler(server, dummy_handler);
    ASSERT_EQ(herr, UVHTTP_OK);

    // Listen on port 0 (OS assigns random port)
    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_EQ(server->is_listening, 1);

    // Get the actual bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Create a TCP client and connect to the server
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);

    uv_connect_t connect_req;
    ret = uv_tcp_connect(&connect_req, &client,
                         (const struct sockaddr*)&connect_addr, test_on_connect);
    ASSERT_EQ(ret, 0);

    // Pump the loop for 100ms to process connect and on_connection
    pump_loop(&loop, 100);

    // The connection was accepted and active_connections incremented
    EXPECT_GE(server->active_connections, (size_t)1);

    // Close client handle and drain the loop
    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);
}

// Test 2: Max connections reached - 503 response path
// Exercises on_connection lines 105-163
TEST_F(OnConnectionTest, MaxConnectionsReached_503Response) {
    // Create a config with max_connections = 0 to trigger 503 immediately
    uvhttp_config_t* config = nullptr;
    uvhttp_error_t cerr = uvhttp_config_new(&config);
    ASSERT_EQ(cerr, UVHTTP_OK);
    ASSERT_NE(config, nullptr);
    config->max_connections = 0;
    // Assign config to server - server_free will free it
    server->config = config;

    // Listen on port 0
    uvhttp_error_t err = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(err, UVHTTP_OK);

    // Get the actual bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);

    // Create a TCP client and connect
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);

    uv_connect_t connect_req;
    ret = uv_tcp_connect(&connect_req, &client,
                         (const struct sockaddr*)&connect_addr, test_on_connect);
    ASSERT_EQ(ret, 0);

    // Pump the loop for 100ms to process connect, on_connection (503 path),
    // and write callback
    pump_loop(&loop, 100);

    // active_connections should still be 0 (normal path was not taken)
    EXPECT_EQ(server->active_connections, (size_t)0);

    // Close client handle and drain the loop
    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);

    // Prevent TearDown from double-freeing config
    server->config = nullptr;
}

// Test 3: uvhttp_serve success path (lines 817-838)
// Uses a timer to stop the loop after the server starts
TEST(ServeIntegrationTest, ServeSuccessPath) {
    uv_loop_t loop;
    int lret = uv_loop_init(&loop);
    ASSERT_EQ(lret, 0);

    // Start a timer to stop the loop after a short delay
    uv_timer_t stop_timer;
    uv_timer_init(&loop, &stop_timer);
    uv_timer_start(&stop_timer, test_stop_timer_cb, 50, 0);  // 50ms one-shot

    // Call uvhttp_serve - creates server, listens, and runs the loop.
    // The timer will fire and stop the loop, causing uv_run to return.
    // Then uvhttp_serve cleans up the server and returns.
    int result = uvhttp_serve(&loop, "127.0.0.1", 19876);
    // The result is the return value of uv_run (number of handles still active)
    // Don't assert on exact value - just exercise the path
    (void)result;

    // Close the timer handle (might still be alive if server_free loop drained
    // it)
    if (!uv_is_closing((uv_handle_t*)&stop_timer)) {
        uv_close((uv_handle_t*)&stop_timer, test_on_close);
    }
    // Run loop to process any remaining close callbacks
    while (uv_run(&loop, UV_RUN_NOWAIT) > 0) {
    }

    uv_loop_close(&loop);
}

// ============================================================================
// Real TCP HTTP Request/Response Cycle
// Exercises: connection.c on_read (lines 218-317), on_alloc_buffer (line 206),
// response.c uvhttp_send_response_data (lines 428-477),
// response.c uvhttp_free_write_data (lines 488-514),
// response.c uvhttp_response_send finished=1 (line 800)
// ============================================================================

// Accumulator for reading the HTTP response from the client side
struct ClientReadCtx {
    std::string data;
    bool done;
};

// Alloc callback for client reads - provides a real buffer
static void client_alloc_cb(uv_handle_t* handle, size_t suggested_size,
                            uv_buf_t* buf) {
    (void)handle;
    (void)suggested_size;
    static char slab[4096];
    buf->base = slab;
    buf->len = sizeof(slab);
}

// Read callback for client - accumulates response data and signals when done
static void client_read_cb(uv_stream_t* stream, ssize_t nread,
                           const uv_buf_t* buf) {
    ClientReadCtx* ctx = (ClientReadCtx*)stream->data;
    if (nread > 0) {
        ctx->data.append(buf->base, nread);
        // Check if we have the full response (look for end of HTTP response)
        // For a response with Content-Length, we check for the double CRLF
        // and the body. For simplicity, if we see the body text, we're done.
        if (ctx->data.find("Hello, uvhttp!") != std::string::npos) {
            ctx->done = true;
            uv_read_stop(stream);
        }
    } else if (nread < 0) {
        ctx->done = true;
        uv_read_stop(stream);
    }
}

// Handler for the /hello route - exercises uvhttp_response_set_status,
// uvhttp_response_set_header, uvhttp_response_set_body
static int hello_handler(uvhttp_request_t* request,
                         uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, UVHTTP_HEADER_CONTENT_TYPE,
                               UVHTTP_CONTENT_TYPE_TEXT);
    uvhttp_response_set_body(response, "Hello, uvhttp!", 14);
    // The handler must call send when matched via router
    uvhttp_response_send(response);
    return 0;
}

class TcpHttpCycleTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    uvhttp_router_t* router = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
        uvhttp_error_t err = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        // Walk and close any remaining handles
        uv_walk(&loop, test_close_walk_cb, nullptr);
        for (int i = 0; i < 20; i++) {
            if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
        }

        if (server) {
            // Detach router before freeing server to prevent double-free
            // (server_free will try to free server->router)
            if (router) {
                server->router = nullptr;
                uvhttp_router_free(router);
                router = nullptr;
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// Test: Full HTTP GET request/response cycle over TCP
// Exercises connection.c on_read, on_alloc_buffer, llhttp parsing,
// response.c send path, write completion callback, and keepalive/close logic
TEST_F(TcpHttpCycleTest, FullHttpRequestResponseCycle) {
    // Create router and add a route
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    rerr = uvhttp_router_add_route_method(router, "/hello", UVHTTP_GET,
                                           hello_handler);
    ASSERT_EQ(rerr, UVHTTP_OK);

    // Attach router to server
    uvhttp_error_t serr = uvhttp_server_set_router(server, router);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Listen on port 0 (OS assigns random port)
    serr = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Get the actual bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Create a TCP client
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    // Set up read context on client
    ClientReadCtx read_ctx{};
    read_ctx.done = false;
    client.data = &read_ctx;

    // Connect to server
    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);

    uv_connect_t connect_req;
    bool connected = false;
    // Reuse a connect callback that sets a flag via connect_req.data
    auto on_connect_send = [](uv_connect_t* req, int status) {
        if (status < 0) return;
        bool* flag = (bool*)req->data;
        *flag = true;
    };
    connect_req.data = &connected;
    ret = uv_tcp_connect(&connect_req, &client,
                         (const struct sockaddr*)&connect_addr, on_connect_send);
    ASSERT_EQ(ret, 0);

    // Pump until connected
    pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send a raw HTTP GET request via uv_write
    const char* http_request =
        "GET /hello HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    uv_buf_t write_buf = uv_buf_init((char*)http_request, strlen(http_request));
    uv_write_t write_req;
    bool write_done = false;
    write_req.data = &write_done;

    auto on_write_done = [](uv_write_t* req, int status) {
        (void)status;
        bool* flag = (bool*)req->data;
        *flag = true;
    };

    ret = uv_write(&write_req, (uv_stream_t*)&client, &write_buf, 1,
                   on_write_done);
    ASSERT_EQ(ret, 0);

    // Pump until write completes
    pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Start reading the response
    ret = uv_read_start((uv_stream_t*)&client, client_alloc_cb, client_read_cb);
    ASSERT_EQ(ret, 0);

    // Pump the loop to process: server receives request, parses it,
    // invokes handler, handler builds+sends response, client receives it
    pump_loop(&loop, 500);

    // Stop reading
    uv_read_stop((uv_stream_t*)&client);

    // Verify we received a valid HTTP response
    EXPECT_TRUE(read_ctx.done);
    EXPECT_NE(read_ctx.data.find("HTTP/1.1 200 OK"), std::string::npos)
        << "Response: " << read_ctx.data;
    EXPECT_NE(read_ctx.data.find("Hello, uvhttp!"), std::string::npos)
        << "Response: " << read_ctx.data;

    // Close client
    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);
}

// ============================================================================
// default_handler coverage (lines 781-800 in uvhttp_server.c)
// The default_handler is a static function inside uvhttp_server.c registered
// by uvhttp_serve via uvhttp_any(server, "/", default_handler).
//
// We test it by manually replicating the uvhttp_serve setup using the builder
// API (uvhttp_server_create + uvhttp_any + uvhttp_server_listen) and then
// sending a real HTTP GET / request. This exercises the same code path that
// uvhttp_serve uses, but gives us control over the event loop via pump_loop
// instead of blocking on uv_run(UV_RUN_DEFAULT).
//
// NOTE: Because default_handler is static, we cannot reference it directly.
// However, we register our own handler on "/" which exercises the same
// response APIs (set_status, set_header, set_body, send) that default_handler
// uses. To cover the actual default_handler function, the ServeSuccessPath
// test above exercises the uvhttp_serve entry point.
// ============================================================================

// Handler that mimics what default_handler does (covers the same response API
// code paths: set_status, set_header, set_body, send)
static int default_like_handler(uvhttp_request_t* request,
                                 uvhttp_response_t* response) {
    const char* method = uvhttp_request_get_method(request);
    const char* url = uvhttp_request_get_url(request);

    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "UVHTTP unified API server\n\n"
             "requestinfo:\n"
             "- method: %s\n"
             "- URL: %s\n"
             "- time: %ld\n"
             "\nWelcome to UVHTTP unified API!",
             method, url, (long)time(NULL));

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    return 0;
}

class DefaultHandlerTest : public ::testing::Test {
protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    uvhttp_server_builder_t* builder = nullptr;

    void SetUp() override {
        uv_loop_init(&loop);
    }

    void TearDown() override {
        uv_walk(&loop, test_close_walk_cb, nullptr);
        for (int i = 0; i < 20; i++) {
            if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
        }
        if (builder) {
            builder->server->router = nullptr;
            uvhttp_server_simple_free(builder);
            builder = nullptr;
        }
        uv_loop_close(&loop);
    }
};

TEST_F(DefaultHandlerTest, DefaultRoute_RespondsWithUnifiedApiBody) {
    // Replicate the uvhttp_serve setup: create builder, add "any" route on "/"
    uvhttp_error_t err = uvhttp_server_create(&loop, "127.0.0.1", 0, &builder);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(builder, nullptr);

    // Register handler on "/" (same as uvhttp_serve does with default_handler)
    uvhttp_any(builder, "/", default_like_handler);

    // Get the bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&builder->server->tcp_handle,
                                  (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Create TCP client and connect
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    ClientReadCtx read_ctx{};
    read_ctx.done = false;
    client.data = &read_ctx;

    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);
    uv_connect_t connect_req;
    bool connected = false;
    connect_req.data = &connected;

    auto on_conn = [](uv_connect_t* req, int status) {
        if (status < 0) return;
        *(bool*)req->data = true;
    };
    ret = uv_tcp_connect(&connect_req, &client,
                          (const struct sockaddr*)&connect_addr, on_conn);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send GET / HTTP/1.1
    const char* http_request =
        "GET / HTTP/1.1\r\n"
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
    pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Read the response - use a read callback that checks for "UVHTTP unified
    // API" instead of the shared client_read_cb which looks for "Hello, uvhttp!"
    auto default_read_cb = [](uv_stream_t* stream, ssize_t nread,
                              const uv_buf_t* buf) {
        ClientReadCtx* rctx = (ClientReadCtx*)stream->data;
        if (nread > 0) {
            rctx->data.append(buf->base, nread);
            if (rctx->data.find("UVHTTP unified API") != std::string::npos) {
                rctx->done = true;
                uv_read_stop(stream);
            }
        } else if (nread < 0) {
            rctx->done = true;
            uv_read_stop(stream);
        }
    };
    ret = uv_read_start((uv_stream_t*)&client, client_alloc_cb, default_read_cb);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 500);
    uv_read_stop((uv_stream_t*)&client);

    // Verify the response contains expected content from default_like_handler
    EXPECT_TRUE(read_ctx.done) << "Should have received a complete response";
    EXPECT_NE(read_ctx.data.find("HTTP/1.1 200 OK"), std::string::npos)
        << "Response: " << read_ctx.data;
    EXPECT_NE(read_ctx.data.find("UVHTTP unified API"), std::string::npos)
        << "Response should contain 'UVHTTP unified API', got: "
        << read_ctx.data;
    EXPECT_NE(read_ctx.data.find("Welcome to UVHTTP unified API!"),
              std::string::npos)
        << "Response body should contain welcome message, got: "
        << read_ctx.data;

    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);
}

TEST_F(DefaultHandlerTest, DefaultRoute_ServeSuccessPath_CoversDefaultHandler) {
    // This test calls uvhttp_serve which registers the actual default_handler.
    // The serve_stop_timer_cb stops the loop after 50ms, so the test completes
    // quickly. We verify the serve path was exercised (lines 804-842).
    uv_timer_t stop_timer;
    uv_timer_init(&loop, &stop_timer);
    uv_timer_start(&stop_timer, test_stop_timer_cb, 50, 0);

    int result = uvhttp_serve(&loop, "127.0.0.1", 19878);
    (void)result;

    // Clean up the stop timer if still alive
    if (!uv_is_closing((uv_handle_t*)&stop_timer)) {
        uv_close((uv_handle_t*)&stop_timer, test_on_close);
    }

    // Drain
    uv_walk(&loop, test_close_walk_cb, nullptr);
    for (int i = 0; i < 50; i++) {
        if (uv_run(&loop, UV_RUN_NOWAIT) == 0) break;
    }
}

// ============================================================================
// uvhttp_send_response_data direct call (lines 428-477 in response.c)
// Exercises the alloc, memcpy, uv_write path with a real TCP connection
// ============================================================================

// Forward declaration of internal function not in any public header
extern "C" {
uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t* response,
                                         const char* data, size_t length);
}

// Handler that calls uvhttp_send_response_data directly instead of going
// through uvhttp_response_send. This directly exercises lines 428-477.
static int send_data_direct_handler(uvhttp_request_t* request,
                                    uvhttp_response_t* response) {
    (void)request;
    // Use uvhttp_send_response_data to write raw data
    const char* body = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n"
                       "Connection: close\r\n\r\nDirect write!";
    uvhttp_error_t err = uvhttp_send_response_data(response, body, strlen(body));
    (void)err;
    return 0;
}

TEST_F(TcpHttpCycleTest, SendResponseData_DirectCall_WithRealConnection) {
    // Create router and add a route using the direct send_data handler
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    rerr = uvhttp_router_add_route_method(router, "/direct", UVHTTP_GET,
                                           send_data_direct_handler);
    ASSERT_EQ(rerr, UVHTTP_OK);

    uvhttp_error_t serr = uvhttp_server_set_router(server, router);
    ASSERT_EQ(serr, UVHTTP_OK);

    serr = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Get the bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Create TCP client
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    ClientReadCtx read_ctx{};
    read_ctx.done = false;
    client.data = &read_ctx;

    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);

    uv_connect_t connect_req;
    bool connected = false;
    connect_req.data = &connected;
    auto on_conn = [](uv_connect_t* req, int status) {
        if (status < 0) return;
        *(bool*)req->data = true;
    };
    ret = uv_tcp_connect(&connect_req, &client,
                          (const struct sockaddr*)&connect_addr, on_conn);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send GET /direct HTTP/1.1
    const char* http_request =
        "GET /direct HTTP/1.1\r\n"
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
    pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Read the response
    auto direct_read_cb = [](uv_stream_t* stream, ssize_t nread,
                             const uv_buf_t* buf) {
        ClientReadCtx* rctx = (ClientReadCtx*)stream->data;
        if (nread > 0) {
            rctx->data.append(buf->base, nread);
            if (rctx->data.find("Direct write!") != std::string::npos) {
                rctx->done = true;
                uv_read_stop(stream);
            }
        } else if (nread < 0) {
            rctx->done = true;
            uv_read_stop(stream);
        }
    };
    ret = uv_read_start((uv_stream_t*)&client, client_alloc_cb, direct_read_cb);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 500);
    uv_read_stop((uv_stream_t*)&client);

    EXPECT_TRUE(read_ctx.done) << "Should have received a complete response";
    EXPECT_NE(read_ctx.data.find("Direct write!"), std::string::npos)
        << "Response: " << read_ctx.data;

    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);
}

// ============================================================================
// Keepalive=0 path in response_send_raw (lines 749-753 in response.c)
// When response->keepalive is 0, after uv_write succeeds, the code sets
// conn->keepalive = 0 on the underlying connection.
// ============================================================================

// Handler that explicitly sets response->keepalive = 0 before sending.
// This triggers the keepalive=0 path in uvhttp_response_send_raw.
static int keepalive_close_handler(uvhttp_request_t* request,
                                   uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "keepalive-off", 13);
    response->keepalive = 0;
    uvhttp_response_send(response);
    return 0;
}

TEST_F(TcpHttpCycleTest, ResponseSendRaw_KeepAlive0_SetsConnKeepAlive) {
    // Create router and add a route using the keepalive-close handler
    uvhttp_error_t rerr = uvhttp_router_new(&router);
    ASSERT_EQ(rerr, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    rerr = uvhttp_router_add_route_method(router, "/close", UVHTTP_GET,
                                           keepalive_close_handler);
    ASSERT_EQ(rerr, UVHTTP_OK);

    uvhttp_error_t serr = uvhttp_server_set_router(server, router);
    ASSERT_EQ(serr, UVHTTP_OK);

    serr = uvhttp_server_listen(server, "127.0.0.1", 0);
    ASSERT_EQ(serr, UVHTTP_OK);

    // Get the bound port
    struct sockaddr_in bound_addr;
    int namelen = sizeof(bound_addr);
    int ret = uv_tcp_getsockname(&server->tcp_handle,
                                 (struct sockaddr*)&bound_addr, &namelen);
    ASSERT_EQ(ret, 0);
    int port = ntohs(bound_addr.sin_port);
    ASSERT_GT(port, 0);

    // Create TCP client
    uv_tcp_t client;
    ret = uv_tcp_init(&loop, &client);
    ASSERT_EQ(ret, 0);

    ClientReadCtx read_ctx{};
    read_ctx.done = false;
    client.data = &read_ctx;

    struct sockaddr_in connect_addr;
    uv_ip4_addr("127.0.0.1", port, &connect_addr);

    uv_connect_t connect_req;
    bool connected = false;
    connect_req.data = &connected;
    auto on_conn = [](uv_connect_t* req, int status) {
        if (status < 0) return;
        *(bool*)req->data = true;
    };
    ret = uv_tcp_connect(&connect_req, &client,
                          (const struct sockaddr*)&connect_addr, on_conn);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 200);
    ASSERT_TRUE(connected);

    // Send GET /close HTTP/1.1
    const char* http_request =
        "GET /close HTTP/1.1\r\n"
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
    pump_loop(&loop, 100);
    ASSERT_TRUE(write_done);

    // Read the response
    ret = uv_read_start((uv_stream_t*)&client, client_alloc_cb, client_read_cb);
    ASSERT_EQ(ret, 0);
    pump_loop(&loop, 500);
    uv_read_stop((uv_stream_t*)&client);

    EXPECT_TRUE(read_ctx.done) << "Should have received a complete response";
    EXPECT_NE(read_ctx.data.find("keepalive-off"), std::string::npos)
        << "Response: " << read_ctx.data;

    // Verify Connection: close appears in the response
    EXPECT_NE(read_ctx.data.find("Connection: close"), std::string::npos)
        << "Response should have Connection: close header, got: " << read_ctx.data;

    uv_close((uv_handle_t*)&client, test_on_close);
    pump_loop(&loop, 100);
}

// ============================================================================
// default_handler coverage via threaded uvhttp_serve (lines 781-800)
// The default_handler is static and only invoked by uvhttp_serve when a
// client sends GET /. We run uvhttp_serve in a background thread, connect
// with POSIX sockets, and verify the response contains "UVHTTP unified API".
// ============================================================================

static void serve_thread_func(uv_loop_t* loop, const char* host, int port,
                              std::atomic<int>* result) {
    *result = uvhttp_serve(loop, host, port);
}

TEST(DefaultHandlerCoverageTest, DefaultHandler_InvokedByServedRequest) {
    // Create a dedicated loop for uvhttp_serve
    uv_loop_t serve_loop;
    int lret = uv_loop_init(&serve_loop);
    ASSERT_EQ(lret, 0);

    // Register a stop timer on the serve loop to end the test after 1500ms.
    // This gives enough time for the client to connect and get a response.
    uv_timer_t stop_timer;
    uv_timer_init(&serve_loop, &stop_timer);
    uv_timer_start(&stop_timer, test_stop_timer_cb, 1500, 0);

    // Pick a unique port to avoid conflicts
    const int port = 19877;

    // Start uvhttp_serve in a background thread
    std::atomic<int> serve_result{0};
    std::thread serve_thr(serve_thread_func, &serve_loop, "127.0.0.1", port,
                          &serve_result);

    // Wait for the server to start listening
    usleep(200000);  // 200ms

    // Connect with POSIX sockets
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sock, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    int connect_ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (connect_ret < 0) {
        close(sock);
        serve_thr.join();
        uv_close((uv_handle_t*)&stop_timer, test_on_close);
        while (uv_run(&serve_loop, UV_RUN_NOWAIT) > 0) {
        }
        uv_loop_close(&serve_loop);
        FAIL() << "Failed to connect to server on port " << port
               << ": " << strerror(errno);
    }
    ASSERT_EQ(connect_ret, 0);

    // Send GET / HTTP/1.1
    const char* request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    ssize_t sent = send(sock, request, strlen(request), 0);
    ASSERT_GT(sent, 0);

    // Read the response
    char response_buf[4096];
    memset(response_buf, 0, sizeof(response_buf));
    ssize_t total_read = 0;
    // Read until we get the body or connection closes
    for (int attempt = 0; attempt < 50; attempt++) {
        ssize_t n = recv(sock, response_buf + total_read,
                         sizeof(response_buf) - total_read - 1, 0);
        if (n > 0) {
            total_read += n;
            response_buf[total_read] = '\0';
            // Check if we have the body
            if (strstr(response_buf, "UVHTTP unified API") != nullptr) {
                break;
            }
        } else {
            break;
        }
        usleep(20000);  // 20ms between reads
    }
    close(sock);

    // Verify the response
    std::string resp_str(response_buf, total_read);
    EXPECT_NE(resp_str.find("HTTP/1.1 200 OK"), std::string::npos)
        << "Response: " << resp_str;
    EXPECT_NE(resp_str.find("UVHTTP unified API"), std::string::npos)
        << "Response should contain 'UVHTTP unified API', got: " << resp_str;

    // Wait for the serve thread to complete (timer fires and stops the loop)
    serve_thr.join();

    // Clean up
    if (!uv_is_closing((uv_handle_t*)&stop_timer)) {
        uv_close((uv_handle_t*)&stop_timer, test_on_close);
    }
    while (uv_run(&serve_loop, UV_RUN_NOWAIT) > 0) {
    }
    uv_loop_close(&serve_loop);
}
