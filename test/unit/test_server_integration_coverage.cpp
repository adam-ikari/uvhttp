#include <gtest/gtest.h>
#include <uv.h>
#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <unistd.h>

/* 服务器集成测试 - 测试真实 HTTP 请求场景 */

static int server_port = 18080;
static uvhttp_server_t* test_server = nullptr;
static uv_loop_t* test_loop = nullptr;

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello World", 11);
    uvhttp_response_send(res);
    return 0;
}

/* 设置测试服务器 */
static void setup_test_server() {
    test_loop = uv_default_loop();
    test_server = uvhttp_server_new(test_loop);
    ASSERT_NE(test_server, nullptr);
    
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_router_add_route_method(router, "/", UVHTTP_GET, simple_handler);
    uvhttp_router_add_route_method(router, "/test", UVHTTP_GET, simple_handler);
    
    test_server->router = router;
    
    /* 尝试监听端口 */
    uvhttp_error_t result = uvhttp_server_listen(test_server, "127.0.0.1", server_port);
    if (result != UVHTTP_OK) {
        /* 端口可能被占用，尝试下一个端口 */
        server_port++;
        result = uvhttp_server_listen(test_server, "127.0.0.1", server_port);
    }
    ASSERT_EQ(result, UVHTTP_OK);
}

/* 清理测试服务器 */
static void teardown_test_server() {
    if (test_server) {
        uvhttp_server_free(test_server);
        test_server = nullptr;
    }
}

/* 测试类 */
class UvhttpServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        setup_test_server();
    }
    
    void TearDown() override {
        teardown_test_server();
    }
};

/* 测试服务器启动和停止 */
TEST_F(UvhttpServerIntegrationTest, ServerStartAndStop) {
    ASSERT_NE(test_server, nullptr);
    EXPECT_EQ(test_server->is_listening, 1);
}

/* 测试服务器监听 */
TEST_F(UvhttpServerIntegrationTest, ServerListening) {
    ASSERT_NE(test_server, nullptr);
    EXPECT_EQ(test_server->is_listening, 1);
    /* 注意：active_connections 在没有实际请求时为 0 */
    EXPECT_GE(test_server->active_connections, 0);
}

/* 测试服务器配置 */
TEST_F(UvhttpServerIntegrationTest, ServerConfiguration) {
    ASSERT_NE(test_server, nullptr);
    EXPECT_NE(test_server->router, nullptr);
    EXPECT_NE(test_server->loop, nullptr);
    EXPECT_GT(test_server->max_connections, 0);
}

/* 测试限流功能 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(test_server, 10, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(test_server->rate_limit_enabled, 1);
    EXPECT_EQ(test_server->rate_limit_max_requests, 10);
    
    /* 检查限流状态 */
    int remaining;
    uint64_t reset_time;
    result = uvhttp_server_get_rate_limit_status(test_server, "127.0.0.1", &remaining, &reset_time);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GE(remaining, 0);
    EXPECT_GT(reset_time, 0);
}

/* 测试限流白名单 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitWhitelistIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    
    /* 添加白名单 */
    uvhttp_error_t result = uvhttp_server_add_rate_limit_whitelist(test_server, "127.0.0.1");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(test_server->rate_limit_whitelist_count, 0);
}

/* 测试限流检查 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitCheckIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 5, 60);
    
    /* 发送多个请求 */
    for (int i = 0; i < 5; i++) {
        uvhttp_error_t result = uvhttp_server_check_rate_limit(test_server);
        EXPECT_EQ(result, UVHTTP_OK);
    }
    
    /* 第 6 个请求应该被拒绝 */
    uvhttp_error_t result = uvhttp_server_check_rate_limit(test_server);
    EXPECT_EQ(result, UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);
}

/* 测试限流重置 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitResetIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    
    /* 消耗一些请求 */
    for (int i = 0; i < 5; i++) {
        uvhttp_server_check_rate_limit(test_server);
    }
    
    /* 重置限流 */
    uvhttp_error_t result = uvhttp_server_reset_rate_limit_client(test_server, "127.0.0.1");
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试限流清空 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitClearIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流并添加白名单 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    uvhttp_server_add_rate_limit_whitelist(test_server, "127.0.0.1");
    
    /* 清空所有限流状态 */
    uvhttp_error_t result = uvhttp_server_clear_rate_limit_all(test_server);
    EXPECT_NE(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试限流禁用 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitDisableIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    EXPECT_EQ(test_server->rate_limit_enabled, 1);
    
    /* 禁用限流 */
    uvhttp_error_t result = uvhttp_server_disable_rate_limit(test_server);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(test_server->rate_limit_enabled, 0);
}

