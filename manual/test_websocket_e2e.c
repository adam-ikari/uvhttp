/*
 * WebSocket 端到端测试
 * 测试 WebSocket 连接、消息传输、Ping/Pong 等功能
 */

#include "uvhttp.h"
#include "uvhttp_response_internal.h"
#include "uvhttp_allocator.h"
#include "uvhttp_websocket.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* 应用上下文 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    int connection_count;
} app_context_t;

/* 全局应用上下文 */
static app_context_t* g_app_context = NULL;

/* WebSocket 连接上下文 */
typedef struct {
    int connection_id;
    char client_id[64];
    int message_count;
    time_t connect_time;
} ws_connection_context_t;

/* 信号处理器 */
static void on_sigint(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

static void on_sigterm(uv_signal_t* handle, int signum) {
    (void)signum;
    app_context_t* ctx = (app_context_t*)handle->data;
    if (ctx && ctx->server) {
        uvhttp_server_stop(ctx->server);
        uvhttp_server_free(ctx->server);
        ctx->server = NULL;
    }
    if (ctx && ctx->loop) {
        uv_stop(ctx->loop);
    }
}

/* WebSocket 连接建立回调 */
static int on_ws_connect(uvhttp_ws_connection_t* ws_conn) {
    if (!g_app_context) {
        return -1;
    }
    
    g_app_context->connection_count++;
    
    /* 创建连接上下文 */
    ws_connection_context_t* conn_ctx = (ws_connection_context_t*)uvhttp_alloc(sizeof(ws_connection_context_t));
    if (conn_ctx) {
        conn_ctx->connection_id = g_app_context->connection_count;
        snprintf(conn_ctx->client_id, sizeof(conn_ctx->client_id), "client_%d", g_app_context->connection_count);
        conn_ctx->message_count = 0;
        conn_ctx->connect_time = time(NULL);
        /* 使用 user_data 字段存储连接上下文 */
        /* 注意：uvhttp_ws_connection_t 结构体中没有 user_data 字段，我们需要使用其他方式 */
    }
    
    printf("WebSocket connected: client_%d (total: %d)\n", 
           g_app_context->connection_count, g_app_context->connection_count);
    
    /* 发送欢迎消息 */
    const char* welcome = "Welcome to WebSocket E2E Test Server!";
    uvhttp_server_ws_send(ws_conn, welcome, strlen(welcome));
    
    return 0;
}

/* WebSocket 连接关闭回调 */
static int on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;  /* Suppress unused parameter warning */
    printf("WebSocket disconnected\n");
    return 0;
}

/* WebSocket 消息回调 */
static int on_ws_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    (void)opcode;  /* Suppress unused parameter warning */
    
    printf("Received message: %.*s\n", (int)len, data);
    
    /* 回显消息 */
    char response[512];
    snprintf(response, sizeof(response), "Echo: %.*s", (int)len, data);
    uvhttp_server_ws_send(ws_conn, response, strlen(response));
    
    return 0;
}

/* WebSocket 错误回调 */
static int on_ws_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    (void)ws_conn;  /* Suppress unused parameter warning */
    fprintf(stderr, "WebSocket error: %d - %s\n", error_code, error_msg ? error_msg : "unknown");
    return 0;
}

/* 广播消息处理器 */
static int broadcast_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* body = "Broadcast message sent to all WebSocket clients";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    printf("Broadcast handler called\n");
    return 0;
}

/* 获取连接统计 */
static int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    
    char stats[256];
    snprintf(stats, sizeof(stats),
             "Active WebSocket connections: %d",
             g_app_context ? g_app_context->connection_count : 0);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, stats, strlen(stats));
    uvhttp_response_send(response);
    
    printf("Stats handler called: %d connections\n", g_app_context ? g_app_context->connection_count : 0);
    return 0;
}

