/* uvhttp_websocket_auth.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_websocket_auth.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* Token 验证回调函数 */
static int valid_token_validator(const char* token, void* user_data) {
    /* 简单验证：token 必须是 "valid_token" */
    return strcmp(token, "valid_token") == 0 ? 0 : -1;
}

static int always_fail_validator(const char* token, void* user_data) {
    return -1;
}

TEST(UvhttpWebSocketAuthTest, AuthConfigCreateAndDestroy) {
    /* 测试创建和销毁配置 */
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 验证默认值 */
    EXPECT_EQ(config->enable_token_auth, 0);
    EXPECT_STREQ(config->token_param_name, "token");
    EXPECT_EQ(config->token_validator, nullptr);
    EXPECT_EQ(config->token_validator_data, nullptr);
    EXPECT_EQ(config->enable_ip_whitelist, 0);
    EXPECT_EQ(config->ip_whitelist, nullptr);
    EXPECT_EQ(config->enable_ip_blacklist, 0);
    EXPECT_EQ(config->ip_blacklist, nullptr);
    EXPECT_EQ(config->send_auth_failed_response, 1);
    EXPECT_STREQ(config->auth_failed_message, "Authentication failed");
    
    /* 销毁配置 */
    uvhttp_ws_auth_config_destroy(config);
    
    /* 测试销毁 NULL 配置 */
    uvhttp_ws_auth_config_destroy(nullptr);
}

TEST(UvhttpWebSocketAuthTest, AuthConfigCreateMemoryFailure) {
    /* 这个测试无法直接模拟内存分配失败，因为 uvhttp_alloc 是宏 */
    /* 但我们可以测试多次创建和销毁以确保没有内存泄漏 */
    for (int i = 0; i < 100; i++) {
        uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
        ASSERT_NE(config, nullptr);
        uvhttp_ws_auth_config_destroy(config);
    }
}

