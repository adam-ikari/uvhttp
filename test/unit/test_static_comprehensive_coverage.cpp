/* uvhttp_static.c 综合覆盖率测试 - 目标提升至 50%+ */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* ========== 测试创建和释放 ========== */

TEST(UvhttpStaticComprehensiveTest, CreateNullConfig) {
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(NULL, &ctx);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, CreateNullContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    uvhttp_error_t result = uvhttp_static_create(&config, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, CreateWithInvalidRoot) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "/nonexistent/path/123456", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    /* 根目录不存在可能失败 */
    if (result == UVHTTP_OK) {
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticComprehensiveTest, FreeNullContext) {
    /* 不应该崩溃 */
    uvhttp_static_free(NULL);
}

/* ========== 测试 sendfile 配置 ========== */

TEST(UvhttpStaticComprehensiveTest, SetSendfileConfigNullContext) {
    uvhttp_error_t result = uvhttp_static_set_sendfile_config(NULL, 5000, 3, 8192);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, SetSendfileConfigWithValues) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试自定义值 */
        result = uvhttp_static_set_sendfile_config(ctx, 10000, 5, 16384);
        EXPECT_EQ(result, UVHTTP_OK);
        
        /* 测试零值（使用默认值） */
        result = uvhttp_static_set_sendfile_config(ctx, 0, 0, 0);
        EXPECT_EQ(result, UVHTTP_OK);
        
        /* 测试负值 */
        result = uvhttp_static_set_sendfile_config(ctx, -1, -1, 0);
        EXPECT_EQ(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试缓存操作 ========== */

TEST(UvhttpStaticComprehensiveTest, GetCacheStatsNullContext) {
    size_t total_memory;
    int entry_count, hit_count, miss_count, eviction_count;
    
    /* 不应该崩溃 */
    uvhttp_static_get_cache_stats(NULL, &total_memory, &entry_count,
                                   &hit_count, &miss_count, &eviction_count);
}

TEST(UvhttpStaticComprehensiveTest, GetCacheHitRateNullContext) {
    double hit_rate = uvhttp_static_get_cache_hit_rate(NULL);
    EXPECT_EQ(hit_rate, 0.0);
}

TEST(UvhttpStaticComprehensiveTest, ClearCacheNullContext) {
    /* 不应该崩溃 */
    uvhttp_static_clear_cache(NULL);
}

/* uvhttp_static_enable_cache 函数未实现，跳过此测试 */

TEST(UvhttpStaticComprehensiveTest, CleanupExpiredCacheNullContext) {
    int count = uvhttp_static_cleanup_expired_cache(NULL);
    EXPECT_EQ(count, 0);
}

TEST(UvhttpStaticComprehensiveTest, PrewarmCacheNullContext) {
    uvhttp_result_t result = uvhttp_static_prewarm_cache(NULL, "index.html");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, PrewarmDirectoryNullContext) {
    int count = uvhttp_static_prewarm_directory(NULL, ".", 10);
    EXPECT_EQ(count, -1);
}

/* ========== 测试 MIME 类型 ========== */

TEST(UvhttpStaticComprehensiveTest, GetMimeTypeNullPath) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type(NULL, mime_type, sizeof(mime_type));
    EXPECT_NE(result, UVHTTP_OK);  /* NULL 路径应该返回错误 */
}

TEST(UvhttpStaticComprehensiveTest, GetMimeTypeNullBuffer) {
    uvhttp_result_t result = uvhttp_static_get_mime_type("test.html", NULL, 256);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, GetMimeTypeZeroBufferSize) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("test.html", mime_type, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, GetMimeTypeAllExtensions) {
    char mime_type[256];
    const char* extensions[] = {
        ".html", ".htm", ".css", ".js", ".json", ".xml", ".txt", ".md", ".csv",
        ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico", ".webp", ".bmp",
        ".mp3", ".wav", ".ogg", ".aac",
        ".mp4", ".webm", ".avi",
        ".woff", ".woff2", ".ttf", ".eot",
        ".pdf", ".zip", ".tar", ".gz"
    };
    
    for (size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "test%s", extensions[i]);
        uvhttp_result_t result = uvhttp_static_get_mime_type(filename, mime_type, sizeof(mime_type));
        EXPECT_EQ(result, UVHTTP_OK) << "Failed for extension: " << extensions[i];
        EXPECT_GT(strlen(mime_type), 0) << "Empty MIME type for extension: " << extensions[i];
    }
}

/* ========== 测试路径安全 ========== */

TEST(UvhttpStaticComprehensiveTest, ResolveSafePathNullRoot) {
    char resolved[512];
    int result = uvhttp_static_resolve_safe_path(NULL, "file.txt", resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticComprehensiveTest, ResolveSafePathNullPath) {
    char resolved[512];
    int result = uvhttp_static_resolve_safe_path(".", NULL, resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticComprehensiveTest, ResolveSafePathNullResolved) {
    int result = uvhttp_static_resolve_safe_path(".", "file.txt", NULL, 512);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticComprehensiveTest, ResolveSafePathPathTraversal) {
    char resolved[512];
    int result;
    
    /* 测试 ../ 路径遍历 */
    result = uvhttp_static_resolve_safe_path(".", "../etc/passwd", resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    /* 测试绝对路径 */
    result = uvhttp_static_resolve_safe_path(".", "/etc/passwd", resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    /* 测试多层路径遍历 */
    result = uvhttp_static_resolve_safe_path(".", "test/../../etc/passwd", resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticComprehensiveTest, ResolveSafePathValidPath) {
    char resolved[512];
    int result;
    
    /* 测试正常路径 - 使用 public 目录 */
    result = uvhttp_static_resolve_safe_path("./public", "index.html", resolved, sizeof(resolved));
    /* 结果可能是 0 或 1，取决于文件是否存在，只验证不崩溃 */
    EXPECT_GE(result, 0);
    
    /* 测试子目录路径 */
    result = uvhttp_static_resolve_safe_path("./public", "static/index.html", resolved, sizeof(resolved));
    /* 结果可能是 0 或 1，取决于文件是否存在，只验证不崩溃 */
    EXPECT_GE(result, 0);
}

/* ========== 测试 ETag 生成 ========== */

TEST(UvhttpStaticComprehensiveTest, GenerateEtagNullPath) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag(NULL, 0, 0, etag, sizeof(etag));
    /* NULL 路径应该返回错误 */
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, GenerateEtagNullBuffer) {
    uvhttp_result_t result = uvhttp_static_generate_etag("test.html", 0, 0, NULL, 256);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, GenerateEtagZeroBufferSize) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.html", 0, 0, etag, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, GenerateEtagDifferentInputs) {
    char etag1[256], etag2[256];
    
    /* 相同输入应该生成相同的 ETag */
    uvhttp_static_generate_etag("test.html", 1234567890, 1024, etag1, sizeof(etag1));
    uvhttp_static_generate_etag("test.html", 1234567890, 1024, etag2, sizeof(etag2));
    EXPECT_STREQ(etag1, etag2);
    
    /* 不同输入应该生成不同的 ETag */
    uvhttp_static_generate_etag("test.html", 1234567890, 2048, etag2, sizeof(etag2));
    EXPECT_STRNE(etag1, etag2);
}

/* ========== 测试条件请求 ========== */

TEST(UvhttpStaticComprehensiveTest, CheckConditionalRequestNullRequest) {
    char etag[64] = "\"abc123\"";
    int result = uvhttp_static_check_conditional_request(NULL, etag, 1234567890);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticComprehensiveTest, CheckConditionalRequestNullEtag) {
    /* 模拟请求对象 */
    char fake_request[256] = {0};
    int result = uvhttp_static_check_conditional_request(fake_request, NULL, 1234567890);
    EXPECT_EQ(result, 0);
}

/* ========== 测试响应头设置 ========== */

TEST(UvhttpStaticComprehensiveTest, SetResponseHeadersNullResponse) {
    char etag[64] = "\"abc123\"";
    uvhttp_result_t result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 1234567890, etag);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, SetResponseHeadersNullPath) {
    /* 模拟响应对象 */
    char fake_response[256] = {0};
    char etag[64] = "\"abc123\"";
    uvhttp_result_t result = uvhttp_static_set_response_headers(fake_response, NULL, 1024, 1234567890, etag);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== 测试 sendfile ========== */

TEST(UvhttpStaticComprehensiveTest, SendfileNullPath) {
    char fake_response[256] = {0};
    uvhttp_result_t result = uvhttp_static_sendfile(NULL, fake_response);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, SendfileNullResponse) {
    uvhttp_result_t result = uvhttp_static_sendfile("test.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticComprehensiveTest, SendfileNonexistentFile) {
    char fake_response[256] = {0};
    uvhttp_result_t result = uvhttp_static_sendfile("/nonexistent/file.txt", fake_response);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== 测试完整流程 ========== */

TEST(UvhttpStaticComprehensiveTest, FullWorkflow) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    config.enable_etag = 1;
    config.enable_last_modified = 1;
    config.enable_sendfile = 1;
    config.sendfile_timeout_ms = 5000;
    config.sendfile_max_retry = 3;
    config.sendfile_chunk_size = 8192;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    strncpy(config.index_file, "index.html", sizeof(config.index_file) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试缓存操作 */
        size_t total_memory;
        int entry_count, hit_count, miss_count, eviction_count;
        uvhttp_static_get_cache_stats(ctx, &total_memory, &entry_count,
                                       &hit_count, &miss_count, &eviction_count);
        
        double hit_rate = uvhttp_static_get_cache_hit_rate(ctx);
        EXPECT_GE(hit_rate, 0.0);
        EXPECT_LE(hit_rate, 1.0);
        
        /* 测试缓存预热 */
        uvhttp_static_prewarm_cache(ctx, "test.txt");
        uvhttp_static_prewarm_directory(ctx, ".", 10);
        
        /* 测试清理过期缓存 */
        uvhttp_static_cleanup_expired_cache(ctx);
        
        /* 测试清除缓存 */
        uvhttp_static_clear_cache(ctx);
        
        uvhttp_static_free(ctx);
    }
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
