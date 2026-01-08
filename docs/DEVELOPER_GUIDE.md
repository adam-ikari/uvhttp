# UVHTTP 开发指南

## 目录
1. [快速开始](#快速开始)
2. [核心API使用](#核心api使用)
3. [配置管理](#配置管理)
4. [测试指南](#测试指南)
5. [性能优化](#性能优化)
6. [常见问题](#常见问题)
7. [贡献指南](#贡献指南)

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

## 贡献指南

### 提交规范

```
类型(范围): 简短描述

详细描述

相关问题: #123
```

**类型**: feat, fix, docs, style, refactor, test, chore

### 代码审查

- [ ] 代码符合项目规范
- [ ] 错误处理完整
- [ ] 内存管理安全
- [ ] 测试覆盖充分
- [ ] 文档更新及时

### 发布流程

1. 更新版本号
2. 更新CHANGELOG
3. 所有测试通过
4. 性能基准测试通过
5. 创建Git标签

## 许可证

MIT License - 详见 LICENSE 文件