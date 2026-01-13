/* UVHTTP 错误处理辅助函数完整覆盖率测试 - 最小化版本 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_constants.h"

/* 测试清理连接 - NULL参数 */
void test_cleanup_connection_null(void) {
    uvhttp_cleanup_connection(NULL, NULL);
    uvhttp_cleanup_connection(NULL, "error message");
    
    printf("test_cleanup_connection_null: PASSED\n");
}

/* 测试内存失败处理 - NULL参数 */
void test_handle_memory_failure_null(void) {
    uvhttp_handle_memory_failure(NULL, NULL, NULL);
    uvhttp_handle_memory_failure("test context", NULL, NULL);
    uvhttp_handle_memory_failure(NULL, (void (*)(void*))0x1234, NULL);
    
    printf("test_handle_memory_failure_null: PASSED\n");
}

/* 测试写错误处理 - NULL参数 */
void test_handle_write_error_null(void) {
    uvhttp_handle_write_error(NULL, 0, "test context");
    
    printf("test_handle_write_error_null: PASSED\n");
}

/* 测试清理错误消息 - NULL参数 */
void test_sanitize_error_message_null(void) {
    char buffer[256];
    
    assert(uvhttp_sanitize_error_message(NULL, buffer, sizeof(buffer)) != 0);
    assert(uvhttp_sanitize_error_message("message", NULL, sizeof(buffer)) != 0);
    assert(uvhttp_sanitize_error_message("message", buffer, 0) != 0);
    
    printf("test_sanitize_error_message_null: PASSED\n");
}

/* 测试清理错误消息 - 正常参数 */
void test_sanitize_error_message_normal(void) {
    char buffer[256];
    
    assert(uvhttp_sanitize_error_message("normal error message", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "normal error message") == 0);
    
    printf("test_sanitize_error_message_normal: PASSED\n");
}

