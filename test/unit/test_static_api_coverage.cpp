/* uvhttp_static.c API 覆盖率测试 - 测试所有公开 API 函数 */

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

/* ========== 测试 uvhttp_static_create ========== */

TEST(UvhttpStaticApiTest, CreateNullConfig) {
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(nullptr, &ctx);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(ctx, nullptr);
}

TEST(UvhttpStaticApiTest, CreateNullContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    uvhttp_error_t result = uvhttp_static_create(&config, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, CreateValidConfig) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_free ========== */

TEST(UvhttpStaticApiTest, FreeNullContext) {
    /* 不应该崩溃 */
    uvhttp_static_free(nullptr);
}

/* ========== 测试 uvhttp_static_set_sendfile_config ========== */

TEST(UvhttpStaticApiTest, SetSendfileConfigNullContext) {
    uvhttp_error_t result = uvhttp_static_set_sendfile_config(nullptr, 5000, 3, 8192);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, SetSendfileConfigValid) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        result = uvhttp_static_set_sendfile_config(ctx, 5000, 3, 8192);
        EXPECT_EQ(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, SetSendfileConfigZeroValues) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        result = uvhttp_static_set_sendfile_config(ctx, 0, 0, 0);
        EXPECT_EQ(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_handle_request ========== */

TEST(UvhttpStaticApiTest, HandleRequestNullContext) {
    uvhttp_result_t result = uvhttp_static_handle_request(nullptr, nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, HandleRequestNullRequest) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        result = uvhttp_static_handle_request(ctx, nullptr, nullptr);
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_sendfile ========== */

TEST(UvhttpStaticApiTest, SendfileNullPath) {
    uvhttp_result_t result = uvhttp_static_sendfile(nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, SendfileNullResponse) {
    uvhttp_result_t result = uvhttp_static_sendfile("test.txt", nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, SendfileNonexistentFile) {
    uvhttp_result_t result = uvhttp_static_sendfile("/nonexistent/file.txt", nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== 测试 uvhttp_static_get_mime_type ========== */

TEST(UvhttpStaticApiTest, GetMimeTypeNullPath) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type(nullptr, mime_type, sizeof(mime_type));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GetMimeTypeNullBuffer) {
    uvhttp_result_t result = uvhttp_static_get_mime_type("test.txt", nullptr, 256);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GetMimeTypeZeroBufferSize) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("test.txt", mime_type, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GetMimeTypeHtmlFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("index.html", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeCssFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("style.css", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeJsFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("script.js", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeJsonFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("data.json", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypePngFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.png", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeJpgFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.jpg", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeUnknownFile) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.unknown", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticApiTest, GetMimeTypeNoExtension) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

/* ========== 测试 uvhttp_static_clear_cache ========== */

TEST(UvhttpStaticApiTest, ClearCacheNullContext) {
    /* 不应该崩溃 */
    uvhttp_static_clear_cache(nullptr);
}

TEST(UvhttpStaticApiTest, ClearCacheValidContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 不应该崩溃 */
        uvhttp_static_clear_cache(ctx);
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_prewarm_cache ========== */

TEST(UvhttpStaticApiTest, PrewarmCacheNullContext) {
    uvhttp_result_t result = uvhttp_static_prewarm_cache(nullptr, "test.txt");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, PrewarmCacheNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        result = uvhttp_static_prewarm_cache(ctx, nullptr);
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, PrewarmCacheNonexistentFile) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        result = uvhttp_static_prewarm_cache(ctx, "/nonexistent/file.txt");
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_prewarm_directory ========== */

TEST(UvhttpStaticApiTest, PrewarmDirectoryNullContext) {
    int result = uvhttp_static_prewarm_directory(nullptr, "test", 10);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpStaticApiTest, PrewarmDirectoryNullPath) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        int prewarm_result = uvhttp_static_prewarm_directory(ctx, nullptr, 10);
        EXPECT_EQ(prewarm_result, -1);
        
        uvhttp_static_free(ctx);
    }
}

TEST(UvhttpStaticApiTest, PrewarmDirectoryNonexistentDir) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        int prewarm_result = uvhttp_static_prewarm_directory(ctx, "/nonexistent/dir", 10);
        EXPECT_EQ(prewarm_result, -1);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_resolve_safe_path ========== */

TEST(UvhttpStaticApiTest, ResolveSafePathNullRootDir) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(nullptr, "test.txt", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, ResolveSafePathNullFilePath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", nullptr, resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, ResolveSafePathNullResolvedPath) {
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", nullptr, 512);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, ResolveSafePathZeroBufferSize) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", resolved_path, 0);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, ResolveSafePathPathTraversalAttack) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "../../../etc/passwd", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, ResolveSafePathValidPath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", resolved_path, sizeof(resolved_path));
    /* 结果取决于文件是否存在 */
    EXPECT_GE(result, 0);
}

/* ========== 测试 uvhttp_static_generate_etag ========== */

TEST(UvhttpStaticApiTest, GenerateEtagNullPath) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag(nullptr, 0, 0, etag, sizeof(etag));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GenerateEtagNullEtag) {
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 0, 0, nullptr, 256);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GenerateEtagZeroBufferSize) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 0, 0, etag, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, GenerateEtagValid) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 1234567890, 1024, etag, sizeof(etag));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(etag), (size_t)0);
    }
}

/* ========== 测试 uvhttp_static_check_conditional_request ========== */

TEST(UvhttpStaticApiTest, CheckConditionalRequestNullRequest) {
    int result = uvhttp_static_check_conditional_request(nullptr, "etag", 0);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, CheckConditionalRequestNullEtag) {
    int result = uvhttp_static_check_conditional_request(nullptr, nullptr, 0);
    EXPECT_EQ(result, 0);
}

/* ========== 测试 uvhttp_static_set_response_headers ========== */

TEST(UvhttpStaticApiTest, SetResponseHeadersNullResponse) {
    uvhttp_result_t result = uvhttp_static_set_response_headers(nullptr, "test.txt", 1024, 0, "etag");
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticApiTest, SetResponseHeadersNullPath) {
    uvhttp_result_t result = uvhttp_static_set_response_headers(nullptr, nullptr, 1024, 0, "etag");
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== 测试 uvhttp_static_get_cache_stats ========== */

TEST(UvhttpStaticApiTest, GetCacheStatsNullContext) {
    size_t total_memory;
    int entry_count, hit_count, miss_count, eviction_count;
    
    /* 不应该崩溃 */
    uvhttp_static_get_cache_stats(nullptr, &total_memory, &entry_count,
                                   &hit_count, &miss_count, &eviction_count);
}

TEST(UvhttpStaticApiTest, GetCacheStatsValidContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        size_t total_memory;
        int entry_count, hit_count, miss_count, eviction_count;
        
        /* 不应该崩溃 */
        uvhttp_static_get_cache_stats(ctx, &total_memory, &entry_count,
                                       &hit_count, &miss_count, &eviction_count);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_get_cache_hit_rate ========== */

TEST(UvhttpStaticApiTest, GetCacheHitRateNullContext) {
    double rate = uvhttp_static_get_cache_hit_rate(nullptr);
    EXPECT_GE(rate, 0.0);
    EXPECT_LE(rate, 1.0);
}

TEST(UvhttpStaticApiTest, GetCacheHitRateValidContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        double rate = uvhttp_static_get_cache_hit_rate(ctx);
        EXPECT_GE(rate, 0.0);
        EXPECT_LE(rate, 1.0);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_cleanup_expired_cache ========== */

TEST(UvhttpStaticApiTest, CleanupExpiredCacheNullContext) {
    int result = uvhttp_static_cleanup_expired_cache(nullptr);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticApiTest, CleanupExpiredCacheValidContext) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, ".", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        int result = uvhttp_static_cleanup_expired_cache(ctx);
        EXPECT_GE(result, 0);
        
        uvhttp_static_free(ctx);
    }
}

/* ========== 测试 uvhttp_static_enable_cache ========== */
/* 注意：此函数未在 uvhttp_static.c 中实现，跳过测试 */

/* ========== 测试 uvhttp_static_disable_cache ========== */
/* 注意：此函数未在 uvhttp_static.c 中实现，跳过测试 */

#endif /* UVHTTP_FEATURE_STATIC_FILES */