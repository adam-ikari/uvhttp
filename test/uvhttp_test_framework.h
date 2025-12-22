/**
 * @file uvhttp_test_framework.h
 * @brief UVHTTP 测试框架头文件
 * 
 * 提供简单、稳定、可靠的单元测试框架
 */

#ifndef UVHTTP_TEST_FRAMEWORK_H
#define UVHTTP_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 测试结果统计 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} uvhttp_test_stats_t;

/* 测试上下文 */
typedef struct {
    const char* current_suite;
    const char* current_test;
    uvhttp_test_stats_t stats;
    int verbose;
} uvhttp_test_context_t;

/* 全局测试上下文 */
extern uvhttp_test_context_t g_test_context;

/* 测试断言宏 */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s:%d - Assertion failed: %s\n", __func__, __LINE__, #condition); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s:%d - Expected %d == %d\n", __func__, __LINE__, (int)(expected), (int)(actual)); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            printf("FAIL: %s:%d - Expected %d != %d\n", __func__, __LINE__, (int)(expected), (int)(actual)); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("FAIL: %s:%d - Expected NULL\n", __func__, __LINE__); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("FAIL: %s:%d - Expected non-NULL\n", __func__, __LINE__); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_STREQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("FAIL: %s:%d - Expected \"%s\" == \"%s\"\n", __func__, __LINE__, (expected), (actual)); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_MEMEQ(expected, actual, size) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            printf("FAIL: %s:%d - Memory content mismatch\n", __func__, __LINE__); \
            g_test_context.stats.failed_tests++; \
            return -1; \
        } \
    } while(0)

/* 测试套件和测试用例宏 */
#define TEST_SUITE(name) \
    int test_suite_##name() { \
        g_test_context.current_suite = #name; \
        printf("=== 测试套件: %s ===\n", #name);

#define TEST_CASE(name) \
        g_test_context.current_test = #name; \
        g_test_context.stats.total_tests++; \
        if (g_test_context.verbose) printf("  运行 %s...\n", #name); \
        if (test_##name() == 0) { \
            g_test_context.stats.passed_tests++; \
            if (g_test_context.verbose) printf("  ✓ %s 通过\n", #name); \
        } else { \
            printf("  ✗ %s 失败\n", #name); \
        }

#define END_TEST_SUITE() \
        printf("--- %s: %d/%d 通过 ---\n\n", g_test_context.current_suite, \
               g_test_context.stats.passed_tests, g_test_context.stats.total_tests); \
        return (g_test_context.stats.failed_tests == 0) ? 0 : -1; \
    }

/* 测试函数声明 */
#define TEST_FUNC(name) int test_##name()

/* 框架函数 */
void uvhttp_test_init(int verbose);
void uvhttp_test_cleanup(void);
void uvhttp_test_print_summary(void);
int uvhttp_test_run_all(void);

/* 模拟对象函数 */
void* uvhttp_test_create_mock_client(void);
void uvhttp_test_destroy_mock_client(void* client);
void* uvhttp_test_create_mock_server(void);
void uvhttp_test_destroy_mock_server(void* server);

/* 内存测试辅助函数 */
void uvhttp_test_check_memory_leaks(void);
void uvhttp_test_reset_memory_tracker(void);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_TEST_FRAMEWORK_H */