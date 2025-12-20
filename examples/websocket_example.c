/* WebSocket 简单示例 */
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WebSocket 消息处理器 */
static void websocket_message_handler(uvhttp_websocket_t* ws, 
                                     const uvhttp_websocket_message_t* msg, 
                                     void* user_data) {
    printf("收到 WebSocket 消息: 类型=%d, 长度=%zu\n", msg->type, msg->length);
    if (msg->type == UVHTTP_WEBSOCKET_TEXT) {
        printf("文本内容: %.*s\n", (int)msg->length, msg->data);
    }
    
    /* 回复消息 */
    const char* reply = "收到消息!";
    uvhttp_websocket_send_text(ws, reply);
}

int main() {
    printf("WebSocket 示例程序\n");
    
    /* 创建模拟的请求和响应 */
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
    
    mock_request_t request = {
        .method = "GET",
        .url = "/ws",
        .headers = "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    };
    mock_response_t response = {0};
    
    /* 创建 WebSocket 连接 */
    printf("创建 WebSocket 连接...\n");
    uvhttp_websocket_t* ws = uvhttp_websocket_new(
        (uvhttp_request_t*)&request, 
        (uvhttp_response_t*)&response
    );
    
    if (!ws) {
        printf("WebSocket 创建失败\n");
        return 1;
    }
    
    printf("WebSocket 创建成功\n");
    
    /* 设置消息处理器 */
    uvhttp_websocket_set_handler(ws, websocket_message_handler, NULL);
    
    /* 配置 mTLS（可选） */
    uvhttp_websocket_mtls_config_t mtls_config = {
        .server_cert_path = "certs/server.crt",
        .server_key_path = "certs/server.key",
        .ca_cert_path = "certs/ca.crt",
        .require_client_cert = 0,
        .verify_depth = 3
    };
    
    if (uvhttp_websocket_enable_mtls(ws, &mtls_config) == UVHTTP_WEBSOCKET_ERROR_NONE) {
        printf("mTLS 配置成功\n");
    } else {
        printf("mTLS 配置失败（可能是证书文件不存在）\n");
    }
    
    /* 发送测试消息 */
    const char* test_message = "Hello WebSocket!";
    printf("发送测试消息: %s\n", test_message);
    
    uvhttp_websocket_error_t result = uvhttp_websocket_send_text(ws, test_message);
    if (result == UVHTTP_WEBSOCKET_ERROR_NONE) {
        printf("消息发送成功\n");
    } else {
        printf("消息发送失败，错误码: %d\n", result);
    }
    
    /* 验证证书 */
    printf("验证对端证书...\n");
    result = uvhttp_websocket_verify_peer_cert(ws);
    if (result == UVHTTP_WEBSOCKET_ERROR_NONE) {
        printf("证书验证成功\n");
    } else {
        printf("证书验证失败，错误码: %d\n", result);
    }
    
    /* 关闭 WebSocket 连接 */
    printf("关闭 WebSocket 连接...\n");
    uvhttp_websocket_close(ws, 1000, "正常关闭");
    
    /* 释放资源 */
    uvhttp_websocket_free(ws);
    
    /* 清理全局资源 */
    uvhttp_websocket_cleanup_global();
    
    printf("WebSocket 示例程序结束\n");
    return 0;
}