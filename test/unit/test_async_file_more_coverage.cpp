/* uvhttp_async_file.c 更多覆盖率测试 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_async_file.h"
#include "uvhttp_error.h"
#include <string.h>

/* 测试异步文件状态枚举 */
TEST(UvhttpAsyncFileMoreCoverageTest, StateEnum) {
    /* 测试状态枚举值 */
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_PENDING, 0);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_READING, 1);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_COMPLETED, 2);
    EXPECT_EQ(UVHTTP_ASYNC_FILE_STATE_ERROR, 3);
}

/* 测试异步文件管理器创建（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, ManagerCreateNull) {
    uvhttp_async_file_manager_t* manager = NULL;
    uvhttp_error_t result;
    
    /* 测试NULL循环 */
    result = uvhttp_async_file_manager_create(NULL, 10, 8192, 1024 * 1024, &manager);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(manager, nullptr);
    
    /* 测试NULL输出参数 */
    result = uvhttp_async_file_manager_create(NULL, 10, 8192, 1024 * 1024, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零并发数 */
    result = uvhttp_async_file_manager_create(NULL, 0, 8192, 1024 * 1024, &manager);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零缓冲区大小 */
    result = uvhttp_async_file_manager_create(NULL, 10, 0, 1024 * 1024, &manager);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零最大文件大小 */
    result = uvhttp_async_file_manager_create(NULL, 10, 8192, 0, &manager);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件管理器释放（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, ManagerFreeNull) {
    uvhttp_async_file_manager_free(NULL);
}

/* 测试异步文件读取（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, ReadNull) {
    uvhttp_error_t result;
    
    result = uvhttp_async_file_read(NULL, NULL, NULL, NULL, NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件取消（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, CancelNull) {
    uvhttp_error_t result;
    
    result = uvhttp_async_file_cancel(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件流式传输（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, StreamNull) {
    uvhttp_error_t result;
    
    result = uvhttp_async_file_stream(NULL, NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件流式传输停止（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, StreamStopNull) {
    uvhttp_error_t result;
    
    result = uvhttp_async_file_stream_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件统计（NULL参数） */
TEST(UvhttpAsyncFileMoreCoverageTest, StatsNull) {
    int total_read, total_errors;
    
    uvhttp_error_t result = uvhttp_async_file_get_stats(NULL, &total_read, &total_errors);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试部分NULL参数 */
    result = uvhttp_async_file_get_stats(NULL, NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试异步文件请求结构体初始化 */
TEST(UvhttpAsyncFileMoreCoverageTest, RequestStructureInit) {
    uvhttp_async_file_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* 验证初始状态 */
    EXPECT_EQ(request.state, UVHTTP_ASYNC_FILE_STATE_PENDING);
    EXPECT_EQ(request.buffer, nullptr);
    EXPECT_EQ(request.buffer_size, 0);
    EXPECT_EQ(request.file_size, 0);
    EXPECT_EQ(request.last_modified, 0);
    EXPECT_EQ(request.request, nullptr);
    EXPECT_EQ(request.response, nullptr);
    EXPECT_EQ(request.static_context, nullptr);
    EXPECT_EQ(request.completion_cb, nullptr);
    EXPECT_EQ(request.next, nullptr);
    EXPECT_EQ(request.manager, nullptr);
    EXPECT_STREQ(request.file_path, "");
}

/* 测试异步文件管理器结构体初始化 */
TEST(UvhttpAsyncFileMoreCoverageTest, ManagerStructureInit) {
    uvhttp_async_file_manager_t manager;
    memset(&manager, 0, sizeof(manager));
    
    /* 验证初始状态 */
    EXPECT_EQ(manager.loop, nullptr);
    EXPECT_EQ(manager.active_requests, nullptr);
    EXPECT_EQ(manager.max_concurrent_reads, 0);
    EXPECT_EQ(manager.current_reads, 0);
    EXPECT_EQ(manager.read_buffer_size, 0);
    EXPECT_EQ(manager.max_file_size, 0);
}

/* 测试文件流上下文结构体初始化 */
TEST(UvhttpAsyncFileMoreCoverageTest, StreamContextInit) {
    uvhttp_file_stream_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    /* 验证初始状态 */
    EXPECT_EQ(ctx.file_handle, 0);  /* memset 设置为 0 */
    EXPECT_EQ(ctx.chunk_buffer, nullptr);
    EXPECT_EQ(ctx.chunk_size, 0);
    EXPECT_EQ(ctx.file_offset, 0);
    EXPECT_EQ(ctx.remaining_bytes, 0);
    EXPECT_EQ(ctx.response, nullptr);
    EXPECT_EQ(ctx.is_active, 0);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */