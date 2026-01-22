/**
 * @file restful_blog_api.c
 * @brief UVHTTP RESTful Blog API 演示
 * 
 * 本示例演示完整的 RESTful API 实现：
 * - GET /api/posts - 获取文章列表
 * - GET /api/posts/:id - 获取单个文章
 * - POST /api/posts - 创建文章
 * - PUT /api/posts/:id - 更新文章
 * - DELETE /api/posts/:id - 删除文章
 * 
 * 特性：
 * - 标准 RESTful 设计
 * - JSON 请求/响应
 * - 错误处理
 * - 内存管理
 */

#include "../include/uvhttp.h"
#include "../../deps/cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static uvhttp_server_t* g_server = NULL;

// 文章数据结构
typedef struct {
    int id;
    char title[256];
    char content[4096];
    char author[128];
    char created_at[64];
    char updated_at[64];
    int views;
} blog_post_t;

// 模拟数据库
static blog_post_t g_posts[100];
static int g_post_count = 0;

// 初始化模拟数据
static void init_sample_data() {
    const char* titles[] = {
        "UVHTTP 入门指南",
        "高性能 HTTP 服务器设计",
        "RESTful API 最佳实践",
        "C 语言内存管理技巧",
        "libuv 事件循环详解"
    };
    
    const char* authors[] = {
        "张三",
        "李四",
        "王五",
        "赵六",
        "钱七"
    };
    
    for (int i = 0; i < 5; i++) {
        g_posts[i].id = i + 1;
        strncpy(g_posts[i].title, titles[i], sizeof(g_posts[i].title) - 1);
        snprintf(g_posts[i].content, sizeof(g_posts[i].content),
                 "这是 %s 的内容。本文详细介绍了相关概念和实践。", titles[i]);
        strncpy(g_posts[i].author, authors[i], sizeof(g_posts[i].author) - 1);
        snprintf(g_posts[i].created_at, sizeof(g_posts[i].created_at),
                 "2025-01-%02dT10:00:00Z", i + 1);
        snprintf(g_posts[i].updated_at, sizeof(g_posts[i].updated_at),
                 "2025-01-%02dT10:00:00Z", i + 1);
        g_posts[i].views = (i + 1) * 100;
    }
    
    g_post_count = 5;
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

// 工具函数：将文章转换为 JSON
static cJSON* post_to_json(const blog_post_t* post) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "id", post->id);
    cJSON_AddStringToObject(json, "title", post->title);
    cJSON_AddStringToObject(json, "content", post->content);
    cJSON_AddStringToObject(json, "author", post->author);
    cJSON_AddStringToObject(json, "created_at", post->created_at);
    cJSON_AddStringToObject(json, "updated_at", post->updated_at);
    cJSON_AddNumberToObject(json, "views", post->views);
    return json;
}

// 工具函数：从 JSON 创建文章
static int json_to_post(cJSON* json, blog_post_t* post) {
    cJSON* title = cJSON_GetObjectItem(json, "title");
    cJSON* content = cJSON_GetObjectItem(json, "content");
    cJSON* author = cJSON_GetObjectItem(json, "author");
    
    if (!cJSON_IsString(title) || !cJSON_IsString(content) || !cJSON_IsString(author)) {
        return -1;
    }
    
    strncpy(post->title, cJSON_GetStringValue(title), sizeof(post->title) - 1);
    strncpy(post->content, cJSON_GetStringValue(content), sizeof(post->content) - 1);
    strncpy(post->author, cJSON_GetStringValue(author), sizeof(post->author) - 1);
    
    return 0;
}

// 工具函数：从路径中提取 ID
static int extract_id_from_path(const char* path) {
    const char* id_str = strrchr(path, '/');
    if (!id_str || strlen(id_str) < 2) {
        return -1;
    }
    
    // 验证是否为数字
    for (const char* p = id_str + 1; *p; p++) {
        if (!isdigit(*p)) {
            return -1;
        }
    }
    
    return atoi(id_str + 1);
}

