# UVHTTP API 完整参考

## 核心 API

### 统一 API（简化开发）

#### uvhttp_server_create
```c
uvhttp_server_builder_t* uvhttp_server_create(const char* host, int port);
```
快速创建服务器构建器，提供链式 API 支持。

**详细说明:**
此函数创建一个服务器构建器实例，用于通过链式 API 快速配置和启动服务器。构建器模式简化了服务器的创建过程，无需手动管理 libuv 循环和路由器。

**参数:**
- `host`: 监听地址
  - 常用值: `"0.0.0.0"` (监听所有网络接口), `"127.0.0.1"` (仅本地)
  - IPv6: `"::"` (监听所有 IPv6 接口)
  - 默认值: `UVHTTP_DEFAULT_HOST` (`"0.0.0.0"`)
- `port`: 监听端口
  - 范围: 1-65535
  - 常用值: 80 (HTTP), 443 (HTTPS), 8080 (开发), 3000 (Node.js 风格)
  - 默认值: `UVHTTP_DEFAULT_PORT` (8080)

**返回值:**
- 成功: 服务器构建器指针
- 失败: NULL（内存分配失败）

**注意事项:**
- 构建器内部会创建 libuv 事件循环
- 使用完毕后必须调用 `uvhttp_server_simple_free()` 释放资源
- 端口小于 1024 需要管理员权限

**示例:**
```c
// 创建监听所有接口的 8080 端口服务器
uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);
if (!server) {
    fprintf(stderr, "服务器创建失败\n");
    return 1;
}

// 配置并运行服务器
uvhttp_get(server, "/", handler);
uvhttp_server_run(server);

// 清理
uvhttp_server_simple_free(server);
```

---

#### 链式路由 API

##### uvhttp_get
```c
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加 GET 路由。

**详细说明:**
注册一个 GET 请求处理器到指定路径。GET 请求通常用于获取资源，应该具有幂等性（多次请求结果相同）。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
  - 支持静态路径: `"/api/users"`, `"/home"`
  - 支持参数路径: `"/users/:id"`, `"/posts/:postId/comments/:commentId"`
  - 支持通配符: `"/api/*"`（所有以 `/api/` 开头的请求）
- `handler`: 请求处理函数
  - 类型: `void (*uvhttp_request_handler_t)(uvhttp_request_t*, uvhttp_response_t*)`

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
void get_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    // 处理获取用户逻辑
    char response[256];
    snprintf(response, sizeof(response), "{\"id\":\"%s\",\"name\":\"John\"}", user_id);
    uvhttp_quick_response(res, 200, "application/json", response);
}

uvhttp_get(server, "/users/:id", get_user);
```

---

##### uvhttp_post
```c
uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server,
                                     const char* path,
                                     uvhttp_request_handler_t handler);
```
添加 POST 路由。

**详细说明:**
注册一个 POST 请求处理器到指定路径。POST 请求通常用于创建新资源，请求体包含资源数据。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径（同 GET）
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
void create_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_get_body(req);
    // 解析 JSON 并创建用户
    uvhttp_quick_response(res, 201, "application/json", "{\"status\":\"created\"}");
}

uvhttp_post(server, "/users", create_user);
```

---

##### uvhttp_put
```c
uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加 PUT 路由。

**详细说明:**
注册一个 PUT 请求处理器到指定路径。PUT 请求通常用于更新或替换整个资源，具有幂等性。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径（同 GET）
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
void update_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    const char* body = uvhttp_get_body(req);
    // 更新用户信息
    uvhttp_quick_response(res, 200, "application/json", "{\"status\":\"updated\"}");
}

uvhttp_put(server, "/users/:id", update_user);
```

---

##### uvhttp_delete
```c
uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
```
添加 DELETE 路由。

**详细说明:**
注册一个 DELETE 请求处理器到指定路径。DELETE 请求用于删除资源，具有幂等性。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径（同 GET）
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
void delete_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    // 删除用户
    uvhttp_quick_response(res, 204, "application/json", "");
}

uvhttp_delete(server, "/users/:id", delete_user);
```

---

##### uvhttp_any
```c
uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
```
添加任意方法路由。

**详细说明:**
注册一个处理器到指定路径，该处理器将接收所有 HTTP 方法（GET, POST, PUT, DELETE 等）的请求。适用于需要统一处理所有方法的场景。

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径（同 GET）
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
void handle_any_method(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    char response[256];
    snprintf(response, sizeof(response), "{\"method\":\"%s\"}", method);
    uvhttp_quick_response(res, 200, "application/json", response);
}

uvhttp_any(server, "/echo", handle_any_method);
```

**路由优先级:**
1. 具体方法路由（如 `uvhttp_get`）优先级高于 `uvhttp_any`
2. 更具体的路径优先级高于通配符路径
3. 先注册的路由优先级高于后注册的路由

**参数:**
- `server`: 服务器构建器
- `path`: 路由路径
- `handler`: 请求处理函数

**返回值:**
- 服务器构建器指针（支持链式调用）

---

#### 配置 API

##### uvhttp_set_max_connections
```c
uvhttp_server_builder_t* uvhttp_set_max_connections(uvhttp_server_builder_t* server,
                                                    int max_conn);
```
设置最大连接数。

**详细说明:**
限制服务器同时处理的最大连接数。当达到此限制时，新连接将被拒绝并返回 `UVHTTP_ERROR_CONNECTION_LIMIT` 错误。

**参数:**
- `server`: 服务器构建器
- `max_conn`: 最大连接数
  - 范围: 1-10000
  - 默认值: 2048
  - 推荐值:
    - 开发环境: 100-500
    - 生产环境: 2048-5000
    - 高并发场景: 5000-10000（需要系统调优）

**返回值:**
- 服务器构建器指针（支持链式调用）

**性能影响:**
- 连接数越大，内存占用越高
- 每个连接约占用 4-8KB 内存
- 过高的连接数可能导致性能下降

**示例:**
```c
// 设置最大连接数为 5000
uvhttp_set_max_connections(server, 5000);
```

**系统调优建议:**
```bash
# 提高系统文件描述符限制
ulimit -n 65536

# 永久修改（/etc/security/limits.conf）
* soft nofile 65536
* hard nofile 65536
```

---

##### uvhttp_set_timeout
```c
uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server,
                                            int timeout);
```
设置超时时间。

**详细说明:**
设置请求处理超时时间。如果请求在指定时间内未完成，连接将被关闭。

**参数:**
- `server`: 服务器构建器
- `timeout`: 超时时间（秒）
  - 范围: 1-3600
  - 默认值: 30
  - 推荐值:
    - API 服务: 10-30 秒
    - 文件上传: 300-600 秒
    - WebSocket: 不适用（使用 WebSocket 专用超时）

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
// 设置超时时间为 60 秒
uvhttp_set_timeout(server, 60);
```

**注意事项:**
- 超时时间过短可能导致长时间请求失败
- 超时时间过长可能导致资源占用时间过长
- 建议根据业务需求合理设置

---

##### uvhttp_set_max_body_size
```c
uvhttp_server_builder_t* uvhttp_set_max_body_size(uvhttp_server_builder_t* server,
                                                   size_t size);
```
设置最大请求体大小。

**详细说明:**
限制 HTTP 请求体的最大大小。超过此限制的请求将被拒绝并返回 413 错误。

**参数:**
- `server`: 服务器构建器
- `size`: 最大请求体大小（字节）
  - 默认值: 1MB (1048576)
  - 常用值:
    - JSON API: 1MB
    - 表单提交: 10MB
    - 文件上传: 100MB-1GB
  - 计算方式: `size_t size = 100 * 1024 * 1024;` (100MB)

**返回值:**
- 服务器构建器指针（支持链式调用）

**示例:**
```c
// 设置最大请求体为 10MB
uvhttp_set_max_body_size(server, 10 * 1024 * 1024);
```

**安全建议:**
- 对于文件上传，建议使用专用的文件上传 API
- 避免设置过大的值，防止 DoS 攻击
- 考虑使用流式处理大文件

---

#### 快速响应 API

##### uvhttp_quick_response
```c
void uvhttp_quick_response(uvhttp_response_t* response,
                           int status,
                           const char* content_type,
                           const char* body);
```
快速发送响应。

**详细说明:**
这是一个便捷函数，用于快速发送 HTTP 响应。它自动设置状态码、内容类型和响应体，并发送响应。

**参数:**
- `response`: 响应实例
- `status`: HTTP 状态码
  - 常用状态码:
    - 200: OK
    - 201: Created
    - 204: No Content
    - 400: Bad Request
    - 401: Unauthorized
    - 403: Forbidden
    - 404: Not Found
    - 500: Internal Server Error
