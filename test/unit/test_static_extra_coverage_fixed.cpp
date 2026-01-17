/* uvhttp_static.c 扩展覆盖率测试 - 修复版本 */

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* 测试静态文件上下文创建和销毁 */
TEST(UvhttpStaticExtraCoverageTest, StaticContextCreateAndFree) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    config.enable_sendfile = 1;
    config.enable_etag = 1;
    config.enable_last_modified = 1;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    strncpy(config.index_file, "index.html", sizeof(config.index_file) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_static_free(ctx);
}

/* 测试 NULL 配置创建 */
TEST(UvhttpStaticExtraCoverageTest, StaticContextCreateNullConfig) {
    uvhttp_static_context_t* ctx = uvhttp_static_create(nullptr);
    EXPECT_EQ(ctx, nullptr);
}

/* 测试 NULL 上下文销毁 */
TEST(UvhttpStaticExtraCoverageTest, StaticContextFreeNull) {
    uvhttp_static_free(nullptr);
}

/* 测试设置 sendfile 配置 */
TEST(UvhttpStaticExtraCoverageTest, SetSendfileConfig) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_error_t result = uvhttp_static_set_sendfile_config(ctx, 5000, 3, 8192);
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_static_set_sendfile_config(nullptr, 5000, 3, 8192);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
}

/* 测试清理缓存 */
TEST(UvhttpStaticExtraCoverageTest, ClearCache) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_static_clear_cache(ctx);
    uvhttp_static_clear_cache(nullptr);
    
    uvhttp_static_free(ctx);
}

/* 测试获取缓存统计信息 */
TEST(UvhttpStaticExtraCoverageTest, GetCacheStats) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    size_t total_memory_usage;
    int entry_count;
    int hit_count;
    int miss_count;
    int eviction_count;
    
    uvhttp_static_get_cache_stats(ctx, &total_memory_usage, &entry_count, 
                                   &hit_count, &miss_count, &eviction_count);
    
    uvhttp_static_get_cache_stats(nullptr, &total_memory_usage, &entry_count,
                                   &hit_count, &miss_count, &eviction_count);
    
    uvhttp_static_get_cache_stats(ctx, nullptr, nullptr, nullptr, nullptr, nullptr);
    
    uvhttp_static_free(ctx);
}

/* 测试获取缓存命中率 */
TEST(UvhttpStaticExtraCoverageTest, GetCacheHitRate) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    double hit_rate = uvhttp_static_get_cache_hit_rate(ctx);
    EXPECT_GE(hit_rate, 0.0);
    EXPECT_LE(hit_rate, 1.0);
    
    hit_rate = uvhttp_static_get_cache_hit_rate(nullptr);
    EXPECT_EQ(hit_rate, 0.0);
    
    uvhttp_static_free(ctx);
}

/* 测试清理过期缓存 */
TEST(UvhttpStaticExtraCoverageTest, CleanupExpiredCache) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 1;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int cleaned = uvhttp_static_cleanup_expired_cache(ctx);
    EXPECT_GE(cleaned, 0);
    
    cleaned = uvhttp_static_cleanup_expired_cache(nullptr);
    EXPECT_EQ(cleaned, 0);
    
    uvhttp_static_free(ctx);
}

/* 测试解析安全路径 */
TEST(UvhttpStaticExtraCoverageTest, ResolveSafePath) {
    char resolved_path[512];
    
    int result = uvhttp_static_resolve_safe_path("/tmp", "test.txt", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path("/tmp", "../etc/passwd", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path(nullptr, "test.txt", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path("/tmp", nullptr, resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path("/tmp", "test.txt", nullptr, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试生成 ETag */
TEST(UvhttpStaticExtraCoverageTest, GenerateEtag) {
    char etag[256];
    time_t now = time(nullptr);
    
    int result = uvhttp_static_generate_etag("/tmp/test.txt", now, 1024, etag, sizeof(etag));
    EXPECT_EQ(result, 0);
    EXPECT_GT(strlen(etag), 0);
    
    result = uvhttp_static_generate_etag(nullptr, now, 1024, etag, sizeof(etag));
    EXPECT_EQ(result, -1);
    
    result = uvhttp_static_generate_etag("/tmp/test.txt", now, 1024, nullptr, sizeof(etag));
    EXPECT_EQ(result, -1);
    
    char small_buffer[10];
    result = uvhttp_static_generate_etag("/tmp/test.txt", now, 1024, small_buffer, sizeof(small_buffer));
    EXPECT_EQ(result, 0);
}

/* 测试缓存预热 */
TEST(UvhttpStaticExtraCoverageTest, PrewarmCache) {
    const char* test_file = "/tmp/uvhttp_test_prewarm.txt";
    int fd = open(test_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "test content", 12);
        close(fd);
    }
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_result_t result = uvhttp_static_prewarm_cache(ctx, "uvhttp_test_prewarm.txt");
    
    result = uvhttp_static_prewarm_cache(nullptr, "test.txt");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_prewarm_cache(ctx, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
    
    unlink(test_file);
}

/* 测试目录预热 */
TEST(UvhttpStaticExtraCoverageTest, PrewarmDirectory) {
    const char* test_dir = "/tmp/uvhttp_test_prewarm";
    mkdir(test_dir, 0755);
    
    const char* test_file1 = "/tmp/uvhttp_test_prewarm/file1.txt";
    const char* test_file2 = "/tmp/uvhttp_test_prewarm/file2.txt";
    
    int fd = open(test_file1, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "test content 1", 14);
        close(fd);
    }
    
    fd = open(test_file2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "test content 2", 14);
        close(fd);
    }
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    int count = uvhttp_static_prewarm_directory(ctx, "uvhttp_test_prewarm", 10);
    EXPECT_GE(count, 0);
    
    count = uvhttp_static_prewarm_directory(nullptr, "test_dir", 10);
    EXPECT_EQ(count, -1);
    
    count = uvhttp_static_prewarm_directory(ctx, nullptr, 10);
    EXPECT_EQ(count, -1);
    
    uvhttp_static_free(ctx);
    
    unlink(test_file1);
    unlink(test_file2);
    rmdir(test_dir);
}

/* 测试配置边界值 */
TEST(UvhttpStaticExtraCoverageTest, ConfigBoundaryValues) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 0;
    config.cache_ttl = 0;
    config.max_cache_entries = 0;
    strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    uvhttp_static_free(ctx);
    
    config.max_cache_size = SIZE_MAX;
    config.cache_ttl = INT_MAX;
    config.max_cache_entries = INT_MAX;
    
    ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    uvhttp_static_free(ctx);
}

/* 测试内存泄漏 */
TEST(UvhttpStaticExtraCoverageTest, MemoryLeaks) {
    for (int i = 0; i < 100; i++) {
        uvhttp_static_config_t config;
        memset(&config, 0, sizeof(config));
        config.max_cache_size = 1024 * 1024;
        strncpy(config.root_directory, "/tmp", sizeof(config.root_directory) - 1);
        
        uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
        ASSERT_NE(ctx, nullptr);
        
        uvhttp_static_clear_cache(ctx);
        uvhttp_static_free(ctx);
    }
}
