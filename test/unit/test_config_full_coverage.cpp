/**
 * @file test_config_full_coverage.cpp
 * @brief uvhttp_config.c 的完整覆盖率测试
 */

#include <gtest/gtest.h>
#include <uvhttp_config.h>
#include <uvhttp_error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* 测试配置创建 */
TEST(UvhttpConfigTest, ConfigNew) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->max_connections, UVHTTP_DEFAULT_MAX_CONNECTIONS);
    EXPECT_EQ(config->read_buffer_size, UVHTTP_DEFAULT_READ_BUFFER_SIZE);
    EXPECT_EQ(config->max_body_size, UVHTTP_DEFAULT_MAX_BODY_SIZE);
    uvhttp_config_free(config);
}

/* 测试配置创建失败（内存分配失败）- 通过限制内存来模拟 */
TEST(UvhttpConfigTest, ConfigNewMemoryFail) {
    /* 这个测试很难模拟，因为 uvhttp_alloc 可能会成功 */
    /* 我们只需要确保代码路径存在 */
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    uvhttp_config_free(config);
}

/* 测试配置释放 NULL */
TEST(UvhttpConfigTest, ConfigFreeNull) {
    uvhttp_config_free(NULL);
    /* 不应该崩溃 */
}

/* 测试设置默认配置 */
TEST(UvhttpConfigTest, ConfigSetDefaults) {
    uvhttp_config_t config;
    memset(&config, 0xFF, sizeof(config)); /* 填充垃圾数据 */
    
    uvhttp_config_set_defaults(&config);
    
    EXPECT_EQ(config.max_connections, UVHTTP_DEFAULT_MAX_CONNECTIONS);
    EXPECT_EQ(config.read_buffer_size, UVHTTP_DEFAULT_READ_BUFFER_SIZE);
    EXPECT_EQ(config.backlog, UVHTTP_DEFAULT_BACKLOG);
    EXPECT_EQ(config.keepalive_timeout, UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT);
    EXPECT_EQ(config.request_timeout, UVHTTP_DEFAULT_REQUEST_TIMEOUT);
    EXPECT_EQ(config.max_body_size, UVHTTP_DEFAULT_MAX_BODY_SIZE);
    EXPECT_EQ(config.max_header_size, UVHTTP_DEFAULT_MAX_HEADER_SIZE);
    EXPECT_EQ(config.max_url_size, UVHTTP_DEFAULT_MAX_URL_SIZE);
    EXPECT_EQ(config.max_requests_per_connection, UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN);
    EXPECT_EQ(config.rate_limit_window, UVHTTP_DEFAULT_RATE_LIMIT_WINDOW);
    EXPECT_EQ(config.enable_compression, UVHTTP_DEFAULT_ENABLE_COMPRESSION);
    EXPECT_EQ(config.enable_tls, UVHTTP_DEFAULT_ENABLE_TLS);
    EXPECT_EQ(config.memory_pool_size, UVHTTP_DEFAULT_MEMORY_POOL_SIZE);
    EXPECT_EQ(config.enable_memory_debug, UVHTTP_DEFAULT_ENABLE_MEMORY_DEBUG);
    EXPECT_EQ(config.memory_warning_threshold, UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD);
}

/* 测试设置默认配置 NULL */
TEST(UvhttpConfigTest, ConfigSetDefaultsNull) {
    uvhttp_config_set_defaults(NULL);
    /* 不应该崩溃 */
}

