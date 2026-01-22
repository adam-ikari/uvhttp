/**
 * @file restful_user_api.c
 * @brief UVHTTP RESTful User Management API 演示
 * 
 * 本示例演示用户管理 API：
 * - GET /api/users - 获取用户列表
 * - GET /api/users/:id - 获取单个用户
 * - POST /api/users - 创建用户
 * - PUT /api/users/:id - 更新用户
 * - DELETE /api/users/:id - 删除用户
 * - POST /api/users/:id/activate - 激活用户
 * - POST /api/users/:id/deactivate - 停用用户
 * 
 * 特性：
 * - 用户认证（模拟）
 * - 用户状态管理
 * - 密码哈希（模拟）
 * - 角色管理
 */

#include "../include/uvhttp.h"
#include "../../deps/cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static uvhttp_server_t* g_server = NULL;

// 用户数据结构
typedef struct {
    int id;
    char username[64];
    char email[128];
    char password_hash[256];  // 模拟密码哈希
    char full_name[128];
    char role[32];  // admin, user, guest
    int active;     // 0 = 停用, 1 = 激活
    char created_at[64];
    char updated_at[64];
    char last_login[64];
} user_t;

// 模拟数据库
static user_t g_users[100];
static int g_user_count = 0;
static int g_next_id = 1;

// 初始化示例数据
static void init_sample_data() {
    const char* usernames[] = {"admin", "user1", "user2", "user3"};
    const char* emails[] = {"admin@example.com", "user1@example.com", "user2@example.com", "user3@example.com"};
    const char* full_names[] = {"管理员", "用户一", "用户二", "用户三"};
    const char* roles[] = {"admin", "user", "user", "user"};
    
    for (int i = 0; i < 4; i++) {
        g_users[i].id = g_next_id++;
        strncpy(g_users[i].username, usernames[i], sizeof(g_users[i].username) - 1);
        strncpy(g_users[i].email, emails[i], sizeof(g_users[i].email) - 1);
        strncpy(g_users[i].full_name, full_names[i], sizeof(g_users[i].full_name) - 1);
        strncpy(g_users[i].role, roles[i], sizeof(g_users[i].role) - 1);
        snprintf(g_users[i].password_hash, sizeof(g_users[i].password_hash),
                 "hash_%s", usernames[i]);
        g_users[i].active = UVHTTP_TRUE;
        snprintf(g_users[i].created_at, sizeof(g_users[i].created_at),
                 "2025-01-%02dT08:00:00Z", i + 1);
        snprintf(g_users[i].updated_at, sizeof(g_users[i].updated_at),
                 "2025-01-%02dT08:00:00Z", i + 1);
        snprintf(g_users[i].last_login, sizeof(g_users[i].last_login),
                 "2025-01-%02dT10:30:00Z", i + 1);
    }
    
    g_user_count = 4;
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

// 工具函数：将用户转换为 JSON（不包含密码）
static cJSON* user_to_json(const user_t* user) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "id", user->id);
    cJSON_AddStringToObject(json, "username", user->username);
    cJSON_AddStringToObject(json, "email", user->email);
    cJSON_AddStringToObject(json, "full_name", user->full_name);
    cJSON_AddStringToObject(json, "role", user->role);
    cJSON_AddBoolToObject(json, "active", user->active);
    cJSON_AddStringToObject(json, "created_at", user->created_at);
    cJSON_AddStringToObject(json, "updated_at", user->updated_at);
    if (strlen(user->last_login) > 0) {
        cJSON_AddStringToObject(json, "last_login", user->last_login);
    } else {
        cJSON_AddNullToObject(json, "last_login");
    }
    return json;
}

