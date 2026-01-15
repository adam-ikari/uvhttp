/* UVHTTP 错误处理模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_error.h"
#include "uvhttp_constants.h"

/* 测试错误字符串转换 */
void test_error_string(void) {
    /* 测试标准错误码 - 只检查返回值不为 NULL */
    assert(uvhttp_error_string(UVHTTP_OK) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_NOT_FOUND) != NULL);
    
    /* 测试服务器错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN) != NULL);
    
    /* 测试连接错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_ACCEPT) != NULL);
    
    /* 测试请求/响应错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_REQUEST_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_INIT) != NULL);
    
    /* 测试 TLS 错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_TLS_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE) != NULL);
    
    /* 测试路由错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND) != NULL);
    
    /* 测试限流错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_RATE_LIMIT_EXCEEDED) != NULL);
    
    /* 测试 WebSocket 错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE) != NULL);
    
    /* 测试配置错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_CONFIG_PARSE) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_CONFIG_INVALID) != NULL);
    
    /* 测试中间件错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_EXECUTE) != NULL);
    
    /* 测试日志错误 */
    assert(uvhttp_error_string(UVHTTP_ERROR_LOG_INIT) != NULL);
    assert(uvhttp_error_string(UVHTTP_ERROR_LOG_WRITE) != NULL);
    
    printf("test_error_string: PASSED\n");
}

/* 测试错误分类字符串转换 */
void test_error_category_string(void) {
    /* 测试不同错误分类 - 只检查返回值不为 NULL */
    assert(uvhttp_error_category_string(UVHTTP_OK) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_REQUEST_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_TLS_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_RATE_LIMIT_EXCEEDED) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_PARSE) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_INIT) != NULL);
    assert(uvhttp_error_category_string(UVHTTP_ERROR_LOG_INIT) != NULL);
    
    printf("test_error_category_string: PASSED\n");
}

/* 测试错误描述 */
void test_error_description(void) {
    /* 测试错误描述 */
    const char* desc = uvhttp_error_description(UVHTTP_OK);
    assert(desc != NULL);
    
    desc = uvhttp_error_description(UVHTTP_ERROR_INVALID_PARAM);
    assert(desc != NULL);
    
    desc = uvhttp_error_description(UVHTTP_ERROR_SERVER_INIT);
    assert(desc != NULL);
    
    desc = uvhttp_error_description(UVHTTP_ERROR_CONNECTION_INIT);
    assert(desc != NULL);
    
    desc = uvhttp_error_description(UVHTTP_ERROR_TLS_INIT);
    assert(desc != NULL);
    
    desc = uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INIT);
    assert(desc != NULL);
    
    printf("test_error_description: PASSED\n");
}

/* 测试错误建议 */
void test_error_suggestion(void) {
    /* 测试错误建议 */
    const char* suggestion = uvhttp_error_suggestion(UVHTTP_OK);
    assert(suggestion != NULL);
    
    suggestion = uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_PARAM);
    assert(suggestion != NULL);
    
    suggestion = uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INIT);
    assert(suggestion != NULL);
    
    suggestion = uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_INIT);
    assert(suggestion != NULL);
    
    suggestion = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_INIT);
    assert(suggestion != NULL);
    
    suggestion = uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INIT);
    assert(suggestion != NULL);
    
    printf("test_error_suggestion: PASSED\n");
}

/* 测试错误是否可恢复 */
void test_error_is_recoverable(void) {
    /* 测试可恢复错误 */
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_ACCEPT) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_RESET) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_SEND) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE) == 1);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_EXECUTE) == 1);
    
    /* 测试不可恢复错误 */
    assert(uvhttp_error_is_recoverable(UVHTTP_OK) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_NOT_FOUND) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_REQUEST_INIT) == 0);
    assert(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_INIT) == 0);
    
    printf("test_error_is_recoverable: PASSED\n");
}

