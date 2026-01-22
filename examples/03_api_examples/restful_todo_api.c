/**
 * @file restful_todo_api.c
 * @brief UVHTTP RESTful Todo API 演示
 * 
 * 本示例演示简洁的 RESTful API 实现：
 * - GET /api/todos - 获取待办事项列表
 * - GET /api/todos/:id - 获取单个待办事项
 * - POST /api/todos - 创建待办事项
 * - PUT /api/todos/:id - 更新待办事项
 * - DELETE /api/todos/:id - 删除待办事项
 * - PATCH /api/todos/:id/complete - 标记完成
 * 
 * 特性：
 * - 简洁的代码结构
 * - 自动 ID 生成
 * - 状态管理
 * - 优先级支持
 */

#include "../include/uvhttp.h"
#include "../../deps/cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static uvhttp_server_t* g_server = NULL;

// 待办事项数据结构
typedef struct {
    int id;
    char title[256];
    char description[1024];
    int completed;  // 0 = 未完成, 1 = 已完成
    int priority;   // 1 = 低, 2 = 中, 3 = 高
    char created_at[64];
    char completed_at[64];
} todo_item_t;

// 模拟数据库
static todo_item_t g_todos[100];
static int g_todo_count = 0;
static int g_next_id = 1;

// 初始化示例数据
static void init_sample_data() {
    const char* titles[] = {
        "学习 UVHTTP",
        "完成 RESTful API 示例",
        "编写单元测试",
        "优化性能",
        "更新文档"
    };
    
    const char* descriptions[] = {
        "学习 UVHTTP 的核心 API 和最佳实践",
        "创建完整的 RESTful API 示例代码",
        "为所有功能编写单元测试",
        "优化代码性能和内存使用",
        "更新项目文档和 README"
    };
    
    int priorities[] = {3, 3, 2, 2, 1};
    
    for (int i = 0; i < 5; i++) {
        g_todos[i].id = g_next_id++;
        strncpy(g_todos[i].title, titles[i], sizeof(g_todos[i].title) - 1);
        strncpy(g_todos[i].description, descriptions[i], sizeof(g_todos[i].description) - 1);
        g_todos[i].completed = (i == 2) ? UVHTTP_TRUE : UVHTTP_FALSE;  // 第3个已完成
        g_todos[i].priority = priorities[i];
        snprintf(g_todos[i].created_at, sizeof(g_todos[i].created_at),
                 "2025-01-%02dT09:00:00Z", i + 1);
        if (g_todos[i].completed) {
            snprintf(g_todos[i].completed_at, sizeof(g_todos[i].completed_at),
                     "2025-01-%02dT15:30:00Z", i + 1);
        } else {
            strcpy(g_todos[i].completed_at, "");
        }
    }
    
    g_todo_count = 5;
}

// 工具函数：创建 JSON 响应
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
static char* create_error_response(int status, const char* error, const char* details) {
    cJSON* error_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(error_obj, "error", error);
    if (details) {
        cJSON_AddStringToObject(error_obj, "details", details);
    }
    
    char* result = create_json_response(status, "请求失败", error_obj);
    cJSON_Delete(error_obj);
    return result;
}

// 工具函数：将待办事项转换为 JSON
static cJSON* todo_to_json(const todo_item_t* todo) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "id", todo->id);
    cJSON_AddStringToObject(json, "title", todo->title);
    cJSON_AddStringToObject(json, "description", todo->description);
    cJSON_AddBoolToObject(json, "completed", todo->completed);
    cJSON_AddNumberToObject(json, "priority", todo->priority);
    cJSON_AddStringToObject(json, "created_at", todo->created_at);
    if (todo->completed && strlen(todo->completed_at) > 0) {
        cJSON_AddStringToObject(json, "completed_at", todo->completed_at);
    } else {
        cJSON_AddNullToObject(json, "completed_at");
    }
    return json;
}

// 工具函数：从路径中提取 ID
static int extract_id_from_path(const char* path) {
    const char* id_str = strrchr(path, '/');
    if (!id_str || strlen(id_str) < 2) {
        return -1;
    }
    
    // 检查是否是 "complete" 路径
    if (strcmp(id_str + 1, "complete") == 0) {
        return -2;  // 特殊标记
    }
    
    // 验证是否为数字
    for (const char* p = id_str + 1; *p; p++) {
        if (!isdigit(*p)) {
            return -1;
        }
    }
    
    return atoi(id_str + 1);
}

