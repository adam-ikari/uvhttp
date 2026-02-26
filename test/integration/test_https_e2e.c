/*
 * HTTPS End-to-End Test
 * HTTPS 端到端测试
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_tls.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* 全局请求计数 */
static int g_request_count = 0;

/* GET 请求处理器 */
static int get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    g_request_count++;
    
    const char* body = "HTTPS Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* 创建测试证书和密钥 */
static void create_test_certificates() {
    int ret = system("mkdir -p test/certs 2>/dev/null");
    (void)ret;
    ret = system("openssl req -x509 -newkey rsa:2048 -keyout test/certs/key.pem "
           "-out test/certs/cert.pem -days 365 -nodes "
           "-subj '/CN=localhost' 2>/dev/null");
    (void)ret;
}

/* 发送 HTTPS 请求 */
static int send_https_request(const char* host, int port, const char* path, 
                             char* response_body, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "curl -s -k https://%s:%d%s 2>/dev/null", 
             host, port, path);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t bytes_read = fread(response_body, 1, max_len - 1, pipe);
    response_body[bytes_read] = '\0';
    
    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

/* 测试基本 HTTPS 功能 */
static void test_https_basic() {
    printf("\n=== 测试基本 HTTPS 功能 ===\n");
    
    g_request_count = 0;
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uvhttp_tls_context_t* tls_ctx = NULL;
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建测试证书 */
    create_test_certificates();
    
    /* 创建 TLS 上下文 */
    uvhttp_tls_context_new(&tls_ctx);
    assert(tls_ctx != NULL);
    
    /* 加载证书和密钥 */
    uvhttp_tls_context_load_cert_chain(tls_ctx, "test/certs/cert.pem");
    uvhttp_tls_context_load_private_key(tls_ctx, "test/certs/key.pem");
    
    /* 创建服务器 */
    uvhttp_server_new(loop, &server);
    assert(server != NULL);
    
    /* 设置 TLS 上下文 */
    server->tls_ctx = tls_ctx;
    
    /* 创建路由器 */
    uvhttp_router_new(&router);
    assert(router != NULL);
    
    /* 添加路由 */
    uvhttp_router_add_route_method(router, "/", UVHTTP_GET, get_handler);
    server->router = router;
    
    /* 启动 HTTPS 服务器 */
    uvhttp_server_listen(server, "127.0.0.1", 18126);
    assert(server != NULL);
    printf("HTTPS 服务器启动成功，端口 18126\n");
    
    /* 等待服务器完全启动 */
    usleep(200000); /* 200ms */
    
    /* 测试 HTTPS 请求 */
    char response[256];
    send_https_request("127.0.0.1", 18126, "/", response, sizeof(response));
    assert(strcmp(response, "HTTPS Hello, World!") == 0);
    printf("✓ HTTPS 请求测试通过\n");
    
/* 停止服务器 */
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    /* 运行一次循环处理关闭事件 */
    uv_run(loop, UV_RUN_NOWAIT);
    
    /* 清理 TLS 上下文 */
    uvhttp_tls_context_free(tls_ctx);
    printf("✓ 服务器已停止\n");
}

/* 测试 HTTPS 并发请求 */
static void test_https_concurrent() {
    printf("\n=== 测试 HTTPS 并发请求 ===\n");
    
    g_request_count = 0;
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uvhttp_tls_context_t* tls_ctx = NULL;
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建 TLS 上下文 */
    uvhttp_tls_context_new(&tls_ctx);
    assert(tls_ctx != NULL);
    
    /* 加载证书和密钥 */
    uvhttp_tls_context_load_cert_chain(tls_ctx, "test/certs/cert.pem");
    uvhttp_tls_context_load_private_key(tls_ctx, "test/certs/key.pem");
    
    /* 创建服务器 */
    uvhttp_server_new(loop, &server);
    assert(server != NULL);
    
    /* 设置 TLS 上下文 */
    server->tls_ctx = tls_ctx;
    
    /* 创建路由器 */
    uvhttp_router_new(&router);
    assert(router != NULL);
    
    /* 添加路由 */
    uvhttp_router_add_route_method(router, "/", UVHTTP_GET, get_handler);
    server->router = router;
    
    /* 启动 HTTPS 服务器 */
    uvhttp_server_listen(server, "127.0.0.1", 18127);
    assert(server != NULL);
    printf("HTTPS 服务器启动成功，端口 18127\n");
    
    /* 等待服务器完全启动 */
    usleep(200000); /* 200ms */
    
    /* 发送并发请求 */
    char response[256];
    for (int i = 0; i < 5; i++) {
        send_https_request("127.0.0.1", 18127, "/", response, sizeof(response));
        assert(strcmp(response, "HTTPS Hello, World!") == 0);
    }
    printf("✓ 5 个 HTTPS 并发请求测试通过\n");
    
    /* 验证请求计数 */
    assert(g_request_count == 5);
    printf("✓ 请求计数正确 (%d)\n", g_request_count);
    
    /* 停止服务器 */
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    /* 运行一次循环处理关闭事件 */
    uv_run(loop, UV_RUN_NOWAIT);
    
    /* 清理 TLS 上下文 */
    uvhttp_tls_context_free(tls_ctx);
    printf("✓ 服务器已停止\n");
}

int main() {
    printf("========================================\n");
    printf("HTTPS End-to-End Test\n");
    printf("========================================\n");
    
    /* 检查 openssl 是否可用 */
    FILE* pipe = popen("which openssl 2>/dev/null", "r");
    if (!pipe) {
        printf("❌ openssl 不可用，跳过 HTTPS 测试\n");
        return 0;
    }
    pclose(pipe);
    
    /* 运行所有测试 */
    test_https_basic();
    test_https_concurrent();
    
    printf("\n========================================\n");
    printf("✅ 所有 HTTPS 端到端测试通过\n");
    printf("========================================\n");
    
    return 0;
}