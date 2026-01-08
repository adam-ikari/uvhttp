# UVHTTP 示例程序

本目录包含 UVHTTP 的示例程序，按照难度和主题组织。

## 目录结构

```
examples/
├── 01_basics/          # 基础示例
│   ├── 01_hello_world.c
│   └── README.md
├── 02_routing/         # 路由系统
│   ├── 01_simple_routing.c
│   ├── 02_method_routing.c
│   └── README.md
├── 03_requests/        # 请求处理
│   └── README.md
├── 04_responses/       # 响应处理
│   └── README.md
├── 05_advanced/        # 高级特性
│   ├── 01_libuv_data_pointer.c
│   └── README.md
└── 06_production/      # 生产环境
    └── README.md
```

## 学习路径

### 初学者路径

1. **01_basics** - 学习基础概念
   - Hello World - 最简单的 HTTP 服务器
   - 理解服务器创建和事件循环

2. **02_routing** - 学习路由系统
   - 简单路由 - 基本的路由注册
   - HTTP 方法路由 - 处理不同的 HTTP 方法

### 进阶路径

3. **03_requests** - 请求处理
   - 获取请求参数
   - 解析请求体
   - 处理查询参数

4. **04_responses** - 响应处理
   - JSON 响应
   - HTML 响应
   - 错误响应

5. **05_advanced** - 高级特性
   - libuv 数据指针 - 避免全局变量
   - WebSocket 实时通信
   - 静态文件服务

### 高级路径

6. **06_production** - 生产环境
   - 配置管理
   - 性能优化
   - 安全加固

## 编译和运行

### 编译单个示例

```bash
# 编译 Hello World 示例
gcc -o hello_world examples/01_basics/01_hello_world.c \
    -Iinclude \
    -Lbuild \
    -luvhttp -luv

# 运行
./hello_world
```

### 编译所有示例

```bash
cd build
cmake ..
make examples
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

## 常见问题

### Q: 如何选择合适的示例？

A: 按照学习路径从基础到高级逐步学习。每个示例都建立在前一个示例的基础上。

### Q: 示例可以用于生产环境吗？

A: 基础示例主要用于学习，生产环境请参考 `06_production` 目录。

### Q: 如何调试示例？

A: 使用 `gdb` 或添加调试输出：

```bash
gcc -g -o example example.c -Iinclude -Lbuild -luvhttp -luv
gdb ./example
```

## 贡献

欢迎提交新的示例！请遵循以下规范：

1. 代码风格遵循项目规范
2. 添加详细的注释
3. 包含测试命令
4. 更新 README 文档

## 相关文档

- [教程](../docs/TUTORIAL.md)
- [API 参考](../docs/API_REFERENCE.md)
- [开发指南](../docs/DEVELOPER_GUIDE.md)
- [libuv 数据指针](../docs/LIBUV_DATA_POINTER.md)
