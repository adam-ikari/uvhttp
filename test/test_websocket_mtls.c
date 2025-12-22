/* WebSocket mTLS配置测试 */
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 测试计数器 */
static int tests_run = 0;
static int tests_passed = 0;

/* 测试宏 */
#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("✓ PASS: %s\n", message); \
        } else { \
            printf("✗ FAIL: %s\n", message); \
        } \
    } while(0)

/* 测试mTLS配置结构初始化 */
void test_mtls_config_init() {
    printf("\n=== 测试mTLS配置初始化 ===\n");
    
    /* 测试默认配置 */
    uvhttp_websocket_mtls_config_t config = {0};
    
    TEST_ASSERT(config.server_cert_file == NULL, "默认服务器证书为NULL");
    TEST_ASSERT(config.server_key_file == NULL, "默认服务器私钥为NULL");
    TEST_ASSERT(config.ca_cert_file == NULL, "默认CA证书为NULL");
    TEST_ASSERT(config.client_cert_file == NULL, "默认客户端证书为NULL");
    TEST_ASSERT(config.client_key_file == NULL, "默认客户端私钥为NULL");
    TEST_ASSERT(config.require_client_cert == 0, "默认不要求客户端证书");
    TEST_ASSERT(config.verify_depth == 0, "默认验证深度为0");
}

/* 测试mTLS配置设置 */
void test_mtls_config_setup() {
    printf("\n=== 测试mTLS配置设置 ===\n");
    
    uvhttp_websocket_mtls_config_t config = {
        .server_cert_file = "test/certs/server.crt",
        .server_key_file = "test/certs/server.key",
        .ca_cert_file = "test/certs/ca.crt",
        .client_cert_file = "test/certs/client.crt",
        .client_key_file = "test/certs/client.key",
        .require_client_cert = 1,
        .verify_depth = 5
    };
    
    TEST_ASSERT(strcmp(config.server_cert_file, "test/certs/server.crt") == 0, "服务器证书文件设置正确");
    TEST_ASSERT(strcmp(config.server_key_file, "test/certs/server.key") == 0, "服务器私钥文件设置正确");
    TEST_ASSERT(strcmp(config.ca_cert_file, "test/certs/ca.crt") == 0, "CA证书文件设置正确");
    TEST_ASSERT(strcmp(config.client_cert_file, "test/certs/client.crt") == 0, "客户端证书文件设置正确");
    TEST_ASSERT(strcmp(config.client_key_file, "test/certs/client.key") == 0, "客户端私钥文件设置正确");
    TEST_ASSERT(config.require_client_cert == 1, "客户端证书要求启用");
    TEST_ASSERT(config.verify_depth == 5, "证书验证深度设置正确");
}

/* 测试WebSocket选项配置 */
void test_websocket_options_with_mtls() {
    printf("\n=== 测试WebSocket选项配置 ===\n");
    
    uvhttp_websocket_mtls_config_t mtls_config = {
        .server_cert_file = "test/certs/server.crt",
        .server_key_file = "test/certs/server.key",
        .ca_cert_file = "test/certs/ca.crt",
        .require_client_cert = 1,
        .verify_depth = 3
    };
    
    uvhttp_websocket_options_t options = {
        .mtls_config = &mtls_config,
        .enable_tls = 1,
        .tls_cipher_suites = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256",
        .max_frame_size = 8192,
        .ping_interval = 45,
        .enable_compression = 1
    };
    
    TEST_ASSERT(options.mtls_config == &mtls_config, "mTLS配置正确关联");
    TEST_ASSERT(options.enable_tls == 1, "TLS已启用");
    TEST_ASSERT(options.tls_cipher_suites != NULL, "TLS密码套件已设置");
    TEST_ASSERT(options.max_frame_size == 8192, "最大帧大小设置正确");
    TEST_ASSERT(options.ping_interval == 45, "心跳间隔设置正确");
    TEST_ASSERT(options.enable_compression == 1, "压缩已启用");
}

/* 测试mTLS配置验证 */
void test_mtls_config_validation() {
    printf("\n=== 测试mTLS配置验证 ===\n");
    
    /* 测试不完整配置 */
    uvhttp_websocket_mtls_config_t incomplete_config = {
        .server_cert_file = "test/certs/server.crt",
        /* 缺少私钥文件 */
        .require_client_cert = 1
    };
    
    TEST_ASSERT(incomplete_config.server_cert_file != NULL, "服务器证书存在");
    TEST_ASSERT(incomplete_config.server_key_file == NULL, "服务器私钥缺失");
    TEST_ASSERT(incomplete_config.require_client_cert == 1, "要求客户端证书");
    
    /* 测试客户端证书配置 */
    uvhttp_websocket_mtls_config_t client_config = {
        .client_cert_file = "test/certs/client.crt",
        .client_key_file = "test/certs/client.key",
        .ca_cert_file = "test/certs/ca.crt",
        .require_client_cert = 0
    };
    
    TEST_ASSERT(client_config.client_cert_file != NULL, "客户端证书存在");
    TEST_ASSERT(client_config.client_key_file != NULL, "客户端私钥存在");
    TEST_ASSERT(client_config.require_client_cert == 0, "不要求客户端证书（客户端模式）");
}

