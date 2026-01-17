# 最佳实践

## 代码规范

### 命名约定

- **函数**: `uvhttp_module_action`（如 `uvhttp_server_new`）
- **类型**: `uvhttp_name_t`（如 `uvhttp_server_t`）
- **常量**: `UVHTTP_UPPER_CASE`（如 `UVHTTP_MAX_HEADERS`）
- **宏**: `UVHTTP_UPPER_CASE`（如 `UVHTTP_MALLOC`）

### 代码风格

- **缩进**: 4 个空格
- **大括号**: K&R 风格
- **注释**: 解释为什么，而不是做什么
- **函数长度**: 不超过 50 行

### 示例

```c
// 好的命名
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);

// 好的注释
// 使用 mimalloc 分配器以提高性能
void* ptr = UVHTTP_MALLOC(size);

// 好的函数长度
void handle_request(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_body(res, "OK");
    uvhttp_response_send(res);
}
```

## 内存管理

### 使用统一分配器

```c
// 使用 UVHTTP_MALLOC 和 UVHTTP_FREE
void* ptr = UVHTTP_MALLOC(size);
// ... 使用 ptr
UVHTTP_FREE(ptr);

// 不要混用 malloc/free
// 错误示例：
// void* ptr = malloc(size);
// UVHTTP_FREE(ptr);  // 错误！
```

### 检查分配结果

```c
void* ptr = UVHTTP_MALLOC(size);
if (!ptr) {
    // 处理内存不足
    return UVHTTP_ERR_OUT_OF_MEMORY;
}
```

### 避免内存泄漏

```c
// 确保每个分配都有对应的释放
void* ptr = UVHTTP_MALLOC(size);
if (ptr) {
    // 使用 ptr
    UVHTTP_FREE(ptr);
}
```

## 错误处理

### 检查所有可能失败的函数

```c
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return result;
}
```

### 提供有意义的错误信息

```c
// 好的错误处理
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to listen on %s:%d: %s\n",
            host, port, uvhttp_error_string(result));
    return result;
}
```

### 错误恢复

```c
if (result != UVHTTP_OK && uvhttp_error_is_recoverable(result)) {
    // 尝试恢复操作
    result = uvhttp_server_listen(server, "0.0.0.0", 8081);
}
```

## 响应处理

### 标准响应模式

```c
void handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);

    // 1. 设置状态码
    uvhttp_response_set_status(res, 200);

    // 2. 设置响应头
    uvhttp_response_set_header(res, "Content-Type", "text/plain");

    // 3. 设置响应体
    uvhttp_response_set_body(res, "Hello, World!");

    // 4. 发送响应
    uvhttp_response_send(res);
}
```

### JSON 响应

```c
void json_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "message", "Hello");
    char* json_str = cJSON_Print(json);

    uvhttp_response_set_body(res, json_str);

    cJSON_Delete(json);
    free(json_str);
    uvhttp_response_send(res);
}
```

### 文件响应

```c
void file_handler(uvhttp_request_t* req) {
    uvhttp_response_t* res = uvhttp_response_new(req);
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html");

    // 使用零拷贝文件传输
    uvhttp_response_sendfile(res, "/path/to/file.html");
}
```

## 路由设计

### RESTful 路由

```c
// 资源路由
uvhttp_router_add_route(router, "/users", get_users);      // GET
uvhttp_router_add_route(router, "/users/:id", get_user);   // GET /users/123
uvhttp_router_add_route(router, "/users", create_user);    // POST
uvhttp_router_add_route(router, "/users/:id", update_user); // PUT /users/123
uvhttp_router_add_route(router, "/users/:id", delete_user); // DELETE /users/123
```

### 路由参数

```c
void user_handler(uvhttp_request_t* req) {
    const char* user_id = uvhttp_request_get_param(req, "id");
    // 使用 user_id
}
```

### 静态文件

```c
// 静态文件服务
uvhttp_router_add_route(router, "/static/*", static_handler);

void static_handler(uvhttp_request_t* req) {
    const char* path = uvhttp_request_get_path(req);
    // 处理静态文件
}
```

## 中间件使用

### 日志中间件