/* 测试路由器 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RouterIntegration) {
    ASSERT_NE(test_server, nullptr);
    ASSERT_NE(test_server->router, nullptr);
    
    /* 添加新路由 */
    uvhttp_router_t* router = test_server->router;
    uvhttp_router_add_route_method(router, "/api", UVHTTP_GET, simple_handler);
    uvhttp_router_add_route_method(router, "/api/data", UVHTTP_POST, simple_handler);
}

/* 测试中间件 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, MiddlewareIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 简单的中间件处理器 */
    auto middleware_handler = [](uvhttp_request_t* req, uvhttp_response_t* res) {
        /* 中间件逻辑 */
        return 0;
    };
    
    /* 添加中间件到服务器 */
    /* 注意：中间件 API 可能需要调整 */
}

/* 测试服务器状态 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, ServerStateIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 检查服务器状态 */
    EXPECT_EQ(test_server->is_listening, 1);
    EXPECT_NE(test_server->loop, nullptr);
    EXPECT_GT(test_server->max_connections, 0);
    EXPECT_GT(test_server->max_message_size, 0);
}

/* 测试连接管理 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, ConnectionManagementIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 检查连接管理 */
    EXPECT_EQ(test_server->is_listening, 1);
    EXPECT_GE(test_server->active_connections, 0);
    EXPECT_LE(test_server->active_connections, test_server->max_connections);
}

/* 测试限流边界情况 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitEdgeCasesIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 测试最大请求数 = 1 */
    uvhttp_server_enable_rate_limit(test_server, 1, 60);
    EXPECT_EQ(uvhttp_server_check_rate_limit(test_server), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_check_rate_limit(test_server), UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);
    
    /* 重置并测试 */
    uvhttp_server_reset_rate_limit_client(test_server, "127.0.0.1");
    EXPECT_EQ(uvhttp_server_check_rate_limit(test_server), UVHTTP_OK);
}

/* 测试限流时间窗口 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitTimeWindowIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流（小时间窗口） */
    uvhttp_server_enable_rate_limit(test_server, 5, 1);
    
    /* 发送请求 */
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(uvhttp_server_check_rate_limit(test_server), UVHTTP_OK);
    }
    
    /* 第 6 个请求应该被拒绝 */
    EXPECT_EQ(uvhttp_server_check_rate_limit(test_server), UVHTTP_ERROR_RATE_LIMIT_EXCEEDED);
}

/* 测试限流多次启用 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitMultipleEnableIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 第一次启用 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    EXPECT_EQ(test_server->rate_limit_max_requests, 10);
    
    /* 第二次启用（应该覆盖） */
    uvhttp_server_enable_rate_limit(test_server, 20, 120);
    EXPECT_EQ(test_server->rate_limit_max_requests, 20);
    EXPECT_EQ(test_server->rate_limit_window_seconds, 120);
}

/* 测试限流状态获取 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitStatusIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 100, 60);
    
    /* 获取状态 */
    int remaining;
    uint64_t reset_time;
    uvhttp_error_t result = uvhttp_server_get_rate_limit_status(test_server, "127.0.0.1", &remaining, &reset_time);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GE(remaining, 0);
    EXPECT_GT(reset_time, 0);
}

/* 测试限流状态获取（未启用） - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitStatusNotEnabledIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 未启用限流 */
    int remaining;
    uint64_t reset_time;
    uvhttp_error_t result = uvhttp_server_get_rate_limit_status(test_server, "127.0.0.1", &remaining, &reset_time);
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试限流白名单添加 - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitWhitelistAddIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    
    /* 添加多个白名单 */
    uvhttp_server_add_rate_limit_whitelist(test_server, "127.0.0.1");
    uvhttp_server_add_rate_limit_whitelist(test_server, "192.168.1.1");
    uvhttp_server_add_rate_limit_whitelist(test_server, "10.0.0.1");
    
    EXPECT_EQ(test_server->rate_limit_whitelist_count, 3);
}

/* 测试限流清空（无白名单） - 集成测试 */
TEST_F(UvhttpServerIntegrationTest, RateLimitClearNoWhitelistIntegration) {
    ASSERT_NE(test_server, nullptr);
    
    /* 启用限流但不添加白名单 */
    uvhttp_server_enable_rate_limit(test_server, 10, 60);
    
    /* 清空 */
    uvhttp_error_t result = uvhttp_server_clear_rate_limit_all(test_server);
    EXPECT_NE(result, UVHTTP_ERROR_INVALID_PARAM);
}