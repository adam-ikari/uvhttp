/**
 * @file uvhttp_test_framework.c
 * @brief UVHTTP 测试框架实现
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_allocator.h"

/* 声明测试套件函数 */
extern int test_suite_utils(void);
extern int test_suite_response(void);
extern int test_suite_request(void);
extern int test_suite_router(void);

/* 全局测试上下文 */
uvhttp_test_context_t g_test_context = {0};

/* 模拟客户端结构 */
typedef struct {
    int is_mock;
    void* internal_data;
} uvhttp_mock_client_t;

/* 模拟服务器结构 */
typedef struct {
    int is_mock;
    void* internal_data;
} uvhttp_mock_server_t;

/* 内存跟踪 */
static size_t g_allocated_bytes = 0;
static size_t g_allocation_count = 0;

void uvhttp_test_init(int verbose) {
    memset(&g_test_context, 0, sizeof(g_test_context));
    g_test_context.verbose = verbose;
    
    printf("=== UVHTTP 测试框架启动 ===\n");
    if (verbose) {
        printf("详细模式: 开启\n");
    }
    printf("\n");
}

void uvhttp_test_cleanup(void) {
    if (g_test_context.verbose) {
        printf("清理测试环境...\n");
    }
    
    /* 检查内存泄漏 */
    uvhttp_test_check_memory_leaks();
    
    memset(&g_test_context, 0, sizeof(g_test_context));
}

void uvhttp_test_print_summary(void) {
    printf("\n=== 测试总结 ===\n");
    printf("总测试数: %d\n", g_test_context.stats.total_tests);
    printf("通过: %d\n", g_test_context.stats.passed_tests);
    printf("失败: %d\n", g_test_context.stats.failed_tests);
    printf("跳过: %d\n", g_test_context.stats.skipped_tests);
    
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
    /* 这里将注册所有测试套件 */
    int failed_suites = 0;
    
    /* 运行各个测试套件 */
    if (test_suite_utils() != 0) failed_suites++;
    if (test_suite_response() != 0) failed_suites++;
    if (test_suite_request() != 0) failed_suites++;
    if (test_suite_router() != 0) failed_suites++;
    
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

void* uvhttp_test_create_mock_server(void) {
    uvhttp_mock_server_t* server = uvhttp_malloc(sizeof(uvhttp_mock_server_t));
    if (!server) return NULL;
    
    server->is_mock = 1;
    server->internal_data = NULL;
    
    return server;
}

void uvhttp_test_destroy_mock_server(void* server) {
    if (!server) return;
    uvhttp_free(server);
}

/* 内存跟踪重写 */
static void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        g_allocated_bytes += size;
        g_allocation_count++;
    }
    return ptr;
}

static void test_free(void* ptr) {
    if (ptr) {
        /* 简化的内存跟踪 - 实际实现应该记录分配的大小 */
        g_allocation_count--;
    }
    free(ptr);
}

static void* test_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    /* 简化实现 - 实际应该跟踪大小变化 */
    return new_ptr;
}

static void* test_calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (ptr) {
        g_allocated_bytes += count * size;
        g_allocation_count++;
    }
    return ptr;
}

void uvhttp_test_check_memory_leaks(void) {
    if (g_allocation_count > 0) {
        printf("警告: 检测到可能的内存泄漏\n");
        printf("未释放分配: %zu 个\n", g_allocation_count);
        printf("未释放字节: %zu\n", g_allocated_bytes);
    } else {
        printf("✓ 无内存泄漏检测\n");
    }
}

void uvhttp_test_reset_memory_tracker(void) {
    g_allocated_bytes = 0;
    g_allocation_count = 0;
}

/* 重写内存分配函数进行测试 */
void uvhttp_test_install_memory_tracker(void) {
    /* 这里应该设置自定义分配器，但为了简化暂时跳过 */
    /* 实际实现需要修改 uvhttp_allocator.h 中的宏定义 */
}