- `content_type`: 内容类型
  - 常用类型:
    - `"text/plain"`: 纯文本
    - `"text/html"`: HTML
    - `"application/json"`: JSON
    - `"application/xml"`: XML
    - `"image/jpeg"`: JPEG 图片
    - `"image/png"`: PNG 图片
- `body`: 响应体内容
  - 可以是 NULL（对于 204 状态码）
  - 会自动计算长度

**示例:**
```c
// 发送 JSON 响应
void api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 200, "application/json",
                         "{\"status\":\"success\",\"data\":123}");
}

// 发送错误响应
void error_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 400, "application/json",
                         "{\"error\":\"Invalid request\"}");
}

// 发送空响应（204）
void delete_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 204, "text/plain", NULL);
}
```

---

##### uvhttp_html_response
```c
void uvhttp_html_response(uvhttp_response_t* response,
                          const char* html_body);
```
快速发送 HTML 响应。

**详细说明:**
便捷函数，用于发送 HTML 响应。自动设置 `Content-Type: text/html` 头部。

**参数:**
- `response`: 响应实例
- `html_body`: HTML 内容
  - 必须是有效的 HTML
  - 可以是 NULL（发送空 HTML）

**示例:**
```c
void home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP</title></head>"
        "<body>"
        "<h1>Welcome to UVHTTP</h1>"
        "<p>A high-performance HTTP server library</p>"
        "</body>"
        "</html>";

    uvhttp_html_response(res, html);
}

void template_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_get_param(req, "name");
    char html[1024];
    snprintf(html, sizeof(html),
        "<!DOCTYPE html>"
        "<html><body><h1>Hello, %s!</h1></body></html>",
        name ? name : "Guest");

    uvhttp_html_response(res, html);
}
```

---

##### uvhttp_file_response
```c
void uvhttp_file_response(uvhttp_response_t* response,
                          const char* file_path);
```
快速发送文件响应。

**详细说明:**
便捷函数，用于发送文件响应。自动检测文件类型并设置正确的 `Content-Type` 头部。

**参数:**
- `response`: 响应实例
- `file_path`: 文件路径
  - 必须是绝对路径或相对于工作目录的路径
  - 文件必须存在且可读
  - 支持常见文件类型（HTML, CSS, JS, 图片等）

**支持的文件类型:**
- HTML: `.html`, `.htm` → `text/html`
- CSS: `.css` → `text/css`
- JavaScript: `.js` → `application/javascript`
- JSON: `.json` → `application/json`
- 图片: `.jpg`, `.jpeg` → `image/jpeg`
- 图片: `.png` → `image/png`
- 图片: `.gif` → `image/gif`
- 图片: `.svg` → `image/svg+xml`
- PDF: `.pdf` → `application/pdf`

**示例:**
```c
void image_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_file_response(res, "/var/www/images/logo.png");
}

void download_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* filename = uvhttp_get_param(req, "file");
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/downloads/%s", filename);
    uvhttp_file_response(res, filepath);
}
```

**注意事项:**
- 文件大小受 `UVHTTP_MAX_BODY_SIZE` 限制
- 大文件建议使用静态文件服务 API
- 文件不存在时会返回 404 错误

---

#### 便捷请求 API

##### uvhttp_get_param
```c
const char* uvhttp_get_param(uvhttp_request_t* request,
                             const char* name);
```
获取请求参数。

**详细说明:**
从 URL 路径或查询字符串中获取参数值。支持路径参数（如 `/users/:id`）和查询参数（如 `/users?id=123`）。

**参数:**
- `request`: 请求实例
- `name`: 参数名称
  - 路径参数: 如 `"id"`, `"postId"`, `"commentId"`
  - 查询参数: 如 `"page"`, `"limit"`, `"sort"`

**返回值:**
- 找到: 参数值字符串
- 未找到: NULL

**参数优先级:**
1. 路径参数优先（如 `/users/:id` 中的 `id`）
2. 查询参数次之（如 `?id=123` 中的 `id`）

**示例:**
```c
// 路由: GET /users/:id
void get_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    if (!user_id) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Missing user id\"}");
        return;
    }

    const char* page = uvhttp_get_param(req, "page");  // 查询参数
    const char* limit = uvhttp_get_param(req, "limit"); // 查询参数

    char response[512];
    snprintf(response, sizeof(response),
             "{\"user_id\":\"%s\",\"page\":\"%s\",\"limit\":\"%s\"}",
             user_id, page ? page : "1", limit ? limit : "10");

    uvhttp_quick_response(res, 200, "application/json", response);
}
```

**路由定义示例:**
```c
// 路由定义
uvhttp_get(server, "/users/:id/posts/:postId", handler);

// 访问: /users/123/posts/456?page=2&limit=20
// 可用参数:
// - id = "123" (路径参数)
// - postId = "456" (路径参数)
// - page = "2" (查询参数)
// - limit = "20" (查询参数)
```

---

##### uvhttp_get_header
```c
const char* uvhttp_get_header(uvhttp_request_t* request,
                              const char* name);
```
获取请求头。

**详细说明:**
从 HTTP 请求中获取指定的头部值。头部名称不区分大小写。

**参数:**
- `request`: 请求实例
- `name`: 头部名称
  - 常用头部:
    - `"Content-Type"`: 内容类型
    - `"Content-Length"`: 内容长度
    - `"Authorization"`: 认证信息
    - `"User-Agent"`: 用户代理
    - `"Accept"`: 接受的内容类型
    - `"Cookie"`: Cookie
    - `"X-Forwarded-For"`: 客户端真实 IP

**返回值:**
- 找到: 头部值字符串
- 未找到: NULL

**示例:**
```c
void api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取内容类型
    const char* content_type = uvhttp_get_header(req, "Content-Type");
    if (!content_type || strcmp(content_type, "application/json") != 0) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Invalid content type\"}");
        return;
    }

    // 获取认证信息
    const char* auth = uvhttp_get_header(req, "Authorization");
    if (!auth) {
        uvhttp_quick_response(res, 401, "application/json",
                             "{\"error\":\"Unauthorized\"}");
        return;
    }

    // 获取客户端 IP
    const char* ip = uvhttp_get_header(req, "X-Forwarded-For");
    if (!ip) {
        ip = "127.0.0.1";  // 默认值
    }

    // 处理请求...
}

void log_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_agent = uvhttp_get_header(req, "User-Agent");
    const char* referer = uvhttp_get_header(req, "Referer");

    printf("Request from: %s, Referer: %s\n",
           user_agent ? user_agent : "Unknown",
           referer ? referer : "None");

    uvhttp_quick_response(res, 200, "text/plain", "OK");
}
```

**常见头部值:**
```c
// Content-Type
"application/json"
"application/x-www-form-urlencoded"
"multipart/form-data"
"text/plain"
"text/html"

// Authorization
"Bearer <token>"
"Basic <credentials>"

// Accept
"application/json"
"text/html"
"*/*"
```

---

##### uvhttp_get_body
```c
const char* uvhttp_get_body(uvhttp_request_t* request);
```
获取请求体。

**详细说明:**
获取 HTTP 请求体的原始数据。适用于 POST、PUT 等包含请求体的方法。

**参数:**
- `request`: 请求实例

**返回值:**
- 请求体数据指针
- 如果没有请求体（如 GET 请求），返回 NULL 或空字符串

**使用场景:**
- JSON 数据解析
- 表单数据处理
- 文件上传（小文件）
- 自定义协议数据

**示例:**
```c
void json_api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_get_body(req);
    if (!body || strlen(body) == 0) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Empty body\"}");
        return;
    }

    // 解析 JSON（需要 JSON 库，如 cJSON）
    // cJSON* json = cJSON_Parse(body);
    // if (!json) {
    //     uvhttp_quick_response(res, 400, "application/json",
    //                          "{\"error\":\"Invalid JSON\"}");
    //     return;
    // }

    // 处理 JSON 数据...
    // cJSON_Delete(json);

    uvhttp_quick_response(res, 200, "application/json",
                         "{\"status\":\"success\"}");
}

void form_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_get_body(req);
    if (!body) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"No form data\"}");
        return;
    }

    // 解析表单数据
    // 格式: key1=value1&key2=value2
    char body_copy[4096];
    strncpy(body_copy, body, sizeof(body_copy) - 1);
    body_copy[sizeof(body_copy) - 1] = '\0';

    char* token = strtok(body_copy, "&");
    while (token != NULL) {
        printf("Form field: %s\n", token);
        token = strtok(NULL, "&");
    }

    uvhttp_quick_response(res, 200, "application/json",
                         "{\"status\":\"received\"}");
}
```