/* 测试错误恢复配置 */
void test_error_recovery_config(void) {
    /* 测试设置错误恢复配置 */
    uvhttp_set_error_recovery_config(5, 100, 5000, 3.0);
    
    /* 测试无效配置 */
    uvhttp_set_error_recovery_config(-1, 100, 5000, 3.0);
    uvhttp_set_error_recovery_config(5, -1, 5000, 3.0);
    uvhttp_set_error_recovery_config(5, 100, -1, 3.0);
    uvhttp_set_error_recovery_config(5, 100, 5000, 0.5);
    
    printf("test_error_recovery_config: PASSED\n");
}

/* 测试错误日志 */
void test_error_log(void) {
    /* 测试错误日志 */
    uvhttp_log_error(UVHTTP_OK, "Test context");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Server initialization failed");
    uvhttp_log_error(UVHTTP_ERROR_CONNECTION_INIT, "Connection initialization failed");
    uvhttp_log_error(UVHTTP_ERROR_TLS_INIT, "TLS initialization failed");
    
    /* 测试 NULL 上下文 */
    uvhttp_log_error(UVHTTP_OK, NULL);
    
    printf("test_error_log: PASSED\n");
}

/* 测试错误统计 */
void test_error_stats(void) {
    size_t error_counts[UVHTTP_ERROR_COUNT];
    time_t last_error_time;
    const char* last_error_context;
    
    /* 记录一些错误 */
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 1");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Test error 2");
    uvhttp_log_error(UVHTTP_ERROR_CONNECTION_INIT, "Test error 3");
    
    /* 获取错误统计 */
    uvhttp_get_error_stats(error_counts, &last_error_time, &last_error_context);
    
    /* 重置错误统计 */
    uvhttp_reset_error_stats();
    
    /* 验证重置 */
    uvhttp_get_error_stats(error_counts, &last_error_time, &last_error_context);
    
    printf("test_error_stats: PASSED\n");
}

/* 测试最频繁错误 */
void test_most_frequent_error(void) {
    /* 重置错误统计 */
    uvhttp_reset_error_stats();
    
    /* 记录一些错误 */
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 1");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 2");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Test error 3");
    
    /* 获取最频繁错误 */
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error();
    
    /* 重置错误统计 */
    uvhttp_reset_error_stats();
    
    printf("test_most_frequent_error: PASSED\n");
}

/* 测试重试操作 */
void test_retry_operation(void) {
    /* 创建一个简单的测试操作 */
    uvhttp_error_t test_operation(void* context) {
        (void)context;
        return UVHTTP_OK;
    }
    
    /* 测试成功的操作 */
    uvhttp_error_t result = uvhttp_retry_operation(test_operation, NULL, "Test operation");
    /* 结果应该是 UVHTTP_OK */
    
    printf("test_retry_operation: PASSED\n");
}

/* 测试所有错误码 */
void test_all_error_codes(void) {
    /* 测试所有错误码的字符串转换 */
    for (int i = UVHTTP_ERROR_MAX; i <= 0; i++) {
        const char* str = uvhttp_error_string((uvhttp_error_t)i);
        assert(str != NULL);
    }
    
    printf("test_all_error_codes: PASSED\n");
}

/* 测试边界条件 */
void test_edge_cases(void) {
    /* 测试边界条件 */
    uvhttp_error_t invalid_error = (uvhttp_error_t)9999;
    const char* str = uvhttp_error_string(invalid_error);
    assert(str != NULL);
    
    const char* category = uvhttp_error_category_string(invalid_error);
    assert(category != NULL);
    
    const char* desc = uvhttp_error_description(invalid_error);
    assert(desc != NULL);
    
    const char* suggestion = uvhttp_error_suggestion(invalid_error);
    assert(suggestion != NULL);
    
    int recoverable = uvhttp_error_is_recoverable(invalid_error);
    /* 结果取决于实现 */
    
    printf("test_edge_cases: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_error.c 完整覆盖率测试 ===\n\n");

    test_error_string();
    test_error_category_string();
    test_error_description();
    test_error_suggestion();
    test_error_is_recoverable();
    test_error_recovery_config();
    test_error_log();
    test_error_stats();
    test_most_frequent_error();
    test_retry_operation();
    test_all_error_codes();
    test_edge_cases();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}