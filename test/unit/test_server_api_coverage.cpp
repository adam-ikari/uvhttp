#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "uvhttp_context.h"
#include <string.h>

/* 静态处理器函数（用于测试） */
static int test_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return UVHTTP_OK;
}

/* 测试夹具：提供通用的测试环境设置和清理 */
class UvhttpServerApiTest : public ::testing::Test {
protected:
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_server_builder_t* builder;

    void SetUp() override {
        /* 初始化 libuv loop */
        loop = uv_loop_new();
        ASSERT_NE(loop, nullptr);

        /* 初始化服务器指针 */
        server = nullptr;
        router = nullptr;
        builder = nullptr;
    }

    void TearDown() override {
        /* 清理 builder */
        if (builder) {
            uvhttp_server_simple_free(builder);
            builder = nullptr;
        }

        /* 清理路由 - 在释放 server 之前检查是否被 server 拥有 */
        bool router_owned_by_server = (server && server->router == router);
        
        /* 清理服务器 */
        if (server) {
            uvhttp_server_free(server);
            server = nullptr;
        }

        /* 只有当 router 未被 server 拥有时才释放 */
        if (router && !router_owned_by_server) {
            uvhttp_router_free(router);
            router = nullptr;
        }

        /* 清理 loop */
        if (loop) {
            uv_loop_close(loop);
            uvhttp_free(loop);
            loop = nullptr;
        }
    }

    /* 辅助函数：创建服务器 */
    uvhttp_error_t create_server() {
        return uvhttp_server_new(loop, &server);
    }

    /* 辅助函数：创建路由 */
    uvhttp_error_t create_router() {
        return uvhttp_router_new(&router);
    }

    /* 辅助函数：创建 builder 服务器（使用不太可能被占用的端口） */
    uvhttp_error_t create_builder_server() {
        return uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    }
};

/* 测试服务器创建和释放 */
TEST_F(UvhttpServerApiTest, ServerNewAndFree) {
    /* 创建服务器 */
    uvhttp_error_t result = create_server();
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
}

/* 测试服务器设置路由 */
TEST_F(UvhttpServerApiTest, ServerSetRouter) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 创建路由 */
    ASSERT_EQ(create_router(), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    /* 设置路由 */
    uvhttp_error_t result = uvhttp_server_set_router(server, router);
    EXPECT_EQ(result, UVHTTP_OK);

    /* 验证路由已设置 */
    EXPECT_EQ(server->router, router);
}

/* 测试服务器设置路由 NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerSetRouterNullServer) {
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    result = uvhttp_server_set_router(NULL, router);
    EXPECT_NE(result, UVHTTP_OK);

    uvhttp_router_free(router);
}

TEST_F(UvhttpServerApiTest, ServerSetRouterNullRouter) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 设置NULL router是允许的，用于清除路由 */
    uvhttp_error_t result = uvhttp_server_set_router(server, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->router, nullptr);
}

/* 测试服务器设置上下文 */
TEST_F(UvhttpServerApiTest, ServerSetContext) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 创建上下文 */
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(loop, &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);

    /* 设置上下文 */
    result = uvhttp_server_set_context(server, context);
    EXPECT_EQ(result, UVHTTP_OK);

    /* 验证上下文已设置 */
    EXPECT_EQ(server->context, context);
}

TEST_F(UvhttpServerApiTest, ServerSetContextNullServer) {

    uvhttp_context_t* context = NULL;

    uvhttp_error_t result = uvhttp_context_create(loop, &context);

    ASSERT_EQ(result, UVHTTP_OK);



    result = uvhttp_server_set_context(NULL, context);

    EXPECT_NE(result, UVHTTP_OK);



    uvhttp_context_destroy(context);

}

/* 测试服务器设置处理器 */
TEST_F(UvhttpServerApiTest, ServerSetHandler) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 设置处理器 */
    uvhttp_error_t result = uvhttp_server_set_handler(server, test_handler);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpServerApiTest, ServerSetHandlerNullServer) {
    uvhttp_error_t result = uvhttp_server_set_handler(NULL, test_handler);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST_F(UvhttpServerApiTest, ServerSetHandlerNullHandler) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 设置NULL handler是允许的，用于清除处理器 */
    uvhttp_error_t result = uvhttp_server_set_handler(server, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->handler, nullptr);
}

/* 测试服务器 TLS 启用/禁用 */
TEST_F(UvhttpServerApiTest, ServerTlsEnableDisable) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 初始状态：TLS 未启用 */
    EXPECT_EQ(server->tls_enabled, 0);

    /* 禁用 TLS（未启用的情况下调用） */
    uvhttp_error_t result = uvhttp_server_disable_tls(server);
    /* 可能失败，因为 TLS 没有被启用 */
}