**注意事项:**
- 请求体大小受 `UVHTTP_MAX_BODY_SIZE` 限制
- 大文件上传建议使用流式处理
- 请求体数据在请求处理完成后会被释放
- 不要保存请求体指针供后续使用

---

#### 服务器运行和清理

##### uvhttp_server_run
```c
int uvhttp_server_run(uvhttp_server_builder_t* server);
```
运行服务器。

**详细说明:**
启动服务器并开始监听请求。此函数会阻塞当前线程，直到服务器停止。内部会调用 `uv_run()` 启动 libuv 事件循环。

**参数:**
- `server`: 服务器构建器

**返回值:**
- 成功: 0
- 失败: 负数错误码
  - `UVHTTP_ERROR_SERVER_INIT`: 服务器初始化失败
  - `UVHTTP_ERROR_SERVER_LISTEN`: 监听失败
  - `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**行为:**
1. 初始化服务器
2. 绑定到指定地址和端口
3. 开始监听连接
4. 进入事件循环
5. 处理传入的请求

**示例:**
```c
int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_get(server, "/", home_handler);
    uvhttp_post(server, "/api", api_handler);

    int result = uvhttp_server_run(server);
    if (result != 0) {
        fprintf(stderr, "服务器运行失败: %d\n", result);
        return 1;
    }

    uvhttp_server_simple_free(server);
    return 0;
}
```

**注意事项:**
- 此函数会阻塞，直到服务器停止
- 在单独的线程中运行可以实现非阻塞
- 服务器停止后会自动清理资源

---

##### uvhttp_server_stop_simple
```c
void uvhttp_server_stop_simple(uvhttp_server_builder_t* server);
```
停止服务器。

**详细说明:**
优雅地停止服务器，完成当前正在处理的请求后关闭。

**参数:**
- `server`: 服务器构建器

**行为:**
1. 停止接受新连接
2. 等待当前请求完成
3. 关闭所有连接
4. 停止事件循环

**示例:**
```c
#include <signal.h>

uvhttp_server_builder_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到停止信号，正在关闭服务器...\n");
    if (g_server) {
        uvhttp_server_stop_simple(g_server);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    g_server = uvhttp_server_create("0.0.0.0", 8080);
    uvhttp_get(g_server, "/", handler);

    uvhttp_server_run(g_server);
    uvhttp_server_simple_free(g_server);

    return 0;
}
```

**注意事项:**
- 停止过程可能需要几秒钟
- 强制终止可能导致数据丢失
- 建议使用信号处理实现优雅关闭

---

##### uvhttp_server_simple_free
```c
void uvhttp_server_simple_free(uvhttp_server_builder_t* server);
```
释放服务器资源。

**详细说明:**
释放服务器构建器及其关联的所有资源，包括服务器实例、路由器、配置等。

**参数:**
- `server`: 服务器构建器

**释放的资源:**
- 服务器实例
- 路由器
- 配置对象
- libuv 事件循环
- 所有连接
- WebSocket 连接

**示例:**
```c
int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_get(server, "/", handler);

    int result = uvhttp_server_run(server);

    // 清理资源
    uvhttp_server_simple_free(server);

    return result;
}
```

**注意事项:**
- 必须在服务器停止后调用
- 不要重复调用
- 释放后不要再使用服务器指针

---

#### 一键启动

##### uvhttp_serve
```c
int uvhttp_serve(const char* host, int port);
```
一键启动服务器（最简 API）。

**详细说明:**
这是最简单的服务器启动方式。它会自动创建服务器、配置默认设置、添加默认路由处理器，并启动服务器。适用于快速原型开发和测试。

**参数:**
- `host`: 监听地址
  - 默认: `"0.0.0.0"`
- `port`: 监听端口
  - 默认: 8080

**返回值:**
- 成功: 0
- 失败: 负数错误码

**默认配置:**
- 最大连接数: 2048
- 超时时间: 30 秒
- 最大请求体: 1MB
- Keep-Alive: 启用

**默认行为:**
- 自动添加根路径 `/` 处理器
- 返回简单的欢迎消息
- 支持所有 HTTP 方法

**示例:**
```c
// 最简单的 HTTP 服务器
int main() {
    return uvhttp_serve("0.0.0.0", 8080);
}
```

**访问示例:**
```bash
# 访问服务器
curl http://localhost:8080/

# 响应
# HTTP/1.1 200 OK
# Content-Type: text/plain
#
# UVHTTP Server v2.0.0
# Server is running on 0.0.0.0:8080
```

**使用场景:**
1. 快速原型开发
2. 功能测试
3. 学习和演示
4. 最小化部署

**限制:**
- 无法自定义路由
- 无法修改配置
- 无法添加中间件
- 不适合生产环境

**生产环境建议:**
```c
// 生产环境使用完整 API
int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    // 自定义配置
    uvhttp_set_max_connections(server, 5000);
    uvhttp_set_timeout(server, 60);
    uvhttp_set_max_body_size(server, 10 * 1024 * 1024);

    // 添加路由
    uvhttp_get(server, "/", home_handler);
    uvhttp_get(server, "/api/users", get_users_handler);
    uvhttp_post(server, "/api/users", create_user_handler);

    // 运行服务器
    int result = uvhttp_server_run(server);
    uvhttp_server_simple_free(server);

    return result;
}
```

---

### 统一 API 使用示例

#### 示例 1: 基础 REST API

```c
#include "uvhttp.h"

// 获取所有用户
void get_users(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 200, "application/json",
                         "[{\"id\":1,\"name\":\"John\"},{\"id\":2,\"name\":\"Jane\"}]");
}

// 获取单个用户
void get_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    char response[256];
    snprintf(response, sizeof(response),
             "{\"id\":%s,\"name\":\"John\",\"email\":\"john@example.com\"}",
             user_id ? user_id : "0");
    uvhttp_quick_response(res, 200, "application/json", response);
}

// 创建用户
void create_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_get_body(req);
    // 解析 JSON 并创建用户
    uvhttp_quick_response(res, 201, "application/json",
                         "{\"id\":3,\"status\":\"created\"}");
}

// 更新用户
void update_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    const char* body = uvhttp_get_body(req);
    // 更新用户
    char response[256];
    snprintf(response, sizeof(response),
             "{\"id\":%s,\"status\":\"updated\"}", user_id);
    uvhttp_quick_response(res, 200, "application/json", response);
}

// 删除用户
void delete_user(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* user_id = uvhttp_get_param(req, "id");
    // 删除用户
    uvhttp_quick_response(res, 204, "application/json", "");
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    // 配置服务器
    uvhttp_set_max_connections(server, 5000);
    uvhttp_set_timeout(server, 30);
    uvhttp_set_max_body_size(server, 10 * 1024 * 1024);

    // 添加路由
    uvhttp_get(server, "/api/users", get_users);
    uvhttp_get(server, "/api/users/:id", get_user);
    uvhttp_post(server, "/api/users", create_user);
    uvhttp_put(server, "/api/users/:id", update_user);
    uvhttp_delete(server, "/api/users/:id", delete_user);

    // 运行服务器
    int result = uvhttp_server_run(server);
    uvhttp_server_simple_free(server);

    return result;
}
```

**测试 API:**
```bash
# 获取所有用户
curl http://localhost:8080/api/users

# 获取单个用户
curl http://localhost:8080/api/users/1

# 创建用户
curl -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Alice","email":"alice@example.com"}'

# 更新用户
curl -X PUT http://localhost:8080/api/users/1 \
  -H "Content-Type: application/json" \
  -d '{"name":"John Doe"}'

# 删除用户
curl -X DELETE http://localhost:8080/api/users/1
```

---

#### 示例 2: 静态文件服务器

```c
void file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* filename = uvhttp_get_param(req, "filename");
    if (!filename) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Missing filename\"}");
        return;
    }

    // 安全检查：防止路径遍历攻击
    if (strstr(filename, "..") != NULL) {
        uvhttp_quick_response(res, 403, "application/json",
                             "{\"error\":\"Invalid filename\"}");
        return;
    }

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/var/www/%s", filename);
    uvhttp_file_response(res, filepath);
}

