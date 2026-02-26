/*
 * Simple Memory Leak Test - Test library code for memory leaks
 */

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_context.h"
#include "uvhttp_server.h"

#include <gtest/gtest.h>
#include <string.h>

/* Simple memory leak test */
TEST(SimpleMemoryLeakTest, NoLeaksInBasicOperations) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);

    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);

    /* Cleanup */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);

    /* Run event loop to clean up all resources */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test connection lifecycle */
TEST(SimpleMemoryLeakTest, ConnectionLifecycle) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Create multiple connections */
    uvhttp_connection_t* conns[10];
    for (int i = 0; i < 10; i++) {
        result = uvhttp_connection_new(server, &conns[i]);
        ASSERT_EQ(result, UVHTTP_OK);
    }

    /* Free all connections */
    for (int i = 0; i < 10; i++) {
        uvhttp_connection_free(conns[i]);
    }

    uvhttp_server_free(server);

    /* Run event loop to clean up */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test context management */
TEST(SimpleMemoryLeakTest, ContextManagement) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_context_create(loop, &ctx);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(ctx, nullptr);

    uvhttp_context_destroy(ctx);

    /* Run event loop */
    for (int i = 0; i < 20; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test calling uvhttp_connection_free twice */
TEST(SimpleMemoryLeakTest, DoubleFreeProtection) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* First call to free */
    uvhttp_connection_free(conn);

    /* Second call to free - should handle safely */
    uvhttp_connection_free(conn);

    uvhttp_server_free(server);

    /* Run event loop to clean up */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test NULL pointer handling */
TEST(SimpleMemoryLeakTest, NullPointerHandling) {
    /* Test NULL connection */
    uvhttp_connection_free(nullptr);

    /* Test NULL server */
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(nullptr, &server);
    ASSERT_NE(result, UVHTTP_OK);

    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    /* Test NULL context */
    uvhttp_context_t* ctx = nullptr;
    result = uvhttp_context_create(loop, nullptr);
    ASSERT_NE(result, UVHTTP_OK);

    result = uvhttp_context_create(loop, &ctx);
    ASSERT_EQ(result, UVHTTP_OK);
    uvhttp_context_destroy(ctx);

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test rapid create and destroy */
TEST(SimpleMemoryLeakTest, RapidCreateDestroy) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Rapidly create and destroy multiple connections */
    for (int i = 0; i < 100; i++) {
        uvhttp_connection_t* conn = nullptr;
        result = uvhttp_connection_new(server, &conn);
        ASSERT_EQ(result, UVHTTP_OK);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);

    /* Run event loop to clean up */
    for (int i = 0; i < 100; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* Test connection resource integrity */
TEST(SimpleMemoryLeakTest, ConnectionResourceIntegrity) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* Verify connection internal resources are properly allocated */
    ASSERT_NE(conn, nullptr);
    ASSERT_NE(conn->read_buffer, nullptr);
    ASSERT_NE(conn->request, nullptr);
    ASSERT_NE(conn->response, nullptr);

    uvhttp_connection_free(conn);
    uvhttp_server_free(server);

    /* Run event loop to clean up */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}