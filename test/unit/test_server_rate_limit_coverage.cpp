/* uvhttp_server.c 限流功能覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include <string.h>

TEST(UvhttpServerRateLimitTest, EnableRateLimit) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_enabled, 1);
    
    result = uvhttp_server_enable_rate_limit(nullptr, 100, 60);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, DisableRateLimit) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    uvhttp_error_t result = uvhttp_server_disable_rate_limit(server);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(server->rate_limit_enabled, 0);
    
    result = uvhttp_server_disable_rate_limit(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, CheckRateLimit) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    uvhttp_error_t result = uvhttp_server_check_rate_limit(server);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_server_check_rate_limit(nullptr);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, AddRateLimitWhitelist) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    uvhttp_error_t result = uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_server_add_rate_limit_whitelist(nullptr, "127.0.0.1");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_add_rate_limit_whitelist(server, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, GetRateLimitStatus) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    int remaining = 0;
    uint64_t reset_time = 0;
    
    uvhttp_error_t result = uvhttp_server_get_rate_limit_status(
        server, "127.0.0.1", &remaining, &reset_time);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_server_get_rate_limit_status(
        nullptr, "127.0.0.1", &remaining, &reset_time);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_get_rate_limit_status(server, nullptr, nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, ResetRateLimitClient) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    uvhttp_error_t result = uvhttp_server_reset_rate_limit_client(server, "127.0.0.1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_server_reset_rate_limit_client(nullptr, "127.0.0.1");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_reset_rate_limit_client(server, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, ClearRateLimitAll) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    uvhttp_error_t result = uvhttp_server_clear_rate_limit_all(server);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_server_clear_rate_limit_all(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, MultipleRateLimitOperations) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(server, 50, 30);
    
    /* 添加多个白名单 */
    uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");
    uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.1");
    uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1");
    
    /* 检查限流 */
    uvhttp_server_check_rate_limit(server);
    
    /* 获取状态 */
    int remaining = 0;
    uint64_t reset_time = 0;
    uvhttp_server_get_rate_limit_status(server, "127.0.0.1", &remaining, &reset_time);
    
    /* 重置客户端 */
    uvhttp_server_reset_rate_limit_client(server, "192.168.1.1");
    
    /* 清除所有 */
    uvhttp_server_clear_rate_limit_all(server);
    
    /* 禁用限流 */
    uvhttp_server_disable_rate_limit(server);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, RateLimitBoundaryValues) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    /* 测试边界值 */
    uvhttp_server_enable_rate_limit(server, 1, 1);
    uvhttp_server_check_rate_limit(server);
    uvhttp_server_disable_rate_limit(server);
    
    uvhttp_server_enable_rate_limit(server, INT_MAX, INT_MAX);
    uvhttp_server_check_rate_limit(server);
    uvhttp_server_disable_rate_limit(server);
    
    uvhttp_server_free(server);
}

TEST(UvhttpServerRateLimitTest, RateLimitWithRouter) {
    uvhttp_server_t* server = uvhttp_server_new(nullptr);
    ASSERT_NE(server, nullptr);
    
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_server_set_router(server, router);
    
    /* 启用限流 */
    uvhttp_server_enable_rate_limit(server, 100, 60);
    
    /* 添加白名单 */
    uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");
    
    /* 检查限流 */
    uvhttp_server_check_rate_limit(server);
    
    uvhttp_server_free(server);
    uvhttp_router_free(router);
}