void home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>文件服务器</title></head>"
        "<body>"
        "<h1>文件服务器</h1>"
        "<p>访问文件: /files/<filename></p>"
        "</body>"
        "</html>";
    uvhttp_html_response(res, html);
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_set_max_body_size(server, 100 * 1024 * 1024);  // 100MB

    uvhttp_get(server, "/", home_handler);
    uvhttp_get(server, "/files/:filename", file_handler);

    return uvhttp_server_run(server);
}
```

---

#### 示例 3: 带认证的 API

```c
void check_auth(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* auth = uvhttp_get_header(req, "Authorization");
    if (!auth || strncmp(auth, "Bearer ", 7) != 0) {
        uvhttp_quick_response(res, 401, "application/json",
                             "{\"error\":\"Unauthorized\"}");
        return;
    }

    const char* token = auth + 7;  // 跳过 "Bearer "
    // 验证 token
    if (strcmp(token, "valid_token_123") != 0) {
        uvhttp_quick_response(res, 403, "application/json",
                             "{\"error\":\"Invalid token\"}");
        return;
    }

    // Token 有效，继续处理
}

void protected_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    check_auth(req, res);
    // 如果 check_auth 没有返回错误，继续处理
    uvhttp_quick_response(res, 200, "application/json",
                         "{\"message\":\"Access granted\",\"data\":\"secret\"}");
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_get(server, "/api/protected", protected_handler);

    return uvhttp_server_run(server);
}
```

**测试:**
```bash
# 无认证访问
curl http://localhost:8080/api/protected
# 返回: 401 Unauthorized

# 使用 token 访问
curl http://localhost:8080/api/protected \
  -H "Authorization: Bearer valid_token_123"
# 返回: 200 OK
```

---

#### 示例 4: JSON API 完整示例

```c
#include <string.h>

void json_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 验证 Content-Type
    const char* content_type = uvhttp_get_header(req, "Content-Type");
    if (!content_type || strcmp(content_type, "application/json") != 0) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Content-Type must be application/json\"}");
        return;
    }

    // 获取请求体
    const char* body = uvhttp_get_body(req);
    if (!body || strlen(body) == 0) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Request body is empty\"}");
        return;
    }

    // 获取查询参数
    const char* pretty = uvhttp_get_param(req, "pretty");
    int is_pretty = pretty && strcmp(pretty, "true") == 0;

    // 处理 JSON（这里简化处理，实际应使用 JSON 库）
    char response[1024];
    if (is_pretty) {
        snprintf(response, sizeof(response),
                 "{\n"
                 "  \"status\": \"success\",\n"
                 "  \"received\": {\n"
                 "    \"body\": \"%s\"\n"
                 "  }\n"
                 "}",
                 body);
    } else {
        snprintf(response, sizeof(response),
                 "{\"status\":\"success\",\"received\":{\"body\":\"%s\"}}",
                 body);
    }

    uvhttp_quick_response(res, 200, "application/json", response);
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_set_max_body_size(server, 5 * 1024 * 1024);  // 5MB
    uvhttp_post(server, "/api/json", json_handler);

    return uvhttp_server_run(server);
}
```

**测试:**
```bash
# 发送 JSON
curl -X POST http://localhost:8080/api/json \
  -H "Content-Type: application/json" \
  -d '{"name":"John","age":30}'

# 美化输出
curl -X POST 'http://localhost:8080/api/json?pretty=true' \
  -H "Content-Type: application/json" \
  -d '{"name":"John","age":30}'
```

---

#### 示例 5: 错误处理和日志

```c
void log_request(uvhttp_request_t* req) {
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    const char* user_agent = uvhttp_get_header(req, "User-Agent");

    printf("[%s] %s %s (User-Agent: %s)\n",
           method ? method : "UNKNOWN",
           url ? url : "/",
           user_agent ? user_agent : "Unknown");
}

void safe_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 记录请求
    log_request(req);

    // 参数验证
    const char* id = uvhttp_get_param(req, "id");
    if (!id) {
        uvhttp_quick_response(res, 400, "application/json",
                             "{\"error\":\"Missing required parameter: id\"}");
        return;
    }

    // 验证 ID 格式
    for (const char* p = id; *p; p++) {
        if (!isdigit(*p)) {
            uvhttp_quick_response(res, 400, "application/json",
                                 "{\"error\":\"Invalid id format\"}");
            return;
        }
    }

    // 处理请求
    char response[256];
    snprintf(response, sizeof(response),
             "{\"id\":%s,\"timestamp\":%ld}",
             id, time(NULL));

    uvhttp_quick_response(res, 200, "application/json", response);
}

int main() {
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

    uvhttp_get(server, "/api/data/:id", safe_handler);

    printf("服务器启动在 http://0.0.0.0:8080\n");
    return uvhttp_server_run(server);
}
```

---

#### 一键启动示例

```c
// 最简单的 HTTP 服务器
int main() {
    printf("启动最简单的 HTTP 服务器...\n");
    return uvhttp_serve("0.0.0.0", 8080);
}
```

**访问:**
```bash
curl http://localhost:8080/
```

**响应:**
```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 68

UVHTTP Server v2.0.0
Server is running on 0.0.0.0:8080
```

---

### 服务器管理

#### uvhttp_server_new
```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```
创建新的 HTTP 服务器实例。

**详细说明:**
创建一个新的 HTTP 服务器实例，绑定到指定的 libuv 事件循环。这是使用核心 API 的第一步。

**参数:**
- `loop`: libuv 事件循环
  - 可以为 NULL，内部会创建新的事件循环
  - 推荐使用 `uv_default_loop()` 获取默认循环
  - 支持自定义循环用于多实例部署

**返回值:**
- 成功: 服务器实例指针
- 失败: NULL（内存分配失败）

**服务器状态:**
- 创建后处于未监听状态
- 需要调用 `uvhttp_server_listen()` 启动监听
- 需要设置路由器或处理器才能处理请求

**示例:**
```c
// 使用默认事件循环
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);

if (!server) {
    fprintf(stderr, "服务器创建失败\n");
    return 1;
}

// 配置服务器
uvhttp_router_t* router = uvhttp_router_new();
uvhttp_router_add_route(router, "/", handler);
uvhttp_server_set_router(server, router);

// 启动服务器
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "监听失败: %s\n", uvhttp_error_string(result));
    return 1;
}

// 运行事件循环
uv_run(loop, UV_RUN_DEFAULT);

// 清理
uvhttp_server_free(server);
```

**多实例示例:**
```c
// 创建多个服务器实例，每个使用独立的事件循环
uv_loop_t* loop1 = uv_loop_new();
uv_loop_t* loop2 = uv_loop_new();

uvhttp_server_t* server1 = uvhttp_server_new(loop1);
uvhttp_server_t* server2 = uvhttp_server_new(loop2);

uvhttp_server_listen(server1, "0.0.0.0", 8080);
uvhttp_server_listen(server2, "0.0.0.0", 8081);

// 在不同线程中运行
uv_run(loop1, UV_RUN_DEFAULT);
uv_run(loop2, UV_RUN_DEFAULT);
```

---

#### uvhttp_server_free
```c
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
```
释放服务器资源。

**详细说明:**
释放服务器实例及其关联的所有资源。此函数会优雅地关闭所有连接，释放路由器、配置等资源。

**参数:**
- `server`: 服务器实例

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

**释放的资源:**
- 服务器实例
- 路由器
- 配置对象
- 所有活跃连接
- WebSocket 连接
- 中间件链

**注意事项:**
- 必须在服务器停止后调用
- 不要在请求处理器中调用
- 释放后不要再使用服务器指针
- 会自动清理所有关联资源

**示例:**
```c
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_server_listen(server, "0.0.0.0", 8080);

// 运行服务器...
uv_run(loop, UV_RUN_DEFAULT);

// 停止并清理
uvhttp_server_stop(server);
uvhttp_error_t result = uvhttp_server_free(server);
if (result != UVHTTP_OK) {
    fprintf(stderr, "清理失败: %s\n", uvhttp_error_string(result));
}
```

---

#### uvhttp_server_listen
```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
```
启动服务器监听。

**详细说明:**
绑定服务器到指定的地址和端口，开始监听传入的连接。

**参数:**
- `server`: 服务器实例
- `host`: 监听地址
  - `"0.0.0.0"`: 监听所有 IPv4 接口
  - `"127.0.0.1"`: 仅本地访问
  - `"::"`: 监听所有 IPv6 接口
  - 具体 IP: 如 `"192.168.1.100"`
- `port`: 监听端口
  - 范围: 1-65535
  - 常用: 80 (HTTP), 443 (HTTPS), 8080 (开发)
  - < 1024 需要管理员权限

**返回值:**
- 成功: UVHTTP_OK
- 失败:
  - `UVHTTP_ERROR_SERVER_LISTEN`: 监听失败（端口被占用）
  - `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
  - `UVHTTP_ERROR_SERVER_ALREADY_RUNNING`: 服务器已在运行

