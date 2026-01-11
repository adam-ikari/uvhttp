/**
 * @file json_api_demo.c
 * @brief UVHTTP JSON API 演示 - 展示如何使用 cJSON 处理 JSON 数据
 * 
 * 本示例演示：
 * 1. 使用 cJSON 创建 JSON 响应
 * 2. 解析请求中的 JSON 数据
 * 3. 错误处理和内存管理
 * 4. 复杂 JSON 结构处理
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_utils.h"
#include "../../deps/cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "uvhttp_allocator.h"

static uvhttp_server_t* g_server = NULL;

// 工具函数：创建标准 JSON 响应
static char* create_json_response(int status, const char* message, cJSON* data) {
    cJSON* response = cJSON_CreateObject();
    if (!response) return NULL;
    
    cJSON_AddNumberToObject(response, "status", status);
    cJSON_AddStringToObject(response, "message", message);
    cJSON_AddNumberToObject(response, "timestamp", time(NULL));
    
    if (data) {
        cJSON_AddItemToObject(response, "data", data);
    } else {
        cJSON_AddNullToObject(response, "data");
    }
    
    char* json_string = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    return json_string;
}

// 工具函数：创建错误响应
static char* create_error_response(const char* error, const char* details) {
    cJSON* error_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(error_obj, "error", error);
    if (details) {
        cJSON_AddStringToObject(error_obj, "details", details);
    }
    
    char* result = create_json_response(400, "请求失败", error_obj);
    cJSON_Delete(error_obj);
    return result;
}

// 处理 GET /api/info - 返回服务器信息（使用统一响应处理）
uvhttp_result_t info_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    cJSON* info = cJSON_CreateObject();
    cJSON_AddStringToObject(info, "server", "UVHTTP");
    cJSON_AddStringToObject(info, "version", "1.0.0");
    cJSON_AddStringToObject(info, "description", "高性能 HTTP 服务器库");
    cJSON_AddStringToObject(info, "response_type", "unified_processing");
    
    cJSON* features = cJSON_CreateArray();
    cJSON_AddItemToArray(features, cJSON_CreateString("HTTP/1.1"));
    cJSON_AddItemToArray(features, cJSON_CreateString("WebSocket"));
    cJSON_AddItemToArray(features, cJSON_CreateString("TLS/SSL"));
    cJSON_AddItemToArray(features, cJSON_CreateString("LRU缓存"));
    cJSON_AddItemToArray(features, cJSON_CreateString("统一响应处理"));
    cJSON_AddItemToObject(info, "features", features);
    
    char* json_string = create_json_response(200, "获取成功", info);
    cJSON_Delete(info);
    
    // 设置 Content-Type 为 JSON
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    
    // 使用统一响应处理 - 由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, json_string, strlen(json_string), 200);
    free(json_string);
    
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

// 处理 POST /api/users - 创建用户
uvhttp_result_t create_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    if (!body || strlen(body) == 0) {
        char* error_json = create_error_response("missing_body", "请求体为空");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 解析 JSON
    cJSON* user_data = cJSON_Parse(body);
    if (!user_data) {
        const char* error_ptr = cJSON_GetErrorPtr();
        char* error_json = create_error_response("invalid_json", error_ptr ? error_ptr : "JSON 格式错误");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 提取用户信息
    cJSON* name = cJSON_GetObjectItem(user_data, "name");
    cJSON* email = cJSON_GetObjectItem(user_data, "email");
    cJSON* age = cJSON_GetObjectItem(user_data, "age");
    
    // 验证必需字段
    if (!cJSON_IsString(name) || !cJSON_IsString(email)) {
        char* error_json = create_error_response("missing_fields", "缺少必需字段: name, email");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(user_data);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 创建响应数据
    cJSON* created_user = cJSON_CreateObject();
    cJSON_AddStringToObject(created_user, "id", "12345");
    cJSON_AddStringToObject(created_user, "name", cJSON_GetStringValue(name));
    cJSON_AddStringToObject(created_user, "email", cJSON_GetStringValue(email));
    if (cJSON_IsNumber(age)) {
        cJSON_AddNumberToObject(created_user, "age", cJSON_GetNumberValue(age));
    }
    cJSON_AddStringToObject(created_user, "created_at", "2025-01-01T00:00:00Z");
    
    char* json_string = create_json_response(201, "用户创建成功", created_user);
    cJSON_Delete(created_user);
    cJSON_Delete(user_data);
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return UVHTTP_OK;
}

// 处理 GET /api/users - 获取用户列表
uvhttp_result_t list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取查询参数
    const char* page_str = uvhttp_request_get_header(req, "X-Page");
    const char* limit_str = uvhttp_request_get_header(req, "X-Limit");
    
    int page = page_str ? atoi(page_str) : 1;
    int limit = limit_str ? atoi(limit_str) : 10;
    
    // 创建用户列表
    cJSON* users = cJSON_CreateArray();
    
    // 模拟用户数据
    for (int i = 0; i < limit; i++) {
        cJSON* user = cJSON_CreateObject();
        char id_str[20];
        char name_str[50];
        sprintf(id_str, "%d", (page - 1) * limit + i + 1);
        sprintf(name_str, "用户%d", (page - 1) * limit + i + 1);
        
        cJSON_AddStringToObject(user, "id", id_str);
        cJSON_AddStringToObject(user, "name", name_str);
        cJSON_AddStringToObject(user, "email", "user@example.com");
        cJSON_AddBoolToObject(user, "active", true);
        
        cJSON_AddItemToArray(users, user);
    }
    
    // 创建分页信息
    cJSON* pagination = cJSON_CreateObject();
    cJSON_AddNumberToObject(pagination, "page", page);
    cJSON_AddNumberToObject(pagination, "limit", limit);
    cJSON_AddNumberToObject(pagination, "total", 100);
    cJSON_AddNumberToObject(pagination, "pages", 10);
    
    // 创建响应数据
    cJSON* response_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response_data, "users", users);
    cJSON_AddItemToObject(response_data, "pagination", pagination);
    
    char* json_string = create_json_response(200, "获取成功", response_data);
    cJSON_Delete(response_data);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return UVHTTP_OK;
}

// 处理 GET /api/health - 健康检查
uvhttp_result_t health_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    cJSON* health = cJSON_CreateObject();
    cJSON_AddStringToObject(health, "status", "healthy");
    cJSON_AddNumberToObject(health, "uptime", 3600);
    cJSON_AddStringToObject(health, "version", "1.0.0");
    
    cJSON* checks = cJSON_CreateArray();
    
    cJSON* db_check = cJSON_CreateObject();
    cJSON_AddStringToObject(db_check, "name", "database");
    cJSON_AddStringToObject(db_check, "status", "ok");
    cJSON_AddItemToArray(checks, db_check);
    
    cJSON* cache_check = cJSON_CreateObject();
    cJSON_AddStringToObject(cache_check, "name", "cache");
    cJSON_AddStringToObject(cache_check, "status", "ok");
    cJSON_AddItemToArray(checks, cache_check);
    
    cJSON_AddItemToObject(health, "checks", checks);
    
    char* json_string = create_json_response(200, "服务正常", health);
    cJSON_Delete(health);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return UVHTTP_OK;
}

// 主页处理器 - 返回 HTML 说明页面（使用统一响应处理）
uvhttp_result_t home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP JSON API 演示 - 统一响应处理</title>"
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
        "<h1>🚀 UVHTTP JSON API 演示</h1>"
        "<p>本演示展示如何在 UVHTTP 中使用 cJSON 处理 JSON 数据，以及新的<strong>统一响应处理</strong>功能。</p>"
        
        "<div class='new'>"
        "<h3>🆕 统一响应处理</h3>"
        "<p>现在可以使用 <code>uvhttp_send_unified_response()</code> 自动检测内容类型并设置正确的 Content-Type！</p>"
        "<pre><code>// 旧方式（需要手动设置 Content-Type）\nuvhttp_response_set_status(res, 200);\nuvhttp_response_set_header(res, \"Content-Type\", \"application/json\");\nuvhttp_response_set_body(res, json_string, strlen(json_string));\nuvhttp_response_send(res);\n\n// 新方式（自动检测内容类型）\nuvhttp_send_unified_response(res, json_string, strlen(json_string), 200);</code></pre>"
        "</div>"
        
        "<h2>📋 API 端点</h2>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/info</strong> - 获取服务器信息（已升级为统一处理）"
        "<pre>curl http://localhost:8080/api/info</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method post'>POST</span> <strong>/api/users</strong> - 创建用户"
        "<pre>curl -X POST http://localhost:8080/api/users \\"
"     -H 'Content-Type: application/json' \\"
"     -d '{\"name\":\"张三\",\"email\":\"zhangsan@example.com\",\"age\":25}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/users</strong> - 获取用户列表"
        "<pre>curl -H 'X-Page: 1' -H 'X-Limit: 5' http://localhost:8080/api/users</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/health</strong> - 健康检查"
        "<pre>curl http://localhost:8080/api/health</pre>"
        "</div>"
        
        "<h2>🛠️ 技术特点</h2>"
        "<ul>"
        "<li>✅ 使用 cJSON 轻量级 JSON 库</li>"
        "<li>✅ <strong>统一响应处理</strong> - 自动检测内容类型</li>"
        "<li>✅ 完整的错误处理</li>"
        "<li>✅ 内存安全管理</li>"
        "<li>✅ 复杂数据结构支持</li>"
        "<li>✅ 分页和查询参数</li>"
        "</ul>"
        
        "<h2>📚 统一响应处理 API</h2>"
        "<pre><code>// 自动检测内容类型（推荐）\nuvhttp_send_unified_response(res, content, length, status_code);\n\n// 便捷函数\nuvhttp_send_json_response(res, json_string, 200);\nuvhttp_send_html_response(res, html_string, 200);\nuvhttp_send_text_response(res, text_string, 200);\nuvhttp_send_error_response(res, 400, \"Error\", \"Details\");</code></pre>"
        
        "<h2>📚 cJSON 使用要点</h2>"
        "<ul>"
        "<li>始终检查 cJSON_Parse() 的返回值</li>"
        "<li>使用 cJSON_Delete() 释放内存</li>"
        "<li>使用 cJSON_Is*() 函数验证类型</li>"
        "<li>使用 cJSON_PrintUnformatted() 提高性能</li>"
        "</ul>"
        
        "</div>"
        "</body>"
        "</html>";
    
    // 设置 Content-Type 为 HTML
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    
    // 使用统一响应处理 - 由使用者设置 Content-Type
    uvhttp_error_t result = uvhttp_send_unified_response(res, html, strlen(html), 200);
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

int main() {
    printf("🚀 UVHTTP JSON API 演示\n");
    printf("📝 演示 cJSON 集成和 JSON 处理最佳实践\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建服务器
    uv_loop_t* loop = uv_default_loop();
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        return 1;
    }
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 注册路由处理器
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api/info", info_handler);
    uvhttp_router_add_route(router, "/api/users", create_user_handler);
    uvhttp_router_add_route(router, "/api/health", health_handler);
    
    // 为不同方法设置相同的路径
    uvhttp_router_add_route(router, "/api/users", list_users_handler);
    
    g_server->router = router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        fprintf(stderr, "❌ 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ JSON API 服务器启动成功\n");
    printf("🌐 服务器运行在 http://localhost:8080\n");
    printf("📖 访问主页查看完整 API 文档\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    if (g_server) {
        uvhttp_server_free(g_server);
    }
    
    return 0;
}