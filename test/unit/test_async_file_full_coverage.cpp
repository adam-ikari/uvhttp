#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <uv.h>
#include "uvhttp_async_file.h"

/* 测试完成回调计数器 */
static int completion_count = 0;
static int completion_status = 0;

/* 测试完成回调 */
static void test_completion_callback(uvhttp_async_file_request_t* req, int status) {
    (void)req;
    completion_count++;
    completion_status = status;
}

/* 创建测试文件 */
static int create_test_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t len = strlen(content);
    if (fwrite(content, 1, len, f) != len) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

/* 删除测试文件 */
static void delete_test_file(const char* path) {
    unlink(path);
}

/* 测试管理器创建和销毁 */
TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateNormal) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    EXPECT_EQ(manager->loop, loop);
    EXPECT_EQ(manager->max_concurrent_reads, 10);
    EXPECT_EQ(manager->current_reads, 0);
    EXPECT_EQ(manager->read_buffer_size, 4096);
    EXPECT_EQ(manager->max_file_size, 1024 * 1024);
    EXPECT_EQ(manager->active_requests, nullptr);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateNullLoop) {
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        NULL, 10, 4096, 1024 * 1024);
    EXPECT_EQ(manager, nullptr);
}

TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateInvalidConcurrent) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试零并发 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 0, 4096, 1024 * 1024);
    EXPECT_EQ(manager, nullptr);
    
    /* 测试负并发 */
    manager = uvhttp_async_file_manager_create(
        loop, -1, 4096, 1024 * 1024);
    EXPECT_EQ(manager, nullptr);
}

TEST(UvhttpAsyncFileFullCoverageTest, ManagerCreateZeroBufferSize) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 0, 1024 * 1024);
    EXPECT_EQ(manager, nullptr);
}

TEST(UvhttpAsyncFileFullCoverageTest, ManagerFreeNull) {
    /* 不应该崩溃 */
    uvhttp_async_file_manager_free(NULL);
}

/* 测试异步文件读取 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNormal) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_async_file_read.txt";
    const char* test_content = "Hello, async file!";
    ASSERT_EQ(create_test_file(test_file, test_content), 0);
    
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 异步读取文件 */
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用 */
    EXPECT_EQ(completion_count, 1);
    EXPECT_EQ(completion_status, 0);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNullManager) {
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    int result = uvhttp_async_file_read(NULL, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNullPath) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_read(manager, NULL, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNullRequest) {
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        NULL, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNullResponse) {
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, NULL, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNullCallback) {
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, NULL);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadNonexistentFile) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 尝试读取不存在的文件 */
    const char* test_file = "/tmp/nonexistent_file_12345.txt";
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用，但状态为错误 */
    EXPECT_EQ(completion_count, 1);
    EXPECT_EQ(completion_status, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileReadTooLarge) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建大文件 */
    const char* test_file = "/tmp/test_large_file.txt";
    char large_content[1024 * 1024 + 1];  /* 1MB + 1 byte */
    memset(large_content, 'A', sizeof(large_content));
    ASSERT_EQ(create_test_file(test_file, large_content), 0);
    
    /* 创建管理器，限制最大文件大小为 1MB */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 尝试读取过大的文件 */
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用，但状态为错误 */
    EXPECT_EQ(completion_count, 1);
    EXPECT_EQ(completion_status, -1);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
}

/* 测试取消异步文件读取 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileCancelNullManager) {
    int result = uvhttp_async_file_cancel(NULL, NULL);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileCancelNullRequest) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_cancel(manager, NULL);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

/* 测试文件流传输 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamNormal) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_async_file_stream.txt";
    const char* test_content = "Hello, async file stream!";
    ASSERT_EQ(create_test_file(test_file, test_content), 0);
    
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 流式传输文件 */
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 1024);
    EXPECT_EQ(result, 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamNullManager) {
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    int result = uvhttp_async_file_stream(NULL, test_file, 
        (void*)0x1, 1024);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamNullPath) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_stream(manager, NULL, 
        (void*)0x1, 1024);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamNullResponse) {
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_stream(manager, test_file, 
        NULL, 1024);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamZeroChunkSize) {
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 0);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamNonexistentFile) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 尝试流传输不存在的文件 */
    const char* test_file = "/tmp/nonexistent_file_stream_12345.txt";
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 1024);
    EXPECT_EQ(result, -1);
    
    uvhttp_async_file_manager_free(manager);
}

/* 测试停止文件流传输 */
TEST(UvhttpAsyncFileFullCoverageTest, AsyncFileStreamStopNull) {
    int result = uvhttp_async_file_stream_stop(NULL);
    EXPECT_EQ(result, -1);
}

/* 测试获取统计信息 */
TEST(UvhttpAsyncFileFullCoverageTest, GetStatsNormal) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    int current_reads, max_concurrent;
    int result = uvhttp_async_file_get_stats(manager, &current_reads, &max_concurrent);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(current_reads, 0);
    EXPECT_EQ(max_concurrent, 10);
    
    uvhttp_async_file_manager_free(manager);
}

TEST(UvhttpAsyncFileFullCoverageTest, GetStatsNullManager) {
    int current_reads, max_concurrent;
    int result = uvhttp_async_file_get_stats(NULL, &current_reads, &max_concurrent);
    EXPECT_EQ(result, -1);
}

TEST(UvhttpAsyncFileFullCoverageTest, GetStatsNullOutputs) {
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    
    /* 测试 NULL 输出参数 */
    int result = uvhttp_async_file_get_stats(manager, NULL, NULL);
    EXPECT_EQ(result, 0);
    
    /* 测试部分 NULL 输出参数 */
    int current_reads, max_concurrent;
    result = uvhttp_async_file_get_stats(manager, &current_reads, NULL);
    EXPECT_EQ(result, 0);
    
    result = uvhttp_async_file_get_stats(manager, NULL, &max_concurrent);
    EXPECT_EQ(result, 0);
    
    uvhttp_async_file_manager_free(manager);
}

/* 测试边界条件 */
TEST(UvhttpAsyncFileFullCoverageTest, EdgeCases) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试最大并发数 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 1, 4096, 1024 * 1024);
    ASSERT_NE(manager, nullptr);
    EXPECT_EQ(manager->max_concurrent_reads, 1);
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_edge_case.txt";
    const char* test_content = "Edge case test";
    ASSERT_EQ(create_test_file(test_file, test_content), 0);
    
    /* 尝试读取文件（应该成功） */
    completion_count = 0;
    completion_status = 0;
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, 0);
    
    /* 尝试再次读取（应该失败，因为达到并发限制） */
    result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    EXPECT_EQ(result, -1);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
}