**示例:**
```c
uvhttp_server_t* server = uvhttp_server_new(loop);

// 监听所有接口的 8080 端口
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    if (result == UVHTTP_ERROR_SERVER_LISTEN) {
        fprintf(stderr, "端口 8080 已被占用\n");
    } else {
        fprintf(stderr, "监听失败: %s\n", uvhttp_error_string(result));
    }
    return 1;
}

printf("服务器启动在 http://0.0.0.0:8080\n");
```

**端口冲突处理:**
```c
int try_ports[] = {8080, 8081, 8082, 8083, 8084};
uvhttp_error_t result = UVHTTP_ERROR_SERVER_LISTEN;

for (int i = 0; i < 5; i++) {
    result = uvhttp_server_listen(server, "0.0.0.0", try_ports[i]);
    if (result == UVHTTP_OK) {
        printf("服务器启动在端口 %d\n", try_ports[i]);
        break;
    }
}

if (result != UVHTTP_OK) {
    fprintf(stderr, "无法找到可用端口\n");
}
```

---

#### uvhttp_server_stop
```c
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
```
停止服务器。

**详细说明:**
优雅地停止服务器，完成当前正在处理的请求后关闭。

**参数:**
- `server`: 服务器实例

**返回值:**
- 成功: UVHTTP_OK
- 失败:
  - `UVHTTP_ERROR_SERVER_NOT_RUNNING`: 服务器未运行
  - `UVHTTP_ERROR_SERVER_STOP`: 停止失败

**停止过程:**
1. 停止接受新连接
2. 等待当前请求完成（最多 30 秒）
3. 关闭所有连接
4. 释放监听套接字

**示例:**
```c
#include <signal.h>

uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到停止信号...\n");
    if (g_server) {
        uvhttp_error_t result = uvhttp_server_stop(g_server);
        if (result == UVHTTP_OK) {
            printf("服务器已停止\n");
        } else {
            fprintf(stderr, "停止失败: %s\n", uvhttp_error_string(result));
        }
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uv_loop_t* loop = uv_default_loop();
    g_server = uvhttp_server_new(loop);

    uvhttp_server_listen(g_server, "0.0.0.0", 8080);

    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_free(g_server);
    return 0;
}
```

**注意事项:**
- 停止过程可能需要几秒钟
- 强制终止可能导致数据丢失
- 建议使用信号处理实现优雅关闭
- 停止后可以重新调用 `uvhttp_server_listen()` 重新启动

---

#### uvhttp_server_set_handler
```c
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler);
```
设置默认请求处理器。

**详细说明:**
设置服务器的默认请求处理器，用于处理所有未匹配路由的请求。

**参数:**
- `server`: 服务器实例
- `handler`: 请求处理函数

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

**示例:**
```c
void default_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_quick_response(res, 404, "application/json",
                         "{\"error\":\"Not found\"}");
}

int main() {
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 设置默认处理器（404）
    uvhttp_server_set_handler(server, default_handler);

    // 添加特定路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_server_set_router(server, router);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

---

#### uvhttp_server_set_router
```c
uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server, uvhttp_router_t* router);
```
设置路由器。

**详细说明:**
将路由器绑定到服务器，用于处理路由匹配和请求分发。

**参数:**
- `server`: 服务器实例
- `router`: 路由器实例

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

**示例:**
```c
int main() {
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();

    // 添加路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api/users", get_users_handler);
    uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);

    // 设置路由器
    uvhttp_server_set_router(server, router);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

---

### 路由管理

#### uvhttp_router_new
```c
uvhttp_router_t* uvhttp_router_new(void);
```
创建新的路由器。

**返回值:**
- 成功: 路由器实例指针
- 失败: NULL

---

#### uvhttp_router_free
```c
void uvhttp_router_free(uvhttp_router_t* router);
```
释放路由器资源。

---

#### uvhttp_router_add_route
```c
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                           const char* path,
                           uvhttp_request_handler_t handler);
```
添加路由规则。

**参数:**
- `router`: 路由器实例
- `path`: 路由路径 (如 "/api/users")
- `handler`: 请求处理函数

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_router_find_handler
```c
uvhttp_request_handler_t uvhttp_router_find_handler(const uvhttp_router_t* router,
                                                   const char* path,
                                                   const char* method);
```
查找路径对应的处理器。

**参数:**
- `router`: 路由器实例
- `path`: 请求路径
- `method`: HTTP 方法 ("GET", "POST", 等)

**返回值:**
- 找到: 处理器函数指针
- 未找到: NULL

---

### 请求处理

#### uvhttp_request_get_method
```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
```
获取 HTTP 方法。

**返回值:**
- HTTP 方法字符串 ("GET", "POST", 等)

---

#### uvhttp_request_get_url
```c
const char* uvhttp_request_get_url(uvhttp_request_t* request);
```
获取请求 URL。

**返回值:**
- URL 字符串

---

#### uvhttp_request_get_header
```c
const char* uvhttp_request_get_header(uvhttp_request_t* request, 
                                     const char* name);
```
获取请求头。

**参数:**
- `request`: 请求实例
- `name`: 头部名称 (如 "Content-Type")

**返回值:**
- 找到: 头部值字符串
- 未找到: NULL

---

#### uvhttp_request_get_body
```c
const char* uvhttp_request_get_body(uvhttp_request_t* request);
```
获取请求体。

**返回值:**
- 请求体数据指针

---

#### uvhttp_request_get_body_length
```c
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);
```
获取请求体长度。

**返回值:**
- 请求体字节数

---

### 响应处理

#### uvhttp_response_set_status
```c
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
```
设置响应状态码。

**参数:**
- `response`: 响应实例
- `status_code`: HTTP 状态码 (200, 404, 等)

---

#### uvhttp_response_set_header
```c
void uvhttp_response_set_header(uvhttp_response_t* response, 
                               const char* name, 
                               const char* value);
```
设置响应头。

**参数:**
- `response`: 响应实例
- `name`: 头部名称
- `value`: 头部值

---

#### uvhttp_response_set_body
```c
int uvhttp_response_set_body(uvhttp_response_t* response, 
                            const char* body, 
                            size_t length);
```
设置响应体。

**参数:**
- `response`: 响应实例
- `body`: 响应体数据
- `length`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_response_send
```c
void uvhttp_response_send(uvhttp_response_t* response);
```
发送响应。

---

## WebSocket API

### 服务器端 WebSocket 管理

#### uvhttp_server_register_ws_handler
```c
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server,
                                                 const char* path,
                                                 uvhttp_ws_handler_t* handler);
```
注册 WebSocket 处理器。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径 (如 "/ws")
- `handler`: WebSocket 处理器

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_send
```c
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn,
                                     const char* data,
                                     size_t len);
```
发送 WebSocket 消息。

**参数:**
- `ws_conn`: WebSocket 连接
- `data`: 消息数据
- `len`: 数据长度

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_close
```c
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn,
                                      int code,
                                      const char* reason);
```
关闭 WebSocket 连接。

**参数:**
- `ws_conn`: WebSocket 连接
- `code`: 关闭代码
- `reason`: 关闭原因

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### WebSocket 连接管理

#### uvhttp_server_ws_enable_connection_management
```c
uvhttp_error_t uvhttp_server_ws_enable_connection_management(uvhttp_server_t* server,
                                                             int timeout_seconds,
                                                             int heartbeat_interval);
```
启用 WebSocket 连接管理。

**参数:**
- `server`: 服务器实例
- `timeout_seconds`: 超时时间（秒）
- `heartbeat_interval`: 心跳间隔（秒）

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_get_connection_count
```c
int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server);
```
获取 WebSocket 连接总数。

**参数:**
- `server`: 服务器实例

**返回值:**
- 连接数量

---

#### uvhttp_server_ws_broadcast
```c
uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server,
                                         const char* path,
                                         const char* data,
                                         size_t len);
```
向指定路径的所有连接广播消息。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `data`: 消息数据
- `len`: 数据长度

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### WebSocket 认证

#### uvhttp_server_ws_set_auth_config
```c
uvhttp_error_t uvhttp_server_ws_set_auth_config(uvhttp_server_t* server,
                                                const char* path,
                                                uvhttp_ws_auth_config_t* config);
```
设置 WebSocket 认证配置。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `config`: 认证配置

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

#### uvhttp_server_ws_enable_token_auth
```c
uvhttp_error_t uvhttp_server_ws_enable_token_auth(uvhttp_server_t* server,
                                                  const char* path,
                                                  uvhttp_ws_token_validator_callback validator,
                                                  void* user_data);
```
启用 Token 认证。