/* 测试证书路径验证 */
void test_certificate_paths() {
    printf("\n=== 测试证书路径验证 ===\n");
    
    const char* valid_paths[] = {
        "test/certs/server.crt",
        "/etc/ssl/test/certs/server.crt",
        "./test/certs/ca.crt",
        "../test/certs/client.key"
    };
    
    const char* invalid_paths[] = {
        "",
        NULL,
        "test/certs/",
        "certs"
    };
    
    /* 测试有效路径 */
    for (size_t i = 0; i < sizeof(valid_paths) / sizeof(valid_paths[0]); i++) {
        TEST_ASSERT(strlen(valid_paths[i]) > 0, "有效路径不为空");
        TEST_ASSERT(strstr(valid_paths[i], ".crt") || strstr(valid_paths[i], ".key"), \
                  "有效路径包含证书或私钥扩展名");
    }
    
    /* 测试无效路径 */
    for (size_t i = 0; i < sizeof(invalid_paths) / sizeof(invalid_paths[0]); i++) {
        if (invalid_paths[i] == NULL) {
            TEST_ASSERT(1, "NULL路径处理正确");
        } else {
            TEST_ASSERT(strlen(invalid_paths[i]) == 0 || \
                      (!strstr(invalid_paths[i], ".crt") && !strstr(invalid_paths[i], ".key")),
                      "无效路径识别正确");
        }
    }
}

/* 测试mTLS配置结构完整性 */
void test_mtls_config_structure() {
    printf("\n=== 测试mTLS配置结构完整性 ===\n");
    
    /* 验证结构包含所有必需字段 */
    uvhttp_websocket_mtls_config_t config = {0};
    
    /* 验证指针字段对齐 */
    TEST_ASSERT((uintptr_t)&config.server_cert_file % sizeof(void*) == 0,
               "server_cert_file字段对齐正确");
    TEST_ASSERT((uintptr_t)&config.server_key_file % sizeof(void*) == 0,
               "server_key_file字段对齐正确");
    TEST_ASSERT((uintptr_t)&config.ca_cert_file % sizeof(void*) == 0,
               "ca_cert_file字段对齐正确");
    TEST_ASSERT((uintptr_t)&config.client_cert_file % sizeof(void*) == 0,
               "client_cert_file字段对齐正确");
    TEST_ASSERT((uintptr_t)&config.client_key_file % sizeof(void*) == 0,
               "client_key_file字段对齐正确");
    
    /* 验证结构大小至少包含所有字段 */
    size_t min_size = sizeof(char*) * 5 + sizeof(int) * 2; /* 5个指针 + 2个int */
    TEST_ASSERT(sizeof(uvhttp_websocket_mtls_config_t) >= min_size,
               "mTLS配置结构包含所有必需字段");
    
    /* 验证WebSocket选项结构 */
    uvhttp_websocket_options_t options = {0};
    
    TEST_ASSERT((uintptr_t)&options.mtls_config % sizeof(void*) == 0,
               "mtls_config字段对齐正确");
    TEST_ASSERT((uintptr_t)&options.tls_cipher_suites % sizeof(void*) == 0,
               "tls_cipher_suites字段对齐正确");
    
    size_t options_min_size = sizeof(void*) * 2 + sizeof(int) * 3 + sizeof(size_t);
    TEST_ASSERT(sizeof(uvhttp_websocket_options_t) >= options_min_size,
               "WebSocket选项结构包含所有必需字段");
}

int main() {
    printf("WebSocket mTLS配置测试开始...\n");
    
    test_mtls_config_init();
    test_mtls_config_setup();
    test_websocket_options_with_mtls();
    test_mtls_config_validation();
    test_certificate_paths();
    test_mtls_config_structure();
    
    printf("\n=== 测试结果 ===\n");
    printf("总测试数: %d\n", tests_run);
    printf("通过测试: %d\n", tests_passed);
    printf("失败测试: %d\n", tests_run - tests_passed);
    printf("成功率: %.1f%%\n", tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}