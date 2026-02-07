/*
 * 白名单哈希表功能测试
 * 
 * 测试目标：
 * 1. 验证白名单哈希表初始化
 * 2. 验证添加白名单IP
 * 3. 验证白名单查找功能
 * 4. 验证内存清理
 */

#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_server.h"
#include "test_loop_helper.h"

TEST(UvhttpWhitelistHashTest, HashTableInitialization) {
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop.get(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    // 验证哈希表初始化
    EXPECT_EQ(server->rate_limit_whitelist_hash, nullptr);
    EXPECT_EQ(server->rate_limit_whitelist_count, 0);
    
    uvhttp_server_free(server);
}

TEST(UvhttpWhitelistHashTest, AddWhitelistIP) {
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop.get(), &server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(server, nullptr);
    
    // 启用限流
    result = uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 添加白名单IP
    const char* test_ips[] = {
        "192.168.1.1",
        "192.168.1.2",
        "192.168.1.3",
        "10.0.0.1",
        "172.16.0.1"
    };
    
    for (int i = 0; i < 5; i++) {
        result = uvhttp_server_add_rate_limit_whitelist(server, test_ips[i]);
        if (result != UVHTTP_OK) {
            fprintf(stderr, "Failed to add %s: %s\n", test_ips[i], uvhttp_error_string(result));
        }
        EXPECT_EQ(result, UVHTTP_OK);
    }
    
    EXPECT_EQ(HASH_COUNT(server->rate_limit_whitelist_hash), 5);
    EXPECT_EQ(server->rate_limit_whitelist_count, 5);
    
    uvhttp_server_free(server);
}

TEST(UvhttpWhitelistHashTest, DuplicateAddition) {
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop.get(), &server);
    ASSERT_NE(server, nullptr);
    
    // 启用限流
    result = uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 添加IP
    result = uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 重复添加
    result = uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(HASH_COUNT(server->rate_limit_whitelist_hash), 1);
    EXPECT_EQ(server->rate_limit_whitelist_count, 1);
    
    uvhttp_server_free(server);
}

TEST(UvhttpWhitelistHashTest, HashTableLookup) {
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop.get(), &server);
    ASSERT_NE(server, nullptr);
    
    // 启用限流
    result = uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 添加IP
    result = uvhttp_server_add_rate_limit_whitelist(server, "192.168.1.1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 查找存在的IP
    struct whitelist_item *item;
    HASH_FIND_STR(server->rate_limit_whitelist_hash, "192.168.1.1", item);
    EXPECT_NE(item, nullptr);
    
    // 查找不存在的IP
    HASH_FIND_STR(server->rate_limit_whitelist_hash, "192.168.1.99", item);
    EXPECT_EQ(item, nullptr);
    
    uvhttp_server_free(server);
}

TEST(UvhttpWhitelistHashTest, NullParameterHandling) {
    // 测试NULL服务器参数
    uvhttp_error_t result = uvhttp_server_add_rate_limit_whitelist(NULL, "192.168.1.1");
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(loop.get(), &server);
    ASSERT_NE(server, nullptr);
    
    // 测试NULL IP参数
    result = uvhttp_server_add_rate_limit_whitelist(server, NULL);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_server_free(server);
}

TEST(UvhttpWhitelistHashTest, MemoryCleanup) {
    // 创建事件循环
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop.get(), &server);
    ASSERT_NE(server, nullptr);
    
    // 启用限流
    result = uvhttp_server_enable_rate_limit(server, 100, 60);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 添加多个IP
    const char* test_ips[] = {"192.168.1.1", "192.168.1.2", "192.168.1.3"};
    for (int i = 0; i < 3; i++) {
        result = uvhttp_server_add_rate_limit_whitelist(server, test_ips[i]);
        EXPECT_EQ(result, UVHTTP_OK);
    }
    
    // 清理应该不会崩溃
    uvhttp_server_free(server);
}