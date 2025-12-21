/**
 * @file restful_api_server.c
 * @brief UVHTTP RESTful API 服务器示例
 * 
 * 此示例展示了如何使用 UVHTTP 创建一个完整的 RESTful API 服务器，
 * 包含 CRUD 操作、JSON 处理、错误处理和中间件。
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <cjson/cJSON.h>

/* 任务数据结构 */
typedef struct task {
    int id;
    char title[256];
    char description[512];
    int completed;
    time_t created_at;
    time_t updated_at;
    struct task* next;
} task_t;

/* 任务管理器 */
typedef struct {
    task_t* tasks;
    int next_id;
    size_t count;
} task_manager_t;

static task_manager_t g_task_manager = {NULL, 1, 0};
static uvhttp_server_t* g_server = NULL;
static uv_loop_t* g_loop = NULL;

/* 信号处理 */
void signal_handler(int sig) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
    }
    if (g_loop) {
        uv_stop(g_loop);
    }
}

/* 任务管理函数 */
task_t* create_task(const char* title, const char* description) {
    task_t* task = malloc(sizeof(task_t));
    if (!task) return NULL;
    
    task->id = g_task_manager.next_id++;
    strncpy(task->title, title, sizeof(task->title) - 1);
    strncpy(task->description, description, sizeof(task->description) - 1);
    task->completed = 0;
    task->created_at = time(NULL);
    task->updated_at = time(NULL);
    task->next = NULL;
    
    /* 添加到链表 */
    if (!g_task_manager.tasks) {
        g_task_manager.tasks = task;
    } else {
        task_t* current = g_task_manager.tasks;
        while (current->next) {
            current = current->next;
        }
        current->next = task;
    }
    g_task_manager.count++;
    
    return task;
}

