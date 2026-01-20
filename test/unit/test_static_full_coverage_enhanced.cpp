/**
 * @file test_static_full_coverage_enhanced.cpp
 * @brief 增强的静态文件测试 - 提升覆盖率到 80%
 * 
 * 目标：提升 uvhttp_static.c 覆盖率从 0% 到 80%
 * 
 * 测试内容：
 * - 静态文件上下文创建和释放
 * - MIME 类型获取
 * - 路径安全检查
 * - ETag 生成
 * - 缓存管理
 * - sendfile 配置
 */

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 测试静态文件上下文创建 - NULL 配置 */
TEST(UvhttpStaticEnhancedTest, CreateNullConfig) {
    uvhttp_static_context_t* ctx = uvhttp_static_create(NULL);
    EXPECT_EQ(ctx, nullptr);
}

/* 测试静态文件上下文释放 - NULL */
TEST(UvhttpStaticEnhancedTest, FreeNull) {
    uvhttp_static_free(NULL);
}

/* 测试获取 MIME 类型 - NULL 文件路径 */
TEST(UvhttpStaticEnhancedTest, GetMimeTypeNullPath) {
    char mime_type[256];
    int result = uvhttp_static_get_mime_type(NULL, mime_type, sizeof(mime_type));
    EXPECT_NE(result, 0);
}

/* 测试获取 MIME 类型 - NULL 缓冲区 */
TEST(UvhttpStaticEnhancedTest, GetMimeTypeNullBuffer) {
    int result = uvhttp_static_get_mime_type("test.html", NULL, 256);
    EXPECT_NE(result, 0);
}

/* 测试获取 MIME 类型 - 缓冲区太小 */
TEST(UvhttpStaticEnhancedTest, GetMimeTypeBufferTooSmall) {
    char mime_type[1];
    int result = uvhttp_static_get_mime_type("test.html", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);  /* 函数返回成功，但缓冲区太小 */
}

