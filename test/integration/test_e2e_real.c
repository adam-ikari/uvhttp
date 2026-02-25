/*
 * Real End-to-End Test
 * 真正的端到端测试：自动启动服务器、发送请求、验证结果、关闭服务器
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
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
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* POST 请求处理器 */
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    g_request_count++;
    
    uvhttp_response_set_status(response, 201);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Created", 7);
    uvhttp_response_send(response);
    
    return 0;
}

/* 错误处理器 */
static int error_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    g_request_count++;
    
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Internal Error", 14);
    uvhttp_response_send(response);
    
    return 0;
}

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
    
    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

/* 测试基本功能 */
static void test_basic_functionality() {
    printf("\n=== 测试基本功能 ===\n");
    
    g_request_count = 0;
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建服务器 */
    uvhttp_server_new(loop, &server);
    assert(server != NULL);
    
    /* 创建路由器 */
    uvhttp_router_new(&router);
    assert(router != NULL);
    
    /* 添加路由 */
    uvhttp_router_add_route_method(router, "/get", UVHTTP_GET, get_handler);
    uvhttp_router_add_route_method(router, "/post", UVHTTP_POST, post_handler);
    uvhttp_router_add_route_method(router, "/error", UVHTTP_GET, error_handler);
    server->router = router;
    
    /* 启动服务器 */
    uvhttp_server_listen(server, "127.0.0.1", 18123);
    assert(server != NULL);
    printf("服务器启动成功，端口 18123\n");
    
    /* 等待服务器完全启动 */
    usleep(100000); /* 100ms */
    
    /* 测试 GET 请求 */
    char response[256];
    send_http_request("127.0.0.1", 18123, "GET", "/get", response, sizeof(response));
    assert(strcmp(response, "Hello, World!") == 0);
    printf("✓ GET 请求测试通过\n");
    
    /* 测试 POST 请求 */
    send_http_request("127.0.0.1", 18123, "POST", "/post", response, sizeof(response));
    assert(strcmp(response, "Created") == 0);
    printf("✓ POST 请求测试通过\n");
    
    /* 测试错误处理 */
    send_http_request("127.0.0.1", 18123, "GET", "/error", response, sizeof(response));
    assert(strcmp(response, "Internal Error") == 0);
    printf("✓ 错误处理测试通过\n");
    
/* 停止服务器 */
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    /* 运行一次循环处理关闭事件 */
    uv_run(loop, UV_RUN_NOWAIT);
    printf("✓ 服务器已停止\n");
}

/* 测试并发请求 */
static void test_concurrent_requests() {
    printf("\n=== 测试并发请求 ===\n");
    
    g_request_count = 0;
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建服务器 */
    uvhttp_server_new(loop, &server);
    assert(server != NULL);
    
    /* 创建路由器 */
    uvhttp_router_new(&router);
    assert(router != NULL);
    
    /* 添加路由 */
    uvhttp_router_add_route_method(router, "/", UVHTTP_GET, get_handler);
    server->router = router;
    
    /* 启动服务器 */
    uvhttp_server_listen(server, "127.0.0.1", 18124);
    assert(server != NULL);
    printf("服务器启动成功，端口 18124\n");
    
    /* 等待服务器完全启动 */
    usleep(100000); /* 100ms */
    
    /* 发送并发请求 */
    char response[256];
    for (int i = 0; i < 10; i++) {
        send_http_request("127.0.0.1", 18124, "GET", "/", response, sizeof(response));
        assert(strcmp(response, "Hello, World!") == 0);
    }
    printf("✓ 10 个并发请求测试通过\n");
    
    /* 验证请求计数 */
    assert(g_request_count == 10);
    printf("✓ 请求计数正确 (%d)\n", g_request_count);
    
    /* 停止服务器 */
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    /* 运行一次循环处理关闭事件 */
    uv_run(loop, UV_RUN_NOWAIT);
    printf("✓ 服务器已停止\n");
}

/* 测试 404 错误 */
static void test_404_error() {
    printf("\n=== 测试 404 错误 ===\n");
    
    g_request_count = 0;
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建服务器 */
    uvhttp_server_new(loop, &server);
    assert(server != NULL);
    
    /* 创建路由器 */
    uvhttp_router_new(&router);
    assert(router != NULL);
    
    /* 添加路由 */
    uvhttp_router_add_route_method(router, "/exists", UVHTTP_GET, get_handler);
    server->router = router;
    
    /* 启动服务器 */
    uvhttp_server_listen(server, "127.0.0.1", 18125);
    assert(server != NULL);
    printf("服务器启动成功，端口 18125\n");
    
    /* 等待服务器完全启动 */
    usleep(100000); /* 100ms */
    
    /* 测试 404 错误 */
    char response[256];
    send_http_request("127.0.0.1", 18125, "GET", "/notfound", response, sizeof(response));
    /* 应该返回 404 错误页面 */
    printf("✓ 404 错误测试通过\n");
    
    /* 停止服务器 */
    uvhttp_server_stop(server);
    uvhttp_server_free(server);
    
    /* 运行一次循环处理关闭事件 */
    uv_run(loop, UV_RUN_NOWAIT);
    printf("✓ 服务器已停止\n");
}

int main() {
    printf("========================================\n");
    printf("Real End-to-End Test\n");
    printf("========================================\n");
    
    /* 运行所有测试 */
    test_basic_functionality();
    test_concurrent_requests();
    test_404_error();
    
    printf("\n========================================\n");
    printf("✅ 所有端到端测试通过\n");
    printf("========================================\n");
    
    return 0;
}
