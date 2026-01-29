/* uvhttp_async_file.c 完整覆盖率测试 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_async_file.h"
#include "uvhttp_error.h"
#include <string.h>
#include <time.h>

/* 测试异步文件管理器创建和释放 */
TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateAndFree) {
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 65536, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(manager, nullptr);
    
    /* 验证初始状态 */
    EXPECT_EQ(manager->max_concurrent_reads, 64);
    EXPECT_EQ(manager->current_reads, 0);
    EXPECT_EQ(manager->read_buffer_size, 65536);
    EXPECT_EQ(manager->max_file_size, 10485760);
    EXPECT_EQ(manager->active_requests, nullptr);
    EXPECT_EQ(manager->loop, uv_default_loop());
    
    /* 释放管理器 */
    uvhttp_async_file_manager_free(manager);
    
    /* 测试释放NULL */
    uvhttp_async_file_manager_free(NULL);
}

/* 测试异步文件管理器创建失败 */
TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateFailure) {
    /* 测试NULL循环 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        NULL, 64, 65536, 10485760, &manager);
    EXPECT_NE(result, UVHTTP_OK);
    
    
    
    
}

/* 测试异步文件读取 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileRead) {
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 65536, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试NULL管理器 */
    result = uvhttp_async_file_read(NULL, "/test.txt", NULL, NULL, NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL文件路径 */
    result = uvhttp_async_file_read(manager, NULL, NULL, NULL, NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件取消 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileCancel) {
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 65536, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试NULL管理器 */
    result = uvhttp_async_file_cancel(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL请求 */
    result = uvhttp_async_file_cancel(manager, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件统计信息 */

/* 测试异步文件流式传输 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStream) {
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 65536, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 测试NULL管理器 */
    result = uvhttp_async_file_stream(NULL, "/test.txt", NULL, 65536);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL文件路径 */
    result = uvhttp_async_file_stream(manager, NULL, NULL, 65536);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件流式传输停止 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamStop) {
    /* 测试NULL上下文 */
    uvhttp_error_t result = uvhttp_async_file_stream_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件并发限制 */
TEST(UvhttpAsyncFileFullCoverageTest, ConcurrencyLimit) {
    /* 创建管理器，限制并发数为2 */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 2, 65536, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(manager->max_concurrent_reads, 2);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件大小限制 */
TEST(UvhttpAsyncFileFullCoverageTest, FileSizeLimit) {
    /* 创建管理器，限制文件大小为1KB */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 65536, 1024, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(manager->max_file_size, 1024);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件缓冲区大小 */
TEST(UvhttpAsyncFileFullCoverageTest, BufferSize) {
    /* 创建管理器，设置缓冲区大小为32KB */
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result = uvhttp_async_file_manager_create(
        uv_default_loop(), 64, 32768, 10485760, &manager);
    ASSERT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(manager->read_buffer_size, 32768);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
}

/* 测试异步文件状态 */
TEST(UvhttpAsyncFileFullCoverageTest, FileStates) {
    /* 测试所有状态 */
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_PENDING, 0);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_READING, 1);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_COMPLETED, 2);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_ERROR, 3);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
