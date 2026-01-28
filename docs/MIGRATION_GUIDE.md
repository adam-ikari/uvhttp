# UVHTTP 迁移指南

本指南帮助您从 UVHTTP 2.1.0 迁移到 2.2.0 版本。

## 概述

UVHTTP 2.2.0 是一次重大架构重构，主要目标是：
- 移除所有运行时抽象层
- 实现零开销抽象
- 简化代码架构
- 提高性能和可维护性

## 破坏性变更

### 1. 初始化函数返回值变更

所有初始化函数的返回值从指针类型改为 `uvhttp_error_t`，通过输出参数返回对象。

#### uvhttp_config_new()

```c
// 旧代码（2.1.0 及之前）
uvhttp_config_t* config = uvhttp_config_new();
if (!config) {
    fprintf(stderr, "Failed to create config\n");
    return 1;
}

// 新代码（2.2.0）
uvhttp_config_t* config = NULL;
uvhttp_error_t result = uvhttp_config_new(&config);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}
```

#### uvhttp_context_create()

```c
// 旧代码（2.1.0 及之前）
uvhttp_context_t* context = uvhttp_context_create(loop);
if (!context) {
    fprintf(stderr, "Failed to create context\n");
    return 1;
}

// 新代码（2.2.0）
uvhttp_context_t* context = NULL;
uvhttp_error_t result = uvhttp_context_create(loop, &context);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}
```

#### 其他受影响的函数

- `uvhttp_connection_new()`
- `uvhttp_router_new()`
- `uvhttp_server_new()`
- `uvhttp_static_create()`
- 所有其他创建函数

### 2. 依赖注入系统已移除

`uvhttp_deps.h` 和相关的 provider 抽象已被移除。

#### uvhttp_deps_t

```c
// 旧代码（2.1.0 及之前）
uvhttp_deps_t* deps = uvhttp_deps_new();
uvhttp_deps_set_loop_provider(deps, provider);
uvhttp_deps_set_memory_provider(deps, provider);
uvhttp_set_deps(deps);

// 新代码（2.2.0）
// 直接使用 libuv 和标准库函数
uv_loop_t* loop = uv_default_loop();
```

#### uvhttp_connection_provider_t

```c
// 旧代码（2.1.0 及之前）
uvhttp_connection_provider_t* provider = uvhttp_default_connection_provider_create();
uvhttp_context_set_connection_provider(context, provider);

// 新代码（2.2.0）
// 连接管理直接在应用层实现
uvhttp_connection_t* conn = uvhttp_connection_new(server);
```

#### uvhttp_logger_provider_t

```c
// 旧代码（2.1.0 及之前）
uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
uvhttp_context_set_logger_provider(context, logger);

// 新代码（2.2.0）
// 使用编译期宏
UVHTTP_LOG_INFO("Server started on port %d", port);
```

#### uvhttp_config_provider_t

```c
// 旧代码（2.1.0 及之前）
uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
uvhttp_context_set_config_provider(context, provider);

// 新代码（2.2.0）
// 配置管理直接使用 uvhttp_config 模块
uvhttp_config_t* config = NULL;
uvhttp_config_new(&config);
```

### 3. 日志系统重构

日志系统从运行时 provider 改为编译期宏实现。

```c
// 旧代码（2.1.0 及之前）
#include "uvhttp_logging.h"

uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
logger->log(logger, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
           "Server started on port %d", port);

// 新代码（2.2.0）
#include "uvhttp_logging.h"

UVHTTP_LOG_INFO("Server started on port %d", port);
UVHTTP_LOG_ERROR("Failed to connect: %s", error_msg);
UVHTTP_LOG_DEBUG("Processing request: %s", path);
```

**注意**: Release 模式下（`-DNDEBUG`），日志宏为空操作，零运行时开销。

### 4. 中间件架构变更

中间件从动态链表改为编译期宏定义。

```c
// 旧代码（2.1.0 及之前）
#include "uvhttp_middleware.h"

uvhttp_middleware_chain_t* chain = uvhttp_middleware_chain_new();
uvhttp_middleware_chain_add(chain, auth_middleware);
uvhttp_middleware_chain_add(chain, cors_middleware);
uvhttp_middleware_chain_add(chain, rate_limit_middleware);

// 新代码（2.2.0）
#include "uvhttp_middleware.h"

UVHTTP_DEFINE_MIDDLEWARE_CHAIN(middleware_chain,
    auth_middleware,
    cors_middleware,
    rate_limit_middleware
);

void request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    UVHTTP_EXECUTE_MIDDLEWARE(middleware_chain, req, res);
    // 处理请求
}
```

