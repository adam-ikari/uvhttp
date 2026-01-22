# UVHTTP 示例程序

本目录包含 UVHTTP 的示例程序，按照难度和主题组织。

## 目录结构

```
examples/
├── 01_basics/              # 基础示例
│   ├── 01_hello_world.c
│   ├── helloworld.c
│   ├── helloworld_complete.c
│   ├── simple_test.c
│   ├── simple_api_demo.c
│   └── quick_api_demo.c
├── 02_routing/             # 路由系统
│   ├── 01_simple_routing.c
│   ├── 02_method_routing.c
│   ├── api_demo.c
│   └── json_api_demo.c
├── 03_api_examples/        # RESTful API 示例
│   ├── restful_blog_api.c
│   ├── restful_todo_api.c
│   └── restful_user_api.c
├── 04_responses/           # 响应处理
│   └── unified_response_demo.c
├── 05_advanced/            # 高级特性
│   ├── 01_libuv_data_pointer.c
│   ├── middleware_demo.c
│   ├── middleware_macros_demo.c
│   ├── simple_middleware_demo.c
│   ├── cors_rate_limit_demo.c
│   ├── websocket_auth_server.c
│   ├── websocket_echo_server.c
│   ├── websocket_test_server.c
│   └── test_ws_connection_management.c
├── 06_static_files/        # 静态文件服务
│   ├── static_file_server.c
│   ├── advanced_static_server.c
│   ├── simple_static_test.c
│   ├── static_middleware_demo.c
│   └── cache_test_server.c
├── 07_config/              # 配置管理
│   ├── config_demo.c
│   └── simple_config.c
├── 08_memory/              # 内存管理
│   ├── mimalloc_demo.c
│   ├── app_advanced_memory.c
│   ├── app_custom_pool.c
│   └── hierarchical_allocator_example.c
└── performance_only/       # 性能测试
    ├── performance_static_server.c
    ├── performance_test_static.c
    ├── performance_test_tls.c
    ├── performance_test_websocket.c
    ├── performance_test.c
    └── performance_static_server_broken.c
```

## 学习路径

### 初学者路径

1. **01_basics** - 学习基础概念
   - 01_hello_world.c - 最简单的 HTTP 服务器
   - helloworld.c - 基础的 Hello World 示例
   - helloworld_complete.c - 完整的 Hello World 示例
   - simple_test.c - 简单测试示例
   - simple_api_demo.c - 简单 API 演示
   - quick_api_demo.c - 快速 API 演示

### 进阶路径

2. **02_routing** - 学习路由系统
   - 01_simple_routing.c - 基本的路由注册
   - 02_method_routing.c - 处理不同的 HTTP 方法
   - api_demo.c - API 演示
   - json_api_demo.c - JSON API 演示

3. **03_api_examples** - RESTful API 开发
   - restful_blog_api.c - 博客文章管理 API（CRUD 完整示例）
   - restful_todo_api.c - 待办事项管理 API（状态管理和优先级）
   - restful_user_api.c - 用户管理 API（认证和角色管理）

4. **04_responses** - 响应处理
   - unified_response_demo.c - 统一响应处理演示

### 高级路径

5. **05_advanced** - 高级特性
   - 01_libuv_data_pointer.c - 避免全局变量的最佳实践
   - middleware_demo.c - 中间件演示
   - middleware_macros_demo.c - 中间件宏演示
   - simple_middleware_demo.c - 简单中间件演示
   - cors_rate_limit_demo.c - CORS 和限流演示
   - websocket_auth_server.c - WebSocket 认证服务器
   - websocket_echo_server.c - WebSocket 回显服务器
   - websocket_test_server.c - WebSocket 测试服务器
   - test_ws_connection_management.c - WebSocket 连接管理测试

6. **06_static_files** - 静态文件服务
   - static_file_server.c - 基础静态文件服务器
   - advanced_static_server.c - 高级静态文件服务器
   - simple_static_test.c - 简单静态文件测试
   - static_middleware_demo.c - 静态文件中间件演示
   - cache_test_server.c - 缓存测试服务器

7. **07_config** - 配置管理
   - config_demo.c - 配置演示
   - simple_config.c - 简单配置示例

8. **08_memory** - 内存管理
   - mimalloc_demo.c - mimalloc 分配器演示
   - app_advanced_memory.c - 高级内存管理
   - app_custom_pool.c - 自定义内存池
   - hierarchical_allocator_example.c - 分层分配器示例