/* 测试清理错误消息 - 敏感信息 */
void test_sanitize_error_message_sensitive(void) {
    char buffer[256];
    
    assert(uvhttp_sanitize_error_message("password: secret123", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Sensitive information hidden") == 0);
    
    assert(uvhttp_sanitize_error_message("token: abc123", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Sensitive information hidden") == 0);
    
    assert(uvhttp_sanitize_error_message("secret key", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Sensitive information hidden") == 0);
    
    printf("test_sanitize_error_message_sensitive: PASSED\n");
}

/* 测试清理错误消息 - 长消息 */
void test_sanitize_error_message_long(void) {
    char buffer[20];
    const char* long_message = "This is a very long error message that exceeds the buffer size";
    
    assert(uvhttp_sanitize_error_message(long_message, buffer, sizeof(buffer)) == 0);
    assert(strlen(buffer) < sizeof(buffer));
    assert(strstr(buffer, "...") != NULL);
    
    printf("test_sanitize_error_message_long: PASSED\n");
}

/* 测试清理错误消息 - 大小写敏感 */
void test_sanitize_error_message_case(void) {
    char buffer[256];
    
    assert(uvhttp_sanitize_error_message("PASSWORD: secret", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Sensitive information hidden") == 0);
    
    assert(uvhttp_sanitize_error_message("Password: secret", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Sensitive information hidden") == 0);
    
    printf("test_sanitize_error_message_case: PASSED\n");
}

/* 测试安全释放 - NULL参数 */
void test_safe_free_null(void) {
    uvhttp_safe_free(NULL, NULL);
    
    void* ptr = NULL;
    uvhttp_safe_free(&ptr, NULL);
    
    printf("test_safe_free_null: PASSED\n");
}

/* 测试安全释放 - 正常参数 */
void test_safe_free_normal(void) {
    void* ptr = malloc(100);
    assert(ptr != NULL);
    
    uvhttp_safe_free(&ptr, NULL);
    assert(ptr == NULL);
    
    printf("test_safe_free_normal: PASSED\n");
}

/* 测试安全释放 - 自定义释放函数 */
void test_safe_free_custom(void) {
    static int custom_free_called = 0;
    
    void custom_free(void* data) {
        custom_free_called = 1;
        free(data);
    }
    
    void* ptr = malloc(100);
    assert(ptr != NULL);
    
    custom_free_called = 0;
    uvhttp_safe_free(&ptr, custom_free);
    assert(ptr == NULL);
    assert(custom_free_called == 1);
    
    printf("test_safe_free_custom: PASSED\n");
}

/* 测试安全释放 - 多次释放 */
void test_safe_free_multiple(void) {
    void* ptr = malloc(100);
    assert(ptr != NULL);
    
    uvhttp_safe_free(&ptr, NULL);
    assert(ptr == NULL);
    
    /* 再次释放应该是安全的 */
    uvhttp_safe_free(&ptr, NULL);
    
    printf("test_safe_free_multiple: PASSED\n");
}

/* 测试内存失败处理 - 无清理函数 */
void test_handle_memory_failure_no_cleanup(void) {
    uvhttp_handle_memory_failure("test context", NULL, (void*)0x1234);
    
    printf("test_handle_memory_failure_no_cleanup: PASSED\n");
}

/* 测试内存失败处理 - 无清理数据 */
void test_handle_memory_failure_no_data(void) {
    static int cleanup_called = 0;
    
    void test_cleanup(void* data) {
        cleanup_called = 1;
        (void)data;
    }
    
    cleanup_called = 0;
    uvhttp_handle_memory_failure("test context", test_cleanup, NULL);
    assert(cleanup_called == 0);
    
    printf("test_handle_memory_failure_no_data: PASSED\n");
}

/* 测试清理错误消息 - 空消息 */
void test_sanitize_error_message_empty(void) {
    char buffer[256];
    
    assert(uvhttp_sanitize_error_message("", buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "") == 0);
    
    printf("test_sanitize_error_message_empty: PASSED\n");
}

/* 测试安全释放 - NULL释放函数 */
void test_safe_free_null_func(void) {
    void* ptr = malloc(100);
    assert(ptr != NULL);
    
    uvhttp_safe_free(&ptr, NULL);
    assert(ptr == NULL);
    
    printf("test_safe_free_null_func: PASSED\n");
}

/* 测试安全释放 - NULL指针值 */
void test_safe_free_null_ptr_value(void) {
    void* ptr = NULL;
    
    uvhttp_safe_free(&ptr, (void (*)(void*))0x1234);
    assert(ptr == NULL);
    
    printf("test_safe_free_null_ptr_value: PASSED\n");
}

/* 测试清理错误消息 - 不同敏感关键词 */
void test_sanitize_error_message_keywords(void) {
    char buffer[256];
    
    const char* keywords[] = {
        "password", "passwd", "secret", "key", "token",
        "auth", "credential", "private", "session"
    };
    
    for (int i = 0; i < 9; i++) {
        char test_msg[128];
        snprintf(test_msg, sizeof(test_msg), "%s: value123", keywords[i]);
        
        assert(uvhttp_sanitize_error_message(test_msg, buffer, sizeof(buffer)) == 0);
        assert(strcmp(buffer, "Sensitive information hidden") == 0);
    }
    
    printf("test_sanitize_error_message_keywords: PASSED\n");
}

/* 测试清理错误消息 - 边界缓冲区大小 */
void test_sanitize_error_message_buffer_sizes(void) {
    char buffer1[1];
    char buffer2[10];
    char buffer3[100];
    
    assert(uvhttp_sanitize_error_message("test", buffer1, sizeof(buffer1)) == 0);
    
    assert(uvhttp_sanitize_error_message("test message", buffer2, sizeof(buffer2)) == 0);
    
    assert(uvhttp_sanitize_error_message("normal message", buffer3, sizeof(buffer3)) == 0);
    
    printf("test_sanitize_error_message_buffer_sizes: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_error_helpers.c 完整覆盖率测试 ===\n\n");

    test_cleanup_connection_null();
    test_handle_memory_failure_null();
    test_handle_write_error_null();
    test_sanitize_error_message_null();
    test_sanitize_error_message_normal();
    test_sanitize_error_message_sensitive();
    test_sanitize_error_message_long();
    test_sanitize_error_message_case();
    test_safe_free_null();
    test_safe_free_normal();
    test_safe_free_custom();
    test_safe_free_multiple();
    test_handle_memory_failure_no_cleanup();
    test_handle_memory_failure_no_data();
    test_sanitize_error_message_empty();
    test_safe_free_null_func();
    test_safe_free_null_ptr_value();
    test_sanitize_error_message_keywords();
    test_sanitize_error_message_buffer_sizes();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}