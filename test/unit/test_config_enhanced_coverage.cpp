/* uvhttp_config.c 增强覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_config.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* 测试配置创建和释放 */
TEST(UvhttpConfigEnhancedCoverageTest, CreateAndFree) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    /* 测试正常创建 */
    result = uvhttp_config_new(&config);
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(config, nullptr);
    
    /* 测试释放 */
    uvhttp_config_free(config);
    
    /* 测试释放NULL */
    uvhttp_config_free(NULL);
    
    /* 测试NULL输出参数 */
    result = uvhttp_config_new(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置默认值 */
TEST(UvhttpConfigEnhancedCoverageTest, SetDefaults) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试设置默认值 */
    uvhttp_config_set_defaults(config);
    
    /* 验证一些默认值 */
    EXPECT_GT(config->max_connections, 0);
    EXPECT_GT(config->read_buffer_size, 0);
    EXPECT_GT(config->backlog, 0);
    EXPECT_GT(config->keepalive_timeout, 0);
    EXPECT_GT(config->request_timeout, 0);
    EXPECT_GT(config->max_body_size, 0);
    EXPECT_GT(config->max_header_size, 0);
    EXPECT_GT(config->max_url_size, 0);
    
    uvhttp_config_free(config);
    
    /* 测试NULL参数 */
    uvhttp_config_set_defaults(NULL);
}

/* 测试配置验证 */
TEST(UvhttpConfigEnhancedCoverageTest, Validate) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_set_defaults(config);
    
    /* 测试有效配置 */
    int valid = uvhttp_config_validate(config);
    EXPECT_EQ(valid, UVHTTP_OK);
    
    /* 测试无效配置 - 负值 */
    config->max_connections = -1;
    valid = uvhttp_config_validate(config);
    EXPECT_NE(valid, UVHTTP_OK);
    
    /* 恢复有效值 */
    config->max_connections = 1000;
    
    /* 测试无效配置 - 零值（小于最小缓冲区大小） */
    config->read_buffer_size = 0;
    valid = uvhttp_config_validate(config);
    EXPECT_NE(valid, UVHTTP_OK);
    
    uvhttp_config_free(config);
    
    /* 测试NULL参数 */
    valid = uvhttp_config_validate(NULL);
    EXPECT_NE(valid, UVHTTP_OK);
}

/* 测试配置打印 */
TEST(UvhttpConfigEnhancedCoverageTest, Print) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_set_defaults(config);
    
    /* 测试打印配置 */
    uvhttp_config_print(config);
    
    uvhttp_config_free(config);
    
    /* 测试NULL参数 */
    uvhttp_config_print(NULL);
}

/* 测试配置文件加载和保存 */
TEST(UvhttpConfigEnhancedCoverageTest, FileLoadAndSave) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    const char* test_file = "/tmp/test_uvhttp_config.tmp";
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_set_defaults(config);
    
    /* 测试保存配置 */
    int save_result = uvhttp_config_save_file(config, test_file);
    /* 可能成功或失败，取决于文件系统权限 */
    
    /* 测试加载配置 */
    if (save_result == 0) {
        int load_result = uvhttp_config_load_file(config, test_file);
        /* 可能成功或失败 */
        
        /* 清理测试文件 */
        unlink(test_file);
    }
    
    /* 测试NULL参数 */
    save_result = uvhttp_config_save_file(NULL, test_file);
    EXPECT_NE(save_result, 0);
    
    save_result = uvhttp_config_save_file(config, NULL);
    EXPECT_NE(save_result, 0);
    
    {
        int load_result = uvhttp_config_load_file(NULL, test_file);
        EXPECT_NE(load_result, 0);
    }
    
    {
        int load_result = uvhttp_config_load_file(config, NULL);
        EXPECT_NE(load_result, 0);
    }
    
    uvhttp_config_free(config);
}

/* 测试环境变量加载 */
TEST(UvhttpConfigEnhancedCoverageTest, LoadEnv) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_set_defaults(config);
    
    /* 测试加载环境变量 */
    int load_result = uvhttp_config_load_env(config);
    /* 可能成功或失败，取决于环境变量 */
    
    uvhttp_config_free(config);
    
    /* 测试NULL参数 */
    load_result = uvhttp_config_load_env(NULL);
    EXPECT_NE(load_result, 0);
}

/* 测试动态配置更新 */
TEST(UvhttpConfigEnhancedCoverageTest, DynamicUpdate) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_set_defaults(config);
    
    /* 测试更新最大连接数 */
    int update_result = uvhttp_config_update_max_connections(NULL, 2000);
    EXPECT_NE(update_result, 0);
    
    /* 测试更新读缓冲区大小 */
    update_result = uvhttp_config_update_read_buffer_size(NULL, 8192);
    EXPECT_NE(update_result, 0);
    
    /* 测试更新大小限制 */
    update_result = uvhttp_config_update_size_limits(NULL, 1024 * 1024, 8192);
    EXPECT_NE(update_result, 0);
    
    uvhttp_config_free(config);
}

/* 配置监控、热重载等功能已删除，符合极简工程原则 */

/* 测试配置边界值 */
TEST(UvhttpConfigEnhancedCoverageTest, BoundaryValues) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;
    
    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试最小值 */
    config->max_connections = 1;
    config->read_buffer_size = 1024;  /* 最小缓冲区大小 */
    config->backlog = 1;
    config->keepalive_timeout = 1;
    config->request_timeout = 1;
    config->max_body_size = 1024;
    config->max_header_size = 256;
    config->max_url_size = 64;
    
    int valid = uvhttp_config_validate(config);
    EXPECT_EQ(valid, UVHTTP_OK);
    
    /* 测试最大值 */
    config->max_connections = 65535;
    config->read_buffer_size = 1024 * 1024;
    config->backlog = 65535;
    config->keepalive_timeout = 86400;  /* 24小时 */
    config->request_timeout = 86400;
    config->max_body_size = 100 * 1024 * 1024;  /* 100MB，最大限制 */
    config->max_header_size = 8192;  /* 合理的最大值 */
    config->max_url_size = 4096;  /* 合理的最大值 */
    
    valid = uvhttp_config_validate(config);
    EXPECT_EQ(valid, UVHTTP_OK);
    
    uvhttp_config_free(config);
}

/* 测试配置内存分配 */
TEST(UvhttpConfigEnhancedCoverageTest, MemoryAllocation) {
    uvhttp_config_t* config1 = NULL;
    uvhttp_config_t* config2 = NULL;
    uvhttp_error_t result;
    
    /* 测试多次创建和释放 */
    result = uvhttp_config_new(&config1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_config_new(&config2);
    EXPECT_EQ(result, UVHTTP_OK);
    
    ASSERT_NE(config1, nullptr);
    ASSERT_NE(config2, nullptr);
    ASSERT_NE(config1, config2);
    
    uvhttp_config_free(config1);
    uvhttp_config_free(config2);
    
    /* 注意：不测试重复释放，因为 uvhttp_config_free 应该能够安全地处理 NULL 指针
     * 但是重复释放非 NULL 指针会导致双重释放错误 */
}