### 5. WebSocket 实现重命名

WebSocket 头文件统一为 `uvhttp_websocket.h`。

```c
// 旧代码（2.1.0 及之前）
#include "uvhttp_websocket_native.h"
uvhttp_ws_native_server_t* server = uvhttp_ws_native_server_new();

// 新代码（2.2.0）
#include "uvhttp_websocket.h"
uvhttp_ws_server_t* server = uvhttp_ws_server_new();
```

### 6. 网络接口抽象已移除

`uvhttp_network.h` 和 `uvhttp_network_interface_t` 已被移除。

```c
// 旧代码（2.1.0 及之前）
#include "uvhttp_network.h"

uvhttp_network_interface_t* network = uvhttp_network_interface_create(loop);
uvhttp_network_interface_set_mode(network, UVHTTP_NETWORK_LIBUV);

// 新代码（2.2.0）
// 直接使用 libuv 函数
uv_tcp_t* tcp = (uv_tcp_t*)uvhttp_alloc(sizeof(uv_tcp_t));
uv_tcp_init(loop, tcp);
```

## 错误处理改进

所有函数现在返回 `uvhttp_error_t`，提供详细的错误信息。

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "Description: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "Suggestion: %s\n", uvhttp_error_suggestion(result));
    return 1;
}
```

## 完整迁移示例

### 旧代码（2.1.0）

```c
#include "uvhttp.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建配置
    uvhttp_config_t* config = uvhttp_config_new();
    
    // 创建上下文
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    // 设置日志提供者
    uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    uvhttp_context_set_logger_provider(context, logger);
    
    // 创建服务器
    uvhttp_server_t* server = uvhttp_server_new(context);
    
    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

### 新代码（2.2.0）

```c
#include "uvhttp.h"
#include "uvhttp_logging.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建配置
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create config: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 创建上下文
    uvhttp_context_t* context = NULL;
    result = uvhttp_context_create(loop, &context);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create context: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 初始化上下文
    result = uvhttp_context_init(context);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to init context: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(context, &server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    // 启动服务器
    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    UVHTTP_LOG_INFO("Server started on port 8080");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    uvhttp_server_free(server);
    uvhttp_context_destroy(context);
    uvhttp_config_free(config);
    
    return 0;
}
```

## 性能影响

迁移到 2.2.0 后，您将获得以下性能改进：

1. **零开销抽象**: 编译期宏实现，Release 模式下完全零开销
2. **减少函数调用**: 移除 provider 抽象层，减少函数指针调用
3. **更好的编译器优化**: 编译器可以更好地优化代码

## 常见问题

### Q: 为什么移除依赖注入系统？

A: 依赖注入系统增加了不必要的复杂度，而且违反了"专注核心"的设计原则。应用层如果需要自定义行为，可以直接使用 libuv 和标准库函数。

### Q: 如何实现自定义日志？

A: 日志系统使用编译期宏，您可以通过定义自己的宏来替换默认实现：

```c
// 在编译时定义自定义日志宏
#define UVHTTP_LOG_INFO(fmt, ...) my_custom_log(INFO, fmt, ##__VA_ARGS__)
#define UVHTTP_LOG_ERROR(fmt, ...) my_custom_log(ERROR, fmt, ##__VA_ARGS__)
```

### Q: 如何实现自定义连接管理？

A: 连接管理应该在应用层实现。您可以在请求处理器中直接管理连接：

```c
void request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 自定义连接管理逻辑
    uvhttp_connection_t* conn = uvhttp_connection_get_from_request(req);
    // ... 自定义逻辑 ...
}
```

### Q: 测试代码如何迁移？

A: 测试代码应该使用集成测试或链接时注入（linker wrap）：

```bash
# 使用链接时注入测试 libuv
gcc -Wl,--wrap=uv_tcp_init test.c -luvhttp -luv
```

## 需要帮助？

如果您在迁移过程中遇到问题，请：

1. 查看示例代码：`examples/` 目录
2. 查看文档：`docs/` 目录
3. 提交 Issue：https://github.com/adam-ikari/uvhttp/issues

## 迁移检查清单

- [ ] 更新所有初始化函数调用，使用输出参数模式
- [ ] 移除所有 `uvhttp_deps.h` 相关代码
- [ ] 更新日志调用，使用编译期宏
- [ ] 更新中间件代码，使用编译期宏
- [ ] 更新 WebSocket 头文件引用
- [ ] 更新错误处理代码，使用 `uvhttp_error_t`
- [ ] 测试所有修改的功能
- [ ] 运行性能测试，确保没有性能回归