**参数:**
- `server`: 服务器实例
- `path`: WebSocket 路径
- `validator`: Token 验证回调
- `user_data`: 用户数据

**返回值:**
- 成功: UVHTTP_OK
- 失败: 其他 uvhttp_error_t 值

---

### 底层 WebSocket API

#### uvhttp_ws_connection_create
```c
struct uvhttp_ws_connection* uvhttp_ws_connection_create(int fd,
                                                         mbedtls_ssl_context* ssl,
                                                         int is_server);
```
创建 WebSocket 连接。

**参数:**
- `fd`: 文件描述符
- `ssl`: SSL 上下文
- `is_server`: 是否为服务器端

**返回值:**
- 成功: WebSocket 连接指针
- 失败: NULL

---

#### uvhttp_ws_send_text
```c
int uvhttp_ws_send_text(struct uvhttp_ws_connection* conn,
                        const char* text,
                        size_t len);
```
发送文本消息。

**参数:**
- `conn`: WebSocket 连接
- `text`: 文本数据
- `len`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_send_binary
```c
int uvhttp_ws_send_binary(struct uvhttp_ws_connection* conn,
                          const uint8_t* data,
                          size_t len);
```
发送二进制消息。

**参数:**
- `conn`: WebSocket 连接
- `data`: 二进制数据
- `len`: 数据长度

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_close
```c
int uvhttp_ws_close(struct uvhttp_ws_connection* conn,
                    int code,
                    const char* reason);
```
关闭 WebSocket 连接。

**参数:**
- `conn`: WebSocket 连接
- `code`: 关闭代码
- `reason`: 关闭原因

**返回值:**
- 成功: 0
- 失败: 负数错误码

---

#### uvhttp_ws_set_callbacks
```c
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection* conn,
                             uvhttp_ws_on_message_callback on_message,
                             uvhttp_ws_on_close_callback on_close,
                             uvhttp_ws_on_error_callback on_error);
```
设置 WebSocket 回调函数。

**参数:**
- `conn`: WebSocket 连接
- `on_message`: 消息回调
- `on_close`: 关闭回调
- `on_error`: 错误回调

---

### WebSocket 类型定义

#### WebSocket 操作码
```c
typedef enum {
    UVHTTP_WS_OPCODE_CONTINUATION = 0x0,
    UVHTTP_WS_OPCODE_TEXT = 0x1,
    UVHTTP_WS_OPCODE_BINARY = 0x2,
    UVHTTP_WS_OPCODE_CLOSE = 0x8,
    UVHTTP_WS_OPCODE_PING = 0x9,
    UVHTTP_WS_OPCODE_PONG = 0xA
} uvhttp_ws_opcode_t;
```

#### WebSocket 处理器
```c
typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg);
    void* user_data;
} uvhttp_ws_handler_t;
```

### 使用示例

#### 基本 WebSocket 服务器
```c
void on_ws_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    if (opcode == UVHTTP_WS_OPCODE_TEXT) {
        // 回显消息
        uvhttp_server_ws_send(ws_conn, data, len);
    }
}

void on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket 连接关闭\n");
}

void on_ws_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    fprintf(stderr, "WebSocket 错误: %s\n", error_msg);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    uvhttp_ws_handler_t ws_handler = {
        .on_connect = NULL,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .on_error = on_ws_error,
        .user_data = NULL
    };

    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

---

## 错误处理 API

### 错误码

```c
typedef enum {
    UVHTTP_OK = 0,

    /* General errors */
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NOT_FOUND = -3,
    UVHTTP_ERROR_ALREADY_EXISTS = -4,
    UVHTTP_ERROR_NULL_POINTER = -5,
    UVHTTP_ERROR_BUFFER_TOO_SMALL = -6,
    UVHTTP_ERROR_TIMEOUT = -7,
    UVHTTP_ERROR_CANCELLED = -8,

    /* Server errors */
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_SERVER_LISTEN = -101,
    UVHTTP_ERROR_SERVER_STOP = -102,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,
    UVHTTP_ERROR_SERVER_ALREADY_RUNNING = -104,
    UVHTTP_ERROR_SERVER_NOT_RUNNING = -105,
    UVHTTP_ERROR_SERVER_INVALID_CONFIG = -106,

    /* Connection errors */
    UVHTTP_ERROR_CONNECTION_INIT = -200,
    UVHTTP_ERROR_CONNECTION_ACCEPT = -201,
    UVHTTP_ERROR_CONNECTION_START = -202,
    UVHTTP_ERROR_CONNECTION_CLOSE = -203,
    UVHTTP_ERROR_CONNECTION_RESET = -204,
    UVHTTP_ERROR_CONNECTION_TIMEOUT = -205,
    UVHTTP_ERROR_CONNECTION_REFUSED = -206,
    UVHTTP_ERROR_CONNECTION_BROKEN = -207,

    /* Request/Response errors */
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_INIT = -301,
    UVHTTP_ERROR_RESPONSE_SEND = -302,
    UVHTTP_ERROR_INVALID_HTTP_METHOD = -303,
    UVHTTP_ERROR_INVALID_HTTP_VERSION = -304,
    UVHTTP_ERROR_HEADER_TOO_LARGE = -305,
    UVHTTP_ERROR_BODY_TOO_LARGE = -306,
    UVHTTP_ERROR_MALFORMED_REQUEST = -307,

    /* TLS errors */
    UVHTTP_ERROR_TLS_INIT = -400,
    UVHTTP_ERROR_TLS_CONTEXT = -401,
    UVHTTP_ERROR_TLS_HANDSHAKE = -402,
    UVHTTP_ERROR_TLS_CERT_LOAD = -403,
    UVHTTP_ERROR_TLS_KEY_LOAD = -404,
    UVHTTP_ERROR_TLS_VERIFY_FAILED = -405,
    UVHTTP_ERROR_TLS_EXPIRED = -406,
    UVHTTP_ERROR_TLS_NOT_YET_VALID = -407,

    /* Router errors */
    UVHTTP_ERROR_ROUTER_INIT = -500,
    UVHTTP_ERROR_ROUTER_ADD = -501,
    UVHTTP_ERROR_ROUTE_NOT_FOUND = -502,
    UVHTTP_ERROR_ROUTE_ALREADY_EXISTS = -503,
    UVHTTP_ERROR_INVALID_ROUTE_PATTERN = -504,

    /* Rate limit errors */
    UVHTTP_ERROR_RATE_LIMIT_EXCEEDED = -550,

    /* Allocator errors */
    UVHTTP_ERROR_ALLOCATOR_INIT = -600,
    UVHTTP_ERROR_ALLOCATOR_SET = -601,
    UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED = -602,

    /* WebSocket errors */
    UVHTTP_ERROR_WEBSOCKET_INIT = -700,
    UVHTTP_ERROR_WEBSOCKET_HANDSHAKE = -701,
    UVHTTP_ERROR_WEBSOCKET_FRAME = -702,
    UVHTTP_ERROR_WEBSOCKET_TOO_LARGE = -703,
    UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE = -704,
    UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED = -705,
    UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED = -706,
    UVHTTP_ERROR_WEBSOCKET_CLOSED = -707,

    /* HTTP/2 errors */
    UVHTTP_ERROR_HTTP2_INIT = -800,
    UVHTTP_ERROR_HTTP2_STREAM = -801,
    UVHTTP_ERROR_HTTP2_SETTINGS = -802,
    UVHTTP_ERROR_HTTP2_FLOW_CONTROL = -803,
    UVHTTP_ERROR_HTTP2_HEADER_COMPRESS = -804,
    UVHTTP_ERROR_HTTP2_PRIORITY = -805,

    /* Configuration errors */
    UVHTTP_ERROR_CONFIG_PARSE = -900,
    UVHTTP_ERROR_CONFIG_INVALID = -901,
    UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND = -902,
    UVHTTP_ERROR_CONFIG_MISSING_REQUIRED = -903,

    /* Middleware errors */
    UVHTTP_ERROR_MIDDLEWARE_INIT = -1000,
    UVHTTP_ERROR_MIDDLEWARE_REGISTER = -1001,
    UVHTTP_ERROR_MIDDLEWARE_EXECUTE = -1002,
    UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND = -1003,

    /* Logging errors */
    UVHTTP_ERROR_LOG_INIT = -1100,
    UVHTTP_ERROR_LOG_WRITE = -1101,
    UVHTTP_ERROR_LOG_FILE_OPEN = -1102,
    UVHTTP_ERROR_LOG_NOT_INITIALIZED = -1103,

    UVHTTP_ERROR_MAX
} uvhttp_error_t;
```

### 错误查询

#### uvhttp_error_string
```c
const char* uvhttp_error_string(uvhttp_error_t error);
```
获取错误名称字符串。

**参数:**
- `error`: 错误码

**返回值:**
- 错误名称字符串

---

#### uvhttp_error_category_string
```c
const char* uvhttp_error_category_string(uvhttp_error_t error);
```
获取错误分类字符串。

**参数:**
- `error`: 错误码

**返回值:**
- 错误分类字符串（如 "Server", "Connection", "Request" 等）

---

#### uvhttp_error_description
```c
const char* uvhttp_error_description(uvhttp_error_t error);
```
获取错误描述。

**参数:**
- `error`: 错误码

**返回值:**
- 错误描述字符串

---

#### uvhttp_error_suggestion
```c
const char* uvhttp_error_suggestion(uvhttp_error_t error);
```
获取修复建议。

**参数:**
- `error`: 错误码

**返回值:**
- 修复建议字符串

---

#### uvhttp_error_is_recoverable
```c
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```
检查错误是否可恢复。

**参数:**
- `error`: 错误码

**返回值:**
- 1: 可恢复
- 0: 不可恢复

---

### 错误恢复和重试

#### uvhttp_set_error_recovery_config
```c
void uvhttp_set_error_recovery_config(int max_retries,
                                     int base_delay_ms,
                                     int max_delay_ms,
                                     double backoff_multiplier);