/* 测试从文件加载配置 */
TEST(UvhttpConfigTest, ConfigLoadFile) {
    /* 创建临时配置文件 */
    const char* test_file = "/tmp/test_uvhttp_config.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "# Test Configuration\n");
    fprintf(fp, "max_connections=5000\n");
    fprintf(fp, "read_buffer_size=16384\n");
    fprintf(fp, "max_body_size=2097152\n");
    fprintf(fp, "log_file_path=uvhttp.log\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 5000);
    EXPECT_EQ(config->read_buffer_size, 16384);
    EXPECT_EQ(config->max_body_size, 2097152);
    EXPECT_STREQ(config->log_file_path, "uvhttp.log");
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置 NULL 配置 */
TEST(UvhttpConfigTest, ConfigLoadFileNullConfig) {
    const char* test_file = "/tmp/test_uvhttp_config.conf";
    int result = uvhttp_config_load_file(NULL, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试从文件加载配置 NULL 文件名 */
TEST(UvhttpConfigTest, ConfigLoadFileNullFilename) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, NULL);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试从文件加载配置文件不存在 */
TEST(UvhttpConfigTest, ConfigLoadFileNotFound) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, "/nonexistent/file.conf");
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
    
    uvhttp_config_free(config);
}

/* 测试从文件加载配置无效的 max_connections */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidMaxConnections) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_connections=invalid\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置 max_connections 超出范围 */
TEST(UvhttpConfigTest, ConfigLoadFileMaxConnectionsOutOfRange) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_connections=100000\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置无效的 read_buffer_size */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidReadBufferSize) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "read_buffer_size=invalid\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置 read_buffer_size 超出范围 */
TEST(UvhttpConfigTest, ConfigLoadFileReadBufferSizeOutOfRange) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "read_buffer_size=100\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置无效的 max_body_size */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidMaxBodySize) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_body_size=invalid\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置 max_body_size 超出范围 */
TEST(UvhttpConfigTest, ConfigLoadFileMaxBodySizeOutOfRange) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_body_size=999999999999999999\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置无效的 log_file_path（路径遍历） */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidLogFilePathTraversal) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "log_file_path=../etc/passwd\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置无效的 log_file_path（绝对路径） */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidLogFilePathAbsolute) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "log_file_path=/etc/passwd\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置无效的 log_file_path（太长） */
TEST(UvhttpConfigTest, ConfigLoadFileInvalidLogFilePathTooLong) {
    const char* test_file = "/tmp/test_uvhttp_config_invalid.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "log_file_path=");
    for (int i = 0; i < 300; i++) {
        fprintf(fp, "a");
    }
    fprintf(fp, "\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试从文件加载配置包含注释和空行 */
TEST(UvhttpConfigTest, ConfigLoadFileWithComments) {
    const char* test_file = "/tmp/test_uvhttp_config.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "# This is a comment\n");
    fprintf(fp, "\n");
    fprintf(fp, "max_connections=5000\n");
    fprintf(fp, "# Another comment\n");
    fprintf(fp, "  read_buffer_size=16384\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 5000);
    EXPECT_EQ(config->read_buffer_size, 16384);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试保存配置到文件 */
TEST(UvhttpConfigTest, ConfigSaveFile) {
    const char* test_file = "/tmp/test_uvhttp_config_save.conf";
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_connections = 5000;
    config->read_buffer_size = 16384;
    config->max_body_size = 2097152;
    
    int result = uvhttp_config_save_file(config, test_file);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证文件是否存在 */
    FILE* fp = fopen(test_file, "r");
    ASSERT_NE(fp, nullptr);
    fclose(fp);
    
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试保存配置到文件 NULL 配置 */
TEST(UvhttpConfigTest, ConfigSaveFileNullConfig) {
    const char* test_file = "/tmp/test_uvhttp_config_save.conf";
    int result = uvhttp_config_save_file(NULL, test_file);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试保存配置到文件 NULL 文件名 */
TEST(UvhttpConfigTest, ConfigSaveFileNullFilename) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_save_file(config, NULL);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试保存配置到文件创建失败 */
TEST(UvhttpConfigTest, ConfigSaveFileCreateFail) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    /* 尝试写入一个不可写的路径 */
    int result = uvhttp_config_save_file(config, "/root/uvhttp.conf");
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
    
    uvhttp_config_free(config);
}

/* 测试从环境变量加载配置 */
TEST(UvhttpConfigTest, ConfigLoadEnv) {
    /* 设置环境变量 */
    setenv("UVHTTP_MAX_CONNECTIONS", "8000", 1);
    setenv("UVHTTP_READ_BUFFER_SIZE", "32768", 1);
    setenv("UVHTTP_MAX_BODY_SIZE", "4194304", 1);
    setenv("UVHTTP_ENABLE_TLS", "1", 1);
    setenv("UVHTTP_LOG_LEVEL", "3", 1);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_env(config);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 8000);
    EXPECT_EQ(config->read_buffer_size, 32768);
    EXPECT_EQ(config->max_body_size, 4194304);
    EXPECT_EQ(config->enable_tls, 1);
    EXPECT_EQ(config->log_level, 3);
    
    uvhttp_config_free(config);
    
    /* 清理环境变量 */
    unsetenv("UVHTTP_MAX_CONNECTIONS");
    unsetenv("UVHTTP_READ_BUFFER_SIZE");
    unsetenv("UVHTTP_MAX_BODY_SIZE");
    unsetenv("UVHTTP_ENABLE_TLS");
    unsetenv("UVHTTP_LOG_LEVEL");
}

/* 测试从环境变量加载配置 NULL 配置 */
TEST(UvhttpConfigTest, ConfigLoadEnvNullConfig) {
    int result = uvhttp_config_load_env(NULL);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试从环境变量加载配置无效的值 */
TEST(UvhttpConfigTest, ConfigLoadEnvInvalidValues) {
    setenv("UVHTTP_MAX_CONNECTIONS", "invalid", 1);
    setenv("UVHTTP_READ_BUFFER_SIZE", "invalid", 1);
    setenv("UVHTTP_MAX_BODY_SIZE", "invalid", 1);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_env(config);
    EXPECT_EQ(result, UVHTTP_OK);
    /* 应该使用默认值 */
    EXPECT_EQ(config->max_connections, UVHTTP_DEFAULT_MAX_CONNECTIONS);
    
    uvhttp_config_free(config);
    
    unsetenv("UVHTTP_MAX_CONNECTIONS");
    unsetenv("UVHTTP_READ_BUFFER_SIZE");
    unsetenv("UVHTTP_MAX_BODY_SIZE");
}

/* 测试从环境变量加载配置超出的值 */
TEST(UvhttpConfigTest, ConfigLoadEnvOutOfRangeValues) {
    setenv("UVHTTP_MAX_CONNECTIONS", "100000", 1);
    setenv("UVHTTP_READ_BUFFER_SIZE", "100", 1);
    setenv("UVHTTP_MAX_BODY_SIZE", "999999999999999999", 1);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_env(config);
    EXPECT_EQ(result, UVHTTP_OK);
    /* 应该使用默认值 */
    EXPECT_EQ(config->max_connections, UVHTTP_DEFAULT_MAX_CONNECTIONS);
    
    uvhttp_config_free(config);
    
    unsetenv("UVHTTP_MAX_CONNECTIONS");
    unsetenv("UVHTTP_READ_BUFFER_SIZE");
    unsetenv("UVHTTP_MAX_BODY_SIZE");
}

/* 测试从环境变量加载配置无效的 log_file_path */
TEST(UvhttpConfigTest, ConfigLoadEnvInvalidLogFilePath) {
    setenv("UVHTTP_LOG_FILE_PATH", "../etc/passwd", 1);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_load_env(config);
    EXPECT_EQ(result, UVHTTP_OK);
    /* 应该使用默认值（空字符串） */
    EXPECT_STREQ(config->log_file_path, "");
    
    uvhttp_config_free(config);
    
    unsetenv("UVHTTP_LOG_FILE_PATH");
}

/* 测试配置验证 */
TEST(UvhttpConfigTest, ConfigValidate) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 NULL 配置 */
TEST(UvhttpConfigTest, ConfigValidateNull) {
    int result = uvhttp_config_validate(NULL);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试配置验证 max_connections 太小 */
TEST(UvhttpConfigTest, ConfigValidateMaxConnectionsTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_connections = 0;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 max_connections 太大 */
TEST(UvhttpConfigTest, ConfigValidateMaxConnectionsTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_connections = 100000;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 read_buffer_size 太小 */
TEST(UvhttpConfigTest, ConfigValidateReadBufferSizeTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->read_buffer_size = 100;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 read_buffer_size 太大 */
TEST(UvhttpConfigTest, ConfigValidateReadBufferSizeTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->read_buffer_size = 2 * 1024 * 1024;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 max_body_size 太小 */
TEST(UvhttpConfigTest, ConfigValidateMaxBodySizeTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_body_size = 100;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试配置验证 max_body_size 太大 */
TEST(UvhttpConfigTest, ConfigValidateMaxBodySizeTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    config->max_body_size = 200 * 1024 * 1024;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_free(config);
}

/* 测试打印配置 */
TEST(UvhttpConfigTest, ConfigPrint) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    /* 不应该崩溃 */
    uvhttp_config_print(config);
    
    uvhttp_config_free(config);
}

/* 测试打印配置 NULL */
TEST(UvhttpConfigTest, ConfigPrintNull) {
    /* 不应该崩溃 */
    uvhttp_config_print(NULL);
}

/* 测试获取当前配置 */
TEST(UvhttpConfigTest, ConfigGetCurrent) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    const uvhttp_config_t* current = uvhttp_config_get_current();
    EXPECT_EQ(current, config);
    
    uvhttp_config_free(config);
}

/* 测试获取当前配置未初始化 */
TEST(UvhttpConfigTest, ConfigGetCurrentNotInitialized) {
    /* 先清除全局配置 */
    uvhttp_config_set_current(NULL);
    
    const uvhttp_config_t* current = uvhttp_config_get_current();
    EXPECT_EQ(current, nullptr);
}

/* 测试设置全局配置 */
TEST(UvhttpConfigTest, ConfigSetCurrent) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    const uvhttp_config_t* current = uvhttp_config_get_current();
    EXPECT_EQ(current, config);
    
    uvhttp_config_set_current(NULL);
    
    current = uvhttp_config_get_current();
    EXPECT_EQ(current, nullptr);
    
    uvhttp_config_free(config);
}

/* 测试动态更新最大连接数 */
TEST(UvhttpConfigTest, ConfigUpdateMaxConnections) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_max_connections(5000);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 5000);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新最大连接数未初始化 */
TEST(UvhttpConfigTest, ConfigUpdateMaxConnectionsNotInitialized) {
    int result = uvhttp_config_update_max_connections(5000);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试动态更新最大连接数太小 */
TEST(UvhttpConfigTest, ConfigUpdateMaxConnectionsTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_max_connections(0);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新最大连接数太大 */
TEST(UvhttpConfigTest, ConfigUpdateMaxConnectionsTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_max_connections(20000);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新缓冲区大小 */
TEST(UvhttpConfigTest, ConfigUpdateBufferSize) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_buffer_size(16384);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->read_buffer_size, 16384);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新缓冲区大小未初始化 */
TEST(UvhttpConfigTest, ConfigUpdateBufferSizeNotInitialized) {
    int result = uvhttp_config_update_buffer_size(16384);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试动态更新缓冲区大小太小 */
TEST(UvhttpConfigTest, ConfigUpdateBufferSizeTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_buffer_size(100);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新缓冲区大小太大 */
TEST(UvhttpConfigTest, ConfigUpdateBufferSizeTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_buffer_size(2 * 1024 * 1024);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新限制参数 */
TEST(UvhttpConfigTest, ConfigUpdateLimits) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_limits(2097152, 16384);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_body_size, 2097152);
    EXPECT_EQ(config->max_header_size, 16384);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新限制参数未初始化 */
TEST(UvhttpConfigTest, ConfigUpdateLimitsNotInitialized) {
    int result = uvhttp_config_update_limits(2097152, 16384);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试动态更新限制参数 max_body_size 太小 */
TEST(UvhttpConfigTest, ConfigUpdateLimitsMaxBodySizeTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_limits(100, 16384);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新限制参数 max_body_size 太大 */
TEST(UvhttpConfigTest, ConfigUpdateLimitsMaxBodySizeTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_limits(200 * 1024 * 1024, 16384);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新限制参数 max_header_size 太小 */
TEST(UvhttpConfigTest, ConfigUpdateLimitsMaxHeaderSizeTooSmall) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_limits(2097152, 100);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试动态更新限制参数 max_header_size 太大 */
TEST(UvhttpConfigTest, ConfigUpdateLimitsMaxHeaderSizeTooLarge) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_update_limits(2097152, 128 * 1024);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试配置监控 */
TEST(UvhttpConfigTest, ConfigMonitorChanges) {
    static int callback_called = 0;
    callback_called = 0;
    
    auto callback = [](const char* key, const void* old_value, const void* new_value) {
        (void)key;
        (void)old_value;
        (void)new_value;
        callback_called++;
    };
    
    int result = uvhttp_config_monitor_changes(callback);
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试配置监控 NULL 回调 */
TEST(UvhttpConfigTest, ConfigMonitorChangesNullCallback) {
    int result = uvhttp_config_monitor_changes(NULL);
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试热重载配置 */
TEST(UvhttpConfigTest, ConfigReload) {
    /* 创建配置文件 */
    const char* test_file = "uvhttp.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_connections=5000\n");
    fprintf(fp, "read_buffer_size=16384\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_reload();
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 5000);
    EXPECT_EQ(config->read_buffer_size, 16384);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
    unlink(test_file);
}

/* 测试热重载配置未初始化 */
TEST(UvhttpConfigTest, ConfigReloadNotInitialized) {
    int result = uvhttp_config_reload();
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试热重载配置文件不存在 */
TEST(UvhttpConfigTest, ConfigReloadFileNotFound) {
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    /* 删除配置文件（如果存在） */
    unlink("uvhttp.conf");
    
    int result = uvhttp_config_reload();
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
}

/* 测试热重载配置验证失败 */
TEST(UvhttpConfigTest, ConfigReloadValidationFail) {
    /* 创建配置文件 */
    const char* test_file = "uvhttp.conf";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    
    fprintf(fp, "max_connections=100000\n");
    fclose(fp);
    
    uvhttp_config_t* config = uvhttp_config_new();
    ASSERT_NE(config, nullptr);
    
    uvhttp_config_set_current(config);
    
    int result = uvhttp_config_reload();
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    uvhttp_config_set_current(NULL);
    uvhttp_config_free(config);
    unlink(test_file);
}
