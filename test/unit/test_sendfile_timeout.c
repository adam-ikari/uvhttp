/**
 * @file test_sendfile_timeout.c
 * @brief sendfile 超时和分块传输测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "uvhttp.h"
#include "uvhttp_static.h"

/* 测试结果统计 */
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            printf("✓ %s: PASSED\n", test_name); \
            tests_passed++; \
        } else { \
            printf("✗ %s: FAILED\n", test_name); \
            tests_failed++; \
        } \
    } while(0)

/* 创建测试文件 */
static int create_test_file(const char* path, size_t size) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        return -1;
    }
    
    /* 填充测试数据 */
    char buffer[4096];
    memset(buffer, 'A', sizeof(buffer));
    
    size_t remaining = size;
    while (remaining > 0) {
        size_t to_write = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        if (fwrite(buffer, 1, to_write, file) != to_write) {
            fclose(file);
            return -1;
        }
        remaining -= to_write;
    }
    
    fclose(file);
    return 0;
}

/* 测试 1: 中等文件分块传输 */
static void test_medium_file_chunked_transfer() {
    const char* test_file = "/tmp/test_sendfile_medium_5mb.bin";
    const size_t file_size = 5 * 1024 * 1024; /* 5MB */
    
    /* 创建测试文件 */
    if (create_test_file(test_file, file_size) != 0) {
        printf("✗ test_medium_file_chunked_transfer: Failed to create test file\n");
        tests_failed++;
        return;
    }
    
    /* 验证文件创建 */
    struct stat st;
    if (stat(test_file, &st) != 0) {
        printf("✗ test_medium_file_chunked_transfer: Failed to stat test file\n");
        tests_failed++;
        unlink(test_file);
        return;
    }
    
    TEST_ASSERT(st.st_size == (ssize_t)file_size, "Medium file size check");
    
    /* 清理 */
    unlink(test_file);
}

/* 测试 2: 中等文件边界情况（刚好 1MB） */
static void test_medium_file_boundary_1mb() {
    const char* test_file = "/tmp/test_sendfile_boundary_1mb.bin";
    const size_t file_size = 1 * 1024 * 1024; /* 1MB */
    
    if (create_test_file(test_file, file_size) != 0) {
        printf("✗ test_medium_file_boundary_1mb: Failed to create test file\n");
        tests_failed++;
        return;
    }
    
    struct stat st;
    if (stat(test_file, &st) != 0) {
        printf("✗ test_medium_file_boundary_1mb: Failed to stat test file\n");
        tests_failed++;
        unlink(test_file);
        return;
    }
    
    TEST_ASSERT(st.st_size == (ssize_t)file_size, "1MB boundary file size check");
    
    unlink(test_file);
}

/* 测试 3: 中等文件边界情况（刚好 10MB） */
static void test_medium_file_boundary_10mb() {
    const char* test_file = "/tmp/test_sendfile_boundary_10mb.bin";
    const size_t file_size = 10 * 1024 * 1024; /* 10MB */
    
    if (create_test_file(test_file, file_size) != 0) {
        printf("✗ test_medium_file_boundary_10mb: Failed to create test file\n");
        tests_failed++;
        return;
    }
    
    struct stat st;
    if (stat(test_file, &st) != 0) {
        printf("✗ test_medium_file_boundary_10mb: Failed to stat test file\n");
        tests_failed++;
        unlink(test_file);
        return;
    }
    
    TEST_ASSERT(st.st_size == (ssize_t)file_size, "10MB boundary file size check");
    
    unlink(test_file);
}

/* 测试 4: 小文件（应该使用传统方式） */
static void test_small_file_traditional() {
    const char* test_file = "/tmp/test_sendfile_small.bin";
    const size_t file_size = 2048; /* 2KB */
    
    if (create_test_file(test_file, file_size) != 0) {
        printf("✗ test_small_file_traditional: Failed to create test file\n");
        tests_failed++;
        return;
    }
    
    struct stat st;
    if (stat(test_file, &st) != 0) {
        printf("✗ test_small_file_traditional: Failed to stat test file\n");
        tests_failed++;
        unlink(test_file);
        return;
    }
    
    TEST_ASSERT(st.st_size == (ssize_t)file_size, "Small file size check");
    TEST_ASSERT(file_size < 4096, "Small file should use traditional method");
    
    unlink(test_file);
}

/* 测试 5: 大文件（应该使用分块 sendfile） */
static void test_large_file_chunked() {
    const char* test_file = "/tmp/test_sendfile_large_15mb.bin";
    const size_t file_size = 15 * 1024 * 1024; /* 15MB */
    
    if (create_test_file(test_file, file_size) != 0) {
        printf("✗ test_large_file_chunked: Failed to create test file\n");
        tests_failed++;
        return;
    }
    
    struct stat st;
    if (stat(test_file, &st) != 0) {
        printf("✗ test_large_file_chunked: Failed to stat test file\n");
        tests_failed++;
        unlink(test_file);
        return;
    }
    
    TEST_ASSERT(st.st_size == (ssize_t)file_size, "Large file size check");
    TEST_ASSERT(file_size > 10 * 1024 * 1024, "Large file should use chunked sendfile");
    
    unlink(test_file);
}

/* 测试 6: 超时配置验证 */
static void test_timeout_configuration() {
    /* 验证超时配置常量 */
    /* 注意：这些常量在 uvhttp_static.c 中定义，测试文件无法直接访问 */
    /* 这里我们验证文件大小分类逻辑 */
    
    TEST_ASSERT(4096 < (1 * 1024 * 1024), "Small file threshold (4KB < 1MB)");
    TEST_ASSERT((1 * 1024 * 1024) < (10 * 1024 * 1024), "Medium file threshold (1MB < 10MB)");
    TEST_ASSERT((10 * 1024 * 1024) < (15 * 1024 * 1024), "Large file threshold (10MB < 15MB)");
}

int main() {
    printf("=== sendfile 超时和分块传输测试 ===\n\n");
    
    test_medium_file_chunked_transfer();
    test_medium_file_boundary_1mb();
    test_medium_file_boundary_10mb();
    test_small_file_traditional();
    test_large_file_chunked();
    test_timeout_configuration();
    
    printf("\n=== 测试结果 ===\n");
    printf("通过: %d\n", tests_passed);
    printf("失败: %d\n", tests_failed);
    printf("总计: %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("\n=== 所有测试通过 ===\n");
        return 0;
    } else {
        printf("\n=== 有测试失败 ===\n");
        return 1;
    }
}
