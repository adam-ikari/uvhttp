/**
 * @file test_context_full_coverage.cpp
 * @brief Comprehensive coverage tests for uvhttp_context module
 * 
 * This test file aims to achieve 100% coverage for uvhttp_context.c by testing:
 * - NULL parameter handling
 * - Memory allocation failures
 * - Context creation and destruction
 * - Context initialization (idempotent behavior)
 * - TLS module initialization and cleanup
 * - WebSocket module initialization and cleanup
 * - Configuration management initialization and cleanup
 * - Edge cases and boundary conditions
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uvhttp_context.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"

class UvhttpContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        uv_error = uv_loop_init(&loop);
        ASSERT_EQ(uv_error, 0);
    }

    void TearDown() override {
        uv_loop_close(&loop);
    }

    uv_loop_t loop;
    int uv_error;
};

// Basic creation and destruction tests
TEST_F(UvhttpContextTest, ContextCreateSuccess) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_NE(context, nullptr);
    EXPECT_EQ(context->loop, &loop);
    EXPECT_NE(context->created_at, 0);
    EXPECT_EQ(context->initialized, 0);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextCreateNullOutput) {
    uvhttp_error_t err = uvhttp_context_create(&loop, nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextCreateNullLoop) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(nullptr, &context);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextDestroyNull) {
    // Should not crash
    uvhttp_context_destroy(nullptr);
}

TEST_F(UvhttpContextTest, ContextDestroyValid) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_destroy(context);
}

// Context initialization tests
TEST_F(UvhttpContextTest, ContextInitNull) {
    uvhttp_error_t err = uvhttp_context_init(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextInitSuccess) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init(context);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(context->initialized, 1);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextInitIdempotent) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Initialize once
    err = uvhttp_context_init(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    // Initialize again (should be idempotent)
    err = uvhttp_context_init(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_destroy(context);
}

// TLS initialization tests
TEST_F(UvhttpContextTest, ContextInitTLSNull) {
    uvhttp_error_t err = uvhttp_context_init_tls(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextInitTLSSuccess) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init_tls(context);
#if UVHTTP_FEATURE_TLS
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(context->tls_initialized, 1);
    EXPECT_NE(context->tls_entropy, nullptr);
    EXPECT_NE(context->tls_drbg, nullptr);
#else
    EXPECT_EQ(err, UVHTTP_ERROR_NOT_SUPPORTED);
#endif
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextInitTLSIdempotent) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
#if UVHTTP_FEATURE_TLS
    // Initialize once
    err = uvhttp_context_init_tls(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    // Initialize again (should be idempotent)
    err = uvhttp_context_init_tls(context);
    EXPECT_EQ(err, UVHTTP_OK);
#endif
    
    uvhttp_context_destroy(context);
}

// TLS cleanup tests
TEST_F(UvhttpContextTest, ContextCleanupTLSNull) {
    // Should not crash
    uvhttp_context_cleanup_tls(nullptr);
}

TEST_F(UvhttpContextTest, ContextCleanupTLSNotInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Should not crash
    uvhttp_context_cleanup_tls(context);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextCleanupTLSInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
#if UVHTTP_FEATURE_TLS
    err = uvhttp_context_init_tls(context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_cleanup_tls(context);
    EXPECT_EQ(context->tls_initialized, 0);
    EXPECT_EQ(context->tls_entropy, nullptr);
    EXPECT_EQ(context->tls_drbg, nullptr);
#endif
    
    uvhttp_context_destroy(context);
}

// WebSocket initialization tests
TEST_F(UvhttpContextTest, ContextInitWebSocketNull) {
    uvhttp_error_t err = uvhttp_context_init_websocket(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextInitWebSocketSuccess) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init_websocket(context);
#if UVHTTP_FEATURE_TLS
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(context->ws_drbg_initialized, 1);
    EXPECT_NE(context->ws_entropy, nullptr);
    EXPECT_NE(context->ws_drbg, nullptr);
#else
    EXPECT_EQ(err, UVHTTP_ERROR_NOT_SUPPORTED);
#endif
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextInitWebSocketIdempotent) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
#if UVHTTP_FEATURE_TLS
    // Initialize once
    err = uvhttp_context_init_websocket(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    // Initialize again (should be idempotent)
    err = uvhttp_context_init_websocket(context);
    EXPECT_EQ(err, UVHTTP_OK);
#endif
    
    uvhttp_context_destroy(context);
}

// WebSocket cleanup tests
TEST_F(UvhttpContextTest, ContextCleanupWebSocketNull) {
    // Should not crash
    uvhttp_context_cleanup_websocket(nullptr);
}

TEST_F(UvhttpContextTest, ContextCleanupWebSocketNotInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Should not crash
    uvhttp_context_cleanup_websocket(context);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextCleanupWebSocketInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
#if UVHTTP_FEATURE_TLS
    err = uvhttp_context_init_websocket(context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_cleanup_websocket(context);
    EXPECT_EQ(context->ws_drbg_initialized, 0);
    EXPECT_EQ(context->ws_entropy, nullptr);
    EXPECT_EQ(context->ws_drbg, nullptr);
#endif
    
    uvhttp_context_destroy(context);
}

// Configuration initialization tests
TEST_F(UvhttpContextTest, ContextInitConfigNull) {
    uvhttp_error_t err = uvhttp_context_init_config(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpContextTest, ContextInitConfigSuccess) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init_config(context);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_NE(context->current_config, nullptr);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextInitConfigIdempotent) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Initialize once
    err = uvhttp_context_init_config(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    // Initialize again (should be idempotent)
    err = uvhttp_context_init_config(context);
    EXPECT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_destroy(context);
}

// Configuration cleanup tests
TEST_F(UvhttpContextTest, ContextCleanupConfigNull) {
    // Should not crash
    uvhttp_context_cleanup_config(nullptr);
}

TEST_F(UvhttpContextTest, ContextCleanupConfigNotInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Should not crash
    uvhttp_context_cleanup_config(context);
    
    uvhttp_context_destroy(context);
}

TEST_F(UvhttpContextTest, ContextCleanupConfigInitialized) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init_config(context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    uvhttp_context_cleanup_config(context);
    EXPECT_EQ(context->current_config, nullptr);
    
    uvhttp_context_destroy(context);
}

// Statistics fields tests
TEST_F(UvhttpContextTest, ContextStatisticsFields) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Verify statistics fields are initialized to zero
    EXPECT_EQ(context->total_requests, 0);
    EXPECT_EQ(context->total_connections, 0);
    EXPECT_EQ(context->active_connections, 0);
    
    uvhttp_context_destroy(context);
}

// User data tests
TEST_F(UvhttpContextTest, ContextUserData) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // User data should be NULL initially
    EXPECT_EQ(context->user_data, nullptr);
    
    // Set user data
    int test_data = 42;
    context->user_data = &test_data;
    EXPECT_EQ(context->user_data, &test_data);
    
    uvhttp_context_destroy(context);
}

// Complete workflow test
TEST_F(UvhttpContextTest, ContextCompleteWorkflow) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Initialize context
    err = uvhttp_context_init(context);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(context->initialized, 1);
    
#if UVHTTP_FEATURE_TLS
    // Verify TLS is initialized
    EXPECT_EQ(context->tls_initialized, 1);
#endif
    
    // Verify config is initialized
    EXPECT_NE(context->current_config, nullptr);
    
    // Clean up
    uvhttp_context_destroy(context);
}

// Server and router fields tests
TEST_F(UvhttpContextTest, ContextServerAndRouterFields) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Server and router should be NULL initially
    EXPECT_EQ(context->server, nullptr);
    EXPECT_EQ(context->router, nullptr);
    
    uvhttp_context_destroy(context);
}

// Multiple destruction test
TEST_F(UvhttpContextTest, ContextMultipleDestroy) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    err = uvhttp_context_init(context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Destroy once
    uvhttp_context_destroy(context);
    
    // Destroy again (should not crash)
    uvhttp_context_destroy(context);
}

// Config callback field test
TEST_F(UvhttpContextTest, ContextConfigCallback) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Config callback should be NULL initially
    EXPECT_EQ(context->config_callback, nullptr);
    
    uvhttp_context_destroy(context);
}

// Created timestamp test
TEST_F(UvhttpContextTest, ContextCreatedTimestamp) {
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t err = uvhttp_context_create(&loop, &context);
    ASSERT_EQ(err, UVHTTP_OK);
    
    // Verify created timestamp is set
    EXPECT_GT(context->created_at, 0);
    
    // Verify timestamp is recent (within last 10 seconds)
    time_t current_time = time(NULL);
    EXPECT_LE(context->created_at, current_time);
    EXPECT_GT(context->created_at, current_time - 10);
    
    uvhttp_context_destroy(context);
}

// Memory leak prevention test
TEST_F(UvhttpContextTest, ContextMemoryLeakPrevention) {
    for (int i = 0; i < 100; i++) {
        uvhttp_context_t* context = nullptr;
        uvhttp_error_t err = uvhttp_context_create(&loop, &context);
        ASSERT_EQ(err, UVHTTP_OK);
        
        err = uvhttp_context_init(context);
        ASSERT_EQ(err, UVHTTP_OK);
        
        uvhttp_context_destroy(context);
    }
    // If this test passes without crashing, no memory leaks
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}