// GET /api/posts - 获取文章列表
int get_posts_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取查询参数
    const char* page_str = uvhttp_request_get_query_param(req, "page");
    const char* limit_str = uvhttp_request_get_query_param(req, "limit");
    const char* author = uvhttp_request_get_query_param(req, "author");
    
    int page = page_str ? atoi(page_str) : 1;
    int limit = limit_str ? atoi(limit_str) : 10;
    
    if (page < 1) page = 1;
    if (limit < 1) limit = 10;
    if (limit > 100) limit = 100;
    
    // 创建文章列表
    cJSON* posts = cJSON_CreateArray();
    int count = 0;
    
    for (int i = 0; i < g_post_count; i++) {
        // 过滤作者
        if (author && strcmp(g_posts[i].author, author) != 0) {
            continue;
        }
        
        // 分页
        count++;
        if (count < (page - 1) * limit + 1 || count > page * limit) {
            continue;
        }
        
        cJSON* post_json = post_to_json(&g_posts[i]);
        cJSON_AddItemToArray(posts, post_json);
    }
    
    // 创建分页信息
    cJSON* pagination = cJSON_CreateObject();
    cJSON_AddNumberToObject(pagination, "page", page);
    cJSON_AddNumberToObject(pagination, "limit", limit);
    cJSON_AddNumberToObject(pagination, "total", count);
    cJSON_AddNumberToObject(pagination, "pages", (count + limit - 1) / limit);
    
    // 创建响应数据
    cJSON* response_data = cJSON_CreateObject();
    cJSON_AddItemToObject(response_data, "posts", posts);
    cJSON_AddItemToObject(response_data, "pagination", pagination);
    
    char* json_string = create_json_response(200, "获取成功", response_data);
    cJSON_Delete(response_data);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// GET /api/posts/:id - 获取单个文章
