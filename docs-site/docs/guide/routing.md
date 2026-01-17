# 路由

## 概述

UVHTTP 提供灵活的路由系统，支持路径匹配、参数提取、HTTP 方法匹配等功能。

## 基本路由

### 简单路由

```c
#include <uvhttp.h>

void home_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, "Home Page");
    uvhttp_response_send(res);
}

void about_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, "About Page");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/about", about_handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 路由参数

### 命名参数

```c
void user_handler(uvhttp_request_t* req) {
    const char* user_id = uvhttp_request_get_param(req, "id");

    uvhttp_response_t* res = uvhttp_response_new(req);
    char body[256];
    snprintf(body, sizeof(body), "User ID: %s", user_id);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, body);
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    // 添加带参数的路由
    uvhttp_router_add_route(router, "/users/:id", user_handler);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### 多个参数

```c
void post_handler(uvhttp_request_t* req) {
    const char* user_id = uvhttp_request_get_param(req, "user_id");
    const char* post_id = uvhttp_request_get_param(req, "post_id");

    uvhttp_response_t* res = uvhttp_response_new(req);
    char body[256];
    snprintf(body, sizeof(body), "User: %s, Post: %s", user_id, post_id);

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, body);
    uvhttp_response_send(res);
}

// 添加路由
uvhttp_router_add_route(router, "/users/:user_id/posts/:post_id", post_handler);
```

## HTTP 方法

### 方法匹配

```c
void get_handler(uvhttp_request_t* req) {
    if (uvhttp_request_get_method(req) != UVHTTP_METHOD_GET) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 405);
        uvhttp_response_set_body(res, "Method Not Allowed");
        uvhttp_response_send(res);
        return;
    }
    // 处理 GET 请求
}

void post_handler(uvhttp_request_t* req) {
    if (uvhttp_request_get_method(req) != UVHTTP_METHOD_POST) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 405);
        uvhttp_response_set_body(res, "Method Not Allowed");
        uvhttp_response_send(res);
        return;
    }
    // 处理 POST 请求
}
```

### RESTful 路由

```c
// GET /users - 获取用户列表
void get_users(uvhttp_request_t* req);
uvhttp_router_add_route(router, "/users", get_users);

// GET /users/:id - 获取单个用户
void get_user(uvhttp_request_t* req);
uvhttp_router_add_route(router, "/users/:id", get_user);

// POST /users - 创建用户
void create_user(uvhttp_request_t* req);
uvhttp_router_add_route(router, "/users", create_user);

// PUT /users/:id - 更新用户
void update_user(uvhttp_request_t* req);
uvhttp_router_add_route(router, "/users/:id", update_user);

// DELETE /users/:id - 删除用户
void delete_user(uvhttp_request_t* req);
uvhttp_router_add_route(router, "/users/:id", delete_user);
```

## 通配符路由

### 前缀匹配

```c
void static_handler(uvhttp_request_t* req) {
    const char* path = uvhttp_request_get_path(req);
    // 处理静态文件
}

// 匹配 /static/* 下的所有路径
uvhttp_router_add_route(router, "/static/*", static_handler);
```

### 后缀匹配

```c
void api_handler(uvhttp_request_t* req) {
    // 处理 API 请求
}

// 匹配所有以 /api 开头的路径
uvhttp_router_add_route(router, "/api/*", api_handler);
```

## 路由组

### 组织路由

```c
// API 路由组
void api_v1_users(uvhttp_request_t* req);
void api_v1_posts(uvhttp_request_t* req);

uvhttp_router_add_route(router, "/api/v1/users", api_v1_users);
uvhttp_router_add_route(router, "/api/v1/posts", api_v1_posts);

// Admin 路由组
void admin_dashboard(uvhttp_request_t* req);
void admin_users(uvhttp_request_t* req);

uvhttp_router_add_route(router, "/admin/dashboard", admin_dashboard);
uvhttp_router_add_route(router, "/admin/users", admin_users);
```

## 路由优先级

路由按照添加顺序匹配，更具体的路由应该先添加：

```c
// 先添加具体的路由
uvhttp_router_add_route(router, "/users/profile", profile_handler);

// 后添加通配路由
uvhttp_router_add_route(router, "/users/*", users_handler);
```

## 路由缓存

### 启用路由缓存

```bash
cmake -DUVHTTP_FEATURE_ROUTER_CACHE=ON ..
```

路由缓存可以显著提高路由匹配性能，特别是对于大量路由的情况。

## 路由性能

### 优化建议

1. **使用具体路由而非通配符**
   ```c
   // 推荐
   uvhttp_router_add_route(router, "/api/users", users_handler);

   // 避免
   uvhttp_router_add_route(router, "/api/*", api_handler);
   ```

2. **启用路由缓存**
   ```bash
   cmake -DUVHTTP_FEATURE_ROUTER_CACHE=ON ..
   ```

3. **减少路由数量**
   - 合并相似的路由
   - 使用参数提取

### 性能对比

| 路由类型 | 匹配时间 | 相对性能 |
|---------|---------|---------|
| 精确匹配 | ~10ns | 100% |
| 参数匹配 | ~50ns | 80% |
| 通配符匹配 | ~200ns | 20% |

## 路由错误处理

### 404 Not Found

```c
void not_found_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 404);
    uvhttp_response_set_body(res, "Not Found");
    uvhttp_response_send(res);
}

// 设置默认处理器
uvhttp_router_set_default_handler(router, not_found_handler);
```

### 405 Method Not Allowed

```c
void method_not_allowed_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 405);
    uvhttp_response_set_body(res, "Method Not Allowed");
    uvhttp_response_send(res);
}
```

## 高级路由

### 条件路由

```c
void conditional_handler(uvhttp_request_t* req) {
    const char* user_agent = uvhttp_request_get_header(req, "User-Agent");

    if (strstr(user_agent, "mobile")) {
        // 移动端处理
    } else {
        // 桌面端处理
    }
}
```

### 路由重定向

```c
void redirect_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 301);
    uvhttp_response_set_header(res, "Location", "/new-path");
    uvhttp_response_send(res);
}
```

## 示例项目

### 完整的 RESTful API

```c
#include <uvhttp.h>

void get_users(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "[{\"id\":1,\"name\":\"Alice\"},{\"id\":2,\"name\":\"Bob\"}]");
    uvhttp_response_send(res);
}

void get_user(uvhttp_request_t* req) {
    const char* id = uvhttp_request_get_param(req, "id");
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");

    char body[256];
    snprintf(body, sizeof(body), "{\"id\":%s,\"name\":\"User %s\"}", id, id);

    uvhttp_response_set_body(res, body);
    uvhttp_response_send(res);
}

void create_user(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "{\"id\":3,\"name\":\"Charlie\"}");
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

    uvhttp_router_add_route(router, "/users", get_users);
    uvhttp_router_add_route(router, "/users/:id", get_user);
    uvhttp_router_add_route(router, "/users", create_user);

    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

## 更多资源

- [API 文档](/api/router)
- [中间件](/guide/middleware)
- [最佳实践](/guide/best-practices)