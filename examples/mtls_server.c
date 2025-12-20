#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

void secure_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    printf("Secure connection established!\n");
    printf("Method: %s\n", uvhttp_request_get_method(request));
    printf("URL: %s\n", uvhttp_request_get_url(request));
    
    // 获取客户端证书信息（如果使用mTLS）
    // 注意：这里需要扩展请求结构来包含TLS信息
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    
    const char* body = 
        "<html>"
        "<head><title>mTLS Server</title></head>"
        "<body>"
        "<h1>Welcome to mTLS Protected Server!</h1>"
        "<p>This connection is secured with mutual TLS authentication.</p>"
        "<p>Your certificate has been verified.</p>"
        "</body>"
        "</html>";
    
    if (uvhttp_response_set_body(response, body, strlen(body)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
    }
    
    uvhttp_response_send(response);
}

int main() {
    printf("Starting mTLS enabled UVHTTP server...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建TLS上下文
    uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
    if (!tls_ctx) {
        fprintf(stderr, "Failed to create TLS context\n");
        return 1;
    }
    
    // 加载服务器证书和私钥
    if (uvhttp_tls_context_load_cert_chain(tls_ctx, "test/certs/server.crt") != 0) {
        fprintf(stderr, "Failed to load server certificate\n");
        return 1;
    }
    
    if (uvhttp_tls_context_load_private_key(tls_ctx, "test/certs/server.key") != 0) {
        fprintf(stderr, "Failed to load server private key\n");
        return 1;
    }
    
    // 加载CA证书用于验证客户端证书
    if (uvhttp_tls_context_load_ca_file(tls_ctx, "test/certs/ca.crt") != 0) {
        fprintf(stderr, "Failed to load CA certificate\n");
        return 1;
    }
    
    // 启用客户端证书验证（mTLS）
    if (uvhttp_tls_context_enable_client_auth(tls_ctx, 1) != 0) {
        fprintf(stderr, "Failed to enable client authentication\n");
        return 1;
    }
    
    // 设置验证深度
    uvhttp_tls_context_set_verify_depth(tls_ctx, 2);
    
    // 应用安全配置
    uvhttp_tls_context_set_cipher_suites(tls_ctx, NULL); // 使用默认安全密码套件
    uvhttp_tls_context_enable_session_tickets(tls_ctx, 0); // 禁用会话票据
    uvhttp_tls_context_set_session_cache(tls_ctx, 0); // 禁用会话缓存
    uvhttp_tls_context_set_dh_parameters(tls_ctx, NULL); // 使用默认DH参数
    
    // 启用服务器TLS
    if (uvhttp_server_enable_tls(server, tls_ctx) != 0) {
        fprintf(stderr, "Failed to enable TLS on server\n");
        return 1;
    }
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", secure_handler);
    
    server->router = router;
    
    // 启动服务器
    int ret = uvhttp_server_listen(server, "0.0.0.0", 8443);
    if (ret != 0) {
        fprintf(stderr, "Failed to start server: %s\n", uv_strerror(ret));
        return 1;
    }
    
    printf("mTLS server listening on https://0.0.0.0:8443\n");
    printf("Client certificate is required for connection.\n");
    printf("Try connecting with: curl --cert client.crt --key client.key --cacert ca.crt https://localhost:8443/\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    uvhttp_server_free(server);
    uv_loop_close(loop);
    free(loop);
    
    return 0;
}