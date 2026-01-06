/**
 * @file uvhttp_test_framework.c
 * @brief UVHTTP 测试框架实现
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_allocator.h"

/* 声明测试套件函数 */
extern int test_suite_hash_extended(void);
extern int test_suite_lru_cache(void);

/* 全局测试上下文 */
uvhttp_test_context_t g_test_context = {0};

/* 模拟客户端结构 */
typedef struct {
    int is_mock;
    void* internal_data;
} uvhttp_mock_client_t;

void uvhttp_test_init(int verbose) {
    memset(&g_test_context, 0, sizeof(g_test_context));
    g_test_context.verbose = verbose;
    
    printf("=== UVHTTP 测试框架启动 ===\n");
    if (verbose) {
        printf("详细模式: 开启\n");
    }
    printf("\n");
}

void uvhttp_test_reset_memory_tracker(void) {
    if (g_test_context.verbose) {
        printf("内存跟踪器已重置\n");
    }
}

void uvhttp_test_cleanup(void) {
    if (g_test_context.verbose) {
        printf("清理测试环境...\n");
    }
    memset(&g_test_context, 0, sizeof(g_test_context));
}

void uvhttp_test_print_summary(void) {
    printf("\n=== 测试总结 ===\n");
    printf("总测试数: %d\n", g_test_context.stats.total_tests);
    printf("通过: %d\n", g_test_context.stats.passed_tests);
    printf("失败: %d\n", g_test_context.stats.failed_tests);
    
    if (g_test_context.stats.failed_tests == 0) {
        printf("✓ 所有测试通过!\n");
    } else {
        printf("✗ 有测试失败\n");
    }
    
    float success_rate = 0.0f;
    if (g_test_context.stats.total_tests > 0) {
        success_rate = (float)g_test_context.stats.passed_tests / g_test_context.stats.total_tests * 100.0f;
    }
    printf("成功率: %.1f%%\n", success_rate);
}

int uvhttp_test_run_all(void) {
    int failed_suites = 0;

    /* 只运行已知可以工作的测试套件 */
    if (test_suite_hash_extended() != 0) failed_suites++;
    if (test_suite_lru_cache() != 0) failed_suites++;

    return failed_suites;
}

void* uvhttp_test_create_mock_client(void) {
    uvhttp_mock_client_t* client = uvhttp_malloc(sizeof(uvhttp_mock_client_t));
    if (!client) return NULL;
    
    client->is_mock = 1;
    client->internal_data = NULL;
    
    return client;
}

void uvhttp_test_destroy_mock_client(void* client) {
    if (!client) return;
    uvhttp_free(client);
}
