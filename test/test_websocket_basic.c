/* WebSocket基本功能测试 */
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

/* 模拟的请求和响应结构 */
typedef struct {
    char method[16];
    char url[256];
    char headers[1024];
} mock_request_t;

typedef struct {
    int status;
    char headers[1024];
    char body[4096];
} mock_response_t;

/* 模拟函数 */
static const char* mock_request_get_header(void* req, const char* name) {
    mock_request_t* request = (mock_request_t*)req;
    if (strcmp(name, "Upgrade") == 0 && strstr(request->headers, "Upgrade: websocket")) {
        return "websocket";
    }
    if (strcmp(name, "Connection") == 0 && strstr(request->headers, "Connection: Upgrade")) {
        return "Upgrade";
    }
    if (strcmp(name, "Sec-WebSocket-Key") == 0) {
        return "dGhlIHNhbXBsZSBub25jZQ==";
    }
    return NULL;
}

static void mock_response_set_status(void* resp, int status) {
    mock_response_t* response = (mock_response_t*)resp;
    response->status = status;
}

static void mock_response_set_header(void* resp, const char* name, const char* value) {
    mock_response_t* response = (mock_response_t*)resp;
    char header[256];
    snprintf(header, sizeof(header), "%s: %s", name, value);
    strcat(response->headers, header);
    strcat(response->headers, "\r\n");
}

/* 测试WebSocket创建 */
void test_websocket_creation() {
    printf("\n=== 测试WebSocket创建 ===\n");
    
    /* 测试空参数 */
    uvhttp_websocket_t* ws = uvhttp_websocket_new(NULL, NULL);
    TEST_ASSERT(ws == NULL, "空参数应返回NULL");
    
    /* 测试有效的WebSocket升级请求 */
    mock_request_t request = {
        .method = "GET",
        .url = "/ws",
        .headers = "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    };
    mock_response_t response = {0};
    
    /* 注意：由于libwebsockets依赖，这里只测试API可用性 */
    printf("注意：完整WebSocket创建测试需要libwebsockets环境\n");
}

/* 测试WebSocket选项配置 */
void test_websocket_options() {
    printf("\n=== 测试WebSocket选项配置 ===\n");
    
    /* 测试mTLS配置结构 */
    uvhttp_websocket_mtls_config_t mtls_config = {
        .server_cert_path = "test/certs/server.crt",
        .server_key_path = "test/certs/server.key",
        .ca_cert_path = "test/certs/ca.crt",
        .require_client_cert = 1,
        .verify_depth = 3
    };
    
    TEST_ASSERT(mtls_config.server_cert_path != NULL, "服务器证书文件路径设置");
    TEST_ASSERT(mtls_config.require_client_cert == 1, "客户端证书要求设置");
    TEST_ASSERT(mtls_config.verify_depth == 3, "证书验证深度设置");
    
    /* 测试WebSocket选项结构 */
    uvhttp_websocket_options_t ws_options = {
        .mtls_config = &mtls_config,
        .enable_tls = 1,
        .max_frame_size = 4096,
        .ping_interval = 30,
        .enable_compression = 0
    };
    
    TEST_ASSERT(ws_options.mtls_config == &mtls_config, "mTLS配置关联");
    TEST_ASSERT(ws_options.enable_tls == 1, "TLS启用设置");
    TEST_ASSERT(ws_options.max_frame_size == 4096, "最大帧大小设置");
}

/* 测试错误码定义 */
void test_error_codes() {
    printf("\n=== 测试错误码定义 ===\n");
    
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_NONE == 0, "无错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_INVALID_PARAM == -1, "无效参数错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_MEMORY == -2, "内存错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_TLS_CONFIG == -3, "TLS配置错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_CONNECTION == -4, "连接错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_NOT_CONNECTED == -5, "未连接错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_CERT_VERIFY == -6, "证书验证错误码定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_ERROR_PROTOCOL == -7, "协议错误码定义");
}

/* 测试WebSocket消息类型 */
void test_message_types() {
    printf("\n=== 测试WebSocket消息类型 ===\n");
    
    TEST_ASSERT(UVHTTP_WEBSOCKET_TEXT == 1, "文本消息类型定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_BINARY == 2, "二进制消息类型定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_PING == 9, "Ping消息类型定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_PONG == 10, "Pong消息类型定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_CLOSE == 8, "关闭消息类型定义");
    TEST_ASSERT(UVHTTP_WEBSOCKET_CONTINUATION == 0, "继续消息类型定义");
}

/* 测试全局清理函数 */
void test_cleanup() {
    printf("\n=== 测试全局清理函数 ===\n");
    
    /* 测试清理函数存在且可调用 */
    uvhttp_websocket_cleanup_global();
    TEST_ASSERT(1, "全局清理函数可调用");
}

int main() {
    printf("WebSocket基本功能测试开始...\n");
    
    test_websocket_creation();
    test_websocket_options();
    test_error_codes();
    test_message_types();
    test_cleanup();
    
    printf("\n=== 测试结果 ===\n");
    printf("总测试数: %d\n", tests_run);
    printf("通过测试: %d\n", tests_passed);
    printf("失败测试: %d\n", tests_run - tests_passed);
    printf("成功率: %.1f%%\n", tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    return (tests_passed == tests_run) ? 0 : 1;
}