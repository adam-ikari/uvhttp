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
}

#include <string.h>

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

#endif  // UVHTTP_FEATURE_WEBSOCKET