```
配置错误重试策略。

**参数:**
- `max_retries`: 最大重试次数
- `base_delay_ms`: 基础延迟（毫秒）
- `max_delay_ms`: 最大延迟（毫秒）
- `backoff_multiplier`: 退避乘数

---

#### uvhttp_retry_operation
```c
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*),
                                     void* context,
                                     const char* operation_name);
```
执行带重试的操作。

**参数:**
- `operation`: 操作函数
- `context`: 上下文数据
- `operation_name`: 操作名称（用于日志）

**返回值:**
- 成功: UVHTTP_OK
- 失败: 错误码

---

### 错误日志和统计

#### uvhttp_log_error
```c
void uvhttp_log_error(uvhttp_error_t error, const char* context);
```
记录错误日志。

**参数:**
- `error`: 错误码
- `context`: 上下文信息

---

#### uvhttp_get_error_stats
```c
void uvhttp_get_error_stats(uvhttp_context_t* context,
                           size_t* error_counts,
                           time_t* last_error_time,
                           const char** last_error_context);
```
获取错误统计信息。

**参数:**
- `context`: 应用上下文
- `error_counts`: 错误计数数组（输出）
- `last_error_time`: 最后错误时间（输出）
- `last_error_context`: 最后错误上下文（输出）

---

#### uvhttp_reset_error_stats
```c
void uvhttp_reset_error_stats(uvhttp_context_t* context);
```
重置错误统计信息。

**参数:**
- `context`: 应用上下文

---

#### uvhttp_get_most_frequent_error
```c
uvhttp_error_t uvhttp_get_most_frequent_error(uvhttp_context_t* context);
```
获取最频繁的错误。

**参数:**
- `context`: 应用上下文

**返回值:**
- 最频繁的错误码

---

### 使用示例

#### 基本错误处理
```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "分类: %s\n", uvhttp_error_category_string(result));
    fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "建议: %s\n", uvhttp_error_suggestion(result));
    return 1;
}
```

#### 错误恢复
```c
uvhttp_set_error_recovery_config(3, 100, 5000, 2.0);

uvhttp_error_t connect_operation(void* ctx) {
    return uvhttp_server_connect((uvhttp_server_t*)ctx);
}

uvhttp_error_t result = uvhttp_retry_operation(connect_operation, server, "连接服务器");
if (result != UVHTTP_OK) {
    fprintf(stderr, "连接失败，已重试多次\n");
}
```

#### 错误统计
```c
size_t error_counts[UVHTTP_ERROR_COUNT] = {0};
time_t last_error_time = 0;
const char* last_error_context = NULL;

uvhttp_get_error_stats(context, error_counts, &last_error_time, &last_error_context);

uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error(context);
printf("最频繁的错误: %s (%zu 次)\n",
       uvhttp_error_string(most_frequent),
       error_counts[most_frequent]);
```

---

## 配置管理

### 配置结构体

#### uvhttp_config_t
```c
typedef struct {
    /* 服务器配置 */
    int max_connections;              // 最大并发连接数 (1-10000)
    int max_requests_per_connection;  // 每个连接的最大请求数
    
    /* 性能配置 */
    size_t max_body_size;             // 最大请求体大小
    size_t max_header_size;           // 最大请求头大小
    
    /* 安全配置 */
    int rate_limit_window;            // 速率限制窗口时间(秒)
    
    /* 其他配置... */
} uvhttp_config_t;
```

### 配置管理函数

#### uvhttp_config_new
```c
uvhttp_config_t* uvhttp_config_new(void);
```
创建新的配置实例。

**返回值:**
- 成功: 配置实例指针
- 失败: NULL

#### uvhttp_config_set_defaults
```c
void uvhttp_config_set_defaults(uvhttp_config_t* config);
```
设置默认配置值。

**默认值:**
- `max_connections`: 2048
- `max_requests_per_connection`: 100
- `max_body_size`: 1MB
- `max_header_size`: 8KB

#### uvhttp_config_update_max_connections
```c
int uvhttp_config_update_max_connections(uvhttp_context_t* context, int max_connections);
```
动态更新最大并发连接数。

**参数:**
- `context`: 应用上下文
- `max_connections`: 新的最大连接数 (1-10000)

**返回值:**
- `UVHTTP_OK`: 更新成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数超出范围
- `UVHTTP_ERROR_INVALID_STATE`: 配置未初始化

**示例:**
```c
// 运行时动态调整连接限制
int result = uvhttp_config_update_max_connections(context, 5000);
if (result == UVHTTP_OK) {
    printf("连接限制已更新为5000\n");
}
```

#### uvhttp_config_load_file
```c
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename);
```
从配置文件加载设置。

**配置文件格式:**
```
max_connections=3000
max_requests_per_connection=200
max_body_size=2097152
max_header_size=16384
```

#### uvhttp_config_load_env
```c
int uvhttp_config_load_env(uvhttp_config_t* config);
```
从环境变量加载配置。

**支持的环境变量:**
- `UVHTTP_MAX_CONNECTIONS`: 最大连接数
- `UVHTTP_MAX_REQUESTS_PER_CONNECTION`: 每连接最大请求数

**示例:**
```bash
export UVHTTP_MAX_CONNECTIONS=4000
export UVHTTP_MAX_REQUESTS_PER_CONNECTION=150
./your_server
```

#### uvhttp_config_validate
```c
int uvhttp_config_validate(const uvhttp_config_t* config);
```
验证配置参数的有效性。

**返回值:**
- `UVHTTP_OK`: 配置有效
- `UVHTTP_ERROR_INVALID_PARAM`: 存在无效参数

#### uvhttp_config_get_current
```c
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);
```
获取当前全局配置实例。

**参数:**
- `context`: 应用上下文

**返回值:**
- 成功: 配置实例指针
- 失败: NULL

#### uvhttp_config_monitor_changes
```c
int uvhttp_config_monitor_changes(uvhttp_context_t* context, uvhttp_config_change_callback_t callback);
```
监控配置变化。

**参数:**
- `context`: 应用上下文
- `callback`: 配置变化回调函数

**回调函数类型:**
```c
typedef void (*uvhttp_config_change_callback_t)(const char* key,
                                                 const void* old_value,
                                                 const void* new_value);
```

**示例:**
```c
void on_config_change(const char* key, const void* old_value, const void* new_value) {
    if (strcmp(key, "max_connections") == 0) {
        printf("最大连接数从 %d 变更为 %d\n",
               *(int*)old_value, *(int*)new_value);
    }
}