/* 测试获取 MIME 类型 - 未知扩展名 */
TEST(UvhttpStaticEnhancedTest, GetMimeTypeUnknownExtension) {
    char mime_type[256];
    int result = uvhttp_static_get_mime_type("test.xyz", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(mime_type, "application/octet-stream");
}

/* 测试获取 MIME 类型 - 无扩展名 */
TEST(UvhttpStaticEnhancedTest, GetMimeTypeNoExtension) {
    char mime_type[256];
    int result = uvhttp_static_get_mime_type("testfile", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(mime_type, "application/octet-stream");
}

/* 测试路径安全检查 - NULL 根目录 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathNullRoot) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(NULL, "test.html", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试路径安全检查 - NULL 文件路径 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathNullFilePath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path("/var/www", NULL, resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试路径安全检查 - NULL 解析路径 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathNullResolved) {
    int result = uvhttp_static_resolve_safe_path("/var/www", "test.html", NULL, 512);
    EXPECT_EQ(result, 0);
}

/* 测试路径安全检查 - 缓冲区太小 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathBufferTooSmall) {
    char resolved_path[1];
    int result = uvhttp_static_resolve_safe_path("/var/www", "test.html", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试路径安全检查 - 路径遍历攻击 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathTraversalAttack) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path("/var/www", "../etc/passwd", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试路径安全检查 - 绝对路径 */
TEST(UvhttpStaticEnhancedTest, ResolveSafePathAbsolutePath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path("/var/www", "/etc/passwd", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

/* 测试 ETag 生成 - NULL 文件路径 */
TEST(UvhttpStaticEnhancedTest, GenerateEtagNullPath) {
    char etag[256];
    int result = uvhttp_static_generate_etag(NULL, 0, 0, etag, sizeof(etag));
    EXPECT_NE(result, 0);
}

/* 测试 ETag 生成 - NULL ETag 缓冲区 */
TEST(UvhttpStaticEnhancedTest, GenerateEtagNullBuffer) {
    int result = uvhttp_static_generate_etag("test.html", 0, 0, NULL, 256);
    EXPECT_NE(result, 0);
}

/* 测试 ETag 生成 - 缓冲区太小 */
TEST(UvhttpStaticEnhancedTest, GenerateEtagBufferTooSmall) {
    char etag[1];
    int result = uvhttp_static_generate_etag("test.html", 0, 0, etag, sizeof(etag));
    // snprintf 会自动截断，所以返回 0（成功），但 etag 会是空字符串
    EXPECT_EQ(result, 0);
    EXPECT_EQ(etag[0], '\0');  // 缓冲区太小，结果为空
}

/* 测试清除缓存 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, ClearCacheNull) {
    uvhttp_static_clear_cache(NULL);
}

/* 测试缓存预热 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, PrewarmCacheNull) {
    int result = uvhttp_static_prewarm_cache(NULL, "test.html");
    EXPECT_NE(result, 0);
}

/* 测试缓存预热 - NULL 文件路径 */
TEST(UvhttpStaticEnhancedTest, PrewarmCacheNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    if (ctx != NULL) {
        int result = uvhttp_static_prewarm_cache(ctx, NULL);
        EXPECT_NE(result, 0);
        
        uvhttp_static_free(ctx);
    }
}

/* 测试缓存预热目录 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, PrewarmDirectoryNull) {
    int result = uvhttp_static_prewarm_directory(NULL, "dir", 10);
    EXPECT_EQ(result, -1);
}

/* 测试缓存预热目录 - NULL 目录路径 */
TEST(UvhttpStaticEnhancedTest, PrewarmDirectoryNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strcpy(config.root_directory, "/tmp");
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    if (ctx != NULL) {
        int result = uvhttp_static_prewarm_directory(ctx, NULL, 10);
        EXPECT_EQ(result, -1);
        
        uvhttp_static_free(ctx);
    }
}

/* 测试获取缓存统计 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, GetCacheStatsNull) {
    size_t total_memory_usage;
    int entry_count;
    int hit_count;
    int miss_count;
    int eviction_count;
    
    uvhttp_static_get_cache_stats(NULL, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
}

/* 测试获取缓存命中率 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, GetCacheHitRateNull) {
    double rate = uvhttp_static_get_cache_hit_rate(NULL);
    EXPECT_EQ(rate, 0.0);
}

/* 测试清理过期缓存 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, CleanupExpiredCacheNull) {
    int result = uvhttp_static_cleanup_expired_cache(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试设置 sendfile 配置 - NULL 上下文 */
TEST(UvhttpStaticEnhancedTest, SetSendfileConfigNull) {
    int result = uvhttp_static_set_sendfile_config(NULL, 1000, 3, 4096);
    EXPECT_NE(result, 0);
}

/* 测试 sendfile - NULL 文件路径 */
TEST(UvhttpStaticEnhancedTest, SendfileNullPath) {
    int result = uvhttp_static_sendfile(NULL, NULL);
    EXPECT_NE(result, 0);
}

/* 测试设置响应头 - NULL 响应 */
TEST(UvhttpStaticEnhancedTest, SetResponseHeadersNull) {
    int result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 0, "etag");
    EXPECT_NE(result, 0);
}

/* 测试检查条件请求 - NULL 请求 */
TEST(UvhttpStaticEnhancedTest, CheckConditionalRequestNull) {
    int result = uvhttp_static_check_conditional_request(NULL, "etag", 0);
    EXPECT_EQ(result, 0);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpStaticEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_static_free(NULL);
        uvhttp_static_clear_cache(NULL);
        uvhttp_static_cleanup_expired_cache(NULL);
        
        char mime_type[256];
        uvhttp_static_get_mime_type(NULL, mime_type, sizeof(mime_type));
        uvhttp_static_get_mime_type("test.html", NULL, 256);
        
        char resolved_path[512];
        uvhttp_static_resolve_safe_path(NULL, "test.html", resolved_path, sizeof(resolved_path));
        uvhttp_static_resolve_safe_path("/var/www", NULL, resolved_path, sizeof(resolved_path));
        uvhttp_static_resolve_safe_path("/var/www", "test.html", NULL, 512);
        
        char etag[256];
        uvhttp_static_generate_etag(NULL, 0, 0, etag, sizeof(etag));
        uvhttp_static_generate_etag("test.html", 0, 0, NULL, 256);
        
        uvhttp_static_prewarm_cache(NULL, "test.html");
        uvhttp_static_prewarm_directory(NULL, "dir", 10);
        
        size_t total_memory_usage;
        int entry_count;
        int hit_count;
        int miss_count;
        int eviction_count;
        uvhttp_static_get_cache_stats(NULL, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
        
        uvhttp_static_get_cache_hit_rate(NULL);
        
        uvhttp_static_set_sendfile_config(NULL, 1000, 3, 4096);
        uvhttp_static_sendfile(NULL, NULL);
        uvhttp_static_set_response_headers(NULL, "test.html", 1024, 0, "etag");
        uvhttp_static_check_conditional_request(NULL, "etag", 0);
    }
}

/* 测试结构体大小 */
TEST(UvhttpStaticEnhancedTest, StructSizes) {
    EXPECT_GT(sizeof(uvhttp_static_config_t), 0);
    EXPECT_GT(sizeof(uvhttp_static_context_t), 0);
}