// GET /api/todos - 获取待办事项列表
int get_todos_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取查询参数
    const char* completed_str = uvhttp_request_get_query_param(req, "completed");
    const char* priority_str = uvhttp_request_get_query_param(req, "priority");
    
    int filter_completed = -1;  // -1 = 不过滤, 0 = 未完成, 1 = 已完成
    int filter_priority = -1;   // -1 = 不过滤, 1-3 = 优先级
    
    if (completed_str) {
        if (strcmp(completed_str, "true") == 0 || strcmp(completed_str, "1") == 0) {
            filter_completed = 1;
        } else if (strcmp(completed_str, "false") == 0 || strcmp(completed_str, "0") == 0) {
            filter_completed = 0;
        }
    }
    
    if (priority_str) {
        filter_priority = atoi(priority_str);
        if (filter_priority < 1 || filter_priority > 3) {
            filter_priority = -1;
        }
    }
    
    // 创建待办事项列表
    cJSON* todos = cJSON_CreateArray();
    int completed_count = 0;
    int pending_count = 0;
    
    for (int i = 0; i < g_todo_count; i++) {
        // 过滤
        if (filter_completed != -1 && g_todos[i].completed != filter_completed) {
            continue;
        }
        if (filter_priority != -1 && g_todos[i].priority != filter_priority) {
            continue;
        }
        
        cJSON* todo_json = todo_to_json(&g_todos[i]);
        cJSON_AddItemToArray(todos, todo_json);
        
        if (g_todos[i].completed) {
            completed_count++;
        } else {
            pending_count++;
        }
    }
    
    // 创建统计信息
    cJSON* stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats, "total", g_todo_count);
    cJSON_AddNumberToObject(stats, "completed", completed_count);
    cJSON_AddNumberToObject(stats, "pending", pending_count);
    
    // 创建响应数据
    cJSON* response_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response_data, "todos", todos);
    cJSON_AddItemToObject(response_data, "stats", stats);
    
    char* json_string = create_json_response(200, "获取成功", response_data);
    cJSON_Delete(response_data);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// GET /api/todos/:id - 获取单个待办事项
