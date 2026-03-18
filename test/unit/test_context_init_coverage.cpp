/**
 * @file test_context_init_coverage.cpp
 * @brief Coverage tests for uvhttp_context initialization
 * 
 * This test file aims to improve coverage for uvhttp_context.c by testing:
 * - Full context initialization flow
 * - Idempotent initialization
 * - NULL parameter handling
 * - Context destruction
 */

#include <gtest/gtest.h>
#include <stddef.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "uvhttp_error.h"

// Test full context initialization flow
TEST(UvhttpContextInitCoverageTest, FullInitialization) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_context_t* context = nullptr;
    uvhttp_error_t result = uvhttp_context_create(loop, &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);

    // Test complete initialization
    result = uvhttp_context_init(context);
    ASSERT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(context->initialized, 1);

    // Test idempotency - re-initialization should succeed
    result = uvhttp_context_init(context);
    ASSERT_EQ(result, UVHTTP_OK);

    // Cleanup
    uvhttp_context_destroy(context);
    uv_loop_delete(loop);
}

// Test context initialization failure scenarios
TEST(UvhttpContextInitCoverageTest, InitWithNullContext) {
    uvhttp_error_t result = uvhttp_context_init(nullptr);
    ASSERT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// Test context destruction with null pointer
TEST(UvhttpContextInitCoverageTest, DestroyNullContext) {
    uvhttp_context_destroy(nullptr);
    // Should not crash
}