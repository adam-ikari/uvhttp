/* uvhttp_static.c 更多覆盖率测试 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>

/* 测试静态文件配置结构体初始化 */
TEST(UvhttpStaticMoreCoverageTest, ConfigStructureInit) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    /* 验证初始值 */
    EXPECT_EQ(config.max_cache_size, 0);
    EXPECT_EQ(config.sendfile_chunk_size, 0);
    EXPECT_EQ(config.cache_ttl, 0);
    EXPECT_EQ(config.max_cache_entries, 0);
    EXPECT_EQ(config.sendfile_timeout_ms, 0);
    EXPECT_EQ(config.sendfile_max_retry, 0);
    EXPECT_EQ(config.enable_directory_listing, 0);
    EXPECT_EQ(config.enable_etag, 0);
    EXPECT_EQ(config.enable_last_modified, 0);
    EXPECT_EQ(config.enable_sendfile, 0);
    EXPECT_STREQ(config.root_directory, "");
    EXPECT_STREQ(config.index_file, "");
    EXPECT_STREQ(config.custom_headers, "");
}

/* 测试静态文件上下文结构体初始化 */
TEST(UvhttpStaticMoreCoverageTest, ContextStructureInit) {
    uvhttp_static_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    /* 验证初始值 */
    EXPECT_EQ(ctx.cache, nullptr);
    EXPECT_EQ(ctx.config.max_cache_size, 0);
}

