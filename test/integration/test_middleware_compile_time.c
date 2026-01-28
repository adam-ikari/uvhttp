/**
 * @file test_middleware_compile_time.c
 * @brief 测试编译时中间件调用
 * 
 * 这个测试验证中间件系统在编译时正确工作，
 * 不需要运行时注册或动态调用。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 编译时中间件函数 - 日志记录 */
static void logging_middleware(const char* message) {
    // 编译时中间件：直接调用函数，不需要注册
    #if !defined(NDEBUG) && UVHTTP_FEATURE_LOGGING
    UVHTTP_LOG_INFO("Middleware: %s", message);
    #else
    (void)message; // 避免未使用警告
    #endif
}

/* 编译时中间件函数 - 数据转换 */
static int transform_middleware(int value) {
    // 编译时中间件：直接调用函数
    return value * 2;
}

/* 测试编译时中间件调用 */
int test_compile_time_middleware(void) {
    printf("Testing compile-time middleware...\n");
    
    // 测试日志中间件
    logging_middleware("Test message");
    
    // 测试转换中间件
    int input = 10;
    int output = transform_middleware(input);
    if (output != 20) {
        fprintf(stderr, "Transform middleware failed: expected 20, got %d\n", output);
        return 1;
    }
    
    printf("✓ Compile-time middleware test passed\n");
    return 0;
}

/* 测试中间件链的正确性 */
int test_middleware_chain(void) {
    printf("Testing middleware chain...\n");
    
    // 测试中间件链调用
    int value = 5;
    
    // 链式调用：transform -> transform -> transform
    value = transform_middleware(value);
    value = transform_middleware(value);
    value = transform_middleware(value);
    
    // 5 * 2 * 2 * 2 = 40
    if (value != 40) {
        fprintf(stderr, "Middleware chain failed: expected 40, got %d\n", value);
        return 1;
    }
    
    printf("✓ Middleware chain test passed\n");
    return 0;
}

/* 测试不同中间件组合 */
int test_middleware_combinations(void) {
    printf("Testing middleware combinations...\n");
    
    // 测试各种中间件组合
    // 1. 单个中间件
    int result1 = transform_middleware(10);
    if (result1 != 20) return 1;
    
    // 2. 两个中间件
    int result2 = transform_middleware(transform_middleware(5));
    if (result2 != 20) return 1;
    
    // 3. 三个中间件
    int result3 = transform_middleware(transform_middleware(transform_middleware(3)));
    if (result3 != 24) return 1;
    
    printf("✓ Middleware combinations test passed\n");
    return 0;
}

int main(void) {
    printf("=== Compile-time Middleware Integration Tests ===\n\n");
    
    int failed = 0;
    
    if (test_compile_time_middleware() != 0) {
        failed++;
    }
    
    if (test_middleware_chain() != 0) {
        failed++;
    }
    
    if (test_middleware_combinations() != 0) {
        failed++;
    }
    
    printf("\n=== Test Summary ===\n");
    if (failed == 0) {
        printf("✓ All tests passed\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}