#include "../include/uvhttp.h"
#include <stdio.h>

int tls_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "TLS Hello World!", 16);
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("TLS测试服务器启动中...\n");
    
    // 初始化TLS模块
    if (uvhttp_tls_init() != 0) {
        fprintf(stderr, "TLS初始化失败\n");
        return 1;
    }
    
    // 创建TLS上下文
    uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
    if (!tls_ctx) {
        fprintf(stderr, "创建TLS上下文失败\n");
        return 1;
    }
    
    // 配置证书（使用测试证书）
    if (uvhttp_tls_context_load_cert_chain(tls_ctx, "test/certs/server.crt") != 0) {
        printf("警告: 无法加载服务器证书，使用测试模式\n");
    }
    
    if (uvhttp_tls_context_load_private_key(tls_ctx, "test/certs/server.key") != 0) {
        printf("警告: 无法加载私钥，使用测试模式\n");
    }
    
    // 启用客户端认证
    uvhttp_tls_context_enable_client_auth(tls_ctx, 0);
    
    // 设置安全参数
    uvhttp_tls_context_set_verify_depth(tls_ctx, 3);
    uvhttp_tls_context_enable_session_tickets(tls_ctx, 0);
    uvhttp_tls_context_set_session_cache(tls_ctx, 100);
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 启用TLS
    if (uvhttp_server_enable_tls(server, tls_ctx) != 0) {
        printf("警告: TLS启用失败，使用HTTP模式\n");
    }
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", tls_handler);
    
    server->router = router;
    
    // 监听8443端口（HTTPS标准端口）
    if (uvhttp_server_listen(server, "0.0.0.0", 8443) != 0) {
        fprintf(stderr, "启动服务器失败\n");
        uvhttp_tls_context_free(tls_ctx);
        return 1;
    }
    
    printf("TLS测试服务器运行在 https://localhost:8443\n");
    printf("注意: 这是测试实现，实际TLS加密需要完整的mbedtls集成\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    uvhttp_tls_context_free(tls_ctx);
    uvhttp_tls_cleanup();
    
    return 0;
}