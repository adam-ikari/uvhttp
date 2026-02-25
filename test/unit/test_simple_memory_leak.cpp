/*
 * Simple Memory Leak Test - 测试库代码是否有内存泄漏
 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_context.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 简单的内存泄漏测试 */
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

    /* 清理 */
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);

    /* 运行事件循环清理所有资源 */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试连接生命周期 */
TEST(SimpleMemoryLeakTest, ConnectionLifecycle) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    /* 创建多个连接 */
    uvhttp_connection_t* conns[10];
    for (int i = 0; i < 10; i++) {
        result = uvhttp_connection_new(server, &conns[i]);
        ASSERT_EQ(result, UVHTTP_OK);
    }

    /* 释放所有连接 */
    for (int i = 0; i < 10; i++) {
        uvhttp_connection_free(conns[i]);
    }

    uvhttp_server_free(server);

    /* 运行事件循环清理 */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试上下文管理 */
TEST(SimpleMemoryLeakTest, ContextManagement) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_context_create(loop, &ctx);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(ctx, nullptr);

    uvhttp_context_destroy(ctx);

    /* 运行事件循环 */
    for (int i = 0; i < 20; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试双重调用 uvhttp_connection_free */
TEST(SimpleMemoryLeakTest, DoubleFreeProtection) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* 第一次调用 free */
    uvhttp_connection_free(conn);

    /* 第二次调用 free - 应该安全地处理 */
    uvhttp_connection_free(conn);

    uvhttp_server_free(server);

    /* 运行事件循环清理 */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试 NULL 指针处理 */
TEST(SimpleMemoryLeakTest, NullPointerHandling) {
    /* 测试 NULL 连接 */
    uvhttp_connection_free(nullptr);

    /* 测试 NULL 服务器 */
    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(nullptr, &server);
    ASSERT_NE(result, UVHTTP_OK);

    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    /* 测试 NULL 上下文 */
    uvhttp_context_t* ctx = nullptr;
    result = uvhttp_context_create(loop, nullptr);
    ASSERT_NE(result, UVHTTP_OK);

    result = uvhttp_context_create(loop, &ctx);
    ASSERT_EQ(result, UVHTTP_OK);
    uvhttp_context_destroy(ctx);

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试快速创建和销毁 */
TEST(SimpleMemoryLeakTest, RapidCreateDestroy) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    /* 快速创建和销毁多个连接 */
    for (int i = 0; i < 100; i++) {
        uvhttp_connection_t* conn = nullptr;
        result = uvhttp_connection_new(server, &conn);
        ASSERT_EQ(result, UVHTTP_OK);
        uvhttp_connection_free(conn);
    }

    uvhttp_server_free(server);

    /* 运行事件循环清理 */
    for (int i = 0; i < 100; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* 测试连接资源完整性 */
TEST(SimpleMemoryLeakTest, ConnectionResourceIntegrity) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = nullptr;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_connection_t* conn = nullptr;
    result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);

    /* 验证连接内部资源已正确分配 */
    ASSERT_NE(conn, nullptr);
    ASSERT_NE(conn->read_buffer, nullptr);
    ASSERT_NE(conn->request, nullptr);
    ASSERT_NE(conn->response, nullptr);

    uvhttp_connection_free(conn);
    uvhttp_server_free(server);

    /* 运行事件循环清理 */
    for (int i = 0; i < 50; i++) {
        uv_run(loop, UV_RUN_ONCE);
    }

    uv_loop_close(loop);
    uvhttp_free(loop);
}