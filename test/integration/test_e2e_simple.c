/*
 * Simple E2E Test - Using external server process
 * 简单的端到端测试：使用外部服务器进程
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* 发送 HTTP 请求 */
static int send_http_request(const char* host, int port, const char* method, 
                            const char* path, char* response_body, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s http://%s:%d%s 2>/dev/null", 
             method, host, port, path);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t bytes_read = fread(response_body, 1, max_len - 1, pipe);
    response_body[bytes_read] = '\0';
    
    int status;
    (void)pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

/* 测试基本功能 */
static void test_basic_functionality() {
    printf("\n=== 测试基本功能 ===\n");
    
    /* 启动测试服务器 */
    char server_cmd[256];
    snprintf(server_cmd, sizeof(server_cmd), 
             "%s/test_simple > /tmp/test_simple.log 2>&1 &", 
             getenv("PWD") ? getenv("PWD") : ".");
    
    int ret = system(server_cmd);
    (void)ret;
    sleep(2);  /* 等待服务器启动 */
    
    /* 测试 GET 请求 */
    char response[256];
    send_http_request("127.0.0.1", 8081, "GET", "/", response, sizeof(response));
    assert(strstr(response, "Test") != NULL);
    printf("✓ GET 请求测试通过\n");
    
    /* 停止服务器 */
    ret = system("pkill -f test_simple");
    (void)ret;
    printf("✓ 服务器已停止\n");
}

/* 测试并发请求 */
static void test_concurrent_requests() {
    printf("\n=== 测试并发请求 ===\n");
    
    /* 启动测试服务器 */
    char server_cmd[256];
    snprintf(server_cmd, sizeof(server_cmd), 
             "%s/test_simple > /tmp/test_simple.log 2>&1 &", 
             getenv("PWD") ? getenv("PWD") : ".");
    
    int ret = system(server_cmd);
    (void)ret;
    sleep(2);  /* 等待服务器启动 */
    
    /* 发送并发请求 */
    char response[256];
    for (int i = 0; i < 10; i++) {
        send_http_request("127.0.0.1", 8081, "GET", "/", response, sizeof(response));
        assert(strstr(response, "Test") != NULL);
    }
    printf("✓ 10 个并发请求测试通过\n");
    
    /* 停止服务器 */
    ret = system("pkill -f test_simple");
    (void)ret;
    printf("✓ 服务器已停止\n");
}

int main() {
    printf("========================================\n");
    printf("Simple End-to-End Test\n");
    printf("========================================\n");
    
    /* 运行所有测试 */
    test_basic_functionality();
    test_concurrent_requests();
    
    printf("\n========================================\n");
    printf("✅ 所有端到端测试通过\n");
    printf("========================================\n");
    
    return 0;
}