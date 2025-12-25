# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Coverage](https://img.shields.io/badge/coverage-97%25-green.svg)
![Performance](https://img.shields.io/badge/high%20performance-brightgreen.svg)
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
- ⚡ 集成xxHash极快哈希算法（比CRC32快3-5倍）
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

### 📊 **系统配置**

- **最大并发连接**: 2048 (生产环境推荐值)
- **请求体大小限制**: 1MB
- **读取缓冲区**: 8KB
- **监听队列**: 1024

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

📖 **详细构建说明**: 请参考 [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md#快速开始) 获取完整的构建选项和配置说明。

## 示例

### HTTP 服务器

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

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);

    printf("Server running on http://localhost:%d\n", UVHTTP_DEFAULT_PORT);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

📖 **更多示例**: 查看 [examples/](examples/) 目录获取 WebSocket、静态文件服务器等完整示例。

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

## 📝 JSON 处理指南

UVHTTP 采用**轻量级设计原则**，不内置 JSON 序列化/反序列化功能，推荐用户根据需求选择合适的 JSON 库。

### 推荐方案：cJSON

UVHTTP 项目已集成 **cJSON** 作为依赖，提供以下优势：
- ✅ **轻量级**：只有 2 个源文件，无外部依赖
- ✅ **高性能**：优化的解析和生成算法
- ✅ **易集成**：简单的 API 设计
- ✅ **MIT 许可证**：商业友好

### 基础使用示例

```c
#include "../../deps/cjson/cJSON.h"

// 创建 JSON 对象
cJSON* root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "status", "success");
cJSON_AddNumberToObject(root, "code", 200);
cJSON_AddBoolToObject(root, "active", true);

// 添加数组
cJSON* tags = cJSON_CreateArray();
cJSON_AddItemToArray(tags, cJSON_CreateString("developer"));
cJSON_AddItemToArray(tags, cJSON_CreateString("golang"));
cJSON_AddItemToObject(root, "tags", tags);

// 序列化为字符串
char* json_string = cJSON_PrintUnformatted(root);
// 输出: {"status":"success","code":200,"active":true,"tags":["developer","golang"]}

// 发送响应
uvhttp_response_set_status(response, 200);
uvhttp_response_set_header(response, "Content-Type", "application/json");
uvhttp_response_set_body(response, json_string, strlen(json_string));
uvhttp_response_send(response);

// 清理资源
free(json_string);
cJSON_Delete(root);
```

### 高级功能

```c
// 解析 JSON
cJSON* parsed = cJSON_Parse(json_string);
if (!parsed) {
    const char* error_ptr = cJSON_GetErrorPtr();
    fprintf(stderr, "JSON 解析错误: %s\n", error_ptr);
    return UVHTTP_ERROR_PARSE_ERROR;
}

// 获取值
cJSON* status = cJSON_GetObjectItem(parsed, "status");
if (cJSON_IsString(status)) {
    printf("状态: %s\n", cJSON_GetStringValue(status));
}

// 遍历数组
cJSON* tags = cJSON_GetObjectItem(parsed, "tags");
if (cJSON_IsArray(tags)) {
    cJSON* tag = NULL;
    cJSON_ArrayForEach(tag, tags) {
        if (cJSON_IsString(tag)) {
            printf("标签: %s\n", cJSON_GetStringValue(tag));
        }
    }
}

cJSON_Delete(parsed);
```

### 其他 JSON 库选择

| 库 | 特点 | 适用场景 |
|------|------|----------|
| **cJSON** | 轻量级、无依赖 | 嵌入式系统、简单应用 |
| **yyjson** | 超高性能、SIMD 优化 | 高性能需求 |
| **rapidjson** | C++、功能丰富 | C++ 项目、复杂需求 |
| **json-c** | 功能完整、稳定可靠 | 企业级应用 |

### 最佳实践

1. **错误处理**：始终检查解析结果
2. **内存管理**：及时释放 cJSON 对象
3. **性能优化**：使用 `cJSON_PrintUnformatted` 减少内存分配
4. **类型检查**：使用 `cJSON_Is*` 函数验证类型

## 🏃‍♂️ 运行示例
## ⚡ xxHash 高性能哈希集成

UVHTTP 项目已集成 **xxHash** 作为核心哈希算法，提供以下优势：
- ✅ **极高性能**：比 CRC32 快 3-5 倍，接近 RAM 速度限制
- ✅ **优秀分布**：低冲突率，适合哈希表和缓存
- ✅ **跨平台**：支持所有主流平台和架构
- ✅ **简单易用**：统一的 API 接口，无需复杂配置

### 基础使用示例

```c
#include "uvhttp_hash.h"

// 计算字符串哈希
const char* data = "Hello, UVHTTP!";
uint64_t hash = uvhttp_hash_string(data);

// 计算数据哈希
uint64_t hash2 = uvhttp_hash(data, strlen(data), UVHTTP_HASH_DEFAULT_SEED);

// 使用默认种子
uint64_t hash3 = uvhttp_hash_default(data, strlen(data));
```

### 路由系统优化

xxHash 显著提升了路由查找性能：

```c
// 路由哈希计算（内部使用）
uint32_t route_hash = uvhttp_route_hash("/api/users", UVHTTP_GET);

// 缓存键生成
uint64_t cache_key = uvhttp_hash_string("user:123:profile");
```

### 性能对比

| 算法 | 速度 | 冲突率 | 适用场景 |
|------|------|--------|----------|
| **xxHash** | ⚡⚡⚡ 极快 | 低 | 路由、缓存、哈希表 |
| CRC32 | ⚡⚡ 快 | 中 | 校验和、简单哈希 |
| FNV-1a | ⚡ 中等 | 中 | 字符串哈希 |
| MD5 | ⚡ 慢 | 极低 | 安全哈希 |

### 最佳实践

1. **字符串哈希**：使用 `uvhttp_hash_string()` 处理字符串
2. **数据哈希**：使用 `uvhttp_hash()` 处理二进制数据
3. **默认种子**：使用 `UVHTTP_HASH_DEFAULT_SEED` 获得一致性
4. **安全考虑**：xxHash 适用于非加密场景### v1.2.0 (规划中 - 6 个月)

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
