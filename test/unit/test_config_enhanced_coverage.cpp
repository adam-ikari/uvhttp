/* uvhttp_config.c 增强覆盖率测试 */

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"

#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
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

    /* 验证存活字段的默认值（v3.0.0 删除了 14 个无人读取的字段） */
    EXPECT_GT(config->max_connections, 0);
    EXPECT_GT(config->backlog, 0);
    EXPECT_GT(config->connection_timeout, 0);
    EXPECT_GT(config->max_file_size, 0u);
    EXPECT_EQ(config->trust_proxy_headers, 0);

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

    /* 测试无效配置 - 零值（连接超时必须为正，否则 uv_timer_start 立即超时） */
    config->connection_timeout = 0;
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

    /* 配置文件和环境变量加载功能已移除，符合极简工程原则 */

    uvhttp_config_free(config);
}

/* 测试动态配置更新 */
TEST(UvhttpConfigEnhancedCoverageTest, DynamicUpdate) {
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result;

    result = uvhttp_config_new(&config);
    ASSERT_EQ(result, UVHTTP_OK);

    uvhttp_config_set_defaults(config);

    /* 更新入口只剩 max_connections 一个（v3.0.0 删除了
     * update_read_buffer_size / update_size_limits —— 它们写入的字段无人读取，
     * 却返回成功并打印 "Limits updated" 日志） */
    int update_result = uvhttp_config_update_max_connections(NULL, 2000);
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
    config->backlog = 1;
    config->connection_timeout = UVHTTP_CONNECTION_TIMEOUT_MIN;

    int valid = uvhttp_config_validate(config);
    EXPECT_EQ(valid, UVHTTP_OK);

    /* 测试最大值 */
    config->max_connections = 65535;
    config->backlog = 65535;
    config->connection_timeout = UVHTTP_CONNECTION_TIMEOUT_MAX;

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

    /* 注意：不测试重复释放，因为 uvhttp_config_free 应该能够安全地处理 NULL
     * 指针 但是重复释放非 NULL 指针会导致双重释放错误 */
}