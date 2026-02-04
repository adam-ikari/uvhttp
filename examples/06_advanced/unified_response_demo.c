/**
 * @file unified_response_demo.c
 * @brief UVHTTP 统一响应处理演示 - 展示如何使用统一的响应处理方式
 * 
 * 本示例演示：
 * 1. 使用 uvhttp_send_unified_response() 自动检测内容类型
 * 2. 统一处理 HTML 和 JSON 响应
 * 3. 便捷的响应发送函数
 * 4. 错误响应的统一处理
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_utils.h"
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 应用上下文结构
typedef struct {
    uvhttp_server_t* server;
} app_context_t;

static app_context_t* g_app_context = NULL;

// 处理 GET /api/info - 返回服务器信息（使用统一响应处理）
uvhttp_result_t info_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 创建 JSON 响应
    cJSON* info = cJSON_CreateObject();
    cJSON_AddStringToObject(info, "server", "UVHTTP");
    cJSON_AddStringToObject(info, "version", UVHTTP_VERSION_STRING);
    cJSON_AddStringToObject(info, "description", "高性能 HTTP 服务器库 - 统一响应处理演示");
    
    cJSON* features = cJSON_CreateArray();
    cJSON_AddItemToArray(features, cJSON_CreateString("统一响应处理"));
    cJSON_AddItemToArray(features, cJSON_CreateString("使用者控制 Content-Type"));
    cJSON_AddItemToArray(features, cJSON_CreateString("HTTP/1.1"));
    cJSON_AddItemToArray(features, cJSON_CreateString("WebSocket"));
    cJSON_AddItemToArray(features, cJSON_CreateString("TLS/SSL"));
    cJSON_AddItemToArray(features, cJSON_CreateString("LRU缓存"));
    cJSON_AddItemToObject(info, "features", features);
    
    char* json_string = cJSON_PrintUnformatted(info);
    cJSON_Delete(info);
    
    // 设置 Content-Type 为 JSON
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    
    // 使用统一响应处理 - 不自动检测，由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, json_string, strlen(json_string), 200);
    free(json_string);
    
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 主页处理器 - 返回主页（使用统一响应处理）
uvhttp_result_t home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html_content = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP 统一响应处理演示</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        ".endpoint { background: #f8f9fa; padding: 15px; margin: 15px 0; border-radius: 5px; border-left: 4px solid #007bff; }"
        ".method { color: #fff; padding: 3px 8px; border-radius: 3px; font-weight: bold; font-size: 12px; }"
        ".get { background: #28a745; }"
        ".post { background: #007bff; }"
        "pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; border: 1px solid #e9ecef; }"
        ".highlight { background: #e7f3ff; padding: 20px; border-radius: 5px; margin: 20px 0; }"
        ".new { background: #d4edda; padding: 20px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #28a745; }"
        "h1 { color: #007bff; }"
        "h2 { color: #495057; border-bottom: 2px solid #e9ecef; padding-bottom: 10px; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1> UVHTTP 统一响应处理演示</h1>"
        "<p>本演示展示如何使用统一的响应处理方式，由使用者控制 Content-Type 设置。</p>"
        
        "<div class='new'>"
        "<h3>🆕 统一响应处理</h3>"
        "<p>使用 <code>uvhttp_send_unified_response()</code> 简化响应发送，由使用者自行设置 Content-Type！</p>"
        "<pre><code>// 旧方式（多个步骤）\nuvhttp_response_set_status(res, 200);\nuvhttp_response_set_header(res, \"Content-Type\", \"application/json\");\nuvhttp_response_set_body(res, json_string, strlen(json_string));\nuvhttp_response_send(res);\n\n// 新方式（统一处理）\nuvhttp_response_set_header(res, \"Content-Type\", \"application/json\");\nuvhttp_send_unified_response(res, json_string, strlen(json_string), 200);</code></pre>"
        "</div>"
        "</div>"
        "</body>"
        "</html>";
    
    // 设置 Content-Type 为 HTML
    uvhttp_response_set_header(res, "Content-Type", "text/html");
    
    // 使用统一响应处理 - 不自动检测，由使用者设置 Content-Type
    return uvhttp_send_unified_response(res, html_content, strlen(html_content), 200) == UVHTTP_OK 
           ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// JSON 响应演示
uvhttp_result_t demo_json_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json_demo = 
        "{"
        "  \"message\": \"这是一个 JSON 响应演示\","
        "  \"timestamp\": %ld,"
        "  \"data\": {"
        "    \"type\": \"demo\","
        "    \"user_controlled\": true,"
        "    \"content_type\": \"application/json\""
        "  },"
        "  \"features\": [\"使用者控制\", \"统一处理\", \"类型安全\"]"
        "}";
    
    char json_buffer[512];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    snprintf(json_buffer, sizeof(json_buffer), json_demo, time(NULL));
#pragma GCC diagnostic pop
    
    // 设置 Content-Type 为 JSON
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    
    // 使用统一响应处理 - 由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, json_buffer, strlen(json_buffer), 200);
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// HTML 响应演示
uvhttp_result_t demo_html_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html_demo = 
        "<!DOCTYPE html>"
        "<html><head><title>HTML 演示</title></head>"
        "<body><h1>HTML 响应演示</h1>"
        "<p>这是一个简化的 HTML 响应，Content-Type 由使用者设置。</p>"
        "<p>Content-Type 被设置为: <code>text/html</code></p>"
        "</body></html>";
    
    // 设置 Content-Type 为 HTML
    uvhttp_response_set_header(res, "Content-Type", "text/html");
    
    // 使用统一响应处理 - 由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, html_demo, strlen(html_demo), 200);
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 文本响应演示
uvhttp_result_t demo_text_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* text_demo_template = 
        "这是一个纯文本响应演示。\n"
        "Content-Type 由使用者设置为 text/plain。\n"
        "适用于日志文件、配置文件等纯文本内容。\n"
        "时间戳: %ld";
    
    char text_demo[256];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    snprintf(text_demo, sizeof(text_demo), text_demo_template, time(NULL));
#pragma GCC diagnostic pop
    
    // 设置 Content-Type 为文本
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    
    // 使用统一响应处理 - 由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, text_demo, strlen(text_demo), 200);
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 错误响应演示
uvhttp_result_t demo_error_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 使用统一的错误响应函数
    uvhttp_error_t result = uvhttp_send_error_response(res, 400, "演示错误", "这是一个演示错误响应");
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 统一处理演示 - 根据请求内容返回不同类型的响应
uvhttp_result_t demo_unified_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    
    if (!body || strlen(body) == 0) {
        uvhttp_send_error_response(res, 400, "请求体为空", "请提供请求内容");
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 使用统一响应处理 - 由使用者控制 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, body, strlen(body), 200);
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 便捷函数演示
uvhttp_result_t demo_convenience_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* accept_header = uvhttp_request_get_header(req, "Accept");
    
    if (accept_header && strstr(accept_header, "application/json")) {
        // 使用 JSON 便捷函数
        const char* json_response = "{\"message\":\"使用核心API发送的JSON响应\",\"method\":\"uvhttp_response_set_body\"}";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, json_response, strlen(json_response));
        uvhttp_error_t result = uvhttp_response_send(res);
        return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
    }
    else if (accept_header && strstr(accept_header, "text/html")) {
        // 使用 HTML 便捷函数
        const char* html_response = "<html><body><h1>HTML 核心API演示</h1><p>使用 uvhttp_response_set_body 发送</p></body></html>";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
        uvhttp_response_set_body(res, html_response, strlen(html_response));
        uvhttp_error_t result = uvhttp_response_send(res);
        return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
    }
    else {
        // 使用文本便捷函数
        const char* text_response = "使用 uvhttp_response_set_body 发送的文本响应";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
        uvhttp_response_set_body(res, text_response, strlen(text_response));
        uvhttp_error_t result = uvhttp_response_send(res);
        return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
    }
}

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_app_context && g_app_context->server) {
        uvhttp_server_stop(g_app_context->server);
        uvhttp_server_free(g_app_context->server);
        g_app_context->server = NULL;
    }
    if (g_app_context) {
        free(g_app_context);
        g_app_context = NULL;
    }
    exit(0);
}

int main() {
    printf(" UVHTTP 统一响应处理演示\n");
    printf(" 演示如何使用统一的响应处理方式，自动检测内容类型\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建应用上下文
    g_app_context = (app_context_t*)malloc(sizeof(app_context_t));
    if (!g_app_context) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(g_app_context, 0, sizeof(app_context_t));
    
    // 创建服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_error_t server_result = uvhttp_server_new(loop, &g_app_context->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        free(g_app_context);
        return 1;
    }
    if (!g_app_context->server) {
        fprintf(stderr, " 服务器创建失败\n");
        free(g_app_context);
        return 1;
    }
    
    // 创建路由
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    
    // 注册路由处理器
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api/info", info_handler);
    uvhttp_router_add_route(router, "/api/demo/json", demo_json_handler);
    uvhttp_router_add_route(router, "/api/demo/html", demo_html_handler);
    uvhttp_router_add_route(router, "/api/demo/text", demo_text_handler);
    uvhttp_router_add_route(router, "/api/demo/error", demo_error_handler);
    uvhttp_router_add_route(router, "/api/demo/unified", demo_unified_handler);
    uvhttp_router_add_route(router, "/api/demo/convenience", demo_convenience_handler);
    
    g_app_context->server->router = router;
    
    // 启动服务器
    int listen_result = uvhttp_server_listen(g_app_context->server, "0.0.0.0", 8081);
    (void)listen_result;
    if (result != 0) {
        fprintf(stderr, " 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    
    printf(" 统一响应处理演示服务器启动成功\n");
    printf("🌐 服务器运行在 http://localhost:8081\n");
    printf("📖 访问主页查看完整演示文档\n");
    printf(" 尝试不同的 API 端点体验统一响应处理\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    if (g_app_context && g_app_context->server) {
        uvhttp_server_free(g_app_context->server);
    }
    if (g_app_context) {
        free(g_app_context);
    }
    
    return 0;
}