```c
uvhttp_log_middleware_t* log_middleware = uvhttp_log_middleware_new();
uvhttp_server_add_middleware(server, log_middleware);
```

### CORS 中间件

```c
uvhttp_cors_middleware_t* cors = uvhttp_cors_middleware_new();
uvhttp_cors_middleware_allow_origin(cors, "*");
uvhttp_server_add_middleware(server, cors);
```

### 限流中间件

```c
uvhttp_rate_limit_t* rate_limit = uvhttp_rate_limit_new();
uvhttp_rate_limit_set_max_requests(rate_limit, 100);
uvhttp_rate_limit_set_window(rate_limit, 60);  // 60秒
uvhttp_server_add_middleware(server, rate_limit);
```

### 自定义中间件

```c
void auth_middleware(uvhttp_request_t* req, uvhttp_middleware_next_t next) {
    const char* token = uvhttp_request_get_header(req, "Authorization");
    if (!token) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_body(res, "Unauthorized");
        uvhttp_response_send(res);
        return;
    }
    next(req);
}
```

## 安全实践

### 输入验证

```c
void handler(uvhttp_request_t* req) {
    const char* user_id = uvhttp_request_get_param(req, "id");

    // 验证输入
    if (!user_id || strlen(user_id) > 100) {
        uvhttp_response_t* res = uvhttp_response_new(req);
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_body(res, "Invalid user ID");
        uvhttp_response_send(res);
        return;
    }
}
```

### SQL 注入防护

```c
// 使用参数化查询
const char* query = "SELECT * FROM users WHERE id = ?";
sqlite3_bind_int(stmt, 1, user_id);
```

### XSS 防护

```c
// 转义输出
char* escaped = escape_html(user_input);
uvhttp_response_set_body(res, escaped);
free(escaped);
```

### CSRF 防护

```c
// 检查 CSRF token
const char* token = uvhttp_request_get_header(req, "X-CSRF-Token");
if (!validate_csrf_token(token)) {
    uvhttp_response_set_status(res, 403);
    uvhttp_response_send(res);
    return;
}
```

## 性能优化

### 使用缓存

```c
// 启用路由缓存
cmake -DUVHTTP_FEATURE_ROUTER_CACHE=ON ..

// 启用 LRU 缓存
cmake -DUVHTTP_FEATURE_LRU_CACHE=ON ..
```

### 避免阻塞

```c
// 使用异步操作
uv_async_send(async_handle);
```

### 连接复用

```c
// Keep-Alive 默认启用
// 无需额外配置
```

## 测试实践

### 单元测试

```c
void test_server_create() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    assert(server != NULL);
    uvhttp_server_close(server);
}
```

### 集成测试

```c
void test_api_endpoint() {
    // 发送 HTTP 请求
    // 验证响应
    // 清理资源
}
```

### 性能测试

```bash
# 使用 wrk 进行性能测试
wrk -t4 -c100 -d30s http://localhost:8080/api/users
```

## 部署建议

### 生产环境配置

```bash
# Release 模式
cmake -DCMAKE_BUILD_TYPE=Release ..

# 启用优化
cmake -DCMAKE_C_FLAGS="-O3 -march=native" ..

# 禁用调试信息
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-O3 -DNDEBUG" ..
```

### 系统优化

```bash
# 增加文件描述符限制
ulimit -n 65536

# 优化 TCP 参数
sysctl -w net.core.somaxconn=65535
```

### 监控

- 监控 CPU、内存、网络使用
- 监控请求吞吐量和延迟
- 监控错误率
- 设置告警

## 文档

### 代码注释

```c
/**
 * @brief 创建新的服务器实例
 * @param loop libuv 事件循环
 * @return 服务器实例，失败返回 NULL
 */
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

### API 文档

- 为所有公共 API 编写文档
- 提供使用示例
- 说明参数和返回值

### 变更日志

- 记录所有重要变更
- 说明破坏性变更
- 提供迁移指南

## 更多资源

- [API 参考](/api/introduction)
- [架构设计](/guide/architecture)
- [性能优化](/guide/performance)
- [错误码参考](https://github.com/adam-ikari/uvhttp/blob/main/docs/ERROR_CODES.md)