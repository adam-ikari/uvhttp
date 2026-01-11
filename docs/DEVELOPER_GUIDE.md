# UVHTTP 开发指南

## 目录
1. [快速开始](#快速开始)
2. [核心API使用](#核心api使用)
3. [配置管理](#配置管理)
4. [测试指南](#测试指南)
5. [性能优化](#性能优化)
6. [常见问题](#常见问题)
7. [贡献指南](#贡献指南)
8. [开发准则](#开发准则)

## 核心API使用

### 服务器创建标准模式

```c
// 标准服务器创建流程
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;

// 添加路由
uvhttp_router_add_route(router, "/api", api_handler);

// 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);
uv_run(loop, UV_RUN_DEFAULT);
```

### 响应处理标准模式

所有响应处理应遵循以下模式：
1. 设置状态码（`uvhttp_response_set_status`）
2. 设置响应头（`uvhttp_response_set_header`）
3. 设置响应体（`uvhttp_response_set_body`）
4. 发送响应（`uvhttp_response_send`）

**示例**：
```c
int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    const char* body = "{\"message\":\"Hello World\"}";
    uvhttp_response_set_body(res, body, strlen(body));
    return uvhttp_response_send(res);
}
```

### 代码规范要点

**命名约定**：
- 函数：`uvhttp_module_action` (如 `uvhttp_server_new`)
- 类型：`uvhttp_name_t` (如 `uvhttp_server_t`)
- 常量：`UVHTTP_UPPER_CASE` (如 `UVHTTP_MAX_HEADERS`)

**内存管理**：
```c
// 使用统一分配器
void* ptr = UVHTTP_MALLOC(size);
if (!ptr) return UVHTTP_ERROR_OUT_OF_MEMORY;
// 使用后释放
UVHTTP_FREE(ptr);
```

**错误处理**：
```c
// 检查所有可能失败的函数
int result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    // 错误处理
}
```

## 快速开始

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libuv-dev libmbedtls-dev

# CentOS/RHEL
sudo yum install libuv-devel mbedtls-devel

# macOS
brew install libuv mbedtls
```

### 编译项目

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 运行第一个服务器

```c
#include "uvhttp.h"

void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## 配置管理

### 配置方式

UVHTTP支持三种配置方式，优先级从高到低：代码配置 > 环境变量 > 配置文件

```c
// 1. 创建配置对象
uvhttp_config_t* config = uvhttp_config_new();

// 2. 设置默认值
uvhttp_config_set_defaults(config);

// 3. 从文件加载（可选）
uvhttp_config_load_file(config, "uvhttp.conf");

// 4. 从环境变量加载（可选，会覆盖文件配置）
uvhttp_config_load_env(config);

// 5. 代码中直接设置（最高优先级）
config->max_connections = 3000;
config->max_body_size = 2 * 1024 * 1024;

// 6. 验证配置
if (uvhttp_config_validate(config) != UVHTTP_OK) {
    fprintf(stderr, "配置验证失败\n");
    return 1;
}

// 7. 应用到服务器
server->config = config;
```

### 配置文件示例

```
# uvhttp.conf
# 服务器配置
max_connections=3000
max_requests_per_connection=200
backlog=1024

# 性能配置
max_body_size=2097152
read_buffer_size=16384

# 超时配置
keepalive_timeout=30
request_timeout=60
```

### 环境变量配置

```bash
export UVHTTP_MAX_CONNECTIONS=4000
export UVHTTP_MAX_REQUESTS_PER_CONNECTION=150
export UVHTTP_ENABLE_COMPRESSION=1
```

### 关键配置参数

- **max_connections**: 最大并发连接数 (1-10000)
- **max_requests_per_connection**: 每连接最大请求数
- **max_body_size**: 最大请求体大小 (字节)
- **read_buffer_size**: 读取缓冲区大小
- **keepalive_timeout**: Keep-Alive 超时时间 (秒)
- **request_timeout**: 请求超时时间 (秒)

## 测试指南

### 运行测试

```bash
# 编译并运行所有测试
cd build
make test

# 运行特定测试
./test/unit/test_response
./test/unit/test_server

# 内存泄漏检测
valgrind --leak-check=full ./test/unit/test_response
```

### 测试结构

- **单元测试**: 测试单个函数和模块
- **集成测试**: 测试模块间交互
- **性能测试**: 基准测试和性能回归
- **压力测试**: 高并发和长时间运行测试

### 编写测试

```c
#include "uvhttp_test_helpers.h"

static int test_response_status() {
    uvhttp_response_t* response = uvhttp_response_new();
    UVHTTP_TEST_ASSERT_NOT_NULL(response);
    
    int result = uvhttp_response_set_status(response, 200);
    UVHTTP_TEST_ASSERT_SUCCESS(result);
    
    uvhttp_response_free(response);
    return 0;
}
```

## 性能优化

### 内存分配器

UVHTTP支持多种内存分配器：

```c
// 编译时选择分配器
#define UVHTTP_ALLOCATOR_TYPE 0  // mimalloc（默认）
#define UVHTTP_ALLOCATOR_TYPE 1  // 系统分配器
```

**使用系统分配器**：
```bash
# 编译时使用系统分配器
cmake -DUSE_SYSTEM_ALLOCATOR=ON ..
make
```

### 性能调优建议

1. **合理设置连接数**：根据系统内存调整 `max_connections`
2. **使用Keep-Alive**：减少TCP连接建立开销
3. **启用压缩**：对大文本响应启用gzip压缩
4. **缓冲区优化**：根据请求大小调整缓冲区

## 常见问题

### Q: 如何处理静态文件？

A: 参见 `STATIC_FILE_SERVER.md` 文档

### Q: 如何启用HTTPS？

A: 配置TLS证书和密钥：
```c
uvhttp_config_t* config = uvhttp_config_new();
config->enable_tls = 1;
config->tls_cert_file = "server.crt";
config->tls_key_file = "server.key";
```

### Q: 如何调试内存问题？

A: 使用valgrind检测内存泄漏：
```bash
valgrind --leak-check=full --show-leak-kinds=all ./your_server
```

## 分支管理规范

### 分支策略

UVHTTP 采用 Git Flow 分支管理策略，确保代码质量和开发效率。

#### 主要分支

| 分支名称 | 用途 | 保护状态 |
|---------|------|----------|
| `main` | 生产环境代码，始终保持可发布状态 | ✅ 受保护 |
| `develop` | 开发主分支，包含最新的开发功能 | ✅ 受保护 |

#### 辅助分支

| 分支类型 | 命名规范 | 用途 | 合并目标 |
|---------|---------|------|----------|
| 功能分支 | `feature/功能名称` | 开发新功能 | `develop` |
| 修复分支 | `fix/问题描述` | 修复生产问题 | `main` 和 `develop` |
| 重构分支 | `refactor/描述` | 代码重构 | `develop` |
| 发布分支 | `release/v版本号` | 准备发布 | `main` 和 `develop` |
| 热修复分支 | `hotfix/问题描述` | 紧急修复生产问题 | `main` 和 `develop` |

### 分支命名规范

#### 功能分支
```
feature/功能名称-简要描述
feature/websocket-support
feature/static-file-optimization
```

#### 修复分支
```
fix/问题描述
fix/memory-leak-in-connection
fix/rate-limit-bug
```

#### 重构分支
```
refactor/模块名称-重构内容
refactor/router-performance
refactor/memory-management
```

#### 发布分支
```
release/v1.2.0
release/v2.0.0-beta
```

#### 热修复分支
```
hotfix/关键问题描述
hotfix/crash-in-server
hotfix/security-vulnerability
```

### 工作流程

#### 1. 开发新功能

```bash
# 从 develop 创建功能分支
git checkout develop
git pull origin develop
git checkout -b feature/new-feature-name

# 开发和提交
git add .
git commit -m "feat: 添加新功能描述"

# 推送到远程
git push origin feature/new-feature-name

# 创建 Pull Request 到 develop
```

#### 2. 修复生产问题

```bash
# 从 main 创建修复分支
git checkout main
git pull origin main
git checkout -b fix/critical-bug-description

# 修复和测试
# ... 修复代码 ...
git add .
git commit -m "fix: 修复关键问题"

# 推送并创建 PR 到 main
git push origin fix/critical-bug-description

# 合并到 main 后，也要合并到 develop
git checkout main
git merge fix/critical-bug-description
git checkout develop
git merge main
```

#### 3. 准备发布

```bash
# 从 develop 创建发布分支
git checkout develop
git pull origin develop
git checkout -b release/v1.2.0

# 更新版本号和 CHANGELOG
# ... 更新文件 ...
git add .
git commit -m "chore: 准备 v1.2.0 发布"

# 推送并创建 PR 到 main
git push origin release/v1.2.0

# 合并到 main 后打标签
git checkout main
git merge release/v1.2.0
git tag -a v1.2.0 -m "Release v1.2.0"
git push origin v1.2.0

# 合并回 develop
git checkout develop
git merge release/v1.2.0
```

#### 4. 紧急热修复

```bash
# 从 main 创建热修复分支
git checkout main
git pull origin main
git checkout -b hotfix/urgent-fix

# 快速修复
# ... 修复代码 ...
git add .
git commit -m "hotfix: 紧急修复严重问题"

# 推送并创建 PR 到 main
git push origin hotfix/urgent-fix

# 合并到 main 后打标签
git checkout main
git merge hotfix/urgent-fix
git tag -a v1.2.1 -m "Hotfix v1.2.1"
git push origin v1.2.1

# 合并回 develop
git checkout develop
git merge hotfix/urgent-fix
```

### 分支保护规则

#### main 分支
- ✅ 需要至少 1 个审查批准
- ✅ 需要通过所有 CI 检查
- ✅ 禁止直接推送
- ✅ 要求分支是最新的

#### develop 分支
- ✅ 需要至少 1 个审查批准
- ✅ 需要通过所有 CI 检查
- ✅ 禁止直接推送
- ✅ 要求分支是最新的

### 提交规范

#### 提交信息格式

```
类型(范围): 简短描述

详细描述（可选）

相关问题: #123
```

#### 提交类型

| 类型 | 说明 | 示例 |
|-----|------|------|
| `feat` | 新功能 | `feat(router): 添加正则表达式路由支持` |
| `fix` | 修复 bug | `fix(connection): 修复内存泄漏问题` |
| `docs` | 文档更新 | `docs(api): 更新服务器 API 文档` |
| `style` | 代码格式 | `style: 统一代码缩进` |
| `refactor` | 重构 | `refactor(memory): 优化内存分配策略` |
| `test` | 测试相关 | `test(router): 添加路由测试用例` |
| `chore` | 构建/工具 | `chore(deps): 更新 libuv 版本` |
| `perf` | 性能优化 | `perf(server): 优化事件循环性能` |

#### 提交范围

常用范围：`server`, `router`, `connection`, `request`, `response`, `tls`, `websocket`, `static`, `config`, `test`, `docs`, `build`

#### 提交示例

```
feat(router): 添加正则表达式路由支持

- 实现基于 PCRE 的正则表达式匹配
- 支持捕获组和命名组
- 添加相关测试用例

Closes #123
```

```
fix(connection): 修复内存泄漏问题

修复连接关闭时未释放请求对象导致的内存泄漏。
使用 valgrind 验证修复效果。

Fixes #456
```

### 代码审查清单

在提交 Pull Request 前，请确保：

- [ ] 代码符合项目代码规范
- [ ] 所有错误处理完整
- [ ] 内存管理安全，无泄漏
- [ ] 测试覆盖充分（新功能 > 80%）
- [ ] 文档已更新（API 变更）
- [ ] 提交信息符合规范
- [ ] 所有 CI 检查通过
- [ ] 性能基准测试通过（如有影响）

### 发布流程

1. **准备发布**
   - 创建发布分支 `release/vX.Y.Z`
   - 更新版本号（`include/uvhttp_version.h`）
   - 更新 CHANGELOG.md
   - 运行完整测试套件

2. **合并到 main**
   - 创建 Pull Request 到 main
   - 通过代码审查
   - 合并到 main

3. **打标签**
   ```bash
   git tag -a vX.Y.Z -m "Release vX.Y.Z"
   git push origin vX.Y.Z
   ```

4. **合并回 develop**
   ```bash
   git checkout develop
   git merge release/vX.Y.Z
   ```

5. **删除发布分支**
   ```bash
   git branch -d release/vX.Y.Z
   ```

### 分支清理

定期清理已合并的分支：

```bash
# 查看已合并的本地分支
git branch --merged

# 删除已合并的本地分支
git branch -d feature/old-feature

# 查看已合并的远程分支
git branch -r --merged

# 删除已合并的远程分支
git push origin --delete feature/old-feature
```

### 最佳实践

1. **保持分支简短**
   - 功能分支应在 1-2 周内完成
   - 避免长期运行的分支

2. **频繁提交**
   - 每完成一个小功能就提交
   - 保持提交历史清晰

3. **及时同步**
   - 定期从主分支拉取更新
   - 解决冲突时尽早处理

4. **清晰的提交信息**
   - 使用规范的提交格式
   - 提交信息应说明"为什么"而非"什么"

5. **测试驱动**
   - 先写测试，再写代码
   - 确保所有测试通过

### 常见命令速查

```bash
# 查看所有分支
git branch -a

# 查看当前分支状态
git status

# 创建并切换到新分支
git checkout -b feature/new-feature

# 合并分支（不使用 fast-forward）
git merge --no-ff feature/new-feature

# 变基分支
git rebase develop

# 删除本地分支
git branch -d feature/old-feature

# 删除远程分支
git push origin --delete feature/old-feature

# 查看分支差异
git diff develop

# 查看提交历史
git log --oneline --graph --all

# 暂存当前更改
git stash

# 恢复暂存的更改
git stash pop
```

## 贡献指南

## 长期目标

### 云原生支持

#### 健康检查
```c
// 添加健康检查端点
void uvhttp_server_set_health_check(uvhttp_server_t* server, 
                                     uvhttp_health_check_fn fn);
```

#### 指标导出
```c
// 支持 Prometheus 指标
void uvhttp_metrics_export_prometheus(uvhttp_server_t* server, 
                                       char* buffer, size_t len);
```

#### 配置管理
```c
// 支持环境变量和 ConfigMap
void uvhttp_config_load_from_env(uvhttp_config_t* config);
void uvhttp_config_load_from_k8s(uvhttp_config_t* config);
```

### WebAssembly 支持

#### wasilibuv 集成
- 使用 wasilibuv 替代 libuv 支持 WASM 编译
- 保持 API 兼容性

#### WASM 编译配置
```cmake
# CMakeLists.txt
option(BUILD_WASM "Build for WebAssembly" OFF)

if(BUILD_WASM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s WASM=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=['cwrap']")
    add_subdirectory(deps/wasilibuv)
else()
    add_subdirectory(deps/libuv)
endif()
```

#### WASI 抽象层
```c
// 添加 WASI 抽象层
#ifdef UVHTTP_WASI
    #include <wasi/libc.h>
#else
    #include <unistd.h>
#endif
```

#### JavaScript 绑定
```javascript
// JavaScript 绑定
const uvhttp = Module({
    uvhttp_server_new: cwrap('uvhttp_server_new', 'number', []),
    uvhttp_server_listen: cwrap('uvhttp_server_listen', 'number', ['number', 'string', 'number'])
});
```

### 边缘计算优化

#### 冷启动优化
- 减少初始化代码
- 延迟加载非必要模块
- 优化内存分配

#### 内存优化
- 降低静态内存占用
- 优化内存池管理
- 添加内存限制配置

#### 离线模式
- 支持缓存模式
- 支持本地存储
- 优化网络重试

### 性能优化

#### 缩小与 Nginx 的性能差距
- 优化事件循环
- 优化网络 I/O
- 优化内存拷贝

#### 中等文件传输优化
- 修复超时问题
- 优化缓冲区管理
- 添加流式传输

#### 连接池
- 添加连接池支持
- 优化连接复用
- 添加连接限制

## 开发准则

### 1. libuv 循环注入原则

#### 原则说明
UVHTTP 必须支持 libuv 循环注入，避免使用全局变量。这是为了支持多实例、单元测试和云原生场景。

#### 标准模式

**❌ 错误做法 - 使用全局变量**:
```c
// 错误示例
static uvhttp_server_t* g_server = NULL;

void my_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 使用全局变量 - 不可测试、不支持多实例
    g_server->request_count++;
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    g_server = uvhttp_server_new(loop);
    // ...
}
```

**✅ 正确做法 - 使用循环注入**:
```c
// 正确示例
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} app_context_t;

void my_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 从循环获取上下文
    uv_loop_t* loop = req->client->loop;
    app_context_t* ctx = (app_context_t*)loop->data;
    
    ctx->request_count++;
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建并设置上下文
    app_context_t* ctx = malloc(sizeof(app_context_t));
    ctx->server = uvhttp_server_new(loop);
    ctx->router = uvhttp_router_new();
    ctx->request_count = 0;
    
    // 注入到循环
    loop->data = ctx;
    
    // ...
}
```

#### 实现要求

1. **所有处理器必须支持循环注入**
```c
// 处理器必须能够从请求中获取循环
void handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uvhttp_request_get_loop(req);
    app_context_t* ctx = (app_context_t*)loop->data;
    // 使用 ctx 而不是全局变量
}
```

2. **服务器必须支持自定义循环**
```c
// 服务器必须接受自定义循环
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
// 而不是
uvhttp_server_t* uvhttp_server_new(void); // 错误
```

3. **测试必须使用独立循环**
```c
// 测试必须创建独立循环
void test_server() {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 测试代码
    
    uv_loop_close(loop);
    free(loop);
}
```

#### 优势

| 优势 | 说明 |
|-----|------|
| **可测试性** | 每个测试使用独立循环，避免冲突 |
| **多实例** | 支持在同一进程中运行多个服务器实例 |
| **云原生** | 适合容器化和 Serverless 场景 |
| **线程安全** | 避免全局变量导致的线程安全问题 |

#### 最佳实践

**创建上下文结构**:
```c
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    // 应用特定数据
    int request_count;
    time_t start_time;
    // ...
} app_context_t;

app_context_t* app_context_new(uv_loop_t* loop) {
    app_context_t* ctx = malloc(sizeof(app_context_t));
    ctx->server = uvhttp_server_new(loop);
    ctx->router = uvhttp_router_new();
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    return ctx;
}

void app_context_free(app_context_t* ctx) {
    if (ctx) {
        uvhttp_server_free(ctx->server);
        uvhttp_router_free(ctx->router);
        free(ctx);
    }
}
```

**在处理器中使用**:
```c
void api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uvhttp_request_get_loop(req);
    app_context_t* ctx = (app_context_t*)loop->data;
    
    // 使用上下文
    char response[256];
    snprintf(response, sizeof(response), 
             "{\"count\":%d,\"uptime\":%ld}", 
             ctx->request_count, 
             time(NULL) - ctx->start_time);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    uvhttp_response_send(res);
}
```

**完整示例**:
```c
#include "uvhttp.h"

typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} app_context_t;

void hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_loop_t* loop = uvhttp_request_get_loop(req);
    app_context_t* ctx = (app_context_t*)loop->data;
    ctx->request_count++;
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello, World!", 13);
    uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建上下文
    app_context_t* ctx = app_context_new(loop);
    
    // 配置服务器
    uvhttp_router_add_route(ctx->router, "/", hello_handler);
    ctx->server->router = ctx->router;
    
    // 注入到循环
    loop->data = ctx;
    
    // 启动服务器
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    app_context_free(ctx);
    return 0;
}
```

### 2. 其他开发准则

#### 错误处理
- 检查所有可能失败的函数调用
- 使用统一的错误类型 `uvhttp_error_t`
- 提供有意义的错误信息

#### 内存管理
- 使用统一分配器宏: `UVHTTP_MALLOC`、`UVHTTP_FREE`
- 确保每个分配都有对应的释放
- 避免内存泄漏

#### 命名约定
- 函数: `uvhttp_module_action`
- 类型: `uvhttp_name_t`
- 常量: `UVHTTP_UPPER_CASE`

#### 代码风格
- 使用 C11 标准
- 4 空格缩进
- K&R 风格大括号

## 许可证

MIT License - 详见 LICENSE 文件