int get_todo_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的待办事项 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找待办事项
    todo_item_t* todo = NULL;
    for (int i = 0; i < g_todo_count; i++) {
        if (g_todos[i].id == id) {
            todo = &g_todos[i];
            break;
        }
    }
    
    if (!todo) {
        char* error_json = create_error_response(404, "not_found", "待办事项不存在");
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 创建响应
    cJSON* todo_json = todo_to_json(todo);
    char* json_string = create_json_response(200, "获取成功", todo_json);
    cJSON_Delete(todo_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// POST /api/todos - 创建待办事项
int create_todo_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    if (!body || strlen(body) == 0) {
        char* error_json = create_error_response(400, "missing_body", "请求体为空");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 解析 JSON
    cJSON* json = cJSON_Parse(body);
    if (!json) {
        const char* error_ptr = cJSON_GetErrorPtr();
        char* error_json = create_error_response(400, "invalid_json", error_ptr ? error_ptr : "JSON 格式错误");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 提取字段
    cJSON* title = cJSON_GetObjectItem(json, "title");
    cJSON* description = cJSON_GetObjectItem(json, "description");
    cJSON* priority = cJSON_GetObjectItem(json, "priority");
    
    // 验证必需字段
    if (!cJSON_IsString(title) || strlen(cJSON_GetStringValue(title)) == 0) {
        char* error_json = create_error_response(400, "missing_title", "缺少必需字段: title");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    // 检查容量
    if (g_todo_count >= 100) {
        char* error_json = create_error_response(500, "capacity_limit", "待办事项数量已达上限");
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    // 创建新待办事项
    todo_item_t new_todo;
    memset(&new_todo, 0, sizeof(new_todo));
    
    new_todo.id = g_next_id++;
    strncpy(new_todo.title, cJSON_GetStringValue(title), sizeof(new_todo.title) - 1);
    
    if (cJSON_IsString(description)) {
        strncpy(new_todo.description, cJSON_GetStringValue(description), sizeof(new_todo.description) - 1);
    } else {
        strcpy(new_todo.description, "");
    }
    
    new_todo.completed = UVHTTP_FALSE;
    
    if (cJSON_IsNumber(priority)) {
        int prio = cJSON_GetNumberValue(priority);
        if (prio >= 1 && prio <= 3) {
            new_todo.priority = prio;
        } else {
            new_todo.priority = 2;  // 默认中等优先级
        }
    } else {
        new_todo.priority = 2;
    }
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(new_todo.created_at, sizeof(new_todo.created_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
    // 保存待办事项
    g_todos[g_todo_count] = new_todo;
    g_todo_count++;
    
    cJSON_Delete(json);
    
    // 创建响应
    cJSON* todo_json = todo_to_json(&new_todo);
    char* json_string = create_json_response(201, "创建成功", todo_json);
    cJSON_Delete(todo_json);
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_header(res, "Location", "/api/todos");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// PUT /api/todos/:id - 更新待办事项
int update_todo_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的待办事项 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找待办事项
    todo_item_t* todo = NULL;
    for (int i = 0; i < g_todo_count; i++) {
        if (g_todos[i].id == id) {
            todo = &g_todos[i];
            break;
        }
    }
    
    if (!todo) {
        char* error_json = create_error_response(404, "not_found", "待办事项不存在");
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 解析请求体
    const char* body = uvhttp_request_get_body(req);
    if (!body || strlen(body) == 0) {
        char* error_json = create_error_response(400, "missing_body", "请求体为空");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    cJSON* json = cJSON_Parse(body);
    if (!json) {
        const char* error_ptr = cJSON_GetErrorPtr();
        char* error_json = create_error_response(400, "invalid_json", error_ptr ? error_ptr : "JSON 格式错误");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 更新字段
    cJSON* title = cJSON_GetObjectItem(json, "title");
    cJSON* description = cJSON_GetObjectItem(json, "description");
    cJSON* completed = cJSON_GetObjectItem(json, "completed");
    cJSON* priority = cJSON_GetObjectItem(json, "priority");
    
    if (cJSON_IsString(title) && strlen(cJSON_GetStringValue(title)) > 0) {
        strncpy(todo->title, cJSON_GetStringValue(title), sizeof(todo->title) - 1);
    }
    
    if (cJSON_IsString(description)) {
        strncpy(todo->description, cJSON_GetStringValue(description), sizeof(todo->description) - 1);
    }
    
    if (cJSON_IsBool(completed)) {
        int old_completed = todo->completed;
        todo->completed = cJSON_IsTrue(completed);
        
        // 如果从未完成变为完成，设置完成时间
        if (!old_completed && todo->completed) {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            strftime(todo->completed_at, sizeof(todo->completed_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        }
        // 如果从完成变为未完成，清除完成时间
        else if (old_completed && !todo->completed) {
            strcpy(todo->completed_at, "");
        }
    }
    
    if (cJSON_IsNumber(priority)) {
        int prio = cJSON_GetNumberValue(priority);
        if (prio >= 1 && prio <= 3) {
            todo->priority = prio;
        }
    }
    
    cJSON_Delete(json);
    
    // 创建响应
    cJSON* todo_json = todo_to_json(todo);
    char* json_string = create_json_response(200, "更新成功", todo_json);
    cJSON_Delete(todo_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// DELETE /api/todos/:id - 删除待办事项
int delete_todo_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的待办事项 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找并删除待办事项
    int found = 0;
    for (int i = 0; i < g_todo_count; i++) {
        if (g_todos[i].id == id) {
            // 移动数组元素
            for (int j = i; j < g_todo_count - 1; j++) {
                g_todos[j] = g_todos[j + 1];
            }
            g_todo_count--;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        char* error_json = create_error_response(404, "not_found", "待办事项不存在");
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 创建响应
    char* json_string = create_json_response(200, "删除成功", NULL);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// 主页处理器
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 避免未使用参数警告
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP RESTful Todo API</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }"
        ".container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
        ".endpoint { background: #f8f9fa; padding: 15px; margin: 15px 0; border-radius: 5px; border-left: 4px solid #007bff; }"
        ".method { color: #fff; padding: 3px 8px; border-radius: 3px; font-weight: bold; font-size: 12px; margin-right: 8px; }"
        ".get { background: #28a745; }"
        ".post { background: #007bff; }"
        ".put { background: #ffc107; color: #000; }"
        ".delete { background: #dc3545; }"
        ".patch { background: #6f42c1; }"
        "pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; border: 1px solid #e9ecef; }"
        "h1 { color: #007bff; }"
        "h2 { color: #495057; border-bottom: 2px solid #e9ecef; padding-bottom: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin: 20px 0; }"
        "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #dee2e6; }"
        "th { background: #f8f9fa; font-weight: bold; }"
        ".status { padding: 3px 8px; border-radius: 3px; font-size: 12px; font-weight: bold; }"
        ".completed { background: #28a745; color: white; }"
        ".pending { background: #ffc107; color: black; }"
        ".priority { padding: 3px 8px; border-radius: 3px; font-size: 12px; font-weight: bold; }"
        ".high { background: #dc3545; color: white; }"
        ".medium { background: #ffc107; color: black; }"
        ".low { background: #28a745; color: white; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>✅ UVHTTP RESTful Todo API</h1>"
        "<p>简洁的待办事项管理 API 示例，展示完整的 CRUD 操作。</p>"
        
        "<h2>📋 API 端点</h2>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/todos</strong> - 获取待办事项列表"
        "<p>查询参数: completed (true/false), priority (1/2/3)</p>"
        "<pre>curl 'http://localhost:8080/api/todos?completed=false&priority=3'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/todos/:id</strong> - 获取单个待办事项"
        "<pre>curl http://localhost:8080/api/todos/1</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method post'>POST</span> <strong>/api/todos</strong> - 创建待办事项"
        "<pre>curl -X POST http://localhost:8080/api/todos \\"
"     -H 'Content-Type: application/json' \\"
"     -d '{\"title\":\"新任务\",\"description\":\"任务描述\",\"priority\":2}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method put'>PUT</span> <strong>/api/todos/:id</strong> - 更新待办事项"
        "<pre>curl -X PUT http://localhost:8080/api/todos/1 \\"
"     -H 'Content-Type: application/json' \\"
"     -d '{\"title\":\"更新后的标题\",\"completed\":true}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method delete'>DELETE</span> <strong>/api/todos/:id</strong> - 删除待办事项"
        "<pre>curl -X DELETE http://localhost:8080/api/todos/1</pre>"
        "</div>"
        
        "<h2>📊 示例数据</h2>"
        "<table>"
        "<tr><th>ID</th><th>标题</th><th>状态</th><th>优先级</th></tr>"
        "<tr><td>1</td><td>学习 UVHTTP</td><td><span class='status pending'>待完成</span></td><td><span class='priority high'>高</span></td></tr>"
        "<tr><td>2</td><td>完成 RESTful API 示例</td><td><span class='status pending'>待完成</span></td><td><span class='priority high'>高</span></td></tr>"
        "<tr><td>3</td><td>编写单元测试</td><td><span class='status completed'>已完成</span></td><td><span class='priority medium'>中</span></td></tr>"
        "<tr><td>4</td><td>优化性能</td><td><span class='status pending'>待完成</span></td><td><span class='priority medium'>中</span></td></tr>"
        "<tr><td>5</td><td>更新文档</td><td><span class='status pending'>待完成</span></td><td><span class='priority low'>低</span></td></tr>"
        "</table>"
        
        "<h2>🛠️ 技术特点</h2>"
        "<ul>"
        "<li>✅ 简洁的代码结构</li>"
        "<li>✅ 自动 ID 生成</li>"
        "<li>✅ 状态管理</li>"
        "<li>✅ 优先级支持</li>"
        "<li>✅ 查询过滤</li>"
        "<li>✅ 统计信息</li>"
        "<li>✅ 完整的错误处理</li>"
        "</ul>"
        
        "</div>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    uvhttp_response_send(res);
    
    return 0;
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
    printf("✅ UVHTTP RESTful Todo API 演示\n");
    printf("🚀 简洁的 CRUD 操作示例\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化示例数据
    init_sample_data();
    printf("✅ 已加载 %d 个待办事项\n", g_todo_count);
    
    // 创建服务器
    uv_loop_t* loop = uv_default_loop();
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        return 1;
    }
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 注册路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api/todos", get_todos_handler);
    uvhttp_router_add_route(router, "/api/todos", create_todo_handler);
    uvhttp_router_add_route(router, "/api/todos", update_todo_handler);
    uvhttp_router_add_route(router, "/api/todos", delete_todo_handler);
    uvhttp_router_add_route(router, "/api/todos", get_todo_handler);
    
    g_server->router = router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        fprintf(stderr, "❌ 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ Todo API 服务器启动成功\n");
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
