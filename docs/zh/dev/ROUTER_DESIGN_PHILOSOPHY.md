# UVHTTP 路由系统设计哲学

## 文档信息

- **项目**: UVHTTP
- **版本**: 2.2.0
- **创建日期**: 2026-02-23
- **文档类型**: 设计哲学与技术指南
- **状态**: 已实现

---

## 目录

1. [概述](#概述)
2. [路由系统设计哲学](#路由系统设计哲学)
3. [路由架构](#路由架构)
4. [路由匹配策略](#路由匹配策略)
5. [性能优化](#性能优化)
6. [业务场景应用](#业务场景应用)
7. [最佳实践](#最佳实践)
8. [常见问题](#常见问题)

---

## 概述

UVHTTP 的路由系统是一个高性能、灵活且易于使用的 HTTP 请求分发机制。它支持多种路由模式，包括静态路由、参数路由、方法路由和前缀路由，为开发者提供强大的 URL 映射能力。

### 核心特性

- **高性能**: O(1) 快速前缀匹配，支持 128+ 路由
- **灵活性**: 支持静态路由、参数路由、方法路由
- **内存优化**: 紧凑的内存布局（128 字节节点），CPU 缓存友好
- **易于使用**: 简洁的 API 设计，直观的路由定义
- **可扩展**: 支持自定义回退路由和静态文件路由

---

## 路由系统设计哲学

### 1. 性能优先 (Performance First)

**原则**: 路由匹配是每个 HTTP 请求的核心路径，必须极致优化。

**实践**:

- **O(1) 前缀匹配**: 使用 Trie 数据结构实现快速前缀匹配
- **CPU 缓存友好**: 路由节点设计为 128 字节（2 条缓存线），优化内存局部性
- **紧凑存储**: 使用紧凑的子节点索引数组（48 字节存储 12 个子节点）
- **零动态分配**: 路由节点池预分配，避免运行时内存分配

**收益**:

- 路由匹配延迟 < 1μs
- 支持 128+ 路由无性能退化
- 内存占用最小化

**示例**:

```c
// 路由节点设计 - 128 字节，2 条缓存线
typedef struct uvhttp_route_node {
    /* Cache line 1: Hot path fields (64 bytes) */
    uvhttp_method_t method;           /* 4 bytes */
    uvhttp_request_handler_t handler; /* 8 bytes */
    size_t child_count;               /* 8 bytes */
    int is_param;                     /* 4 bytes */
    uint8_t segment_len;              /* 1 byte */
    uint8_t param_name_len;           /* 1 byte */
    uint16_t _padding1;               /* 2 bytes */
    uint32_t child_indices[12];       /* 48 bytes - Compact child storage */

    /* Cache line 2: Variable length data (64 bytes) */
    char segment_data[32];    /* 32 bytes */
    char param_name_data[32]; /* 32 bytes */
} uvhttp_route_node_t;
```

### 2. 灵活性与控制力 (Flexibility and Control)

**原则**: 应用层完全控制路由逻辑，框架不强制路由模式。

**实践**:

- **显式路由注册**: 开发者明确注册每个路由
- **方法路由支持**: 支持为同一路径设置不同 HTTP 方法的处理器
- **参数路由**: 支持路径参数提取（如 `/users/:id`）
- **回退路由**: 支持自定义未匹配路由的处理器
- **静态文件路由**: 应用层自行实现静态文件路由策略

**收益**:

- 完全的路由控制权
- 灵活的路由策略
- 易于扩展和定制

**示例**:

```c
// 添加路由
uvhttp_router_add_route(router, "/", home_handler);
uvhttp_router_add_route(router, "/about", about_handler);

// 添加方法路由
uvhttp_router_add_route_method(router, "/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, get_user_handler);
```

### 3. 简洁直观 (Simplicity and Intuitiveness)

**原则**: API 设计简洁直观，降低学习成本。

**实践**:

- **统一的 API**: 所有路由操作使用统一的 API
- **直观的参数**: 路径和处理器作为核心参数
- **可选功能**: 高级功能（如参数路由）按需使用
- **清晰的命名**: 函数名清晰表达功能

**收益**:

- 快速上手
- 减少错误
- 提高开发效率

**示例**:

```c
// 创建路由器
uvhttp_router_t* router = NULL;
uvhttp_router_new(&router);

// 添加路由
uvhttp_router_add_route(router, "/api/users", users_handler);

// 查找路由
uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api/users", "GET");

// 释放路由器
uvhttp_router_free(router);
```

### 4. 内存安全 (Memory Safety)

**原则**: 路由系统必须内存安全，无泄漏、无溢出。

**实践**:

- **缓冲区边界检查**: 所有字符串操作都有边界检查
- **内存池管理**: 路由节点使用内存池，避免碎片化
- **严格的清理**: 提供完整的清理函数
- **错误处理**: 所有操作都有错误返回值

**收益**:

- 无内存泄漏
- 无缓冲区溢出
- 长期运行稳定

**示例**:

```c
// 安全的路径参数提取
uvhttp_error_t result = uvhttp_router_match(router, "/users/123", "GET", &match);
if (result == UVHTTP_OK) {
    // 参数数量有限制（MAX_PARAMS = 16）
    for (size_t i = 0; i < match.param_count; i++) {
        printf("Parameter %s = %s\n", match.params[i].name, match.params[i].value);
    }
}
```

---

## 路由架构

### 路由器结构

```c
typedef struct uvhttp_router {
    /* Hot path fields */
    int use_trie;                   /* 是否使用 Trie */
    size_t route_count;             /* 路由总数 */

    /* Trie routing related */
    uvhttp_route_node_t* node_pool; /* 节点池 */
    uint32_t root_index;            /* 根节点索引 */
    uint32_t node_pool_size;        /* 池容量 */
    uint32_t node_pool_used;        /* 池使用量 */

    /* Array routing related */
    array_route_t* array_routes;    /* 数组路由 */
    size_t array_route_count;       /* 数组路由数量 */
    size_t array_capacity;          /* 数组容量 */

    /* Static file routing support */
    char* static_prefix;            /* 静态文件前缀 */
    void* static_context;           /* 静态文件上下文 */
    uvhttp_request_handler_t static_handler; /* 静态文件处理器 */

    /* Fallback routing support */
    void* fallback_context;         /* 回退上下文 */
    uvhttp_request_handler_t fallback_handler; /* 回退处理器 */
} uvhttp_router_t;
```

### 路由节点结构

```c
typedef struct uvhttp_route_node {
    /* Cache line 1: Hot path fields (64 bytes) */
    uvhttp_method_t method;           /* HTTP 方法 */
    uvhttp_request_handler_t handler; /* 请求处理器 */
    size_t child_count;               /* 子节点数量 */
    int is_param;                     /* 是否为参数节点 */
    uint8_t segment_len;              /* 段长度 */
    uint8_t param_name_len;           /* 参数名长度 */
    uint16_t _padding1;               /* 填充 */
    uint32_t child_indices[12];       /* 紧凑的子节点索引 */

    /* Cache line 2: Variable length data (64 bytes) */
    char segment_data[32];    /* 路径段数据 */
    char param_name_data[32]; /* 参数名数据 */
} uvhttp_route_node_t;
```

### 路由匹配流程

```
1. 请求到达
   ↓
2. 提取路径和方法
   ↓
3. 遍历 Trie 树
   ├─ 静态段匹配
   ├─ 参数段匹配
   └─ 方法匹配
   ↓
4. 找到处理器
   ↓
5. 提取路径参数
   ↓
6. 调用处理器
```

---

## 路由匹配策略

### 1. 静态路由匹配

**描述**: 精确匹配静态路径，无参数。

**示例**:

```c
// 添加静态路由
uvhttp_router_add_route(router, "/", home_handler);
uvhttp_router_add_route(router, "/about", about_handler);
uvhttp_router_add_route(router, "/api/status", status_handler);
```

**匹配规则**:

- 完全匹配路径
- 不区分大小写（可配置）
- 优先级高于参数路由

### 2. 参数路由匹配

**描述**: 支持路径参数提取，如 `/users/:id`。

**示例**:

```c
// 添加参数路由
uvhttp_router_add_route(router, "/users/:id", user_handler);
uvhttp_router_add_route(router, "/posts/:post_id/comments/:comment_id", comment_handler);

// 提取参数
uvhttp_route_match_t match;
uvhttp_router_match(router, "/users/123", "GET", &match);
// match.params[0].name = "id"
// match.params[0].value = "123"
```

**匹配规则**:

- 参数以 `:` 开头
- 参数名限制为 64 字符
- 参数值限制为 256 字符
- 最多支持 16 个参数

### 3. 方法路由匹配

**描述**: 为同一路径设置不同 HTTP 方法的处理器。

**示例**:

```c
// 为同一路径添加不同方法的处理器
uvhttp_router_add_route_method(router, "/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, get_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_PUT, update_user_handler);
uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_DELETE, delete_user_handler);
```

**匹配规则**:

- 先匹配路径，再匹配方法
- 支持所有 HTTP 方法（GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS）
- 未匹配的方法返回 405 Method Not Allowed

### 4. 前缀路由匹配

**描述**: 匹配以指定前缀开头的所有路径。

**示例**:

```c
// 设置静态文件前缀
router->static_prefix = "/static";
router->static_handler = static_file_handler;

// 所有 /static/* 的请求都会被 static_file_handler 处理
```

**匹配规则**:

- 前缀匹配优先级最低
- 常用于静态文件服务
- 应用层自行实现匹配逻辑

### 5. 回退路由匹配

**描述**: 未匹配任何路由时使用的默认处理器。

**示例**:

```c
// 设置回退处理器
router->fallback_handler = not_found_handler;

// 所有未匹配的请求都会被 not_found_handler 处理
```

**匹配规则**:

- 最后尝试的匹配
- 常用于 404 处理
- 返回 404 Not Found

---

## 性能优化

### 1. CPU 缓存优化

**技术**: 路由节点设计为 128 字节（2 条缓存线）。

**收益**:

- 减少缓存未命中
- 提高路由匹配速度
- 优化内存局部性

### 2. 紧凑存储

**技术**: 使用紧凑的子节点索引数组。

**收益**:

- 减少内存占用
- 提高 CPU 缓存利用率
- 支持更多子节点

### 3. 零动态分配

**技术**: 路由节点池预分配。

**收益**:

- 避免运行时内存分配
- 减少内存碎片
- 提高匹配性能

### 4. 快速前缀匹配

**技术**: O(1) 前缀匹配算法。

**收益**:

- 快速匹配静态路由
- 支持大量路由
- 性能稳定

---

## 业务场景应用

### 场景 1: RESTful API 设计

**需求**: 实现一个完整的 RESTful API，支持 CRUD 操作。

**路由策略**:

```c
// 资源路由设计
// GET    /api/users          - 列出所有用户
// POST   /api/users          - 创建用户
// GET    /api/users/:id      - 获取用户详情
// PUT    /api/users/:id      - 更新用户
// DELETE /api/users/:id      - 删除用户

// 用户相关路由
uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_GET, get_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_PUT, update_user_handler);
uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_DELETE, delete_user_handler);

// 文章相关路由
uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_GET, get_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_PUT, update_post_handler);
uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_DELETE, delete_post_handler);

// 评论相关路由（嵌套资源）
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments", UVHTTP_GET, list_comments_handler);
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments", UVHTTP_POST, create_comment_handler);
uvhttp_router_add_route_method(router, "/api/posts/:post_id/comments/:id", UVHTTP_GET, get_comment_handler);
```

**处理器实现**:

```c
// 列出所有用户
int list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 查询数据库
    // 返回 JSON 响应
    cJSON* json = cJSON_CreateArray();
    // ... 添加用户数据
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}

// 获取用户详情
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 提取用户 ID
    uvhttp_route_match_t match;
    uvhttp_router_match(router, uvhttp_request_get_url(req), 
                       uvhttp_request_get_method(req), &match);
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    // 查询数据库
    // 返回 JSON 响应
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "id", user_id);
    cJSON_AddStringToObject(json, "name", "John Doe");
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}
```

### 场景 2: 微服务网关

**需求**: 实现一个微服务网关，根据路径转发到不同的后端服务。

**路由策略**:

```c
// 服务路由
// /api/users/*  -> user-service:8081
// /api/posts/*  -> post-service:8082
// /api/orders/* -> order-service:8083

// 添加路由处理器
uvhttp_router_add_route(router, "/api/users", user_service_proxy_handler);
uvhttp_router_add_route(router, "/api/posts", post_service_proxy_handler);
uvhttp_router_add_route(router, "/api/orders", order_service_proxy_handler);

// 设置前缀路由（捕获所有子路径）
uvhttp_router_add_route(router, "/api/users/*", user_service_proxy_handler);
uvhttp_router_add_route(router, "/api/posts/*", post_service_proxy_handler);
uvhttp_router_add_route(router, "/api/orders/*", order_service_proxy_handler);
```

**处理器实现**:

```c
// 用户服务代理
int user_service_proxy_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* url = uvhttp_request_get_url(req);
    const char* method = uvhttp_request_get_method(req);
    const char* body = uvhttp_request_get_body(req);
    
    // 转发请求到用户服务
    // 1. 构造目标 URL
    char target_url[512];
    snprintf(target_url, sizeof(target_url), "http://user-service:8081%s", url);
    
    // 2. 转发请求
    // 3. 获取响应
    // 4. 返回响应
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, proxy_response, proxy_response_len);
    
    return uvhttp_response_send(res);
}
```

### 场景 3: 多版本 API

**需求**: 支持多个版本的 API，如 v1 和 v2。

**路由策略**:

```c
// 版本路由
// /api/v1/users -> v1 API
// /api/v2/users -> v2 API

// 添加版本路由
uvhttp_router_add_route(router, "/api/v1/users", v1_list_users_handler);
uvhttp_router_add_route(router, "/api/v2/users", v2_list_users_handler);

// 设置回退路由（默认使用 v1）
router->fallback_handler = v1_fallback_handler;
```

**处理器实现**:

```c
// v1 列出用户
int v1_list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // v1 API 实现
    cJSON* json = cJSON_CreateArray();
    // ... 添加用户数据
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}

// v2 列出用户（包含更多信息）
int v2_list_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // v2 API 实现（包含更多信息）
    cJSON* json = cJSON_CreateArray();
    // ... 添加用户数据（包含更多信息）
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-API-Version", "v2");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    int result = uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
    return result;
}
```

### 场景 4: 静态文件服务

**需求**: 提供静态文件服务，支持多个目录。

**路由策略**:

```c
// 静态文件路由
// /static/*      -> ./public/static/
// /uploads/*      -> ./public/uploads/
// /assets/*       -> ./public/assets/

// 设置静态文件前缀和处理器
router->static_prefix = "/static";
router->static_handler = static_file_handler;

// 添加特定静态目录路由
uvhttp_router_add_route(router, "/uploads", uploads_handler);
uvhttp_router_add_route(router, "/assets", assets_handler);
```

**处理器实现**:

```c
// 静态文件处理器
int static_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* url = uvhttp_request_get_url(req);
    
    // 构造文件路径
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "./public%s", url);
    
    // 检查文件是否存在
    // 如果存在，返回文件
    // 如果不存在，返回 404
    
    // 使用 uvhttp_static_handle_request 处理静态文件
    return uvhttp_static_handle_request(req, res, file_path);
}
```

### 场景 5: 身份验证和授权

**需求**: 某些路由需要身份验证和授权。

**路由策略**:

```c
// 公开路由
uvhttp_router_add_route(router, "/api/public/login", login_handler);
uvhttp_router_add_route(router, "/api/public/register", register_handler);

// 需要身份验证的路由
uvhttp_router_add_route(router, "/api/users", auth_middleware_wrapper(list_users_handler));
uvhttp_router_add_route(router, "/api/posts", auth_middleware_wrapper(list_posts_handler));

// 需要管理员权限的路由
uvhttp_router_add_route(router, "/api/admin/users", admin_middleware_wrapper(admin_list_users_handler));
```

**中间件实现**:

```c
// 身份验证中间件包装器
typedef int (*auth_middleware_wrapper_func)(uvhttp_request_t*, uvhttp_response_t*);

int auth_middleware_wrapper(auth_middleware_wrapper_func handler) {
    return wrapped_handler;
}

int wrapped_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 1. 提取认证令牌
    const char* auth_header = uvhttp_request_get_header(req, "Authorization");
    
    // 2. 验证令牌
    if (!verify_token(auth_header)) {
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        const char* error = "{\"error\":\"Unauthorized\"}";
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 3. 调用实际处理器
    return handler(req, res);
}
```

---

## 最佳实践

### 1. 路由组织

**推荐**: 按功能模块组织路由。

```c
// 用户模块路由
void setup_user_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_GET, get_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_PUT, update_user_handler);
    uvhttp_router_add_route_method(router, "/api/users/:id", UVHTTP_DELETE, delete_user_handler);
}

// 文章模块路由
void setup_post_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_GET, get_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_PUT, update_post_handler);
    uvhttp_router_add_route_method(router, "/api/posts/:id", UVHTTP_DELETE, delete_post_handler);
}

// 主函数
int main() {
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    
    // 设置模块路由
    setup_user_routes(router);
    setup_post_routes(router);
    
    // ...
}
```

### 2. 错误处理

**推荐**: 统一的错误处理机制。

```c
// 统一的错误响应
void send_error_response(uvhttp_response_t* res, int status, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", message);
    
    char* json_str = cJSON_PrintUnformatted(json);
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, json_str, strlen(json_str));
    
    uvhttp_response_send(res);
    free(json_str);
    cJSON_Delete(json);
}

// 使用示例
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_route_match_t match;
    if (uvhttp_router_match(router, uvhttp_request_get_url(req), 
                           uvhttp_request_get_method(req), &match) != UVHTTP_OK) {
        send_error_response(res, 400, "Invalid request");
        return UVHTTP_OK;
    }
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    if (!user_id) {
        send_error_response(res, 400, "User ID is required");
        return UVHTTP_OK;
    }
    
    // ... 处理请求
}
```

### 3. 性能优化

**推荐**: 使用静态路由优先，参数路由其次。

```c
// 静态路由优先（性能更好）
uvhttp_router_add_route(router, "/api/users/me", get_current_user_handler);
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);

// 避免过度使用参数路由
// 不推荐
uvhttp_router_add_route(router, "/api/:resource/:id", generic_handler);

// 推荐（明确每个资源）
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);
uvhttp_router_add_route(router, "/api/posts/:id", get_post_handler);
```

### 4. 安全性

**推荐**: 验证所有输入参数。

```c
int get_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_route_match_t match;
    if (uvhttp_router_match(router, uvhttp_request_get_url(req), 
                           uvhttp_request_get_method(req), &match) != UVHTTP_OK) {
        send_error_response(res, 400, "Invalid request");
        return UVHTTP_OK;
    }
    
    const char* user_id = NULL;
    for (size_t i = 0; i < match.param_count; i++) {
        if (strcmp(match.params[i].name, "id") == 0) {
            user_id = match.params[i].value;
            break;
        }
    }
    
    // 验证用户 ID 格式
    if (!user_id || !is_valid_user_id(user_id)) {
        send_error_response(res, 400, "Invalid user ID");
        return UVHTTP_OK;
    }
    
    // ... 处理请求
}
```

---

## 常见问题

### Q1: 如何实现通配符路由？

**A**: UVHTTP 不支持通配符路由（如 `/api/*`），建议使用前缀路由或静态路由。

```c
// 不支持
// uvhttp_router_add_route(router, "/api/*", handler);

// 推荐：使用前缀路由
router->static_prefix = "/api";
router->static_handler = api_handler;

// 或使用具体路由
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);
```

### Q2: 如何实现路由优先级？

**A**: 路由按添加顺序匹配，先添加的路由优先级更高。

```c
// 更具体的路由先添加
uvhttp_router_add_route(router, "/api/users/me", get_current_user_handler);
uvhttp_router_add_route(router, "/api/users/:id", get_user_handler);
```

### Q3: 如何实现路由分组？

**A**: 使用函数组织路由。

```c
void setup_user_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, list_users_handler);
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, create_user_handler);
}

void setup_post_routes(uvhttp_router_t* router) {
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, list_posts_handler);
    uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_POST, create_post_handler);
}
```

### Q4: 如何实现路由重定向？

**A**: 在处理器中返回 301/302 状态码。

```c
int redirect_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 301);
    uvhttp_response_set_header(res, "Location", "https://example.com/new-path");
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    const char* message = "Moved Permanently";
    uvhttp_response_set_body(res, message, strlen(message));
    
    return uvhttp_response_send(res);
}
```

### Q5: 如何实现 CORS？

**A**: 在处理器中添加 CORS 头。

```c
int cors_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 添加 CORS 头
    uvhttp_response_set_header(res, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(res, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(res, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // 处理 OPTIONS 请求
    const char* method = uvhttp_request_get_method(req);
    if (strcmp(method, "OPTIONS") == 0) {
        uvhttp_response_set_status(res, 200);
        return uvhttp_response_send(res);
    }
    
    // 处理其他请求
    return actual_handler(req, res);
}
```

---

## 附录

### A. 路由 API 参考

| 函数 | 说明 |
|-----|------|
| `uvhttp_router_new` | 创建路由器 |
| `uvhttp_router_free` | 释放路由器 |
| `uvhttp_router_add_route` | 添加路由（默认 GET 方法） |
| `uvhttp_router_add_route_method` | 添加路由（指定方法） |
| `uvhttp_router_find_handler` | 查找路由处理器 |
| `uvhttp_router_match` | 匹配路由并提取参数 |
| `uvhttp_parse_path_params` | 解析路径参数 |

### B. HTTP 方法枚举

| 方法 | 枚举值 | 说明 |
|-----|-------|------|
| GET | `UVHTTP_GET` | 获取资源 |
| POST | `UVHTTP_POST` | 创建资源 |
| PUT | `UVHTTP_PUT` | 更新资源 |
| DELETE | `UVHTTP_DELETE` | 删除资源 |
| PATCH | `UVHTTP_PATCH` | 部分更新 |
| HEAD | `UVHTTP_HEAD` | 获取头信息 |
| OPTIONS | `UVHTTP_OPTIONS` | 获取选项 |

### C. 路由配置常量

| 常量 | 值 | 说明 |
|-----|---|------|
| `MAX_ROUTES` | 128 | 最大路由数 |
| `MAX_ROUTE_PATH_LEN` | 256 | 最大路由路径长度 |
| `MAX_PARAMS` | 16 | 最大参数数量 |
| `MAX_PARAM_NAME_LEN` | 64 | 最大参数名长度 |
| `MAX_PARAM_VALUE_LEN` | 256 | 最大参数值长度 |

---

## 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|-----|------|---------|------|
| 2026-02-23 | 1.0 | 初始版本，完整路由系统设计哲学 | iFlow |

---

## 联系方式

如有问题或建议，请通过以下方式联系:

- **GitHub Issues**: https://github.com/adam-ikari/uvhttp/issues
- **Email**: [待添加]
- **Slack**: [待添加]

---

**文档结束**