// 工具函数：从路径中提取 ID
static int extract_id_from_path(const char* path) {
    const char* id_str = strrchr(path, '/');
    if (!id_str || strlen(id_str) < 2) {
        return -1;
    }
    
    // 检查是否是特殊路径
    if (strcmp(id_str + 1, "activate") == 0 || strcmp(id_str + 1, "deactivate") == 0) {
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

// 工具函数：验证邮箱格式
static int is_valid_email(const char* email) {
    if (!email || strlen(email) < 5 || strlen(email) > 127) {
        return 0;
    }
    
    const char* at = strchr(email, '@');
    if (!at || at == email || at == email + strlen(email) - 1) {
        return 0;
    }
    
    const char* dot = strrchr(email, '.');
    if (!dot || dot < at || dot == email + strlen(email) - 1) {
        return 0;
    }
    
    return 1;
}

// GET /api/users - 获取用户列表
int get_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取查询参数
    const char* role_str = uvhttp_request_get_query_param(req, "role");
    const char* active_str = uvhttp_request_get_query_param(req, "active");
    
    int filter_role = -1;  // -1 = 不过滤
    int filter_active = -1; // -1 = 不过滤, 0 = 停用, 1 = 激活
    
    if (role_str) {
        if (strcmp(role_str, "admin") == 0) filter_role = 1;
        else if (strcmp(role_str, "user") == 0) filter_role = 2;
        else if (strcmp(role_str, "guest") == 0) filter_role = 3;
    }
    
    if (active_str) {
        if (strcmp(active_str, "true") == 0 || strcmp(active_str, "1") == 0) {
            filter_active = 1;
        } else if (strcmp(active_str, "false") == 0 || strcmp(active_str, "0") == 0) {
            filter_active = 0;
        }
    }
    
    // 创建用户列表
    cJSON* users = cJSON_CreateArray();
    int active_count = 0;
    int inactive_count = 0;
    
    for (int i = 0; i < g_user_count; i++) {
        // 过滤
        if (filter_role != -1) {
            int role_match = 0;
            if (filter_role == 1 && strcmp(g_users[i].role, "admin") == 0) role_match = 1;
            else if (filter_role == 2 && strcmp(g_users[i].role, "user") == 0) role_match = 1;
            else if (filter_role == 3 && strcmp(g_users[i].role, "guest") == 0) role_match = 1;
            if (!role_match) continue;
        }
        
        if (filter_active != -1 && g_users[i].active != filter_active) {
            continue;
        }
        
        cJSON* user_json = user_to_json(&g_users[i]);
        cJSON_AddItemToArray(users, user_json);
        
        if (g_users[i].active) {
            active_count++;
        } else {
            inactive_count++;
        }
    }
    
    // 创建统计信息
    cJSON* stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(stats, "total", g_user_count);
    cJSON_AddNumberToObject(stats, "active", active_count);
    cJSON_AddNumberToObject(stats, "inactive", inactive_count);
    
    // 创建响应数据
    cJSON* response_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response_data, "users", users);
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

// GET /api/users/:id - 获取单个用户
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的用户 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找用户
    user_t* user = NULL;
    for (int i = 0; i < g_user_count; i++) {
        if (g_users[i].id == id) {
            user = &g_users[i];
            break;
        }
    }
    
    if (!user) {
        char* error_json = create_error_response(404, "not_found", "用户不存在");
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 创建响应
    cJSON* user_json = user_to_json(user);
    char* json_string = create_json_response(200, "获取成功", user_json);
    cJSON_Delete(user_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// POST /api/users - 创建用户
int create_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
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
    cJSON* username = cJSON_GetObjectItem(json, "username");
    cJSON* email = cJSON_GetObjectItem(json, "email");
    cJSON* password = cJSON_GetObjectItem(json, "password");
    cJSON* full_name = cJSON_GetObjectItem(json, "full_name");
    cJSON* role = cJSON_GetObjectItem(json, "role");
    
    // 验证必需字段
    if (!cJSON_IsString(username) || strlen(cJSON_GetStringValue(username)) == 0) {
        char* error_json = create_error_response(400, "missing_username", "缺少必需字段: username");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    if (!cJSON_IsString(email) || !is_valid_email(cJSON_GetStringValue(email))) {
        char* error_json = create_error_response(400, "invalid_email", "邮箱格式无效");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    if (!cJSON_IsString(password) || strlen(cJSON_GetStringValue(password)) < 6) {
        char* error_json = create_error_response(400, "invalid_password", "密码长度至少为 6 位");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    // 检查用户名是否已存在
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].username, cJSON_GetStringValue(username)) == 0) {
            char* error_json = create_error_response(409, "username_exists", "用户名已存在");
            uvhttp_response_set_status(res, 409);
            uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_body(res, error_json, strlen(error_json));
            uvhttp_response_send(res);
            free(error_json);
            cJSON_Delete(json);
            return 0;
        }
    }
    
    // 检查邮箱是否已存在
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].email, cJSON_GetStringValue(email)) == 0) {
            char* error_json = create_error_response(409, "email_exists", "邮箱已存在");
            uvhttp_response_set_status(res, 409);
            uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
            uvhttp_response_set_body(res, error_json, strlen(error_json));
            uvhttp_response_send(res);
            free(error_json);
            cJSON_Delete(json);
            return 0;
        }
    }
    
    // 检查容量
    if (g_user_count >= 100) {
        char* error_json = create_error_response(500, "capacity_limit", "用户数量已达上限");
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    // 创建新用户
    user_t new_user;
    memset(&new_user, 0, sizeof(new_user));
    
    new_user.id = g_next_id++;
    strncpy(new_user.username, cJSON_GetStringValue(username), sizeof(new_user.username) - 1);
    strncpy(new_user.email, cJSON_GetStringValue(email), sizeof(new_user.email) - 1);
    snprintf(new_user.password_hash, sizeof(new_user.password_hash),
             "hash_%s", cJSON_GetStringValue(password));
    
    if (cJSON_IsString(full_name)) {
        strncpy(new_user.full_name, cJSON_GetStringValue(full_name), sizeof(new_user.full_name) - 1);
    } else {
        strncpy(new_user.full_name, cJSON_GetStringValue(username), sizeof(new_user.full_name) - 1);
    }
    
    if (cJSON_IsString(role)) {
        strncpy(new_user.role, cJSON_GetStringValue(role), sizeof(new_user.role) - 1);
    } else {
        strcpy(new_user.role, "user");
    }
    
    new_user.active = UVHTTP_TRUE;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(new_user.created_at, sizeof(new_user.created_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    strftime(new_user.updated_at, sizeof(new_user.updated_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
    // 保存用户
    g_users[g_user_count] = new_user;
    g_user_count++;
    
    cJSON_Delete(json);
    
    // 创建响应
    cJSON* user_json = user_to_json(&new_user);
    char* json_string = create_json_response(201, "创建成功", user_json);
    cJSON_Delete(user_json);
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_header(res, "Location", "/api/users");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// PUT /api/users/:id - 更新用户
int update_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的用户 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找用户
    user_t* user = NULL;
    for (int i = 0; i < g_user_count; i++) {
        if (g_users[i].id == id) {
            user = &g_users[i];
            break;
        }
    }
    
    if (!user) {
        char* error_json = create_error_response(404, "not_found", "用户不存在");
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
    cJSON* email = cJSON_GetObjectItem(json, "email");
    cJSON* full_name = cJSON_GetObjectItem(json, "full_name");
    cJSON* role = cJSON_GetObjectItem(json, "role");
    cJSON* active = cJSON_GetObjectItem(json, "active");
    
    if (cJSON_IsString(email) && is_valid_email(cJSON_GetStringValue(email))) {
        strncpy(user->email, cJSON_GetStringValue(email), sizeof(user->email) - 1);
    }
    
    if (cJSON_IsString(full_name)) {
        strncpy(user->full_name, cJSON_GetStringValue(full_name), sizeof(user->full_name) - 1);
    }
    
    if (cJSON_IsString(role)) {
        strncpy(user->role, cJSON_GetStringValue(role), sizeof(user->role) - 1);
    }
    
    if (cJSON_IsBool(active)) {
        user->active = cJSON_IsTrue(active);
    }
    
    // 更新时间戳
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(user->updated_at, sizeof(user->updated_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
    cJSON_Delete(json);
    
    // 创建响应
    cJSON* user_json = user_to_json(user);
    char* json_string = create_json_response(200, "更新成功", user_json);
    cJSON_Delete(user_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// DELETE /api/users/:id - 删除用户
int delete_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的用户 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找并删除用户
    int found = 0;
    for (int i = 0; i < g_user_count; i++) {
        if (g_users[i].id == id) {
            // 移动数组元素
            for (int j = i; j < g_user_count - 1; j++) {
                g_users[j] = g_users[j + 1];
            }
            g_user_count--;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        char* error_json = create_error_response(404, "not_found", "用户不存在");
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
        "<title>UVHTTP RESTful User Management API</title>"
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
        "pre { background: #f8f9fa; padding: 15px; border-radius: 5px; overflow-x: auto; border: 1px solid #e9ecef; }"
        "h1 { color: #007bff; }"
        "h2 { color: #495057; border-bottom: 2px solid #e9ecef; padding-bottom: 10px; }"
        "table { width: 100%; border-collapse: collapse; margin: 20px 0; }"
        "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #dee2e6; }"
        "th { background: #f8f9fa; font-weight: bold; }"
        ".status { padding: 3px 8px; border-radius: 3px; font-size: 12px; font-weight: bold; }"
        ".active { background: #28a745; color: white; }"
        ".inactive { background: #6c757d; color: white; }"
        ".role { padding: 3px 8px; border-radius: 3px; font-size: 12px; font-weight: bold; }"
        ".admin { background: #dc3545; color: white; }"
        ".user { background: #007bff; color: white; }"
        ".guest { background: #6c757d; color: white; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>👤 UVHTTP RESTful User Management API</h1>"
        "<p>用户管理 API 示例，展示用户 CRUD 操作和状态管理。</p>"
        
        "<h2>📋 API 端点</h2>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/users</strong> - 获取用户列表"
        "<p>查询参数: role (admin/user/guest), active (true/false)</p>"
        "<pre>curl 'http://localhost:8080/api/users?role=user&active=true'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/users/:id</strong> - 获取单个用户"
        "<pre>curl http://localhost:8080/api/users/1</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method post'>POST</span> <strong>/api/users</strong> - 创建用户"
        "<pre>curl -X POST http://localhost:8080/api/users -H 'Content-Type: application/json' -d '{\"username\":\"newuser\",\"email\":\"new@example.com\",\"password\":\"password123\",\"full_name\":\"新用户\",\"role\":\"user\"}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method put'>PUT</span> <strong>/api/users/:id</strong> - 更新用户"
        "<pre>curl -X PUT http://localhost:8080/api/users/1 -H 'Content-Type: application/json' -d '{\"full_name\":\"更新后的姓名\",\"active\":false}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method delete'>DELETE</span> <strong>/api/users/:id</strong> - 删除用户"
        "<pre>curl -X DELETE http://localhost:8080/api/users/1</pre>"
        "</div>"
        
        "<h2>📊 示例数据</h2>"
        "<table>"
        "<tr><th>ID</th><th>用户名</th><th>邮箱</th><th>角色</th><th>状态</th></tr>"
        "<tr><td>1</td><td>admin</td><td>admin@example.com</td><td><span class='role admin'>管理员</span></td><td><span class='status active'>激活</span></td></tr>"
        "<tr><td>2</td><td>user1</td><td>user1@example.com</td><td><span class='role user'>用户</span></td><td><span class='status active'>激活</span></td></tr>"
        "<tr><td>3</td><td>user2</td><td>user2@example.com</td><td><span class='role user'>用户</span></td><td><span class='status active'>激活</span></td></tr>"
        "<tr><td>4</td><td>user3</td><td>user3@example.com</td><td><span class='role user'>用户</span></td><td><span class='status active'>激活</span></td></tr>"
        "</table>"
        
        "<h2>🛠️ 技术特点</h2>"
        "<ul>"
        "<li>✅ 用户认证（模拟）</li>"
        "<li>✅ 用户状态管理</li>"
        "<li>✅ 密码哈希（模拟）</li>"
        "<li>✅ 角色管理</li>"
        "<li>✅ 邮箱验证</li>"
        "<li>✅ 用户名唯一性检查</li>"
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
    printf("👤 UVHTTP RESTful User Management API 演示\n");
    printf("🚀 用户管理 CRUD 操作示例\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化示例数据
    init_sample_data();
    printf("✅ 已加载 %d 个用户\n", g_user_count);
    
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
    uvhttp_router_add_route(router, "/api/users", get_users_handler);
    uvhttp_router_add_route(router, "/api/users", create_user_handler);
    uvhttp_router_add_route(router, "/api/users", update_user_handler);
    uvhttp_router_add_route(router, "/api/users", delete_user_handler);
    uvhttp_router_add_route(router, "/api/users", get_user_handler);
    
    g_server->router = router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        fprintf(stderr, "❌ 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ User Management API 服务器启动成功\n");
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
