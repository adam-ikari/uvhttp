#include "uvhttp_allocator.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <gtest/gtest.h>
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

    void SetUp() override {
        /* 初始化 libuv loop */
        loop = uv_loop_new();
        ASSERT_NE(loop, nullptr);

        /* 初始化服务器指针 */
        server = nullptr;
        router = nullptr;
    }

    void TearDown() override {
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
    uvhttp_error_t result = uvhttp_server_take_router(server, router);
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

    result = uvhttp_server_take_router(NULL, router);
    EXPECT_NE(result, UVHTTP_OK);

    uvhttp_router_free(router);
}

TEST_F(UvhttpServerApiTest, ServerSetRouterNullRouter) {
    /* 创建服务器 */
    ASSERT_EQ(create_server(), UVHTTP_OK);

    /* NULL router 仅在服务器尚未持有 router 时是空操作（没有 detach 操作，
     * 已 take 的 router 不可摘除或替换，传 NULL 同样返回 INVALID_PARAM）——
     * 见 test_server_atomic_construct.cpp 的 NullRouterAfterTakeIsRejected */
    uvhttp_error_t result = uvhttp_server_take_router(server, NULL);
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
    result = uvhttp_server_take_context(server, context);
    EXPECT_EQ(result, UVHTTP_OK);

    /* 验证上下文已设置 */
    EXPECT_EQ(server->context, context);
}

TEST_F(UvhttpServerApiTest, ServerSetContextNullServer) {

    uvhttp_context_t* context = NULL;

    uvhttp_error_t result = uvhttp_context_create(loop, &context);

    ASSERT_EQ(result, UVHTTP_OK);

    result = uvhttp_server_take_context(NULL, context);

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

    /* 停止未启动的服务器：幂等操作，返回 UVHTTP_OK */
    uvhttp_error_t result = uvhttp_server_stop(server);
    EXPECT_EQ(result, UVHTTP_OK);
}
/* 测试服务器停止 NULL 参数 */
TEST_F(UvhttpServerApiTest, ServerStopNull) {
    uvhttp_error_t result = uvhttp_server_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* stop_simple 已随 builder 移除；等价的停止能力是 uvhttp_server_stop */
TEST_F(UvhttpServerApiTest, ServerStopStopsListening) {
    ASSERT_EQ(create_server(), UVHTTP_OK);
    /* 端口 0 由内核分配空闲端口：写死 18080 的话，别的测试或残留进程占住该口
     * 就会让这条用例变红，而它要测的是 stop 的行为，不是端口可用性。 */
    ASSERT_EQ(uvhttp_server_listen(server, "127.0.0.1", 0), UVHTTP_OK);

    EXPECT_EQ(uvhttp_server_stop(server), UVHTTP_OK);
    EXPECT_EQ(server->is_listening, 0);
}

/* 测试服务器释放简单模式 */
TEST_F(UvhttpServerApiTest, ServerFreeNullIsNoOp) {
    /* NULL 释放是安全的 no-op（v3.0.0：server_free 取代 simple_free） */
    EXPECT_EQ(uvhttp_server_free(NULL), UVHTTP_OK);
}

/* ========== 测试 uvhttp_server_new_with_loop ========== */

TEST_F(UvhttpServerApiTest, ServerNewWithLoopNull) {
    uvhttp_error_t err = uvhttp_server_new_with_loop(NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST_F(UvhttpServerApiTest, ServerNewWithLoopSuccess) {
    uvhttp_server_t* srv = NULL;
    uvhttp_error_t err = uvhttp_server_new_with_loop(&srv);
    /* server_new_with_loop 包装 uvhttp_server_new，对合法参数恒成功。守卫
     * 让断言在它回归返回错误时被静默跳过——改为显式断言（同 #456）。 */
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(srv, nullptr);
    uvhttp_server_free(srv);
}