TEST(UvhttpWebSocketAuthTest, SetTokenValidator) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 设置 Token 验证器 */
    int user_data = 12345;
    uvhttp_ws_auth_set_token_validator(config, valid_token_validator, &user_data);
    
    EXPECT_EQ(config->enable_token_auth, 1);
    EXPECT_EQ(config->token_validator, valid_token_validator);
    EXPECT_EQ(config->token_validator_data, &user_data);
    
    /* 设置 NULL 验证器 */
    uvhttp_ws_auth_set_token_validator(config, nullptr, nullptr);
    EXPECT_EQ(config->enable_token_auth, 0);
    EXPECT_EQ(config->token_validator, nullptr);
    
    /* 测试 NULL 配置 */
    uvhttp_ws_auth_set_token_validator(nullptr, valid_token_validator, &user_data);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AddIpToWhitelist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP 到白名单 */
    int result = uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(config->enable_ip_whitelist, 1);
    EXPECT_NE(config->ip_whitelist, nullptr);
    EXPECT_STREQ(config->ip_whitelist->ip, "192.168.1.1");
    
    /* 添加多个 IP */
    result = uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.2");
    EXPECT_EQ(result, 0);
    result = uvhttp_ws_auth_add_ip_to_whitelist(config, "10.0.0.1");
    EXPECT_EQ(result, 0);
    
    /* 添加重复 IP */
    result = uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    EXPECT_EQ(result, 0);  /* 已存在，返回 0 */
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_add_ip_to_whitelist(nullptr, "192.168.1.1");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_add_ip_to_whitelist(config, nullptr);
    EXPECT_EQ(result, -1);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AddIpToBlacklist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP 到黑名单 */
    int result = uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(config->enable_ip_blacklist, 1);
    EXPECT_NE(config->ip_blacklist, nullptr);
    EXPECT_STREQ(config->ip_blacklist->ip, "192.168.1.100");
    
    /* 添加多个 IP */
    result = uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.101");
    EXPECT_EQ(result, 0);
    result = uvhttp_ws_auth_add_ip_to_blacklist(config, "10.0.0.100");
    EXPECT_EQ(result, 0);
    
    /* 添加重复 IP */
    result = uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    EXPECT_EQ(result, 0);  /* 已存在，返回 0 */
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_add_ip_to_blacklist(nullptr, "192.168.1.100");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_add_ip_to_blacklist(config, nullptr);
    EXPECT_EQ(result, -1);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, RemoveIpFromWhitelist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP */
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.2");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "10.0.0.1");
    
    /* 移除存在的 IP */
    int result = uvhttp_ws_auth_remove_ip_from_whitelist(config, "192.168.1.1");
    EXPECT_EQ(result, 0);
    
    /* 验证 IP 已被移除 */
    int is_whitelisted = uvhttp_ws_auth_is_ip_whitelisted(config, "192.168.1.1");
    EXPECT_EQ(is_whitelisted, 0);
    
    /* 移除不存在的 IP */
    result = uvhttp_ws_auth_remove_ip_from_whitelist(config, "192.168.1.999");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_remove_ip_from_whitelist(nullptr, "192.168.1.1");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_remove_ip_from_whitelist(config, nullptr);
    EXPECT_EQ(result, -1);
    
    /* 移除所有 IP，验证 enable_ip_whitelist 被禁用 */
    uvhttp_ws_auth_remove_ip_from_whitelist(config, "192.168.1.2");
    uvhttp_ws_auth_remove_ip_from_whitelist(config, "10.0.0.1");
    /* 再尝试移除一次不存在的 IP，触发 enable_ip_whitelist 的更新 */
    uvhttp_ws_auth_remove_ip_from_whitelist(config, "nonexistent");
    EXPECT_EQ(config->enable_ip_whitelist, 0);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, RemoveIpFromBlacklist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP */
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.101");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "10.0.0.100");
    
    /* 移除存在的 IP */
    int result = uvhttp_ws_auth_remove_ip_from_blacklist(config, "192.168.1.100");
    EXPECT_EQ(result, 0);
    
    /* 验证 IP 已被移除 */
    int is_blacklisted = uvhttp_ws_auth_is_ip_blacklisted(config, "192.168.1.100");
    EXPECT_EQ(is_blacklisted, 0);
    
    /* 移除不存在的 IP */
    result = uvhttp_ws_auth_remove_ip_from_blacklist(config, "192.168.1.999");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_remove_ip_from_blacklist(nullptr, "192.168.1.100");
    EXPECT_EQ(result, -1);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_remove_ip_from_blacklist(config, nullptr);
    EXPECT_EQ(result, -1);
    
    /* 移除所有 IP，验证 enable_ip_blacklist 被禁用 */
    uvhttp_ws_auth_remove_ip_from_blacklist(config, "192.168.1.101");
    uvhttp_ws_auth_remove_ip_from_blacklist(config, "10.0.0.100");
    /* 再尝试移除一次不存在的 IP，触发 enable_ip_blacklist 的更新 */
    uvhttp_ws_auth_remove_ip_from_blacklist(config, "nonexistent");
    EXPECT_EQ(config->enable_ip_blacklist, 0);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, IsIpWhitelisted) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 未启用白名单时，所有 IP 都应该被允许 */
    int result = uvhttp_ws_auth_is_ip_whitelisted(config, "192.168.1.1");
    EXPECT_EQ(result, 1);
    
    /* 添加 IP 到白名单 */
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "10.0.0.1");
    
    /* 测试白名单中的 IP */
    result = uvhttp_ws_auth_is_ip_whitelisted(config, "192.168.1.1");
    EXPECT_EQ(result, 1);
    
    result = uvhttp_ws_auth_is_ip_whitelisted(config, "10.0.0.1");
    EXPECT_EQ(result, 1);
    
    /* 测试不在白名单中的 IP */
    result = uvhttp_ws_auth_is_ip_whitelisted(config, "192.168.1.2");
    EXPECT_EQ(result, 0);
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_is_ip_whitelisted(nullptr, "192.168.1.1");
    EXPECT_EQ(result, 0);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_is_ip_whitelisted(config, nullptr);
    EXPECT_EQ(result, 0);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, IsIpBlacklisted) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 未启用黑名单时，所有 IP 都不应该被阻止 */
    int result = uvhttp_ws_auth_is_ip_blacklisted(config, "192.168.1.100");
    EXPECT_EQ(result, 0);
    
    /* 添加 IP 到黑名单 */
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "10.0.0.100");
    
    /* 测试黑名单中的 IP */
    result = uvhttp_ws_auth_is_ip_blacklisted(config, "192.168.1.100");
    EXPECT_EQ(result, 1);
    
    result = uvhttp_ws_auth_is_ip_blacklisted(config, "10.0.0.100");
    EXPECT_EQ(result, 1);
    
    /* 测试不在黑名单中的 IP */
    result = uvhttp_ws_auth_is_ip_blacklisted(config, "192.168.1.101");
    EXPECT_EQ(result, 0);
    
    /* 测试 NULL 配置 */
    result = uvhttp_ws_auth_is_ip_blacklisted(nullptr, "192.168.1.100");
    EXPECT_EQ(result, 0);
    
    /* 测试 NULL IP */
    result = uvhttp_ws_auth_is_ip_blacklisted(config, nullptr);
    EXPECT_EQ(result, 0);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateSuccess) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 无认证限制，应该成功 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithToken) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 设置 Token 验证器 */
    uvhttp_ws_auth_set_token_validator(config, valid_token_validator, nullptr);
    
    /* 使用有效的 token */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", "valid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    /* 使用无效的 token */
    result = uvhttp_ws_authenticate(config, "192.168.1.1", "invalid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_INVALID_TOKEN);
    
    /* 不提供 token */
    result = uvhttp_ws_authenticate(config, "192.168.1.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_NO_TOKEN);
    
    /* 提供空 token */
    result = uvhttp_ws_authenticate(config, "192.168.1.1", "");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_NO_TOKEN);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithWhitelist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP 到白名单 */
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "10.0.0.1");
    
    /* 白名单中的 IP 应该成功 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    /* 不在白名单中的 IP 应该失败 */
    result = uvhttp_ws_authenticate(config, "192.168.1.2", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_IP_NOT_ALLOWED);
    
    /* NULL IP 时应该成功（因为无法检查） */
    result = uvhttp_ws_authenticate(config, nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithBlacklist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP 到黑名单 */
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "10.0.0.100");
    
    /* 黑名单中的 IP 应该失败 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.100", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_IP_BLOCKED);
    
    /* 不在黑名单中的 IP 应该成功 */
    result = uvhttp_ws_authenticate(config, "192.168.1.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    /* NULL IP 时应该成功（因为无法检查） */
    result = uvhttp_ws_authenticate(config, nullptr, nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithBothLists) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加 IP 到白名单和黑名单 */
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.2");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.100");
    
    /* 白名单中的 IP 应该成功 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    /* 黑名单中的 IP 应该失败（优先检查黑名单） */
    result = uvhttp_ws_authenticate(config, "192.168.1.100", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_IP_BLOCKED);
    
    /* 既不在白名单也不在黑名单中的 IP 应该失败 */
    result = uvhttp_ws_authenticate(config, "10.0.0.1", nullptr);
    EXPECT_EQ(result, UVHTTP_WS_AUTH_IP_NOT_ALLOWED);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithTokenAndWhitelist) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 设置 Token 验证器和白名单 */
    uvhttp_ws_auth_set_token_validator(config, valid_token_validator, nullptr);
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    
    /* 白名单中的 IP + 有效 token 应该成功 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", "valid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_SUCCESS);
    
    /* 白名单中的 IP + 无效 token 应该失败 */
    result = uvhttp_ws_authenticate(config, "192.168.1.1", "invalid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_INVALID_TOKEN);
    
    /* 不在白名单中的 IP + 有效 token 应该失败 */
    result = uvhttp_ws_authenticate(config, "192.168.1.2", "valid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_IP_NOT_ALLOWED);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateNullConfig) {
    /* 测试 NULL 配置 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(nullptr, "192.168.1.1", "valid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_INTERNAL_ERROR);
}

TEST(UvhttpWebSocketAuthTest, AuthenticateWithoutValidator) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 启用 Token 认证但不设置验证器 */
    config->enable_token_auth = 1;
    
    /* 应该返回内部错误 */
    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(config, "192.168.1.1", "valid_token");
    EXPECT_EQ(result, UVHTTP_WS_AUTH_INTERNAL_ERROR);
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, AuthResultString) {
    /* 测试所有认证结果的字符串描述 */
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_SUCCESS), "Authentication successful");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_FAILED), "Authentication failed");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_NO_TOKEN), "No token provided");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_INVALID_TOKEN), "Invalid token");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_EXPIRED_TOKEN), "Token expired");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_IP_BLOCKED), "IP address blocked");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_IP_NOT_ALLOWED), "IP address not allowed");
    EXPECT_STREQ(uvhttp_ws_auth_result_string(UVHTTP_WS_AUTH_INTERNAL_ERROR), "Internal authentication error");
    EXPECT_STREQ(uvhttp_ws_auth_result_string((uvhttp_ws_auth_result_t)999), "Unknown authentication result");
}