9. **performance_only** - 性能测试
   - performance_static_server.c - 静态文件性能测试
   - performance_test_static.c - 性能测试（静态）
   - performance_test_tls.c - 性能测试（TLS）
   - performance_test_websocket.c - 性能测试（WebSocket）
   - performance_test.c - 通用性能测试
   - performance_static_server_broken.c - 性能测试（有问题版本）

## 编译和运行

### 编译单个示例

```bash
# 编译 Hello World 示例
cd build
make 01_hello_world

# 运行
./dist/bin/01_hello_world
```

### 编译所有示例

```bash
cd build
cmake ..
make examples
```

### 编译特定分类的示例

```bash
# 编译基础示例
cd build
make 01_hello_world helloworld simple_test

# 编译 RESTful API 示例
cd build
make restful_blog_api restful_todo_api restful_user_api

# 编译性能测试
cd build
make performance_static_server performance_test_static
```

## 示例说明

### 01_basics - 基础示例

#### 01_hello_world.c

最简单的 HTTP 服务器，展示：
- 创建服务器
- 添加路由
- 发送响应
- 优雅关闭

**关键点：**
- 使用应用上下文结构避免全局变量
- 完整的错误处理
- 信号处理

#### helloworld.c

基础的 Hello World 示例。

#### helloworld_complete.c

完整的 Hello World 示例，包含更多功能。

#### simple_test.c

简单的测试示例。

#### simple_api_demo.c

简单的 API 演示。

#### quick_api_demo.c

快速 API 演示。

### 02_routing - 路由系统

#### 01_simple_routing.c

简单的路由系统，展示：
- 注册多个路由
- 不同的处理器
- 路径匹配

#### 02_method_routing.c

HTTP 方法路由，展示：
- 处理 GET、POST、PUT、DELETE
- 在处理器内部分发方法
- 返回适当的响应

#### api_demo.c

API 演示，展示核心 API 的使用。

#### json_api_demo.c

JSON API 演示，展示如何使用 cJSON 处理 JSON 数据。

### 03_api_examples - RESTful API 示例

#### restful_blog_api.c

博客文章管理 API，展示完整的 CRUD 操作：
- GET /api/posts - 获取文章列表（支持分页和过滤）
- GET /api/posts/:id - 获取单个文章
- POST /api/posts - 创建文章
- PUT /api/posts/:id - 更新文章
- DELETE /api/posts/:id - 删除文章

**特性：**
- 标准 RESTful 设计
- 分页功能
- 作者过滤
- 浏览量统计
- 完整的错误处理

#### restful_todo_api.c

待办事项管理 API，展示状态管理和优先级：
- GET /api/todos - 获取待办事项列表（支持过滤）
- GET /api/todos/:id - 获取单个待办事项
- POST /api/todos - 创建待办事项
- PUT /api/todos/:id - 更新待办事项
- DELETE /api/todos/:id - 删除待办事项

**特性：**
- 状态管理（已完成/待完成）
- 优先级支持（高/中/低）
- 查询过滤
- 统计信息

#### restful_user_api.c

用户管理 API，展示认证和角色管理：
- GET /api/users - 获取用户列表（支持过滤）
- GET /api/users/:id - 获取单个用户
- POST /api/users - 创建用户（含邮箱验证）
- PUT /api/users/:id - 更新用户
- DELETE /api/users/:id - 删除用户

**特性：**
- 用户认证（模拟）
- 角色管理（admin/user/guest）
- 状态管理（激活/停用）
- 邮箱格式验证
- 用户名唯一性检查

### 04_responses - 响应处理

#### unified_response_demo.c

统一响应处理演示，展示如何使用新的统一响应 API。

### 05_advanced - 高级特性

#### 01_libuv_data_pointer.c

libuv 数据指针使用，展示：
- 创建应用上下文
- 使用 `loop->data` 存储上下文
- 在回调中访问上下文
- 完整的生命周期管理

**关键点：**
- 避免全局变量
- 线程安全
- 代码封装

#### middleware_demo.c

中间件演示，展示如何创建和使用中间件。

#### middleware_macros_demo.c

中间件宏演示，展示如何使用中间件宏简化代码。

#### simple_middleware_demo.c

简单中间件演示。

#### cors_rate_limit_demo.c

CORS 和限流演示，展示如何配置 CORS 和限流。

#### websocket_auth_server.c

WebSocket 认证服务器，展示 WebSocket 连接认证。

#### websocket_echo_server.c

