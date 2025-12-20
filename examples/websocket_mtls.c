#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

/* WebSocket消息处理器 */
void websocket_handler(uvhttp_websocket_t* ws, 
                      const uvhttp_websocket_message_t* msg, 
                      void* user_data) {
    printf("收到WebSocket消息: %.*s\n", (int)msg->length, msg->data);
    
    /* 获取客户端证书信息 */
    const char* peer_cert = uvhttp_websocket_get_peer_cert(ws);
    if (peer_cert) {
        printf("客户端证书CN: %s\n", peer_cert);
    }
    
    /* 验证客户端证书 */
    uvhttp_websocket_error_t verify_result = uvhttp_websocket_verify_peer_cert(ws);
    if (verify_result == UVHTTP_WEBSOCKET_ERROR_NONE) {
        printf("客户端证书验证成功\n");
        
        /* 回复消息 */
        const char* reply = "Hello from secure WebSocket server!";
        uvhttp_websocket_send_text(ws, reply);
    } else {
        printf("客户端证书验证失败，错误码: %d\n", verify_result);
        uvhttp_websocket_close(ws, 1008, "Certificate verification failed");
    }
}

/* HTTP请求处理器 - 升级到WebSocket */
void http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* url = uvhttp_request_get_url(request);
    const char* method = uvhttp_request_get_method(request);
    
    printf("收到请求: %s %s\n", method, url);
    
    /* 检查WebSocket升级请求 */
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    if (upgrade && strcmp(upgrade, "websocket") == 0) {
        /* 配置mTLS */
        uvhttp_websocket_mtls_config_t mtls_config = {
            .server_cert_path = "test/certs/server.crt",
                .server_key_path = "test/certs/server.key",
                .ca_cert_path = "test/certs/ca.crt",            .require_client_cert = 1,
            .verify_depth = 3
        };
        
        /* WebSocket选项 */
        uvhttp_websocket_options_t ws_options = {
            .mtls_config = &mtls_config,
            .enable_tls = 1,
            .max_frame_size = UVHTTP_WEBSOCKET_MAX_FRAME_SIZE,
            .ping_interval = 30,
            .enable_compression = 0
        };
        
        /* 创建带mTLS的WebSocket连接 */
        uvhttp_websocket_t* ws = uvhttp_websocket_new_with_options(request, response, &ws_options);
        if (ws) {
            /* 设置消息处理器 */
            uvhttp_websocket_error_t result = uvhttp_websocket_set_handler(ws, websocket_handler, NULL);
            if (result == UVHTTP_WEBSOCKET_ERROR_NONE) {
                printf("WebSocket连接已建立，启用mTLS\n");
            } else {
                printf("设置WebSocket处理器失败，错误码: %d\n", result);
            }
            return;
        }
    }
    
    /* 普通HTTP响应 */
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    
    const char* body = 
        "<html>"
        "<head><title>mTLS WebSocket Server</title></head>"
        "<body>"
        "<h1>mTLS WebSocket Server</h1>"
        "<p>Connect to this server using WebSocket with client certificate authentication.</p>"
        "<p>Example: ws://localhost:8080/ws</p>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
}

int main() {
    printf("启动带mTLS的WebSocket服务器...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    /* 创建路由 */
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", http_handler);
    uvhttp_router_add_route(router, "/ws", http_handler);
    
    server->router = router;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(server, "0.0.0.0", 8080) == 0) {
        printf("服务器运行在 http://localhost:8080\n");
        printf("WebSocket端点: ws://localhost:8080/ws\n");
        printf("mTLS已启用，需要客户端证书\n");
        
        uv_run(loop, UV_RUN_DEFAULT);
    } else {
        fprintf(stderr, "启动服务器失败\n");
        return 1;
    }
    
    /* 清理资源 */
    uvhttp_websocket_cleanup_global();
    uvhttp_server_free(server);
    uvhttp_router_free(router);
    
    return 0;
}