uvhttp_config_monitor_changes(context, on_config_change);
```

### 配置常量

#### 连接相关常量
```c
#define UVHTTP_MAX_CONNECTIONS           2048    // 编译时默认最大连接数
#define UVHTTP_DEFAULT_MAX_CONNECTIONS   2048    // 运行时默认最大连接数
#define UVHTTP_DEFAULT_BACKLOG           1024    // 监听队列大小
```

### 错误处理

#### 连接限制错误
```c
#define UVHTTP_ERROR_CONNECTION_LIMIT    -103    // 连接数达到上限
```

当活动连接数达到 `max_connections` 限制时，服务器会：
1. 记录警告日志
2. 拒绝新连接
3. 返回 `UVHTTP_ERROR_CONNECTION_LIMIT` 错误

### 使用示例

#### 基本配置使用
```c
#include "uvhttp.h"
#include "uvhttp_config.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建配置
    uvhttp_config_t* config = uvhttp_config_new();
    uvhttp_config_set_defaults(config);
    
    // 自定义连接限制
    config->max_connections = 3000;
    config->max_requests_per_connection = 200;
    
    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        fprintf(stderr, "配置验证失败\n");
        return 1;
    }
    
    // 应用配置
    server->config = config;
    
    // 启动服务器
    uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

#### 动态配置调整
```c
void adjust_connections_based_on_load() {
    // 获取当前配置
    const uvhttp_config_t* current = uvhttp_config_get_current();
    
    // 根据系统负载动态调整
    if (system_load > 0.8) {
        // 高负载时降低连接数
        uvhttp_config_update_max_connections(current->max_connections * 0.8);
    } else if (system_load < 0.3) {
        // 低负载时增加连接数
        uvhttp_config_update_max_connections(current->max_connections * 1.2);
    }
}
```

#### 配置文件和环境变量
```c
int load_config_from_multiple_sources(uvhttp_config_t* config) {
    // 1. 设置默认值
    uvhttp_config_set_defaults(config);
    
    // 2. 从配置文件加载
    if (uvhttp_config_load_file(config, "uvhttp.conf") != UVHTTP_OK) {
        printf("配置文件加载失败，使用默认配置\n");
    }
    
    // 3. 从环境变量加载 (会覆盖配置文件设置)
    if (uvhttp_config_load_env(config) != UVHTTP_OK) {
        printf("环境变量加载失败\n");
    }
    
    // 4. 验证最终配置
    return uvhttp_config_validate(config);
}
```

---

### 错误恢复

#### uvhttp_set_error_recovery_config
```c
void uvhttp_set_error_recovery_config(int max_retries, 
                                     int base_delay_ms, 
                                     int max_delay_ms, 
                                     double backoff_multiplier);
```
配置错误重试策略。

---

#### uvhttp_retry_operation
```c
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*), 
                                     void* context, 
                                     const char* operation_name);
```
执行带重试的操作。

---

## 内存管理 API

### 基本操作

```c
void* uvhttp_malloc(size_t size);
void* uvhttp_realloc(void* ptr, size_t size);
void uvhttp_free(void* ptr);
void* uvhttp_calloc(size_t nmemb, size_t size);
```

### 内存统计 (调试模式)

```c
void uvhttp_get_memory_stats(size_t* total_allocated, 
                             size_t* current_allocated,
                             size_t* allocation_count, 
                             size_t* free_count);
int uvhttp_check_memory_leaks(void);
void uvhttp_reset_memory_stats(void);
```

---

## 工具函数 API

### 字符串处理

#### uvhttp_safe_strcpy
```c
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);
```
安全的字符串复制。

---

#### uvhttp_url_decode
```c
int uvhttp_url_decode(const char* src, char* dest, size_t dest_size);
```
URL 解码。

---

### 哈希函数

#### uvhttp_hash
```c
uint64_t uvhttp_hash(const void* data, size_t length, uint64_t seed);
```
计算数据的64位哈希值（基于xxHash算法）。

**参数:**
- `data`: 要哈希的数据
- `length`: 数据长度
- `seed`: 哈希种子

**返回值:**
- 64位哈希值

**示例:**
```c
const char* data = "Hello, World!";
uint64_t hash = uvhttp_hash(data, strlen(data), 0x12345678);
```

---

#### uvhttp_hash_string
```c
uint64_t uvhttp_hash_string(const char* str);
```
计算字符串的64位哈希值（使用默认种子）。

**参数:**
- `str`: 要哈希的字符串

**返回值:**
- 64位哈希值

**示例:**
```c
uint64_t hash = uvhttp_hash_string("user_session_token");
```

---

#### uvhttp_hash_default
```c
uint64_t uvhttp_hash_default(const void* data, size_t length);
```
使用默认种子计算数据的哈希值。

**参数:**
- `data`: 要哈希的数据
- `length`: 数据长度

**返回值:**
- 64位哈希值

---

### 编码转换

#### uvhttp_base64_encode
```c
int uvhttp_base64_encode(const unsigned char* input, 
                        size_t input_length,
                        char* output, 
                        size_t output_size);
```
Base64 编码。

---

#### uvhttp_base64_decode
```c
int uvhttp_base64_decode(const char* input, 
                        unsigned char* output, 
                        size_t output_size);
```
Base64 解码。

---

### 验证函数

#### uvhttp_validate_header_value
```c
int uvhttp_validate_header_value(const char* name, const char* value);
```
验证 HTTP 头部值。

---

## TLS/SSL API

### TLS 配置

```c
typedef struct {
    const char* cert_file;
    const char* key_file;
    const char* ca_file;
    int verify_peer;
} uvhttp_tls_config_t;
```

#### uvhttp_server_enable_tls
```c
int uvhttp_server_enable_tls(uvhttp_server_t* server, 
                            const uvhttp_tls_config_t* config);
```
启用 TLS 支持。

---

## 中间件 API

### CORS 中间件

```c
void uvhttp_cors_middleware(uvhttp_request_t* request, 
                           uvhttp_response_t* response,
                           const char* allowed_origins,
                           const char* allowed_methods,
                           const char* allowed_headers);
```

### 限流中间件

```c
void uvhttp_rate_limit_middleware(uvhttp_request_t* request, 
                                 uvhttp_response_t* response,
                                 int max_requests_per_minute);
```

---

## 常量定义

### HTTP 状态码

```c
#define UVHTTP_STATUS_OK                 200
#define UVHTTP_STATUS_CREATED            201
#define UVHTTP_STATUS_NO_CONTENT          204
#define UVHTTP_STATUS_BAD_REQUEST        400
#define UVHTTP_STATUS_UNAUTHORIZED       401
#define UVHTTP_STATUS_FORBIDDEN           403
#define UVHTTP_STATUS_NOT_FOUND           404
#define UVHTTP_STATUS_INTERNAL_ERROR      500
```

### 内容类型

```c
#define UVHTTP_CONTENT_TYPE_TEXT         "text/plain"
#define UVHTTP_CONTENT_TYPE_HTML         "text/html"
#define UVHTTP_CONTENT_TYPE_JSON         "application/json"
#define UVHTTP_CONTENT_TYPE_XML          "application/xml"
```

### 限制常量

```c
#define UVHTTP_MAX_CONNECTIONS           2048
#define UVHTTP_MAX_BODY_SIZE             (1024 * 1024)  // 1MB
#define UVHTTP_MAX_HEADERS               64
#define UVHTTP_MAX_URL_SIZE              2048
```

### 网络和服务器配置

```c
#define UVHTTP_DEFAULT_PORT              8080
#define UVHTTP_DEFAULT_HOST              "0.0.0.0"
```

### 缓冲区大小配置

```c
#define UVHTTP_DIR_LISTING_BUFFER_SIZE   4096
#define UVHTTP_DIR_ENTRY_HTML_OVERHEAD   200
#define UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN 256
```

### 连接和池配置

```c
#define UVHTTP_DEFAULT_CONNECTION_POOL_SIZE 100
#define UVHTTP_ROUTER_MAX_CHILDREN       16
```

### WebSocket配置

```c
#define UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE 1024
```

### 错误恢复配置

```c
#define UVHTTP_DEFAULT_BASE_DELAY_MS     100
#define UVHTTP_DEFAULT_MAX_DELAY_MS      5000
```

---

## 使用示例

### 基本 HTTP 服务器

```c
#include "uvhttp.h"

void handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello", 5);
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### WebSocket 服务器

```c
void ws_handler(uvhttp_websocket_t* ws, 
               const uvhttp_websocket_message_t* msg, 
               void* user_data) {
    if (msg->type == UVHTTP_WEBSOCKET_TEXT) {
        uvhttp_websocket_send_text(ws, "Echo: ");
        uvhttp_websocket_send(ws, msg->data, msg->length, msg->type);
    }
}

void upgrade_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_websocket_t* ws = uvhttp_websocket_new(req, res);
    uvhttp_websocket_set_handler(ws, ws_handler, NULL);
}
```

---

## 版本信息

```c
#define UVHTTP_VERSION_MAJOR    2
#define UVHTTP_VERSION_MINOR    0
#define UVHTTP_VERSION_PATCH    0
#define UVHTTP_VERSION_STRING   "2.0.0"
```