int get_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的文章 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找文章
    blog_post_t* post = NULL;
    for (int i = 0; i < g_post_count; i++) {
        if (g_posts[i].id == id) {
            post = &g_posts[i];
            break;
        }
    }
    
    if (!post) {
        char* error_json = create_error_response(404, "not_found", "文章不存在");
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 增加浏览量
    post->views++;
    
    // 创建响应
    cJSON* post_json = post_to_json(post);
    char* json_string = create_json_response(200, "获取成功", post_json);
    cJSON_Delete(post_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// POST /api/posts - 创建文章
int create_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
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
    
    // 创建新文章
    blog_post_t new_post;
    memset(&new_post, 0, sizeof(new_post));
    
    if (json_to_post(json, &new_post) != 0) {
        char* error_json = create_error_response(400, "missing_fields", "缺少必需字段: title, content, author");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        cJSON_Delete(json);
        return 0;
    }
    
    cJSON_Delete(json);
    
    // 检查容量
    if (g_post_count >= 100) {
        char* error_json = create_error_response(500, "capacity_limit", "文章数量已达上限");
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 设置文章属性
    new_post.id = g_post_count + 1;
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(new_post.created_at, sizeof(new_post.created_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    snprintf(new_post.updated_at, sizeof(new_post.updated_at), "%s", new_post.created_at);
    new_post.views = 0;
    
    // 保存文章
    g_posts[g_post_count] = new_post;
    g_post_count++;
    
    // 创建响应
    cJSON* post_json = post_to_json(&new_post);
    char* json_string = create_json_response(201, "创建成功", post_json);
    cJSON_Delete(post_json);
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_header(res, "Location", "/api/posts");  // RESTful 最佳实践
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// PUT /api/posts/:id - 更新文章
int update_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的文章 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找文章
    blog_post_t* post = NULL;
    for (int i = 0; i < g_post_count; i++) {
        if (g_posts[i].id == id) {
            post = &g_posts[i];
            break;
        }
    }
    
    if (!post) {
        char* error_json = create_error_response(404, "not_found", "文章不存在");
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
    
    // 更新文章
    blog_post_t updated_post = *post;
    if (json_to_post(json, &updated_post) == 0) {
        snprintf(post->title, sizeof(post->title), "%s", updated_post.title);
        snprintf(post->content, sizeof(post->content), "%s", updated_post.content);
        snprintf(post->author, sizeof(post->author), "%s", updated_post.author);
        
        // 更新时间戳
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(post->updated_at, sizeof(post->updated_at), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    }
    
    cJSON_Delete(json);
    
    // 创建响应
    cJSON* post_json = post_to_json(post);
    char* json_string = create_json_response(200, "更新成功", post_json);
    cJSON_Delete(post_json);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_string, strlen(json_string));
    uvhttp_response_send(res);
    
    free(json_string);
    return 0;
}

// DELETE /api/posts/:id - 删除文章
int delete_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* path = uvhttp_request_get_url(req);
    int id = extract_id_from_path(path);
    
    if (id < 1) {
        char* error_json = create_error_response(400, "invalid_id", "无效的文章 ID");
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error_json, strlen(error_json));
        uvhttp_response_send(res);
        free(error_json);
        return 0;
    }
    
    // 查找并删除文章
    int found = 0;
    for (int i = 0; i < g_post_count; i++) {
        if (g_posts[i].id == id) {
            // 移动数组元素
            for (int j = i; j < g_post_count - 1; j++) {
                g_posts[j] = g_posts[j + 1];
            }
            g_post_count--;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        char* error_json = create_error_response(404, "not_found", "文章不存在");
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
        "<title>UVHTTP RESTful Blog API</title>"
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
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>UVHTTP RESTful Blog API</h1>"
        "<p>完整的 RESTful API 实现示例，展示 CRUD 操作和最佳实践。</p>"
        
        "<h2>API 端点</h2>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/posts</strong> - 获取文章列表"
        "<p>查询参数: page (页码), limit (每页数量), author (作者)</p>"
        "<pre>curl 'http://localhost:8080/api/posts?page=1&limit=5&author=张三'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method get'>GET</span> <strong>/api/posts/:id</strong> - 获取单个文章"
        "<pre>curl http://localhost:8080/api/posts/1</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method post'>POST</span> <strong>/api/posts</strong> - 创建文章"
        "<pre>curl -X POST http://localhost:8080/api/posts -H 'Content-Type: application/json' -d '{\"title\":\"新文章\",\"content\":\"文章内容\",\"author\":\"作者名\"}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method put'>PUT</span> <strong>/api/posts/:id</strong> - 更新文章"
        "<pre>curl -X PUT http://localhost:8080/api/posts/1 -H 'Content-Type: application/json' -d '{\"title\":\"更新后的标题\",\"content\":\"更新后的内容\",\"author\":\"作者名\"}'</pre>"
        "</div>"
        
        "<div class='endpoint'>"
        "<span class='method delete'>DELETE</span> <strong>/api/posts/:id</strong> - 删除文章"
        "<pre>curl -X DELETE http://localhost:8080/api/posts/1</pre>"
        "</div>"
        
        "<h2>示例数据</h2>"
        "<table>"
        "<tr><th>ID</th><th>标题</th><th>作者</th><th>浏览量</th></tr>"
        "<tr><td>1</td><td>UVHTTP 入门指南</td><td>张三</td><td>100</td></tr>"
        "<tr><td>2</td><td>高性能 HTTP 服务器设计</td><td>李四</td><td>200</td></tr>"
        "<tr><td>3</td><td>RESTful API 最佳实践</td><td>王五</td><td>300</td></tr>"
        "<tr><td>4</td><td>C 语言内存管理技巧</td><td>赵六</td><td>400</td></tr>"
        "<tr><td>5</td><td>libuv 事件循环详解</td><td>钱七</td><td>500</td></tr>"
        "</table>"
        
        "<h2>技术特点</h2>"
        "<ul>"
        "<li>标准 RESTful 设计</li>"
        "<li>完整的 CRUD 操作</li>"
        "<li>JSON 请求/响应</li>"
        "<li>查询参数支持</li>"
        "<li>分页功能</li>"
        "<li>错误处理</li>"
        "<li>内存安全管理</li>"
        "<li>HTTP 状态码正确使用</li>"
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
    printf("📝 UVHTTP RESTful Blog API 演示\n");
    printf("🚀 完整的 CRUD 操作示例\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化示例数据
    init_sample_data();
    printf("✅ 已加载 %d 篇示例文章\n", g_post_count);
    
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
    uvhttp_router_add_route(router, "/api/posts", get_posts_handler);
    uvhttp_router_add_route(router, "/api/posts", create_post_handler);
    uvhttp_router_add_route(router, "/api/posts", update_post_handler);
    uvhttp_router_add_route(router, "/api/posts", delete_post_handler);
    uvhttp_router_add_route(router, "/api/posts", get_post_handler);
    
    g_server->router = router;
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        fprintf(stderr, "❌ 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ Blog API 服务器启动成功\n");
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