task_t* find_task(int id) {
    task_t* current = g_task_manager.tasks;
    while (current) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int update_task(int id, const char* title, const char* description, int completed) {
    task_t* task = find_task(id);
    if (!task) return -1;
    
    if (title) {
        strncpy(task->title, title, sizeof(task->title) - 1);
    }
    if (description) {
        strncpy(task->description, description, sizeof(task->description) - 1);
    }
    task->completed = completed;
    task->updated_at = time(NULL);
    
    return 0;
}

int delete_task(int id) {
    task_t* current = g_task_manager.tasks;
    task_t* prev = NULL;
    
    while (current) {
        if (current->id == id) {
            if (prev) {
                prev->next = current->next;
            } else {
                g_task_manager.tasks = current->next;
            }
            free(current);
            g_task_manager.count--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -1;
}

void free_all_tasks() {
    task_t* current = g_task_manager.tasks;
    while (current) {
        task_t* next = current->next;
        free(current);
        current = next;
    }
    g_task_manager.tasks = NULL;
    g_task_manager.count = 0;
}

/* JSON 序列化函数 */
cJSON* task_to_json(task_t* task) {
    cJSON* json = cJSON_CreateObject();
    if (!json) return NULL;
    
    cJSON_AddNumberToObject(json, "id", task->id);
    cJSON_AddStringToObject(json, "title", task->title);
    cJSON_AddStringToObject(json, "description", task->description);
    cJSON_AddBoolToObject(json, "completed", task->completed);
    cJSON_AddNumberToObject(json, "created_at", task->created_at);
    cJSON_AddNumberToObject(json, "updated_at", task->updated_at);
    
    return json;
}

cJSON* tasks_to_json_array() {
    cJSON* array = cJSON_CreateArray();
    if (!array) return NULL;
    
    task_t* current = g_task_manager.tasks;
    while (current) {
        cJSON* task_json = task_to_json(current);
        if (task_json) {
            cJSON_AddItemToArray(array, task_json);
        }
        current = current->next;
    }
    
    return array;
}

/* CORS 中间件 */
void cors_middleware(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    /* 处理 OPTIONS 预检请求 */
    const char* method = uvhttp_request_get_method(request);
    if (strcmp(method, "OPTIONS") == 0) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_send(response);
        return;
    }
}

/* 获取所有任务 */
void get_tasks_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    cJSON* tasks_array = tasks_to_json_array();
    if (!tasks_array) {
        uvhttp_response_set_status(response, 500);
        const char* error = "{\"error\": \"Internal server error\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    char* json_string = cJSON_PrintUnformatted(tasks_array);
    cJSON_Delete(tasks_array);
    
    if (!json_string) {
        uvhttp_response_set_status(response, 500);
        const char* error = "{\"error\": \"JSON serialization error\"}";
        uvhttp_response_set_body(response, error, strlen(error));
    } else {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        uvhttp_response_set_body(response, json_string, strlen(json_string));
        free(json_string);
    }
    
    uvhttp_response_send(response);
}

/* 获取单个任务 */
void get_task_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    /* 从 URL 中提取 ID */
    const char* url = uvhttp_request_get_url(request);
    int id = atoi(url + 7); /* 跳过 "/tasks/" */
    
    task_t* task = find_task(id);
    if (!task) {
        uvhttp_response_set_status(response, 404);
        const char* error = "{\"error\": \"Task not found\"}";
        uvhttp_response_set_body(response, error, strlen(error));
    } else {
        cJSON* task_json = task_to_json(task);
        char* json_string = cJSON_PrintUnformatted(task_json);
        cJSON_Delete(task_json);
        
        if (json_string) {
            uvhttp_response_set_status(response, 200);
            uvhttp_response_set_header(response, "Content-Type", "application/json");
            uvhttp_response_set_body(response, json_string, strlen(json_string));
            free(json_string);
        } else {
            uvhttp_response_set_status(response, 500);
            const char* error = "{\"error\": \"JSON serialization error\"}";
            uvhttp_response_set_body(response, error, strlen(error));
        }
    }
    
    uvhttp_response_send(response);
}

/* 创建任务 */
void create_task_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    const char* body = uvhttp_request_get_body(request);
    if (!body || strlen(body) == 0) {
        uvhttp_response_set_status(response, 400);
        const char* error = "{\"error\": \"Request body is required\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    /* 解析 JSON */
    cJSON* json = cJSON_Parse(body);
    if (!json) {
        uvhttp_response_set_status(response, 400);
        const char* error = "{\"error\": \"Invalid JSON\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    cJSON* title_item = cJSON_GetObjectItem(json, "title");
    cJSON* description_item = cJSON_GetObjectItem(json, "description");
    
    if (!cJSON_IsString(title_item)) {
        uvhttp_response_set_status(response, 400);
        const char* error = "{\"error\": \"Title is required\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        cJSON_Delete(json);
        uvhttp_response_send(response);
        return;
    }
    
    const char* title = cJSON_GetStringValue(title_item);
    const char* description = description_item ? cJSON_GetStringValue(description_item) : "";
    
    task_t* task = create_task(title, description);
    if (!task) {
        uvhttp_response_set_status(response, 500);
        const char* error = "{\"error\": \"Failed to create task\"}";
        uvhttp_response_set_body(response, error, strlen(error));
    } else {
        cJSON* task_json = task_to_json(task);
        char* json_string = cJSON_PrintUnformatted(task_json);
        cJSON_Delete(task_json);
        
        if (json_string) {
            uvhttp_response_set_status(response, 201);
            uvhttp_response_set_header(response, "Content-Type", "application/json");
            uvhttp_response_set_body(response, json_string, strlen(json_string));
            free(json_string);
        } else {
            uvhttp_response_set_status(response, 500);
            const char* error = "{\"error\": \"JSON serialization error\"}";
            uvhttp_response_set_body(response, error, strlen(error));
        }
    }
    
    cJSON_Delete(json);
    uvhttp_response_send(response);
}

/* 更新任务 */
void update_task_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    const char* url = uvhttp_request_get_url(request);
    int id = atoi(url + 7); /* 跳过 "/tasks/" */
    
    task_t* task = find_task(id);
    if (!task) {
        uvhttp_response_set_status(response, 404);
        const char* error = "{\"error\": \"Task not found\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    const char* body = uvhttp_request_get_body(request);
    if (!body || strlen(body) == 0) {
        uvhttp_response_set_status(response, 400);
        const char* error = "{\"error\": \"Request body is required\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    cJSON* json = cJSON_Parse(body);
    if (!json) {
        uvhttp_response_set_status(response, 400);
        const char* error = "{\"error\": \"Invalid JSON\"}";
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return;
    }
    
    cJSON* title_item = cJSON_GetObjectItem(json, "title");
    cJSON* description_item = cJSON_GetObjectItem(json, "description");
    cJSON* completed_item = cJSON_GetObjectItem(json, "completed");
    
    const char* title = title_item && cJSON_IsString(title_item) ? cJSON_GetStringValue(title_item) : NULL;
    const char* description = description_item && cJSON_IsString(description_item) ? cJSON_GetStringValue(description_item) : NULL;
    int completed = completed_item && cJSON_IsBool(completed_item) ? cJSON_IsTrue(completed_item) : -1;
    
    if (update_task(id, title, description, completed) != 0) {
        uvhttp_response_set_status(response, 500);
        const char* error = "{\"error\": \"Failed to update task\"}";
        uvhttp_response_set_body(response, error, strlen(error));
    } else {
        task = find_task(id); /* 重新获取更新后的任务 */
        cJSON* task_json = task_to_json(task);
        char* json_string = cJSON_PrintUnformatted(task_json);
        cJSON_Delete(task_json);
        
        if (json_string) {
            uvhttp_response_set_status(response, 200);
            uvhttp_response_set_header(response, "Content-Type", "application/json");
            uvhttp_response_set_body(response, json_string, strlen(json_string));
            free(json_string);
        } else {
            uvhttp_response_set_status(response, 500);
            const char* error = "{\"error\": \"JSON serialization error\"}";
            uvhttp_response_set_body(response, error, strlen(error));
        }
    }
    
    cJSON_Delete(json);
    uvhttp_response_send(response);
}

/* 删除任务 */
void delete_task_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    const char* url = uvhttp_request_get_url(request);
    int id = atoi(url + 7); /* 跳过 "/tasks/" */
    
    if (delete_task(id) != 0) {
        uvhttp_response_set_status(response, 404);
        const char* error = "{\"error\": \"Task not found\"}";
        uvhttp_response_set_body(response, error, strlen(error));
    } else {
        uvhttp_response_set_status(response, 204);
    }
    
    uvhttp_response_send(response);
}

/* API 文档处理器 */
void api_docs_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    cors_middleware(request, response);
    
    const char* docs_html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "    <title>Task API 文档</title>"
        "    <meta charset=\"UTF-8\">"
        "    <style>"
        "        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }"
        "        .endpoint { background: #f5f5f5; padding: 15px; margin: 10px 0; border-radius: 5px; }"
        "        .method { display: inline-block; padding: 3px 8px; border-radius: 3px; color: white; font-weight: bold; }"
        "        .get { background: #61affe; }"
        "        .post { background: #49cc90; }"
        "        .put { background: #fca130; }"
        "        .delete { background: #f93e3e; }"
        "    </style>"
        "</head>"
        "<body>"
        "    <h1>📚 Task API 文档</h1>"
        "    <p>这是一个简单的任务管理 RESTful API。</p>"
        "    "
        "    <div class=\"endpoint\">"
        "        <span class=\"method get\">GET</span> <code>/tasks</code>"
        "        <p>获取所有任务</p>"
        "    </div>"
        "    "
        "    <div class=\"endpoint\">"
        "        <span class=\"method get\">GET</span> <code>/tasks/{id}</code>"
        "        <p>获取指定 ID 的任务</p>"
        "    </div>"
        "    "
        "    <div class=\"endpoint\">"
        "        <span class=\"method post\">POST</span> <code>/tasks</code>"
        "        <p>创建新任务</p>"
        "        <pre>{\"title\": \"任务标题\", \"description\": \"任务描述\"}</pre>"
        "    </div>"
        "    "
        "    <div class=\"endpoint\">"
        "        <span class=\"method put\">PUT</span> <code>/tasks/{id}</code>"
        "        <p>更新任务</p>"
        "        <pre>{\"title\": \"新标题\", \"description\": \"新描述\", \"completed\": true}</pre>"
        "    </div>"
        "    "
        "    <div class=\"endpoint\">"
        "        <span class=\"method delete\">DELETE</span> <code>/tasks/{id}</code>"
        "        <p>删除任务</p>"
        "    </div>"
        "    "
        "    <h2>🧪 测试示例</h2>"
        "    <pre>"
        "# 创建任务\n"
        "curl -X POST http://localhost:8080/tasks \\\n"
        "  -H \"Content-Type: application/json\" \\\n"
        "  -d '{\"title\": \"学习 UVHTTP\", \"description\": \"完成示例项目\"}'\n\n"
        "# 获取所有任务\n"
        "curl http://localhost:8080/tasks\n\n"
        "# 更新任务\n"
        "curl -X PUT http://localhost:8080/tasks/1 \\\n"
        "  -H \"Content-Type: application/json\" \\\n"
        "  -d '{\"completed\": true}'\n\n"
        "# 删除任务\n"
        "curl -X DELETE http://localhost:8080/tasks/1"
        "    </pre>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, docs_html, strlen(docs_html));
    uvhttp_response_send(response);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    const char* host = "0.0.0.0";
    
    /* 解析命令行参数 */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "错误: 无效的端口号 %s\n", argv[1]);
            return 1;
        }
    }
    
    printf("🚀 启动 Task API 服务器...\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建一些示例任务 */
    create_task("学习 UVHTTP", "了解如何使用 UVHTTP 创建高性能服务器");
    create_task("编写文档", "为项目编写完整的 API 文档");
    create_task("测试 API", "测试所有的 API 端点");
    
    /* 创建事件循环 */
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "错误: 无法创建事件循环\n");
        free_all_tasks();
        return 1;
    }
    
    /* 创建服务器 */
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        free_all_tasks();
        return 1;
    }
    
    /* 创建路由 */
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "错误: 无法创建路由\n");
        uvhttp_server_free(g_server);
        free_all_tasks();
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/tasks", get_tasks_handler);
    uvhttp_router_add_route(router, "/tasks/", get_task_handler);
    uvhttp_router_add_route(router, "/docs", api_docs_handler);
    
    /* 注意：实际项目中需要更好的路由匹配机制 */
    /* 这里为了简化，使用相同的处理函数 */
    uvhttp_router_add_route(router, "/api/tasks", get_tasks_handler);
    uvhttp_router_add_route(router, "/api/tasks/", get_task_handler);
    
    /* 配置服务器 */
    g_server->router = router;
    g_server->max_connections = 1000;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(g_server, host, port) != 0) {
        fprintf(stderr, "错误: 无法启动服务器在 %s:%d\n", host, port);
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        free_all_tasks();
        return 1;
    }
    
    printf("✅ Task API 服务器已启动\n");
    printf("📍 监听地址: http://%s:%d\n", host, port);
    printf("\n📖 API 端点:\n");
    printf("   GET    http://localhost:%d/tasks        - 获取所有任务\n", port);
    printf("   GET    http://localhost:%d/tasks/{id}    - 获取单个任务\n", port);
    printf("   POST   http://localhost:%d/tasks        - 创建任务\n", port);
    printf("   PUT    http://localhost:%d/tasks/{id}    - 更新任务\n", port);
    printf("   DELETE http://localhost:%d/tasks/{id}    - 删除任务\n", port);
    printf("   GET    http://localhost:%d/docs          - API 文档\n", port);
    printf("\n按 Ctrl+C 停止服务器\n\n");
    
    /* 运行事件循环 */
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    printf("\n🧹 正在清理资源...\n");
    uvhttp_router_free(router);
    uvhttp_server_free(g_server);
    uv_loop_close(g_loop);
    free_all_tasks();
    
    printf("✅ 服务器已关闭\n");
    
    return 0;
}