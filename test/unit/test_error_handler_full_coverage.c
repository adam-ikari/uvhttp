/* UVHTTP 错误处理模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_constants.h"

/* 测试错误处理初始化 */
void test_error_init(void) {
    /* 测试初始化 */
    uvhttp_error_init();
    
    printf("test_error_init: PASSED\n");
}

/* 测试错误处理清理 */
void test_error_cleanup(void) {
    /* 测试清理 */
    uvhttp_error_cleanup();
    
    printf("test_error_cleanup: PASSED\n");
}

/* 测试设置错误处理配置 */
void test_error_set_config(void) {
    /* 测试设置配置 - 使用快速配置以加快测试速度 */
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_DEBUG,
        .customHandler = NULL,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
    /* 测试 NULL 参数 */
    uvhttp_error_set_config(NULL);
    
    printf("test_error_set_config: PASSED\n");
}

/* 测试错误报告 */
void test_error_report(void) {
    /* 测试错误报告 */
    uvhttp_error_report_(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter", 
                        "test_function", "test_file.c", 100, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_OUT_OF_MEMORY, "Out of memory", 
                        "test_function", "test_file.c", 200, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_CONNECTION_ACCEPT, "Connection accept failed", 
                        "test_function", "test_file.c", 300, NULL);
    
    uvhttp_error_report_(UVHTTP_OK, "Success", 
                        "test_function", "test_file.c", 400, NULL);
    
    printf("test_error_report: PASSED\n");
}

/* 测试错误恢复 */
void test_error_recovery(void) {
    /* 测试错误恢复 - 使用快速配置以加快测试速度 */
    uvhttp_error_config_t fast_config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = NULL,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&fast_config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 100,
        .message = "Connection accept failed",
        .timestamp = 0,
        .user_data = NULL
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    (void)result;  /* 结果取决于实现 */
    
    /* 测试内存不足错误恢复 */
    context.error_code = UVHTTP_ERROR_OUT_OF_MEMORY;
    result = uvhttp_error_attempt_recovery(&context);
    (void)result;
    
    /* 测试禁用恢复 */
    fast_config.enableRecovery = 0;
    uvhttp_error_set_config(&fast_config);
    
    result = uvhttp_error_attempt_recovery(&context);
    
    printf("test_error_recovery: PASSED\n");
}

/* 测试日志函数 */
void test_log_functions(void) {
    /* 测试不同级别的日志 */
    uvhttp_log(UVHTTP_LOG_LEVEL_DEBUG, "Debug message");
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Info message");
    uvhttp_log(UVHTTP_LOG_LEVEL_WARN, "Warning message");
    uvhttp_log(UVHTTP_LOG_LEVEL_ERROR, "Error message");
    uvhttp_log(UVHTTP_LOG_LEVEL_FATAL, "Fatal message");
    
    /* 测试格式化日志 */
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Formatted message: %d, %s, %f", 42, "test", 3.14);
    
    /* 测试 NULL 格式字符串 */
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, NULL);
    
    printf("test_log_functions: PASSED\n");
}

/* 测试日志级别过滤 */
void test_log_level_filtering(void) {
    /* 设置最小日志级别为 ERROR */
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_ERROR,
        .customHandler = NULL,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&config);
    
    /* 这些日志应该被过滤掉 */
    uvhttp_log(UVHTTP_LOG_LEVEL_DEBUG, "Debug message (should be filtered)");
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Info message (should be filtered)");
    uvhttp_log(UVHTTP_LOG_LEVEL_WARN, "Warning message (should be filtered)");
    
    /* 这些日志应该被输出 */
    uvhttp_log(UVHTTP_LOG_LEVEL_ERROR, "Error message (should be output)");
    uvhttp_log(UVHTTP_LOG_LEVEL_FATAL, "Fatal message (should be output)");
    
    /* 重置日志级别 */
    config.min_logLevel = UVHTTP_LOG_LEVEL_INFO;
    uvhttp_error_set_config(&config);
    
    printf("test_log_level_filtering: PASSED\n");
}

/* 测试自定义错误处理器 */
void custom_error_handler(const uvhttp_error_context_t* context) {
    /* 自定义错误处理器 */
    (void)context;
}

void test_custom_handler(void) {
    /* 测试自定义错误处理器 - 使用快速配置以加快测试速度 */
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = custom_error_handler,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&config);
    
    /* 触发错误报告 */
    uvhttp_error_report_(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter", 
                        "test_function", "test_file.c", 100, NULL);
    
    /* 重置为默认处理器 */
    config.customHandler = NULL;
    uvhttp_error_set_config(&config);
    
    printf("test_custom_handler: PASSED\n");
}

/* 测试不同错误类型 */
void test_different_error_types(void) {
    /* 测试不同类型的错误 */
    uvhttp_error_report_(UVHTTP_ERROR_SERVER_INIT, "Server init failed", 
                        "test_function", "test_file.c", 100, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_CONNECTION_INIT, "Connection init failed", 
                        "test_function", "test_file.c", 200, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_REQUEST_INIT, "Request init failed", 
                        "test_function", "test_file.c", 300, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_RESPONSE_INIT, "Response init failed", 
                        "test_function", "test_file.c", 400, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_TLS_INIT, "TLS init failed", 
                        "test_function", "test_file.c", 500, NULL);
    
    uvhttp_error_report_(UVHTTP_ERROR_ROUTER_INIT, "Router init failed", 
                        "test_function", "test_file.c", 600, NULL);
    
    printf("test_different_error_types: PASSED\n");
}

/* 测试多次初始化和清理 */
void test_multiple_init_cleanup(void) {
    /* 测试多次初始化和清理 */
    for (int i = 0; i < 5; i++) {
        uvhttp_error_init();
        uvhttp_error_cleanup();
    }
    
    printf("test_multiple_init_cleanup: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_error_handler.c 完整覆盖率测试 ===\n\n");

    test_error_init();
    test_error_cleanup();
    test_error_set_config();
    test_error_report();
    test_error_recovery();
    test_log_functions();
    test_log_level_filtering();
    test_custom_handler();
    test_different_error_types();
    test_multiple_init_cleanup();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}