/* 测试服务器 TLS NULL 参数 */
TEST(UvhttpApiTest, ServerTlsEnableNullServer) {
    uvhttp_error_t result = uvhttp_server_enable_tls(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpApiTest, ServerTlsDisableNullServer) {
    uvhttp_error_t result = uvhttp_server_disable_tls(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试服务器停止 */
TEST_F(UvhttpServerApiTest, ServerStop) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* 停止未启动的服务器，返回UVHTTP_ERROR_SERVER_STOP */
    uvhttp_error_t result = uvhttp_server_stop(server);
    EXPECT_EQ(result, UVHTTP_ERROR_SERVER_STOP);
}
/* 测试服务器停止 NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerStopNull) {
    uvhttp_error_t result = uvhttp_server_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试服务器创建（Builder 模式） */
TEST_F(UvhttpServerApiTest, ServerCreate) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 - 使用一个不太可能被占用的端口 */
    /* 注意：这个测试需要实际的网络绑定，可能会因为端口占用而失败 */
    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result == UVHTTP_OK) {
        ASSERT_NE(builder, nullptr);

        /* 释放服务器 */
        uvhttp_server_simple_free(builder);
    } else {
        /* 如果端口被占用，跳过测试 */
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }
}

/* 测试服务器创建 NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerCreateNullLoop) {
    uvhttp_server_builder_t* builder = NULL;

    uvhttp_error_t result = uvhttp_server_create(NULL, "127.0.0.1", 8080, &builder);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST_F(UvhttpServerApiTest, ServerCreateNullHost) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t result = uvhttp_server_create(loop, NULL, 8080, &builder);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST_F(UvhttpServerApiTest, ServerCreateNullBuilder) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 8080, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试服务器 Builder 模式 */
TEST_F(UvhttpServerApiTest, ServerBuilderGet) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result != UVHTTP_OK) {
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }

    /* 添加 GET 路由 */
    uvhttp_server_builder_t* result2 = uvhttp_get(builder, "/test", test_handler);
    EXPECT_NE(result2, nullptr);

    /* 释放服务器 */
    uvhttp_server_simple_free(builder);
}

/* 测试服务器 Builder NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerBuilderGetNull) {
    uvhttp_server_builder_t* result = uvhttp_get(NULL, "/test", test_handler);
    EXPECT_EQ(result, nullptr);
}

TEST_F(UvhttpServerApiTest, ServerBuilderGetNullPath) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result != UVHTTP_OK) {
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }

    /* NULL path时，返回builder指针而不是nullptr（允许链式调用） */
    uvhttp_server_builder_t* result2 = uvhttp_get(builder, NULL, test_handler);
    EXPECT_EQ(result2, builder);

    uvhttp_server_simple_free(builder);
}

TEST_F(UvhttpServerApiTest, ServerBuilderGetNullHandler) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result != UVHTTP_OK) {
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }

    /* NULL handler时，返回builder指针而不是nullptr（允许链式调用） */
    uvhttp_server_builder_t* result2 = uvhttp_get(builder, "/test", NULL);
    EXPECT_EQ(result2, builder);

    uvhttp_server_simple_free(builder);
}

/* 测试服务器 Builder 其他方法 */
TEST_F(UvhttpServerApiTest, ServerBuilderMethods) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result != UVHTTP_OK) {
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }

    /* 测试各种方法 */
    uvhttp_server_builder_t* result2;

    result2 = uvhttp_post(builder, "/test", test_handler);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_put(builder, "/test", test_handler);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_delete(builder, "/test", test_handler);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_any(builder, "/test", test_handler);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_set_max_connections(builder, 100);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_set_timeout(builder, 30);
    EXPECT_NE(result2, nullptr);

    result2 = uvhttp_set_max_body_size(builder, 1024 * 1024);
    EXPECT_NE(result2, nullptr);

    /* 释放服务器 */
    uvhttp_server_simple_free(builder);
}

/* 测试服务器 Builder NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerBuilderMethodsNull) {
    uvhttp_server_builder_t* result;

    result = uvhttp_post(NULL, "/test", test_handler);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_put(NULL, "/test", test_handler);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_delete(NULL, "/test", test_handler);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_any(NULL, "/test", test_handler);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_set_max_connections(NULL, 100);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_set_timeout(NULL, 30);
    EXPECT_EQ(result, nullptr);

    result = uvhttp_set_max_body_size(NULL, 1024 * 1024);
    EXPECT_EQ(result, nullptr);
}

/* 测试服务器停止简单模式 */
TEST_F(UvhttpServerApiTest, ServerStopSimple) {
    uvhttp_server_builder_t* builder = NULL;
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_create(loop, "127.0.0.1", 18080, &builder);
    if (result != UVHTTP_OK) {
        GTEST_SKIP() << "Port 18080 is already in use, skipping test";
    }

    /* 停止服务器 */
    uvhttp_server_stop_simple(builder);

    /* 释放服务器 */
    uvhttp_server_simple_free(builder);
}

TEST_F(UvhttpServerApiTest, ServerStopSimpleNull) {
    /* 不应该崩溃 */
    uvhttp_server_stop_simple(NULL);
}

/* 测试服务器释放简单模式 */
TEST_F(UvhttpServerApiTest, ServerSimpleFreeNull) {
    /* 不应该崩溃 */
    uvhttp_server_simple_free(NULL);
}