WebSocket 回显服务器，展示基本的 WebSocket 通信。

#### websocket_test_server.c

WebSocket 测试服务器。

#### test_ws_connection_management.c

WebSocket 连接管理测试。

### 06_static_files - 静态文件服务

#### static_file_server.c

基础静态文件服务器。

#### advanced_static_server.c

高级静态文件服务器，包含更多功能。

#### simple_static_test.c

简单静态文件测试。

#### static_middleware_demo.c

静态文件中间件演示。

#### cache_test_server.c

缓存测试服务器。

### 07_config - 配置管理

#### config_demo.c

配置演示，展示如何使用配置系统。

#### simple_config.c

简单配置示例。

### 08_memory - 内存管理

#### mimalloc_demo.c

mimalloc 分配器演示，展示如何使用 mimalloc。

#### app_advanced_memory.c

高级内存管理，展示高级内存技术。

#### app_custom_pool.c

自定义内存池，展示如何创建和使用内存池。

#### hierarchical_allocator_example.c

分层分配器示例。

### performance_only - 性能测试

#### performance_static_server.c

静态文件性能测试。

#### performance_test_static.c

性能测试（静态）。

#### performance_test_tls.c

性能测试（TLS）。

#### performance_test_websocket.c

性能测试（WebSocket）。

#### performance_test.c

通用性能测试。

#### performance_static_server_broken.c

性能测试（有问题版本，用于对比）。

## 最佳实践

### 1. 避免全局变量

```c
// ❌ 不推荐
static uvhttp_server_t* g_server = NULL;

// ✅ 推荐
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

app_context_t* ctx = malloc(sizeof(app_context_t));
loop->data = ctx;
```

### 2. 错误处理

```c
// 始终检查返回值
int result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
    return 1;
}
```

### 3. 资源清理

```c
// 确保所有资源都被正确释放
if (ctx->server) {
    uvhttp_server_free(ctx->server);
}
loop->data = NULL;
free(ctx);
```

### 4. 信号处理

```c
// 优雅关闭
signal(SIGINT, signal_handler);
signal(SIGTERM, signal_handler);
```

### 5. 使用 UVHTTP_TRUE 和 UVHTTP_FALSE

```c
// ✅ 推荐
server->is_listening = UVHTTP_TRUE;
conn->keep_alive = UVHTTP_FALSE;

// ❌ 不推荐
server->is_listening = 1;
conn->keep_alive = 0;
```

## 常见问题

### Q: 如何选择合适的示例？

A: 按照学习路径从基础到高级逐步学习。每个示例都建立在前一个示例的基础上。

### Q: 示例可以用于生产环境吗？

A: 基础示例主要用于学习，生产环境请参考高级示例和性能测试。

### Q: 如何调试示例？

A: 使用 `gdb` 或添加调试输出：

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make 01_hello_world
gdb ./dist/bin/01_hello_world
```

### Q: 如何运行 RESTful API 示例？

A:

```bash
cd build
make restful_blog_api
./dist/bin/restful_blog_api
```

然后访问 http://localhost:8080 查看文档。

### Q: 如何测试 API？

A: 使用 curl：

```bash
# 获取文章列表
curl http://localhost:8080/api/posts

# 创建文章
curl -X POST http://localhost:8080/api/posts \
     -H 'Content-Type: application/json' \
     -d '{"title":"新文章","content":"文章内容","author":"作者名"}'

# 获取单个文章
curl http://localhost:8080/api/posts/1

# 更新文章
curl -X PUT http://localhost:8080/api/posts/1 \
     -H 'Content-Type: application/json' \
     -d '{"title":"更新后的标题"}'

# 删除文章
curl -X DELETE http://localhost:8080/api/posts/1
```

## 贡献

欢迎提交新的示例！请遵循以下规范：

1. 代码风格遵循项目规范
2. 添加详细的注释
3. 包含测试命令
4. 更新 README 文档
5. 使用 UVHTTP_TRUE 和 UVHTTP_FALSE 宏代替数字 1 和 0

## 相关文档

- [教程](../docs/TUTORIAL.md)
- [API 参考](../docs/API_REFERENCE.md)
- [开发指南](../docs/DEVELOPER_GUIDE.md)
- [libuv 数据指针](../docs/LIBUV_DATA_POINTER.md)
- [RESTful API 最佳实践](../docs/RESTFUL_API_GUIDE.md)
- [性能测试标准](../docs/PERFORMANCE_TESTING_STANDARD.md)