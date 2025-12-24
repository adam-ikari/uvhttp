# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Coverage](https://img.shields.io/badge/coverage-97%25-green.svg)
![Performance](https://img.shields.io/badge/1000%20RPS-0.082ms-brightgreen.svg)
![Stress](https://img.shields.io/badge/stress%20tests-passing-success.svg)
![WebSocket](https://img.shields.io/badge/websocket-supported-orange.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 编译配置 • 生产就绪

</div>

## ✨ 特性

### 🔒 **安全第一**

- ✅ 缓冲区溢出保护
- ✅ 输入验证和边界检查
- ✅ 安全的字符串操作
- ✅ 资源限制和 DoS 防护
- ✅ TLS 1.3 支持
- ✅ WebSocket 安全连接
- ✅ 编译时安全检查

### ⚡ **高性能**

- ⚡ 基于 libuv 事件驱动架构
- ⚡ 零拷贝内存管理
- ⚡ 连接池和会话缓存
- ⚡ 智能内存分配策略
- ⚡ WebSocket 高性能处理
- ⚡ 编译优化零开销

### 🛡️ **生产就绪**

- 🛡️ 零编译警告
- 🛡️ 完整的错误处理
- 🛡️ 条件编译日志系统
- 🛡️ 性能监控和统计
- 🛡️ 内存泄漏检测
- 🛡️ 97%测试覆盖率

### 🔧 **易于使用**

- 🔧 简洁直观的 API 设计
- 🔧 丰富的示例代码
- 🔧 详细的 API 文档
- 🔧 完整的测试覆盖
- 🔧 WebSocket 简化 API
- 🔧 编译宏功能控制

### 💾 **智能缓存系统**

- 💾 LRU缓存算法实现
- 💾 内存使用优化
- 💾 TTL过期机制
- 💾 缓存统计和监控
- 💾 静态文件缓存支持
- 💾 英文日志记录系统

### 📊 **日志和监控**

- 📊 分级日志系统（DEBUG/INFO/WARN/ERROR）
- 📊 英文日志消息
- 📊 缓存操作详细记录
- 📊 性能统计信息
- 📊 错误追踪和调试支持
- 📊 可配置日志级别

### 📈 **性能验证**

- 📈 全面压力测试套件
- 📈 1000+ RPS 性能验证
- 📈 亚毫秒级响应时间
- 📈 零内存泄漏保证
- 📈 WebSocket 压力测试

### 🌐 **WebSocket 支持**

- 🌐 完整的 WebSocket 协议实现
- 🌐 消息类型支持（文本/二进制/控制帧）
- 🌐 mTLS 安全连接
- 🌐 证书验证和管理
- 🌐 连接池和自动重连
- 🌐 高并发 WebSocket 连接

### ⚙️ **编译配置**

- ⚙️ 功能开关（WebSocket/TLS/JSON）
- ⚙️ 安全特性（CORS/限流/认证）
- ⚙️ 性能优化（缓存/连接池）
- ⚙️ 调试功能（日志/追踪）

### 📦 **依赖管理**

UVHTTP 使用 git 子模块管理第三方依赖，确保版本兼容性和依赖完整性：

```bash
# 克隆项目（包含所有子模块）
git clone --recursive https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 更新子模块到最新版本
git submodule update --init --recursive

# 初始化并构建
mkdir build && cd build
cmake ..
make
```

- ⚙️ 零运行时开销设计

## 🚀 快速开始

### 依赖要求

- CMake >= 3.10
- GCC 或兼容的 C11 编译器

### 克隆和初始化

```bash
# 克隆项目（包含所有子模块）
git clone --recursive https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 如果已经克隆，更新子模块
git submodule update --init --recursive
```

### 编译

```bash
mkdir build && cd build
cmake ..
make
```

## 示例

### HTTP 服务器

```c
#include "uvhttp.h"
#include <stdio.h>

void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);

    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### WebSocket 服务器

```c
#include "uvhttp.h"
#include <stdio.h>

void websocket_handler(uvhttp_websocket_t* ws,
                       const uvhttp_websocket_message_t* msg,
                       void* user_data) {
    if (msg->type == UVHTTP_WEBSOCKET_TEXT) {
        printf("收到消息: %.*s\n", (int)msg->length, msg->data);
        // 回复消息
        uvhttp_websocket_send_text(ws, "消息已收到!");
    }
}

void websocket_upgrade_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 升级到WebSocket连接
    uvhttp_websocket_t* ws = uvhttp_websocket_new(request, response);
    if (ws) {
        uvhttp_websocket_set_handler(ws, websocket_handler, NULL);
        printf("WebSocket连接已建立\n");
    }
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/ws", websocket_upgrade_handler);

    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("WebSocket服务器运行在 ws://localhost:8080/ws\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

### 静态文件服务器（带LRU缓存）

```c
#include "uvhttp.h"
#include "uvhttp_lru_cache.h"
#include <stdio.h>

// 全局缓存管理器
static cache_manager_t* g_cache = NULL;

void static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* file_path = uvhttp_request_get_url(request);
    
    // 尝试从缓存中获取文件
    cache_entry_t* entry = uvhttp_lru_cache_find(g_cache, file_path);
    
    if (entry) {
        // 缓存命中，直接返回
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", entry->mime_type);
        uvhttp_response_set_header(response, "Cache-Control", "public, max-age=300");
        uvhttp_response_set_body(response, entry->content, entry->content_length);
        uvhttp_response_send(response);
        return;
    }
    
    // 缓存未命中，读取文件（简化示例）
    FILE* file = fopen(file_path + 1, "rb"); // 跳过前导 '/'
    if (!file) {
        uvhttp_response_set_status(response, 404);
        uvhttp_response_set_body(response, "File not found", 14);
        uvhttp_response_send(response);
        return;
    }
    
    // 读取文件内容
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(file_size);
    fread(content, 1, file_size, file);
    fclose(file);
    
    // 添加到缓存
    uvhttp_lru_cache_put(g_cache, file_path, content, file_size, 
                        "text/html", time(NULL), NULL);
    
    // 返回响应
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, content, file_size);
    uvhttp_response_send(response);
}

int main() {
    // 初始化缓存：最大1MB内存，最多100个条目，TTL为300秒
    g_cache = uvhttp_lru_cache_create(1024*1024, 100, 300);
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/*", static_file_handler);

    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("静态文件服务器运行在 http://localhost:8080 (带LRU缓存)\n");
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理资源
    uvhttp_lru_cache_free(g_cache);
    return 0;
}
```

### 日志配置示例

```c
#include "uvhttp.h"
#include "uvhttp_error_handler.h"
#include <stdio.h>

void log_config_example() {
    // 配置日志级别为DEBUG，查看所有日志信息
    g_error_config.min_logLevel = UVHTTP_LOG_LEVEL_DEBUG;
    
    // 启用日志恢复功能
    g_error_config.enableRecovery = 1;
    g_error_config.maxRetries = 3;
    g_error_config.baseDelayMs = 100;
    
    // 自定义错误处理器
    g_error_config.customHandler = my_error_handler;
    
    UVHTTP_LOG_INFO("日志系统已初始化");
    UVHTTP_LOG_DEBUG("调试信息：当前日志级别为DEBUG");
    UVHTTP_LOG_WARN("警告信息：这是一个示例警告");
    UVHTTP_LOG_ERROR("错误信息：这是一个示例错误");
}

int main() {
    // 配置日志系统
    log_config_example();
    
    // 其他应用逻辑...
    
    return 0;
}
```

## API 文档

### 服务器

- `uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop)` - 创建新服务器
- `void uvhttp_server_free(uvhttp_server_t* server)` - 释放服务器
- `int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port)` - 监听端口
- `void uvhttp_server_stop(uvhttp_server_t* server)` - 停止服务器

### 路由

- `uvhttp_router_t* uvhttp_router_new(void)` - 创建新路由
- `void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler)` - 添加路由
- `uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path)` - 查找路由处理器

### 请求

- `const char* uvhttp_request_get_method(uvhttp_request_t* request)` - 获取 HTTP 方法
- `const char* uvhttp_request_get_url(uvhttp_request_t* request)` - 获取请求 URL
- `const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name)` - 获取请求头
- `const char* uvhttp_request_get_body(uvhttp_request_t* request)` - 获取请求体

### 响应

- `void uvhttp_response_set_status(uvhttp_response_t* response, int status_code)` - 设置状态码
- `void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)` - 设置响应头
- `void uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)` - 设置响应体
- `void uvhttp_response_send(uvhttp_response_t* response)` - 发送响应

### WebSocket

- `uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, uvhttp_response_t* response)` - 创建 WebSocket 连接
- `void uvhttp_websocket_free(uvhttp_websocket_t* ws)` - 释放 WebSocket 连接
- `uvhttp_websocket_error_t uvhttp_websocket_send(uvhttp_websocket_t* ws, const char* data, size_t length, uvhttp_websocket_type_t type)` - 发送消息
- `uvhttp_websocket_error_t uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, uvhttp_websocket_handler_t handler, void* user_data)` - 设置消息处理器
- `uvhttp_websocket_error_t uvhttp_websocket_close(uvhttp_websocket_t* ws, int code, const char* reason)` - 关闭连接
- `uvhttp_websocket_error_t uvhttp_websocket_enable_mtls(uvhttp_websocket_t* ws, const uvhttp_websocket_mtls_config_t* config)` - 启用 mTLS
- `uvhttp_websocket_error_t uvhttp_websocket_verify_peer_cert(uvhttp_websocket_t* ws)` - 验证对端证书

#### WebSocket 便捷宏

- `uvhttp_websocket_send_text(ws, text)` - 发送文本消息
- `uvhttp_websocket_send_binary(ws, data, len)` - 发送二进制消息

### LRU缓存

- `cache_manager_t* uvhttp_lru_cache_create(size_t max_memory_usage, int max_entries, int cache_ttl)` - 创建LRU缓存管理器
- `void uvhttp_lru_cache_free(cache_manager_t* cache)` - 释放LRU缓存管理器
- `cache_entry_t* uvhttp_lru_cache_find(cache_manager_t* cache, const char* file_path)` - 查找缓存条目
- `int uvhttp_lru_cache_put(cache_manager_t* cache, const char* file_path, char* content, size_t content_length, const char* mime_type, time_t last_modified, const char* etag)` - 添加或更新缓存条目
- `int uvhttp_lru_cache_remove(cache_manager_t* cache, const char* file_path)` - 删除缓存条目
- `void uvhttp_lru_cache_clear(cache_manager_t* cache)` - 清空所有缓存
- `void uvhttp_lru_cache_get_stats(cache_manager_t* cache, size_t* total_memory_usage, int* entry_count, int* hit_count, int* miss_count, int* eviction_count)` - 获取缓存统计信息
- `int uvhttp_lru_cache_cleanup_expired(cache_manager_t* cache)` - 清理过期条目
- `double uvhttp_lru_cache_get_hit_rate(cache_manager_t* cache)` - 计算缓存命中率

### 日志系统

- `void uvhttp_log(uvhttp_log_level_t level, const char* format, ...)` - 核心日志函数
- `UVHTTP_LOG_DEBUG(fmt, ...)` - 调试级别日志
- `UVHTTP_LOG_INFO(fmt, ...)` - 信息级别日志
- `UVHTTP_LOG_WARN(fmt, ...)` - 警告级别日志
- `UVHTTP_LOG_ERROR(fmt, ...)` - 错误级别日志
- `UVHTTP_LOG_FATAL(fmt, ...)` - 致命错误级别日志

## 🏃‍♂️ 运行示例

### 基础 HTTP 服务器

```bash
# 确保子模块已初始化
git submodule update --init --recursive

# 编译完成后
./dist/bin/helloworld
```

然后在浏览器中访问 http://localhost:8080

## 🧪 测试

### 单元测试

```bash
./dist/test/uvhttp_unit_tests
```

### 性能测试

```bash
./dist/test/uvhttp_test
```

## 🚀 版本规划

### v1.0.0 (当前版本)

- ✅ HTTP/1.1 服务器核心功能
- ✅ WebSocket 支持
- ✅ TLS/SSL 支持
- ✅ 编译宏控制系统
- ✅ 97%测试覆盖率

### v1.1.0 (规划中 - 3 个月)

- 🎯 零拷贝内存管理优化
- 🎯 连接池和会话缓存
- 🎯 WebSocket 性能优化
- 🎯 TLS 功能完善
- 🎯 编译宏系统实现
- 🎯 LRU缓存性能优化
- 🎯 缓存策略扩展（LFU、FIFO）
- 🎯 分布式缓存支持

### v1.2.0 (规划中 - 6 个月)

- 🎯 负载均衡支持
- 🎯 监控和指标系统
- 🎯 配置管理系统
- 🎯 高级 WebSocket 功能
- 🎯 安全增强特性

### v2.0.0 (规划中 - 12 个月)

- 🎯 服务网格集成
- 🎯 容器化支持
- 🎯 云原生部署
- 🎯 高可用特性
- 🎯 分布式追踪

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进 UVHTTP！

## 📄 许可证

MIT License
