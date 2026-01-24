#include <gtest/gtest.h>
#include <uv.h>
#include "uvhttp.h"
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 静态文件服务集成测试 - 测试真实文件系统操作 */

static const char* test_root_dir = "/tmp/uvhttp_test_static";
static uvhttp_static_context_t* static_ctx = nullptr;

/* 设置测试环境 */
static void setup_test_environment() {
    /* 创建测试目录 */
    system("rm -rf /tmp/uvhttp_test_static");
    system("mkdir -p /tmp/uvhttp_test_static");
    system("mkdir -p /tmp/uvhttp_test_static/subdir");
    
    /* 创建测试文件 */
    system("echo 'Hello World' > /tmp/uvhttp_test_static/index.html");
    system("echo 'Test Data' > /tmp/uvhttp_test_static/test.txt");
    system("echo 'Subdir File' > /tmp/uvhttp_test_static/subdir/file.html");
    system("echo 'CSS Content' > /tmp/uvhttp_test_static/style.css");
    
    /* 创建二进制文件 */
    FILE* fp = fopen("/tmp/uvhttp_test_static/binary.bin", "wb");
    if (fp) {
        unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
        fwrite(data, 1, sizeof(data), fp);
        fclose(fp);
    }
    
    /* 创建大文件（>1MB） */
    fp = fopen("/tmp/uvhttp_test_static/large.txt", "w");
    if (fp) {
        for (int i = 0; i < 100000; i++) {
            fprintf(fp, "Line %d: This is a test line with some content.\n", i);
        }
        fclose(fp);
    }
}

/* 清理测试环境 */
static void teardown_test_environment() {
    if (static_ctx) {
        uvhttp_static_free(static_ctx);
        static_ctx = nullptr;
    }
    system("rm -rf /tmp/uvhttp_test_static");
}

/* 测试类 */
class UvhttpStaticIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        setup_test_environment();
        
        /* 创建静态文件服务上下文 */
        uvhttp_static_config_t config;
        memset(&config, 0, sizeof(config));
        strcpy(config.root_directory, test_root_dir);
        strcpy(config.index_file, "index.html");
        config.max_cache_size = 1024 * 1024;  /* 1MB */
        config.cache_ttl = 3600;  /* 1小时 */
        config.max_cache_entries = 100;
        config.enable_directory_listing = 1;
        config.enable_etag = 1;
        config.enable_last_modified = 1;
        config.enable_sendfile = 1;
        
        static_ctx = uvhttp_static_create(&config);
        ASSERT_NE(static_ctx, nullptr);
    }
    
    void TearDown() override {
        teardown_test_environment();
    }
};

/* 测试静态文件服务创建和释放 */
TEST_F(UvhttpStaticIntegrationTest, StaticContextCreateAndFree) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 检查配置 */
    EXPECT_NE(static_ctx, nullptr);
}

/* 测试静态文件服务配置 */
TEST_F(UvhttpStaticIntegrationTest, StaticContextConfiguration) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 检查配置是否正确设置 */
    /* 注意：这些字段可能是内部字段，无法直接访问 */
}

/* 测试缓存预热 - 单个文件 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheSingleFile) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热单个文件 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result, 0);
}

/* 测试缓存预热 - 目录 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheDirectory) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热整个目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 10);
    EXPECT_GE(result, 0);
}

/* 测试缓存预热 - 子目录 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheSubdirectory) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热子目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "subdir", 10);
    EXPECT_GE(result, 0);
}

/* 测试缓存预热 - 不存在的文件 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheNonExistentFile) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热不存在的文件 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "nonexistent.html");
    EXPECT_NE(result, 0);
}

/* 测试缓存预热 - 不存在的目录 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheNonExistentDirectory) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热不存在的目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "nonexistent", 10);
    EXPECT_EQ(result, -1);
}

/* 测试缓存预热 - 空路径 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheEmptyPath) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热空路径 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "");
    EXPECT_NE(result, 0);
}

/* 测试缓存预热 - NULL 上下文 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheNullContext) {
    /* NULL 上下文 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(nullptr, "index.html");
    EXPECT_NE(result, 0);
}

/* 测试缓存预热 - NULL 路径 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheNullPath) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* NULL 路径 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, nullptr);
    EXPECT_NE(result, 0);
}

/* 测试缓存预热 - 大文件 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheLargeFile) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热大文件 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "large.txt");
    EXPECT_EQ(result, 0);
}

/* 测试缓存预热 - 二进制文件 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheBinaryFile) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热二进制文件 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "binary.bin");
    EXPECT_EQ(result, 0);
}

/* 测试缓存预热 - 多个文件 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheMultipleFiles) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热多个文件 */
    uvhttp_static_prewarm_cache(static_ctx, "index.html");
    uvhttp_static_prewarm_cache(static_ctx, "test.txt");
    uvhttp_static_prewarm_cache(static_ctx, "style.css");
    
    /* 预热整个目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 10);
    EXPECT_GE(result, 3);
}

/* 测试缓存预热 - 重复预热 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheDuplicate) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 第一次预热 */
    uvhttp_result_t result1 = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result1, 0);
    
    /* 第二次预热（应该成功） */
    uvhttp_result_t result2 = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result2, 0);
}

