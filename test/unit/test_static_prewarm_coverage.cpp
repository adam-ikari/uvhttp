#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 静态文件缓存预热覆盖率测试 */

TEST(UvhttpStaticPrewarmTest, PrewarmCacheNullContext) {
    uvhttp_result_t result = uvhttp_static_prewarm_cache(nullptr, "index.html");
    EXPECT_NE(result, 0);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, nullptr);
    EXPECT_NE(result, 0);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheEmptyPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "");
    EXPECT_NE(result, 0);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheNonExistentFile) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "nonexistent.html");
    EXPECT_NE(result, 0);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheNormal) {
    /* 创建临时文件 */
    const char* test_file = "/tmp/test_prewarm.html";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    fprintf(fp, "<html><body>Test</body></html>");
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "test_prewarm.html");
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheLargeFile) {
    /* 创建大文件 */
    const char* test_file = "/tmp/test_prewarm_large.html";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    for (int i = 0; i < 10000; i++) {
        fprintf(fp, "Line %d: This is a test line.\n", i);
    }
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "test_prewarm_large.html");
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryNullContext) {
    int result = uvhttp_static_prewarm_directory(nullptr, "/tmp", 10);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int result = uvhttp_static_prewarm_directory(ctx, nullptr, 10);
    EXPECT_EQ(result, -1);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryEmptyPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int result = uvhttp_static_prewarm_directory(ctx, "", 10);
    EXPECT_NE(result, 0);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryNonExistentDir) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int result = uvhttp_static_prewarm_directory(ctx, "nonexistent", 10);
    EXPECT_EQ(result, -1);
    
    uvhttp_static_free(ctx);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryNormal) {
    /* 创建临时目录和文件 */
    system("mkdir -p /tmp/test_prewarm_dir");
    system("echo 'test1' > /tmp/test_prewarm_dir/file1.html");
    system("echo 'test2' > /tmp/test_prewarm_dir/file2.css");
    system("echo 'test3' > /tmp/test_prewarm_dir/file3.js");
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int result = uvhttp_static_prewarm_directory(ctx, "test_prewarm_dir", 10);
    EXPECT_GE(result, 0);
    EXPECT_LE(result, 10);
    
    uvhttp_static_free(ctx);
    system("rm -rf /tmp/test_prewarm_dir");
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryMaxFilesLimit) {
    /* 创建临时目录和多个文件 */
    system("mkdir -p /tmp/test_prewarm_limit");
    for (int i = 0; i < 20; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo 'test%d' > /tmp/test_prewarm_limit/file%d.html", i, i);
        system(cmd);
    }
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 限制最多预热 5 个文件 */
    int result = uvhttp_static_prewarm_directory(ctx, "test_prewarm_limit", 5);
    EXPECT_GE(result, 0);
    EXPECT_LE(result, 5);
    
    uvhttp_static_free(ctx);
    system("rm -rf /tmp/test_prewarm_limit");
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryZeroLimit) {
    /* 创建临时目录和文件 */
    system("mkdir -p /tmp/test_prewarm_zero");
    system("echo 'test' > /tmp/test_prewarm_zero/file.html");
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 限制为 0，应该不预热任何文件 */
    int result = uvhttp_static_prewarm_directory(ctx, "test_prewarm_zero", 0);
    /* 0 限制可能返回错误，这是正常的 */
    EXPECT_GE(result, 0);
    
    uvhttp_static_free(ctx);
    system("rm -rf /tmp/test_prewarm_zero");
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryWithSubdirectories) {
    /* 创建临时目录和子目录 */
    system("mkdir -p /tmp/test_prewarm_sub/subdir1");
    system("mkdir -p /tmp/test_prewarm_sub/subdir2");
    system("echo 'test1' > /tmp/test_prewarm_sub/file1.html");
    system("echo 'test2' > /tmp/test_prewarm_sub/subdir1/file2.html");
    system("echo 'test3' > /tmp/test_prewarm_sub/subdir2/file3.html");
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int result = uvhttp_static_prewarm_directory(ctx, "test_prewarm_sub", 10);
    EXPECT_GE(result, 0);
    
    uvhttp_static_free(ctx);
    system("rm -rf /tmp/test_prewarm_sub");
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheMultipleFiles) {
    /* 创建多个临时文件 */
    const char* files[] = {
        "/tmp/test_multi1.html",
        "/tmp/test_multi2.css",
        "/tmp/test_multi3.js"
    };
    
    for (int i = 0; i < 3; i++) {
        FILE* fp = fopen(files[i], "w");
        ASSERT_NE(fp, nullptr);
        fprintf(fp, "File %d content", i);
        fclose(fp);
    }
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 预热多个文件 */
    EXPECT_EQ(uvhttp_static_prewarm_cache(ctx, "test_multi1.html"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(ctx, "test_multi2.css"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(ctx, "test_multi3.js"), 0);
    
    uvhttp_static_free(ctx);
    
    for (int i = 0; i < 3; i++) {
        unlink(files[i]);
    }
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheDuplicate) {
    /* 创建临时文件 */
    const char* test_file = "/tmp/test_prewarm_dup.html";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    fprintf(fp, "<html><body>Test</body></html>");
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 预热同一个文件两次 */
    EXPECT_EQ(uvhttp_static_prewarm_cache(ctx, "test_prewarm_dup.html"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(ctx, "test_prewarm_dup.html"), 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheSpecialCharacters) {
    /* 创建包含特殊字符的文件名 */
    const char* test_file = "/tmp/test_prewarm_special-123.html";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    fprintf(fp, "<html><body>Test</body></html>");
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "test_prewarm_special-123.html");
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}

TEST(UvhttpStaticPrewarmTest, PrewarmDirectoryEmpty) {
    /* 创建空目录 */
    system("mkdir -p /tmp/test_prewarm_empty");
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 预热空目录 */
    int result = uvhttp_static_prewarm_directory(ctx, "test_prewarm_empty", 10);
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    system("rmdir /tmp/test_prewarm_empty");
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheWithCacheDisabled) {
    /* 创建临时文件 */
    const char* test_file = "/tmp/test_prewarm_nocache.html";
    FILE* fp = fopen(test_file, "w");
    ASSERT_NE(fp, nullptr);
    fprintf(fp, "<html><body>Test</body></html>");
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    config.max_cache_size = 0;  /* 禁用缓存 */
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 即使缓存禁用，预热也应该成功 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "test_prewarm_nocache.html");
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}

TEST(UvhttpStaticPrewarmTest, PrewarmCacheBinaryFile) {
    /* 创建二进制文件 */
    const char* test_file = "/tmp/test_prewarm_binary.png";
    FILE* fp = fopen(test_file, "wb");
    ASSERT_NE(fp, nullptr);
    
    /* 写入一些二进制数据 */
    unsigned char data[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(data, 1, sizeof(data), fp);
    fclose(fp);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "test_prewarm_binary.png");
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    unlink(test_file);
}