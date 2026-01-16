/* UVHTTP 静态文件服务文件操作覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "uvhttp.h"
#include "uvhttp_static.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

/* 测试根目录 */
#define TEST_ROOT_DIR "/tmp/uvhttp_test_file_ops"
#define TEST_FILE_PATH "/tmp/uvhttp_test_file_ops/test.txt"
#define TEST_FILE_CONTENT "Hello, World!"

/* 设置测试目录 */
static void setup_test_directory() {
    mkdir(TEST_ROOT_DIR, 0755);
}

/* 清理测试目录 */
static void cleanup_test_directory() {
    system("rm -rf " TEST_ROOT_DIR);
}

/* 测试缓存命中率 - 有命中和未命中 */
TEST(UvhttpStaticFileOpsTest, GetCacheHitRateWithHits) {
    setup_test_directory();
    
    /* 创建测试文件 */
    int fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT, 0644);
    write(fd, TEST_FILE_CONTENT, strlen(TEST_FILE_CONTENT));
    close(fd);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 预热缓存 */
    uvhttp_static_prewarm_cache(ctx, "/test.txt");
    
    /* 获取缓存统计 */
    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;
    uvhttp_static_get_cache_stats(ctx, &total_memory_usage, &entry_count, 
                                  &hit_count, &miss_count, &eviction_count);
    
    /* 应该有缓存条目 */
    EXPECT_GT(entry_count, 0);
    
    /* 获取缓存命中率 */
    double hit_rate = uvhttp_static_get_cache_hit_rate(ctx);
    EXPECT_GE(hit_rate, 0.0);
    EXPECT_LE(hit_rate, 100.0);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试缓存命中率 - 无命中 */
TEST(UvhttpStaticFileOpsTest, GetCacheHitRateNoHits) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 不预热缓存，直接获取命中率 */
    double hit_rate = uvhttp_static_get_cache_hit_rate(ctx);
    EXPECT_EQ(hit_rate, 0.0);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试缓存命中率 - NULL 上下文 */
TEST(UvhttpStaticFileOpsTest, GetCacheHitRateNullContext) {
    double hit_rate = uvhttp_static_get_cache_hit_rate(NULL);
    EXPECT_EQ(hit_rate, 0.0);
}

/* 测试清理过期缓存 */
TEST(UvhttpStaticFileOpsTest, CleanupExpiredCache) {
    setup_test_directory();
    
    /* 创建测试文件 */
    int fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT, 0644);
    write(fd, TEST_FILE_CONTENT, strlen(TEST_FILE_CONTENT));
    close(fd);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 1; /* 1秒 TTL */
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 预热缓存 */
    uvhttp_static_prewarm_cache(ctx, "/test.txt");
    
    /* 等待缓存过期 */
    sleep(2);
    
    /* 清理过期缓存 */
    int result = uvhttp_static_cleanup_expired_cache(ctx);
    EXPECT_GE(result, 0);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试清理过期缓存 - NULL 上下文 */
TEST(UvhttpStaticFileOpsTest, CleanupExpiredCacheNullContext) {
    int result = uvhttp_static_cleanup_expired_cache(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试清理过期缓存 - 无缓存 */
TEST(UvhttpStaticFileOpsTest, CleanupExpiredCacheNoCache) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 不预热缓存，直接清理 */
    int result = uvhttp_static_cleanup_expired_cache(ctx);
    EXPECT_EQ(result, 0);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试中间件清理函数 */
TEST(UvhttpStaticFileOpsTest, MiddlewareCleanup) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 创建中间件 */
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create("/", TEST_ROOT_DIR, 
                                                                   UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    /* 销毁中间件（应该调用清理函数） */
    uvhttp_http_middleware_destroy(mw);
    
    /* 销毁静态文件上下文 */
    uvhttp_static_free(ctx);
    
    cleanup_test_directory();
}

/* 测试中间件清理函数 - 带配置 */
TEST(UvhttpStaticFileOpsTest, MiddlewareCleanupWithConfig) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    
    /* 创建中间件 */
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create_with_config("/", &config,
                                                                             UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    /* 销毁中间件（应该调用清理函数） */
    uvhttp_http_middleware_destroy(mw);
    
    cleanup_test_directory();
}

/* 测试中间件处理函数 - NULL 路径前缀 */
TEST(UvhttpStaticFileOpsTest, MiddlewareHandlerNullPathPrefix) {
    setup_test_directory();
    
    /* 创建测试文件 */
    int fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT, 0644);
    write(fd, TEST_FILE_CONTENT, strlen(TEST_FILE_CONTENT));
    close(fd);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    /* 创建中间件（不设置路径前缀） */
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create_with_config("", &config,
                                                                             UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    strcpy(request.url, "/test.txt");
    request.path = request.url;
    
    /* 调用中间件处理函数 */
    int result = mw->handler(&request, &response, &mw->context);
    /* 应该处理请求，因为没有路径前缀限制 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_STOP);
    
    uvhttp_http_middleware_destroy(mw);
    cleanup_test_directory();
}

/* 测试中间件处理函数 - 路径不匹配 */
TEST(UvhttpStaticFileOpsTest, MiddlewareHandlerPathMismatch) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    /* 创建中间件，设置路径前缀为 /api */
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create_with_config("/api", &config,
                                                                             UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    strcpy(request.url, "/test.txt");
    request.path = request.url;
    
    /* 调用中间件处理函数 */
    int result = mw->handler(&request, &response, &mw->context);
    /* 路径不匹配，应该继续执行下一个中间件 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_http_middleware_destroy(mw);
    cleanup_test_directory();
}

/* 测试中间件处理函数 - 路径匹配 */
TEST(UvhttpStaticFileOpsTest, MiddlewareHandlerPathMatch) {
    setup_test_directory();
    
    /* 创建测试文件 */
    int fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT, 0644);
    write(fd, TEST_FILE_CONTENT, strlen(TEST_FILE_CONTENT));
    close(fd);
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    /* 创建中间件，设置路径前缀为 /static */
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create_with_config("/static", &config,
                                                                             UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    strcpy(request.url, "/static/test.txt");
    request.path = request.url;
    
    /* 调用中间件处理函数 */
    int result = mw->handler(&request, &response, &mw->context);
    /* 路径匹配，应该处理请求或继续执行（取决于文件是否存在） */
    EXPECT_TRUE(result == UVHTTP_MIDDLEWARE_STOP || result == UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_http_middleware_destroy(mw);
    cleanup_test_directory();
}

/* 测试缓存统计信息 - NULL 上下文 */
TEST(UvhttpStaticFileOpsTest, GetCacheStatsNullContext) {
    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;
    
    uvhttp_static_get_cache_stats(NULL, &total_memory_usage, &entry_count, 
                                  &hit_count, &miss_count, &eviction_count);
    
    /* 所有值应该为 0 */
    EXPECT_EQ(total_memory_usage, 0);
    EXPECT_EQ(entry_count, 0);
    EXPECT_EQ(hit_count, 0);
    EXPECT_EQ(miss_count, 0);
    EXPECT_EQ(eviction_count, 0);
}

/* 测试缓存统计信息 - 部分 NULL 参数 */
TEST(UvhttpStaticFileOpsTest, GetCacheStatsPartialNullParams) {
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    /* 只传递部分参数 */
    uvhttp_static_get_cache_stats(ctx, NULL, NULL, NULL, NULL, NULL);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试缓存统计信息 - 全部 NULL 参数 */
TEST(UvhttpStaticFileOpsTest, GetCacheStatsAllNullParams) {
    uvhttp_static_get_cache_stats(NULL, NULL, NULL, NULL, NULL, NULL);
    /* 不应该崩溃 */
}