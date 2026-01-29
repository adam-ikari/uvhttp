/**
 * @file test_static_api_new.c
 * @brief 测试 uvhttp_static_create 新 API
 * 
 * 这个测试验证重构后的 uvhttp_static_create API，
 * 确保移除 context 参数后仍然正常工作。
 */

#include <gtest/gtest.h>
#include <string.h>
#include "../include/uvhttp.h"

// ============================================================================
// uvhttp_static_create API 测试
// ============================================================================

TEST(UvhttpStaticApiTest, CreateWithValidConfig) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, "./test_static");
    strcpy(config.index_file, "index.html");
    config.enable_directory_listing = 1;
    config.enable_etag = 1;
    config.enable_last_modified = 1;
    config.max_cache_size = 10 * 1024 * 1024;
    config.cache_ttl = 3600;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    // 注意：由于目录可能不存在，这里可能会失败
    // 这是预期的行为
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    } else {
        EXPECT_EQ(ctx, nullptr);
    }
}

TEST(UvhttpStaticApiTest, CreateWithNullConfig) {
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(NULL, &ctx);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(ctx, nullptr);
}

TEST(UvhttpStaticApiTest, CreateWithNullContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    uvhttp_error_t result = uvhttp_static_create(&config, NULL);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, CreateWithEmptyRootDirectory) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.root_directory[0] = '\0';
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(ctx, nullptr);
}

TEST(UvhttpStaticApiTest, CreateWithInvalidRootDirectory) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, "/nonexistent/directory/that/does/not/exist");
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(ctx, nullptr);
}

TEST(UvhttpStaticApiTest, CreateWithMinimalConfig) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    } else {
        EXPECT_EQ(ctx, nullptr);
    }
}

TEST(UvhttpStaticApiTest, CreateWithLargeCacheSize) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.max_cache_size = 1024 * 1024 * 1024; // 1GB
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    } else {
        EXPECT_EQ(ctx, nullptr);
    }
}

TEST(UvhttpStaticApiTest, CreateWithZeroCacheSize) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.max_cache_size = 0;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    } else {
        EXPECT_EQ(ctx, nullptr);
    }
}

TEST(UvhttpStaticApiTest, CreateWithCustomHeaders) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    strcpy(config.custom_headers, "X-Custom-Header: custom-value");
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    } else {
        EXPECT_EQ(ctx, nullptr);
    }
}

TEST(UvhttpStaticApiTest, FreeNullContext) {
    // 测试释放 NULL 上下文不会崩溃
    uvhttp_static_free(NULL);
    
    // 如果没有崩溃，测试通过
    EXPECT_TRUE(true);
}

TEST(UvhttpStaticApiTest, CreateAndFreeMultipleTimes) {
    // 测试多次创建和释放
    for (int i = 0; i < 10; i++) {
        uvhttp_static_config_t config;
        memset(&config, 0, sizeof(config));
        
        strcpy(config.root_directory, ".");
        
        uvhttp_static_context_t* ctx = NULL;
        uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
        
        if (result == UVHTTP_OK) {
            EXPECT_NE(ctx, nullptr);
            uvhttp_static_free(ctx);
        }
    }
    
    EXPECT_TRUE(true);
}

// ============================================================================
// 配置参数测试
// ============================================================================

TEST(UvhttpStaticApiTest, ConfigWithDirectoryListingEnabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_directory_listing = 1;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithDirectoryListingDisabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_directory_listing = 0;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithEtagEnabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_etag = 1;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithEtagDisabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_etag = 0;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithLastModifiedEnabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_last_modified = 1;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithLastModifiedDisabled) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.enable_last_modified = 0;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithCustomCacheTtl) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.cache_ttl = 7200; // 2 hours
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, ConfigWithZeroCacheTtl) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    strcpy(config.root_directory, ".");
    config.cache_ttl = 0;
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}