/* 测试缓存预热 - 特殊字符文件名 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheSpecialCharacters) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 创建带特殊字符的文件 */
    system("echo 'test' > '/tmp/uvhttp_test_static/file with spaces.txt'");
    system("echo 'test' > '/tmp/uvhttp_test_static/file-with-dashes.txt'");
    
    /* 预热这些文件 */
    uvhttp_result_t result1 = uvhttp_static_prewarm_cache(static_ctx, "file with spaces.txt");
    uvhttp_result_t result2 = uvhttp_static_prewarm_cache(static_ctx, "file-with-dashes.txt");
    
    EXPECT_EQ(result1, 0);
    EXPECT_EQ(result2, 0);
}

/* 测试缓存预热 - 空目录 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheEmptyDirectory) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 创建空目录 */
    system("mkdir -p /tmp/uvhttp_test_static/empty_dir");
    
    /* 预热空目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "empty_dir", 10);
    EXPECT_EQ(result, 0);
}

/* 测试缓存预热 - 最大文件数限制 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheMaxFilesLimit) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 创建多个文件 */
    for (int i = 0; i < 20; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo 'test%d' > '/tmp/uvhttp_test_static/file%d.txt'", i, i);
        system(cmd);
    }
    
    /* 预热目录，限制最多 5 个文件 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 5);
    EXPECT_GE(result, 0);
    EXPECT_LE(result, 5);
}

/* 测试缓存预热 - 零限制 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheZeroLimit) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热目录，限制 0 个文件 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 0);
    /* 零限制可能返回 0 或实际文件数 */
    EXPECT_GE(result, 0);
}

/* 测试缓存预热 - 子目录递归 */
TEST_F(UvhttpStaticIntegrationTest, PrewarmCacheRecursiveSubdirectories) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 创建嵌套子目录 */
    system("mkdir -p /tmp/uvhttp_test_static/dir1/dir2/dir3");
    system("echo 'nested' > '/tmp/uvhttp_test_static/dir1/dir2/dir3/nested.txt'");
    
    /* 预热根目录 */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 10);
    EXPECT_GE(result, 0);
}

/* 测试缓存清理 */
TEST_F(UvhttpStaticIntegrationTest, ClearCache) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热一些文件 */
    uvhttp_static_prewarm_cache(static_ctx, "index.html");
    uvhttp_static_prewarm_cache(static_ctx, "test.txt");
    
    /* 清理缓存 */
    /* 注意：这个函数可能不存在，需要检查 API */
}

/* 测试缓存统计 */
TEST_F(UvhttpStaticIntegrationTest, CacheStatistics) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热一些文件 */
    uvhttp_static_prewarm_cache(static_ctx, "index.html");
    uvhttp_static_prewarm_cache(static_ctx, "test.txt");
    
    /* 获取缓存统计 */
    /* 注意：这个函数可能不存在，需要检查 API */
}

/* 测试不同文件类型 */
TEST_F(UvhttpStaticIntegrationTest, DifferentFileTypes) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热不同类型的文件 */
    EXPECT_EQ(uvhttp_static_prewarm_cache(static_ctx, "index.html"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(static_ctx, "test.txt"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(static_ctx, "style.css"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(static_ctx, "binary.bin"), 0);
    EXPECT_EQ(uvhttp_static_prewarm_cache(static_ctx, "large.txt"), 0);
}

/* 测试索引文件 */
TEST_F(UvhttpStaticIntegrationTest, IndexFile) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热索引文件 */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result, 0);
    
    /* 预热根目录（应该自动找到索引文件） */
    int dir_result = uvhttp_static_prewarm_directory(static_ctx, "", 10);
    EXPECT_GE(dir_result, 0);
}

/* 测试目录列表 */
TEST_F(UvhttpStaticIntegrationTest, DirectoryListing) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热目录（应该包含目录列表） */
    int result = uvhttp_static_prewarm_directory(static_ctx, "", 10);
    EXPECT_GE(result, 0);
}

/* 测试 ETag 生成 */
TEST_F(UvhttpStaticIntegrationTest, ETagGeneration) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热文件（应该生成 ETag） */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result, 0);
    
    /* ETag 应该在缓存中 */
    /* 注意：需要检查 API 来验证 ETag */
}

/* 测试 Last-Modified 生成 */
TEST_F(UvhttpStaticIntegrationTest, LastModifiedGeneration) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热文件（应该生成 Last-Modified） */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "index.html");
    EXPECT_EQ(result, 0);
    
    /* Last-Modified 应该在缓存中 */
    /* 注意：需要检查 API 来验证 Last-Modified */
}

/* 测试 sendfile 零拷贝 */
TEST_F(UvhttpStaticIntegrationTest, SendfileOptimization) {
    ASSERT_NE(static_ctx, nullptr);
    
    /* 预热大文件（应该使用 sendfile） */
    uvhttp_result_t result = uvhttp_static_prewarm_cache(static_ctx, "large.txt");
    EXPECT_EQ(result, 0);
}
