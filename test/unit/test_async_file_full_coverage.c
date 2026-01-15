#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <uv.h>
#include <unistd.h>
#include <sys/stat.h>
#include "uvhttp_async_file.h"

/* ============ 测试辅助函数 ============ */

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

/* 完成回调计数器 */
static int completion_count = 0;
static int completion_status = 0;

/* 测试完成回调 */
static void test_completion_callback(uvhttp_async_file_request_t* req, int status) {
    (void)req;
    completion_count++;
    completion_status = status;
}

/* ============ 测试管理器创建和销毁 ============ */

void test_manager_create_normal(void) {
    printf("test_manager_create_normal: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    assert(manager->loop == loop);
    assert(manager->max_concurrent_reads == 10);
    assert(manager->current_reads == 0);
    assert(manager->read_buffer_size == 4096);
    assert(manager->max_file_size == 1024 * 1024);
    assert(manager->active_requests == NULL);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_manager_create_normal: PASSED\n");
}

void test_manager_create_null_loop(void) {
    printf("test_manager_create_null_loop: START\n");
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        NULL, 10, 4096, 1024 * 1024);
    assert(manager == NULL);
    
    printf("test_manager_create_null_loop: PASSED\n");
}

void test_manager_create_invalid_concurrent(void) {
    printf("test_manager_create_invalid_concurrent: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试零并发 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 0, 4096, 1024 * 1024);
    assert(manager == NULL);
    
    /* 测试负并发 */
    manager = uvhttp_async_file_manager_create(
        loop, -1, 4096, 1024 * 1024);
    assert(manager == NULL);
    
    printf("test_manager_create_invalid_concurrent: PASSED\n");
}

void test_manager_create_zero_buffer_size(void) {
    printf("test_manager_create_zero_buffer_size: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 0, 1024 * 1024);
    assert(manager == NULL);
    
    printf("test_manager_create_zero_buffer_size: PASSED\n");
}

void test_manager_free_null(void) {
    printf("test_manager_free_null: START\n");
    
    /* 不应该崩溃 */
    uvhttp_async_file_manager_free(NULL);
    
    printf("test_manager_free_null: PASSED\n");
}

/* ============ 测试异步文件读取 ============ */

void test_async_file_read_normal(void) {
    printf("test_async_file_read_normal: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_async_file_read.txt";
    const char* test_content = "Hello, async file!";
    assert(create_test_file(test_file, test_content) == 0);
    (void)test_content;
    
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 异步读取文件 */
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用 */
    assert(completion_count == 1);
    assert(completion_status == 0);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
    
    printf("test_async_file_read_normal: PASSED\n");
}

void test_async_file_read_null_manager(void) {
    printf("test_async_file_read_null_manager: START\n");
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    int result = uvhttp_async_file_read(NULL, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == -1);
    
    printf("test_async_file_read_null_manager: PASSED\n");
}

void test_async_file_read_null_path(void) {
    printf("test_async_file_read_null_path: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_read(manager, NULL, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_read_null_path: PASSED\n");
}

void test_async_file_read_null_request(void) {
    printf("test_async_file_read_null_request: START\n");
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        NULL, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_read_null_request: PASSED\n");
}

void test_async_file_read_null_response(void) {
    printf("test_async_file_read_null_response: START\n");
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, NULL, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_read_null_response: PASSED\n");
}

void test_async_file_read_null_callback(void) {
    printf("test_async_file_read_null_callback: START\n");
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_read.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, NULL);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_read_null_callback: PASSED\n");
}

void test_async_file_read_nonexistent_file(void) {
    printf("test_async_file_read_nonexistent_file: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 尝试读取不存在的文件 */
    const char* test_file = "/tmp/nonexistent_file_12345.txt";
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用，但状态为错误 */
    assert(completion_count == 1);
    assert(completion_status == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_read_nonexistent_file: PASSED\n");
}

void test_async_file_read_too_large(void) {
    printf("test_async_file_read_too_large: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建大文件 */
    const char* test_file = "/tmp/test_large_file.txt";
    char large_content[1024 * 1024 + 1];  /* 1MB + 1 byte */
    memset(large_content, 'A', sizeof(large_content));
    assert(create_test_file(test_file, large_content) == 0);
    
    /* 创建管理器，限制最大文件大小为 1MB */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 重置回调计数器 */
    completion_count = 0;
    completion_status = 0;
    
    /* 尝试读取过大的文件 */
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 验证回调被调用，但状态为错误 */
    assert(completion_count == 1);
    assert(completion_status == -1);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
    
    printf("test_async_file_read_too_large: PASSED\n");
}

/* ============ 测试取消异步文件读取 ============ */

void test_async_file_cancel_null_manager(void) {
    printf("test_async_file_cancel_null_manager: START\n");
    
    int result = uvhttp_async_file_cancel(NULL, NULL);
    (void)result;
    assert(result == -1);
    
    printf("test_async_file_cancel_null_manager: PASSED\n");
}

void test_async_file_cancel_null_request(void) {
    printf("test_async_file_cancel_null_request: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_cancel(manager, NULL);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_cancel_null_request: PASSED\n");
}

/* ============ 测试文件流传输 ============ */

void test_async_file_stream_normal(void) {
    printf("test_async_file_stream_normal: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_async_file_stream.txt";
    const char* test_content = "Hello, async file stream!";
    assert(create_test_file(test_file, test_content) == 0);
    (void)test_content;
    
    /* 创建管理器 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 流式传输文件 */
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 1024);
    (void)result;
    assert(result == 0);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
    
    printf("test_async_file_stream_normal: PASSED\n");
}

void test_async_file_stream_null_manager(void) {
    printf("test_async_file_stream_null_manager: START\n");
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    int result = uvhttp_async_file_stream(NULL, test_file, 
        (void*)0x1, 1024);
    (void)result;
    assert(result == -1);
    
    printf("test_async_file_stream_null_manager: PASSED\n");
}

void test_async_file_stream_null_path(void) {
    printf("test_async_file_stream_null_path: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_stream(manager, NULL, 
        (void*)0x1, 1024);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_stream_null_path: PASSED\n");
}

void test_async_file_stream_null_response(void) {
    printf("test_async_file_stream_null_response: START\n");
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_stream(manager, test_file, 
        NULL, 1024);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_stream_null_response: PASSED\n");
}

void test_async_file_stream_zero_chunk_size(void) {
    printf("test_async_file_stream_zero_chunk_size: START\n");
    uv_loop_t* loop = uv_default_loop();
    const char* test_file = "/tmp/test_async_file_stream.txt";
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 0);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_stream_zero_chunk_size: PASSED\n");
}

void test_async_file_stream_nonexistent_file(void) {
    printf("test_async_file_stream_nonexistent_file: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 尝试流传输不存在的文件 */
    const char* test_file = "/tmp/nonexistent_file_stream_12345.txt";
    int result = uvhttp_async_file_stream(manager, test_file, 
        (void*)0x1, 1024);
    (void)result;
    assert(result == -1);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_async_file_stream_nonexistent_file: PASSED\n");
}

/* ============ 测试停止文件流传输 ============ */

void test_async_file_stream_stop_null(void) {
    printf("test_async_file_stream_stop_null: START\n");
    
    int result = uvhttp_async_file_stream_stop(NULL);
    (void)result;
    assert(result == -1);
    
    printf("test_async_file_stream_stop_null: PASSED\n");
}

/* ============ 测试获取统计信息 ============ */

void test_get_stats_normal(void) {
    printf("test_get_stats_normal: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    int current_reads, max_concurrent;
    int result = uvhttp_async_file_get_stats(manager, &current_reads, &max_concurrent);
    (void)result;
    assert(result == 0);
    assert(current_reads == 0);
    assert(max_concurrent == 10);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_get_stats_normal: PASSED\n");
}

void test_get_stats_null_manager(void) {
    printf("test_get_stats_null_manager: START\n");
    
    int current_reads, max_concurrent;
    int result = uvhttp_async_file_get_stats(NULL, &current_reads, &max_concurrent);
    (void)result;
    assert(result == -1);
    
    printf("test_get_stats_null_manager: PASSED\n");
}

void test_get_stats_null_outputs(void) {
    printf("test_get_stats_null_outputs: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 10, 4096, 1024 * 1024);
    assert(manager != NULL);
    
    /* 测试 NULL 输出参数 */
    int result = uvhttp_async_file_get_stats(manager, NULL, NULL);
    (void)result;
    assert(result == 0);
    
    /* 测试部分 NULL 输出参数 */
    int current_reads, max_concurrent;
    result = uvhttp_async_file_get_stats(manager, &current_reads, NULL);
    (void)result;
    assert(result == 0);
    
    result = uvhttp_async_file_get_stats(manager, NULL, &max_concurrent);
    (void)result;
    assert(result == 0);
    
    uvhttp_async_file_manager_free(manager);
    printf("test_get_stats_null_outputs: PASSED\n");
}

/* ============ 测试边界条件 ============ */

void test_edge_cases(void) {
    printf("test_edge_cases: START\n");
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试最大并发数 */
    uvhttp_async_file_manager_t* manager = uvhttp_async_file_manager_create(
        loop, 1, 4096, 1024 * 1024);
    assert(manager != NULL);
    assert(manager->max_concurrent_reads == 1);
    
    /* 创建测试文件 */
    const char* test_file = "/tmp/test_edge_case.txt";
    const char* test_content = "Edge case test";
    assert(create_test_file(test_file, test_content) == 0);
    (void)test_content;
    
    /* 尝试读取文件（应该成功） */
    completion_count = 0;
    completion_status = 0;
    int result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == 0);
    
    /* 尝试再次读取（应该失败，因为达到并发限制） */
    result = uvhttp_async_file_read(manager, test_file, 
        (void*)0x1, (void*)0x2, (void*)0x3, test_completion_callback);
    (void)result;
    assert(result == -1);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uvhttp_async_file_manager_free(manager);
    delete_test_file(test_file);
    
    printf("test_edge_cases: PASSED\n");
}

/* ============ 主函数 ============ */

int main(void) {
    printf("=== uvhttp_async_file.c 完整覆盖率测试 ===\n\n");
    
    /* 测试管理器创建和销毁 */
    test_manager_create_normal();
    test_manager_create_null_loop();
    test_manager_create_invalid_concurrent();
    test_manager_create_zero_buffer_size();
    test_manager_free_null();
    
    /* 测试异步文件读取 */
    test_async_file_read_normal();
    test_async_file_read_null_manager();
    test_async_file_read_null_path();
    test_async_file_read_null_request();
    test_async_file_read_null_response();
    test_async_file_read_null_callback();
    test_async_file_read_nonexistent_file();
    test_async_file_read_too_large();
    
    /* 测试取消异步文件读取 */
    test_async_file_cancel_null_manager();
    test_async_file_cancel_null_request();
    
    /* 测试文件流传输 */
    test_async_file_stream_normal();
    test_async_file_stream_null_manager();
    test_async_file_stream_null_path();
    test_async_file_stream_null_response();
    test_async_file_stream_zero_chunk_size();
    test_async_file_stream_nonexistent_file();
    
    /* 测试停止文件流传输 */
    test_async_file_stream_stop_null();
    
    /* 测试获取统计信息 */
    test_get_stats_normal();
    test_get_stats_null_manager();
    test_get_stats_null_outputs();
    
    /* 测试边界条件 */
    test_edge_cases();
    
    printf("\n=== 所有测试通过 ===\n");
    return 0;
}
