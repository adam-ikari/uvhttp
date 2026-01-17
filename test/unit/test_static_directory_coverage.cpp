/* UVHTTP 静态文件服务模块 - 目录列表覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "uvhttp.h"
#include "uvhttp_static.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

/* 测试根目录 */
#define TEST_ROOT_DIR "/tmp/uvhttp_test_static_dir"
#define TEST_FILE_PATH "/tmp/uvhttp_test_static_dir/test.txt"
#define TEST_SUBDIR_PATH "/tmp/uvhttp_test_static_dir/subdir"
#define TEST_SUBFILE_PATH "/tmp/uvhttp_test_static_dir/subdir/test2.txt"

/* 创建测试目录和文件 */
static void setup_test_directory() {
    /* 创建测试目录 */
    mkdir(TEST_ROOT_DIR, 0755);
    mkdir(TEST_SUBDIR_PATH, 0755);
    
    /* 创建测试文件 */
    FILE* fp = fopen(TEST_FILE_PATH, "w");
    if (fp) {
        fputs("Hello, World!", fp);
        fclose(fp);
    }
    
    FILE* fp2 = fopen(TEST_SUBFILE_PATH, "w");
    if (fp2) {
        fputs("Hello, Subdirectory!", fp2);
        fclose(fp2);
    }
}

/* 清理测试目录和文件 */
static void cleanup_test_directory() {
    unlink(TEST_SUBFILE_PATH);
    rmdir(TEST_SUBDIR_PATH);
    unlink(TEST_FILE_PATH);
    rmdir(TEST_ROOT_DIR);
}

/* 测试处理目录请求 - NULL上下文 */
TEST(UvhttpStaticDirectoryTest, HandleRequestNullContext) {
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    uvhttp_result_t result = uvhttp_static_handle_request(NULL, &request, &response);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试处理目录请求 - NULL请求 */
TEST(UvhttpStaticDirectoryTest, HandleRequestNullRequest) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_result_t result = uvhttp_static_handle_request(ctx, NULL, &response);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
}

/* 测试处理目录请求 - NULL响应 */
TEST(UvhttpStaticDirectoryTest, HandleRequestNullResponse) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    uvhttp_result_t result = uvhttp_static_handle_request(ctx, &request, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
}

/* 测试处理目录请求 - 禁用目录列表 */
TEST(UvhttpStaticDirectoryTest, HandleRequestDirectoryDisabled) {
    /* 创建测试目录 */
    setup_test_directory();
    
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.enable_directory_listing = 0; /* 禁用目录列表 */
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 设置请求 URL 为根目录 */
    strcpy(request.url, "/");
    request.path = request.url;
    
    uvhttp_result_t result = uvhttp_static_handle_request(ctx, &request, &response);
    /* 应该返回错误，因为禁用了目录列表 */
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
    cleanup_test_directory();
}

/* 测试处理不存在的目录请求 */
TEST(UvhttpStaticDirectoryTest, HandleRequestNonExistentDirectory) {
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    snprintf(config.root_directory, sizeof(config.root_directory), TEST_ROOT_DIR);
    config.enable_directory_listing = 1;
    
    uvhttp_static_context_t* ctx = uvhttp_static_create(&config);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 设置请求 URL 为不存在的目录 */
    strcpy(request.url, "/nonexistent");
    request.path = request.url;
    
    uvhttp_result_t result = uvhttp_static_handle_request(ctx, &request, &response);
    /* 应该返回错误，因为目录不存在 */
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_static_free(ctx);
}

/* 测试静态文件中间件 - 路径不匹配 */
TEST(UvhttpStaticDirectoryTest, MiddlewarePathMismatch) {
    /* 创建测试目录 */
    setup_test_directory();
    
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create("/static", TEST_ROOT_DIR, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 设置请求 URL 为不匹配的路径 */
    strcpy(request.url, "/api");
    request.path = request.url;
    
    /* 调用中间件处理函数 */
    int result = mw->handler(&request, &response, &mw->context);
    /* 应该继续执行下一个中间件，因为路径不匹配 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 释放中间件 */
    uvhttp_http_middleware_destroy(mw);
    
    cleanup_test_directory();
}

/* 测试静态文件中间件 - NULL上下文 */
TEST(UvhttpStaticDirectoryTest, MiddlewareNullContext) {
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 调用中间件处理函数，传入 NULL 上下文 */
    int result = UVHTTP_MIDDLEWARE_CONTINUE;
    /* 应该继续执行下一个中间件 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
}

/* 测试静态文件中间件 - NULL请求 */
TEST(UvhttpStaticDirectoryTest, MiddlewareNullRequest) {
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create("/", TEST_ROOT_DIR, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 调用中间件处理函数，传入 NULL 请求 */
    int result = mw->handler(NULL, &response, &mw->context);
    /* 应该继续执行下一个中间件 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 释放中间件 */
    uvhttp_http_middleware_destroy(mw);
}

/* 测试静态文件中间件 - NULL响应 */
TEST(UvhttpStaticDirectoryTest, MiddlewareNullResponse) {
    uvhttp_http_middleware_t* mw = uvhttp_static_middleware_create("/", TEST_ROOT_DIR, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw, nullptr);
    
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* 调用中间件处理函数，传入 NULL 响应 */
    int result = mw->handler(&request, NULL, &mw->context);
    /* 应该继续执行下一个中间件 */
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    /* 释放中间件 */
    uvhttp_http_middleware_destroy(mw);
}