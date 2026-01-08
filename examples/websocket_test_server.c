/*
 * WebSocket Test Server
 * 简单的WebSocket回显服务器用于测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "uvhttp.h"
#include "uvhttp_server.h"

static int websocket_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 检查是否为WebSocket升级请求
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    const char* connection = uvhttp_request_get_header(request, "Connection");
    
    if (upgrade && connection && 
        strcasecmp(upgrade, "websocket") == 0 &&
        strcasestr(connection, "upgrade") != NULL) {
        
        printf("收到WebSocket升级请求\n");
        
        // 返回101 Switching Protocols
        uvhttp_response_set_status(response, 101);
        uvhttp_response_set_header(response, "Upgrade", "websocket");
        uvhttp_response_set_header(response, "Connection", "Upgrade");
        
        // 获取Sec-WebSocket-Key
        const char* ws_key = uvhttp_request_get_header(request, "Sec-WebSocket-Key");
        if (ws_key) {
            printf("WebSocket Key: %s\n", ws_key);
            
            // 计算Sec-WebSocket-Accept
            // 这里简化处理，实际应该使用SHA-1和Base64
            char accept[128];
            snprintf(accept, sizeof(accept), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_key);
            uvhttp_response_set_header(response, "Sec-WebSocket-Accept", accept);
        }
        
        uvhttp_response_set_header(response, "Sec-WebSocket-Version", "13");
        
        return 0;
    }
    
    // 普通HTTP请求
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>WebSocket Test</title>"
        "</head>"
        "<body>"
        "<h1>WebSocket Test Server</h1>"
        "<p>Use WebSocket client to connect to ws://localhost:8080/ws</p>"
        "<script>"
        "var ws = new WebSocket('ws://localhost:8080/ws');"
        "ws.onopen = function() { console.log('Connected'); };"
        "ws.onmessage = function(e) { console.log('Received:', e.data); };"
        "ws.onclose = function() { console.log('Disconnected'); };"
        "</script>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_body(response, html, strlen(html));
    
    return 0;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("启动WebSocket测试服务器，端口: %d\n", port);
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器配置
    uvhttp_server_config_t config;
    uvhttp_server_config_init(&config);
    config.port = port;
    config.host = "0.0.0.0";
    config.worker_threads = 2;
    
    // 创建服务器
    uvhttp_server_t* server = uvhttp_server_create(&config);
    if (!server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }
    
    // 注册WebSocket处理器
    uvhttp_server_register_handler(server, "/ws", websocket_handler);
    uvhttp_server_register_handler(server, "/", websocket_handler);
    
    printf("服务器运行中...\n");
    printf("WebSocket URL: ws://localhost:%d/ws\n", port);
    printf("HTTP URL: http://localhost:%d/\n", port);
    printf("按Ctrl+C停止\n\n");
    
    // 运行服务器
    uvhttp_server_run(server);
    
    // 清理
    uvhttp_server_free(server);
    
    return 0;
}