TEST(UvhttpWebSocketAuthTest, MultipleIpManagement) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 添加大量 IP */
    for (int i = 0; i < 50; i++) {
        char ip[32];
        snprintf(ip, sizeof(ip), "192.168.1.%d", i);
        uvhttp_ws_auth_add_ip_to_whitelist(config, ip);
    }
    
    /* 验证所有 IP 都在白名单中 */
    for (int i = 0; i < 50; i++) {
        char ip[32];
        snprintf(ip, sizeof(ip), "192.168.1.%d", i);
        int result = uvhttp_ws_auth_is_ip_whitelisted(config, ip);
        EXPECT_EQ(result, 1);
    }
    
    /* 移除所有 IP */
    for (int i = 0; i < 50; i++) {
        char ip[32];
        snprintf(ip, sizeof(ip), "192.168.1.%d", i);
        uvhttp_ws_auth_remove_ip_from_whitelist(config, ip);
    }
    
    /* 验证所有 IP 都已被移除 */
    for (int i = 0; i < 50; i++) {
        char ip[32];
        snprintf(ip, sizeof(ip), "192.168.1.%d", i);
        int result = uvhttp_ws_auth_is_ip_whitelisted(config, ip);
        EXPECT_EQ(result, 0);
    }
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, ComplexAuthenticationScenario) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 设置复杂的认证场景 */
    uvhttp_ws_auth_set_token_validator(config, valid_token_validator, nullptr);
    
    /* 白名单 */
    uvhttp_ws_auth_add_ip_to_whitelist(config, "192.168.1.1");
    uvhttp_ws_auth_add_ip_to_whitelist(config, "10.0.0.1");
    
    /* 黑名单 */
    uvhttp_ws_auth_add_ip_to_blacklist(config, "192.168.1.2");
    uvhttp_ws_auth_add_ip_to_blacklist(config, "10.0.0.2");
    
    /* 各种测试用例 */
    struct {
        const char* ip;
        const char* token;
        uvhttp_ws_auth_result_t expected;
    } test_cases[] = {
        {"192.168.1.1", "valid_token", UVHTTP_WS_AUTH_SUCCESS},      /* 白名单 + 有效 token */
        {"192.168.1.1", "invalid_token", UVHTTP_WS_AUTH_INVALID_TOKEN}, /* 白名单 + 无效 token */
        {"192.168.1.2", "valid_token", UVHTTP_WS_AUTH_IP_BLOCKED},   /* 黑名单 */
        {"10.0.0.1", "valid_token", UVHTTP_WS_AUTH_SUCCESS},        /* 白名单 + 有效 token */
        {"10.0.0.2", "valid_token", UVHTTP_WS_AUTH_IP_BLOCKED},     /* 黑名单 */
        {"192.168.1.3", "valid_token", UVHTTP_WS_AUTH_IP_NOT_ALLOWED}, /* 不在白名单 */
        {nullptr, "valid_token", UVHTTP_WS_AUTH_SUCCESS},           /* NULL IP */
        {"192.168.1.1", nullptr, UVHTTP_WS_AUTH_NO_TOKEN},          /* NULL token */
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(
            config, test_cases[i].ip, test_cases[i].token);
        EXPECT_EQ(result, test_cases[i].expected) 
            << "Failed for IP: " << (test_cases[i].ip ? test_cases[i].ip : "NULL")
            << ", Token: " << (test_cases[i].token ? test_cases[i].token : "NULL");
    }
    
    uvhttp_ws_auth_config_destroy(config);
}

TEST(UvhttpWebSocketAuthTest, EmptyIpStrings) {
    uvhttp_ws_auth_config_t* config = uvhttp_ws_auth_config_create();
    ASSERT_NE(config, nullptr);
    
    /* 测试空字符串 IP */
    int result = uvhttp_ws_auth_add_ip_to_whitelist(config, "");
    EXPECT_EQ(result, 0);  /* 空字符串也会被添加 */
    
    result = uvhttp_ws_auth_is_ip_whitelisted(config, "");
    EXPECT_EQ(result, 1);  /* 空字符串应该在白名单中 */
    
    result = uvhttp_ws_auth_remove_ip_from_whitelist(config, "");
    EXPECT_EQ(result, 0);  /* 应该能移除 */
    
    uvhttp_ws_auth_config_destroy(config);
}