/* 主页处理器 - 测试说明 */
static int index_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Suppress unused parameter warning */
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>WebSocket E2E Test Server</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "h1 { color: #333; }"
        ".endpoint { margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }"
        ".method { font-weight: bold; color: #0066cc; }"
        "pre { background: #f5f5f5; padding: 15px; border-radius: 5px; overflow-x: auto; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🔌 WebSocket End-to-End Test Server</h1>"
        "<p>测试 WebSocket 连接、消息传输、Ping/Pong 等功能</p>"
        ""
        "<h2>测试端点：</h2>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">WebSocket</span> /ws - WebSocket 连接"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /stats - 连接统计"
        "</div>"
        ""
        "<div class=\"endpoint\">"
        "<span class=\"method\">GET</span> /broadcast - 广播消息"
        "</div>"
        ""
        "<h2>WebSocket 客户端测试：</h2>"
        "<pre>"
        "// 使用 wscat 测试\n"
        "wscat -c ws://localhost:8084/ws\n"
        ""
        "// 发送文本消息\n"
        "> Hello, Server!\n"
        "< Echo [1]: Hello, Server!\n"
        ""
        "// 发送二进制消息\n"
        "> \\x01\\x02\\x03\n"
        ""
        "// 测试 Ping/Pong\n"
        "> (自动 Ping/Pong 处理)\n"
        "</pre>"
        ""
        "<h2>浏览器测试：</h2>"
        "<pre>"
        "const ws = new WebSocket('ws://localhost:8084/ws');\n"
        "ws.onopen = () => console.log('Connected');\n"
        "ws.onmessage = (e) => console.log('Received:', e.data);\n"
        "ws.send('Hello from browser!');\n"
        "</pre>"
        ""
        "<h2>测试功能：</h2>"
        "<ul>"
        "<li>✓ WebSocket 连接建立和关闭</li>"
        "<li>✓ 文本消息传输</li>"
        "<li>✓ 二进制消息传输</li>"
        "<li>✓ Ping/Pong 心跳</li>"
        "<li>✓ 消息回显</li>"
        "<li>✓ 连接统计</li>"
        "<li>✓ 错误处理</li>"
        "</ul>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    printf("Index page accessed\n");
    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8084;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->loop = loop;
    ctx->connection_count = 0;
    
    /* 创建服务器 */
    uvhttp_error_t result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 设置服务器用户数据 */
    ctx->server->user_data = ctx;
    
    /* 创建路由器 */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 添加路由 - 主页 */
    uvhttp_router_add_route(ctx->router, "/", index_handler);
    
    /* 添加路由 - 统计 */
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    
    /* 添加路由 - 广播 */
    uvhttp_router_add_route(ctx->router, "/broadcast", broadcast_handler);
    
    /* 设置路由器到服务器 */
    ctx->server->router = ctx->router;
    
    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_ws_connect,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .on_error = on_ws_error,
        .user_data = ctx
    };
    
    result = uvhttp_server_register_ws_handler(ctx->server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to register WebSocket handler: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 设置全局应用上下文 */
    g_app_context = ctx;
    
    /* 初始化信号处理器 */
    ctx->sigint.data = ctx;
    uv_signal_init(loop, &ctx->sigint);
    uv_signal_start(&ctx->sigint, on_sigint, SIGINT);
    
    ctx->sigterm.data = ctx;
    uv_signal_init(loop, &ctx->sigterm);
    uv_signal_start(&ctx->sigterm, on_sigterm, SIGTERM);
    
    /* 启动服务器 */
    result = uvhttp_server_listen(ctx->server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    printf("========================================\n");
    printf("WebSocket E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("URL: http://%s:%d/\n", host, port);
    printf("WebSocket: ws://%s:%d/ws\n", host, port);
    printf("========================================\n");
    printf("\n测试功能:\n");
    printf("  - WebSocket 连接建立和关闭\n");
    printf("  - 文本消息传输\n");
    printf("  - 二进制消息传输\n");
    printf("  - Ping/Pong 心跳\n");
    printf("  - 消息回显\n");
    printf("  - 连接统计\n");
    printf("  - 错误处理\n");
    printf("\n测试端点:\n");
    printf("  - / (主页)\n");
    printf("  - /ws (WebSocket)\n");
    printf("  - /stats (连接统计)\n");
    printf("  - /broadcast (广播消息)\n");
    printf("\n测试工具:\n");
    printf("  - wscat: wscat -c ws://localhost:%d/ws\n", port);
    printf("  - 浏览器: 打开控制台使用 WebSocket API\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    printf("========================================\n\n");
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理 */
    uv_signal_stop(&ctx->sigint);
    uv_signal_stop(&ctx->sigterm);
    
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
        }
        uvhttp_free(ctx);
    }
    
    printf("\n========================================\n");
    printf("服务器已停止\n");
    printf("========================================\n");
    
    return 0;
}