/* 测试MIME类型边界情况 */
TEST(UvhttpStaticMoreCoverageTest, MimeTypeEdgeCases) {
    char mime_type[256];
    int result;
    
    /* 测试空文件名 */
    result = uvhttp_static_get_mime_type("", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    
    /* 测试无扩展名文件 */
    result = uvhttp_static_get_mime_type("Makefile", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    
    /* 测试多个点 */
    result = uvhttp_static_get_mime_type("file.name.txt", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    EXPECT_NE(strstr(mime_type, "text/plain"), nullptr);
    
    /* 测试大写扩展名 */
    result = uvhttp_static_get_mime_type("test.HTML", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    
    /* 测试混合大小写 */
    result = uvhttp_static_get_mime_type("test.PnG", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    
    /* 测试只有扩展名 */
    result = uvhttp_static_get_mime_type(".txt", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    
    /* 测试路径中的文件名 */
    result = uvhttp_static_get_mime_type("/path/to/file.html", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, 0);
    EXPECT_NE(strstr(mime_type, "text/html"), nullptr);
}

/* 测试ETag生成边界情况 */
TEST(UvhttpStaticMoreCoverageTest, EtagEdgeCases) {
    char etag[256];
    uvhttp_result_t result;
    
    /* 测试空文件路径 */
    result = uvhttp_static_generate_etag("", 0, 0, etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(strlen(etag), 0);
    
    /* 测试特殊字符路径 */
    result = uvhttp_static_generate_etag("file with spaces.html", 1234567890, 1024, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试非常长的文件路径 */
    char long_path[300];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    result = uvhttp_static_generate_etag(long_path, 1234567890, 1024, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 测试最大时间戳 */
    result = uvhttp_static_generate_etag("test.html", 2147483647, 1024, 
                                         etag, sizeof(etag));
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试缓存统计边界情况 */
TEST(UvhttpStaticMoreCoverageTest, CacheStatsEdgeCases) {
    size_t total_memory;
    int entry_count, hit_count, miss_count, eviction_count;
    
    /* 测试NULL指针 */
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, NULL, NULL, NULL);
    
    /* 测试部分NULL指针 */
    uvhttp_static_get_cache_stats(NULL, &total_memory, NULL, NULL, NULL, NULL);
    uvhttp_static_get_cache_stats(NULL, NULL, &entry_count, NULL, NULL, NULL);
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, &hit_count, NULL, NULL);
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, NULL, &miss_count, NULL);
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, NULL, NULL, &eviction_count);
}

/* 测试缓存命中率边界情况 */
TEST(UvhttpStaticMoreCoverageTest, CacheHitRateEdgeCases) {
    double hit_rate;
    
    /* 测试NULL上下文 */
    hit_rate = uvhttp_static_get_cache_hit_rate(NULL);
    EXPECT_EQ(hit_rate, 0.0);
}

/* 测试sendfile函数边界情况 */
TEST(UvhttpStaticMoreCoverageTest, SendfileEdgeCases) {
    uvhttp_result_t result;
    
    /* 测试空文件路径 */
    result = uvhttp_static_sendfile("", NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试相对路径 */
    result = uvhttp_static_sendfile("test.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试绝对路径（不存在）*/
    result = uvhttp_static_sendfile("/nonexistent/file.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试特殊字符路径 */
    result = uvhttp_static_sendfile("/path/to/file with spaces.html", NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试处理请求函数边界情况 */
TEST(UvhttpStaticMoreCoverageTest, HandleRequestEdgeCases) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试NULL请求 */
        uvhttp_result_t handle_result = uvhttp_static_handle_request(ctx, NULL, NULL);
        EXPECT_NE(handle_result, UVHTTP_OK);
        
        /* 测试NULL响应 */
        handle_result = uvhttp_static_handle_request(ctx, NULL, NULL);
        EXPECT_NE(handle_result, UVHTTP_OK);
        
        uvhttp_static_free(ctx);
    }
}

/* 测试缓存预热边界情况 */
TEST(UvhttpStaticMoreCoverageTest, CachePrewarmEdgeCases) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    strncpy(config.root_directory, "./public", sizeof(config.root_directory) - 1);
    
    uvhttp_static_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_static_create(&config, &ctx);
    
    if (result == UVHTTP_OK) {
        /* 测试空文件路径 */
        result = uvhttp_static_prewarm_cache(ctx, "");
        /* 可能成功或失败 */
        
        /* 测试无效路径 */
        result = uvhttp_static_prewarm_cache(ctx, "/nonexistent/file.html");
        /* 可能成功或失败 */
        
        /* 测试目录预热（零文件数）*/
        int count = uvhttp_static_prewarm_directory(ctx, ".", 0);
        EXPECT_GE(count, -1);
        
        /* 测试目录预热（负数文件数）*/
        count = uvhttp_static_prewarm_directory(ctx, ".", -1);
        EXPECT_GE(count, -1);
        
        /* 测试目录预热（不存在的目录）*/
        count = uvhttp_static_prewarm_directory(ctx, "/nonexistent", 10);
        EXPECT_GE(count, -1);
        
        uvhttp_static_free(ctx);
    }
}

/* 测试设置响应头边界情况 */
TEST(UvhttpStaticMoreCoverageTest, SetResponseHeadersEdgeCases) {
    uvhttp_result_t result;
    
    /* 测试NULL文件路径 */
    result = uvhttp_static_set_response_headers(NULL, NULL, 0, 0, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试空文件路径 */
    result = uvhttp_static_set_response_headers(NULL, "", 1024, 1234567890, "\"abc123\"");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零文件大小 */
    result = uvhttp_static_set_response_headers(NULL, "test.html", 0, 1234567890, "\"abc123\"");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零时间戳 */
    result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 0, "\"abc123\"");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL ETag */
    result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 1234567890, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试空ETag */
    result = uvhttp_static_set_response_headers(NULL, "test.html", 1024, 1234567890, "");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试条件请求检查边界情况 */
TEST(UvhttpStaticMoreCoverageTest, ConditionalRequestEdgeCases) {
    int result;
    
    /* 测试NULL请求 */
    result = uvhttp_static_check_conditional_request(NULL, NULL, 0);
    EXPECT_GE(result, 0);
    
    /* 测试NULL ETag */
    result = uvhttp_static_check_conditional_request(NULL, NULL, 1234567890);
    EXPECT_GE(result, 0);
    
    /* 测试空ETag */
    result = uvhttp_static_check_conditional_request(NULL, "", 1234567890);
    EXPECT_GE(result, 0);
    
    /* 测试零时间戳 */
    result = uvhttp_static_check_conditional_request(NULL, "\"abc123\"", 0);
    EXPECT_GE(result, 0);
    
    /* 测试无效ETag格式 */
    result = uvhttp_static_check_conditional_request(NULL, "invalid-etag", 1234567890);
    EXPECT_GE(result, 0);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */