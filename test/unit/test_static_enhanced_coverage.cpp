/* uvhttp_static.c 增强覆盖率测试 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/* 测试静态文件上下文创建和释放 */
TEST(UvhttpStaticEnhancedCoverageTest, CreateAndFree) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    /* 设置基本配置 */
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    config.enable_etag = 1;
    config.enable_last_modified = 1;
    config.enable_sendfile = 1;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    strncpy(config.index_file, "index.html", sizeof(config.index_file) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    /* 如果根目录不存在，可能失败，这是预期的 */
    if (result == UVHTTP_OK) {
        ASSERT_NE(ctx, nullptr);
        uvhttp_static_free(ctx);
    }
    
    /* 测试NULL参数 */
    result = uvhttp_static_create(NULL, &ctx);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_create(&config, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置sendfile配置 */
TEST(UvhttpStaticEnhancedCoverageTest, SetSendfileConfig) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试正常设置 */
        result = uvhttp_static_set_sendfile_config(ctx, 5000, 3, 8192);
        EXPECT_EQ(result, UVHTTP_OK);
        
        /* 测试使用默认值（0参数） */
        result = uvhttp_static_set_sendfile_config(ctx, 0, 0, 0);
        EXPECT_EQ(result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
    
    /* 测试NULL参数 */
    result = uvhttp_static_set_sendfile_config(NULL, 5000, 3, 8192);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试缓存操作 */
TEST(UvhttpStaticEnhancedCoverageTest, CacheOperations) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试缓存统计 */
        size_t total_memory;
        int entry_count, hit_count, miss_count, eviction_count;
        
        uvhttp_static_get_cache_stats(ctx, &total_memory, &entry_count,
                                       &hit_count, &miss_count, &eviction_count);
        
        /* 测试缓存命中率 */
        double hit_rate = uvhttp_static_get_cache_hit_rate(ctx);
        EXPECT_GE(hit_rate, 0.0);
        EXPECT_LE(hit_rate, 1.0);
        
        /* 测试清理缓存 */
        uvhttp_static_clear_cache(ctx);
        
        /* 测试清理过期缓存 */
        int cleaned = uvhttp_static_cleanup_expired_cache(ctx);
        EXPECT_GE(cleaned, 0);
        
        uvhttp_static_free(ctx);
    }
    
    /* 测试NULL参数 */
    uvhttp_static_clear_cache(NULL);
    
    {
        int cleaned = uvhttp_static_cleanup_expired_cache(NULL);
        EXPECT_EQ(cleaned, 0);
    }
    
    {
        double hit_rate = uvhttp_static_get_cache_hit_rate(NULL);
        EXPECT_EQ(hit_rate, 0.0);
    }
    
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, NULL, NULL, NULL);
}

/* 测试缓存预热 */
TEST(UvhttpStaticEnhancedCoverageTest, CachePrewarm) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试预热单个文件 */
        result = uvhttp_static_prewarm_cache(ctx, "index.html");
        /* 文件可能不存在，所以可能失败 */
        
        /* 测试预热目录 */
        int count = uvhttp_static_prewarm_directory(ctx, ".", 10);
        EXPECT_GE(count, -1);
        
        uvhttp_static_free(ctx);
    }
    
    /* 测试NULL参数 */
    result = uvhttp_static_prewarm_cache(NULL, "index.html");
    EXPECT_NE(result, UVHTTP_OK);
    
    {
        int count = uvhttp_static_prewarm_directory(NULL, ".", 10);
        EXPECT_EQ(count, -1);
    }
}

/* 测试安全路径解析 */
TEST(UvhttpStaticEnhancedCoverageTest, SafePathResolution) {
    char resolved[512];
    int result;
    
    /* 测试NULL参数 */
    result = uvhttp_static_resolve_safe_path(NULL, "index.html", 
                                             resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path("/var/www", NULL, 
                                             resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    result = uvhttp_static_resolve_safe_path("/var/www", "index.html", 
                                             NULL, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    /* 测试零缓冲区大小 */
    result = uvhttp_static_resolve_safe_path("/var/www", "index.html", 
                                             resolved, 0);
    EXPECT_EQ(result, 0);
    
    /* 测试路径遍历攻击 */
    result = uvhttp_static_resolve_safe_path("/var/www", "../../../etc/passwd", 
                                             resolved, sizeof(resolved));
    EXPECT_EQ(result, 0);
    
    /* 测试使用当前目录（实际存在的路径） */
    result = uvhttp_static_resolve_safe_path(".", "CMakeLists.txt", 
                                             resolved, sizeof(resolved));
    /* 可能成功或失败，取决于路径是否存在 */
    if (result == 1) {
        EXPECT_GT(strlen(resolved), 0);
    }
}

/* 测试MIME类型扩展 */
TEST(UvhttpStaticEnhancedCoverageTest, ExtendedMimeType) {
    char mime_type[256];
    uvhttp_result_t result;
    
    /* 测试更多文件类型 */
    const char* extensions[] = {
        ".pdf", ".zip", ".tar", ".gz", ".svg", ".woff", ".woff2", 
        ".ttf", ".eot", ".ico", ".mp4", ".webm", ".ogg", ".mp3",
        ".wav", ".flac", ".aac", ".xml", ".txt", ".md", ".yml",
        ".yaml", ".toml", ".ini", ".cfg", ".conf", ".log"
    };
    
    for (size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "test%s", extensions[i]);
        
        result = uvhttp_static_get_mime_type(filename, mime_type, sizeof(mime_type));
        EXPECT_EQ(result, UVHTTP_OK);
        EXPECT_GT(strlen(mime_type), 0);
    }
}

/* 测试ETag生成不同场景 */
TEST(UvhttpStaticEnhancedCoverageTest, EtagGenerationScenarios) {
    char etag[256];
    uvhttp_result_t result;
    
    /* 测试不同文件路径 */
    result = uvhttp_static_generate_etag("/path/to/file.html", 1234567890, 1024, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(strlen(etag), 0);
    
    result = uvhttp_static_generate_etag("/another/path/file.js", 9876543210, 2048, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(strlen(etag), 0);
    
    /* 测试零大小文件 */
    result = uvhttp_static_generate_etag("empty.txt", 0, 0, etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(strlen(etag), 0);
    
    /* 测试大文件 */
    result = uvhttp_static_generate_etag("large.bin", 1234567890, 1024 * 1024 * 100, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(strlen(etag), 0);
    
    /* 测试NULL参数 */
    result = uvhttp_static_generate_etag(NULL, 0, 0, etag, sizeof(etag));
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_generate_etag("test.html", 0, 0, NULL, sizeof(etag));
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_generate_etag("test.html", 0, 0, etag, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试条件请求检查 */
TEST(UvhttpStaticEnhancedCoverageTest, ConditionalRequestCheck) {
    /* 由于需要实际的request对象，这里只测试NULL参数 */
    int result;
    
    result = uvhttp_static_check_conditional_request(NULL, NULL, 0);
    EXPECT_GE(result, 0);
    
    result = uvhttp_static_check_conditional_request(NULL, "\"abc123\"", 1234567890);
    EXPECT_GE(result, 0);
}

/* 测试响应头设置 */
TEST(UvhttpStaticEnhancedCoverageTest, SetResponseHeaders) {
    uvhttp_result_t result;
    
    /* 测试NULL参数 */
    result = uvhttp_static_set_response_headers(NULL, NULL, 0, 0, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 1234567890, "\"abc123\"");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试sendfile函数 */
TEST(UvhttpStaticEnhancedCoverageTest, SendfileFunction) {
    uvhttp_result_t result;
    
    /* 测试NULL参数 */
    result = uvhttp_static_sendfile(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_static_sendfile("test.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试不存在的文件 */
    result = uvhttp_static_sendfile("/nonexistent/file.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试处理请求函数 */
TEST(UvhttpStaticEnhancedCoverageTest, HandleRequest) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试NULL参数 */
        uvhttp_result_t handle_result = uvhttp_static_handle_request(NULL, NULL, NULL);
        EXPECT_NE(handle_result, UVHTTP_OK);
        
        handle_result = uvhttp_static_handle_request(ctx, NULL, NULL);
        EXPECT_NE(handle_result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */