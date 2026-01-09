# 限流 API 文档

## 概述

UVHTTP 提供服务器级别的限流功能，用于防止 DDoS 攻击和过载。限流功能是核心功能，可以在编译时通过宏配置启用或禁用。

## 特性

- **服务器级别限流**：所有请求共享同一个限流计数器
- **固定窗口算法**：简单高效的限流算法
- **IP 白名单**：支持 IP 地址白名单，不受限流限制
- **零开销**：通过条件编译实现，禁用时无运行时开销
- **可配置**：支持自定义最大请求数和时间窗口

## 编译配置

### 启用限流功能（默认）

```bash
cmake ..
make
```

### 禁用限流功能

```bash
cmake -DUVHTTP_FEATURE_RATE_LIMIT=0 ..
make
```

## API 参考

### 启用限流

```c
uvhttp_error_t uvhttp_server_enable_rate_limit(
    uvhttp_server_t* server,
    int max_requests,
    int window_seconds,
    uvhttp_rate_limit_algorithm_t algorithm
);
```

**参数:**

- `server`: 服务器实例
- `max_requests`: 时间窗口内允许的最大请求数（范围：1-1000000）
- `window_seconds`: 时间窗口（秒）（范围：1-86400）
- `algorithm`: 限流算法类型

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
- `UVHTTP_ERROR_OUT_OF_MEMORY`: 内存分配失败

**限流算法类型:**

```c
typedef enum {
    UVHTTP_RATE_LIMIT_TOKEN_BUCKET,    /* 令牌桶算法 */
    UVHTTP_RATE_LIMIT_FIXED_WINDOW,    /* 固定窗口算法 */
    UVHTTP_RATE_LIMIT_LEAKY_BUCKET,    /* 漏桶算法 */
    UVHTTP_RATE_LIMIT_SLIDING_WINDOW   /* 滑动窗口算法 */
} uvhttp_rate_limit_algorithm_t;
```

**示例:**

```c
// 每秒最多 1000 个请求
uvhttp_server_enable_rate_limit(server, 1000, 1, UVHTTP_RATE_LIMIT_FIXED_WINDOW);

// 每分钟最多 6000 个请求
uvhttp_server_enable_rate_limit(server, 6000, 60, UVHTTP_RATE_LIMIT_FIXED_WINDOW);
```

### 禁用限流

```c
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t* server);
```

**参数:**

- `server`: 服务器实例

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**示例:**

```c
uvhttp_server_disable_rate_limit(server);
```

### 添加 IP 白名单

```c
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(
    uvhttp_server_t* server,
    const char* client_ip
);
```

**参数:**

- `server`: 服务器实例
- `client_ip`: 客户端 IP 地址（如 "127.0.0.1"）

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
- `UVHTTP_ERROR_OUT_OF_MEMORY`: 内存分配失败

**示例:**

```c
// 本地回环地址不受限流
uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");

// 内网地址不受限流
uvhttp_server_add_rate_limit_whitelist(server, "10.0.0.1");
```

### 获取限流状态

```c
uvhttp_error_t uvhttp_server_get_rate_limit_status(
    uvhttp_server_t* server,
    const char* client_ip,
    int* remaining,
    uint64_t* reset_time
);
```

**参数:**

- `server`: 服务器实例
- `client_ip`: 客户端 IP 地址（当前未使用，保留以备将来扩展）
- `remaining`: 剩余请求数（输出参数）
- `reset_time`: 重置时间戳（毫秒）（输出参数）

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**注意:**

- 当前实现是服务器级别限流，`client_ip` 参数未使用
- 返回的 `remaining` 和 `reset_time` 是服务器级别的状态

**示例:**

```c
int remaining;
uint64_t reset_time;
uvhttp_server_get_rate_limit_status(server, "127.0.0.1", &remaining, &reset_time);
printf("剩余请求数: %d, 重置时间: %lu\n", remaining, reset_time);
```

### 重置客户端限流状态

```c
uvhttp_error_t uvhttp_server_reset_rate_limit_client(
    uvhttp_server_t* server,
    const char* client_ip
);
```

**参数:**

- `server`: 服务器实例
- `client_ip`: 客户端 IP 地址（当前未使用，保留以备将来扩展）

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**注意:**

- 当前实现会重置整个服务器的限流计数器
- `client_ip` 参数未使用，保留以备将来扩展

**示例:**

```c
uvhttp_server_reset_rate_limit_client(server, "127.0.0.1");
```

### 清空所有限流状态

```c
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server);
```

**参数:**

- `server`: 服务器实例

**返回值:**

- `UVHTTP_OK`: 成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**示例:**

```c
uvhttp_server_clear_rate_limit_all(server);
```

## 使用示例

### 基本使用

```c
#include "uvhttp.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);

    // 启用限流：每秒最多 1000 个请求
    uvhttp_server_enable_rate_limit(server, 1000, 1,
                                    UVHTTP_RATE_LIMIT_FIXED_WINDOW);

    // 添加白名单
    uvhttp_server_add_rate_limit_whitelist(server, "127.0.0.1");

    // 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_server_free(server);
    return 0;
}
```

### 动态调整限流

```c
// 在运行时调整限流参数
void adjust_rate_limit(uvhttp_server_t* server, int new_max_requests) {
    // 先禁用
    uvhttp_server_disable_rate_limit(server);

    // 重新启用
    uvhttp_server_enable_rate_limit(server, new_max_requests, 1,
                                    UVHTTP_RATE_LIMIT_FIXED_WINDOW);
}
```

## 限流响应

当请求超过限流时，服务器会返回：

- **状态码**: 429 Too Many Requests
- **Content-Type**: text/plain
- **Retry-After**: 60（建议 60 秒后重试）
- **响应体**: "Too Many Requests"

## 设计说明

### 服务器级别限流

当前实现使用服务器级别的限流，所有请求共享同一个限流计数器。这种设计适合：

- **DDoS 防护**：限制服务器接收的总请求量
- **资源保护**：防止服务器因过载而崩溃
- **简单高效**：无需维护每个客户端的状态

### 为什么不是客户端级别？

客户端级别的限流需要：

1. 为每个客户端维护独立的限流状态
2. 使用哈希表存储客户端状态
3. 定期清理过期的客户端状态
4. 更复杂的内存管理

对于 DDoS 防护场景，服务器级别限流已经足够，而且：

- **性能更好**：无需哈希表查找
- **内存更少**：只有一个计数器
- **实现简单**：代码更易维护

### 未来扩展

如果需要客户端级别的限流，可以考虑：

1. 使用 `uthash` 库存储客户端状态
2. 实现滑动窗口算法
3. 添加客户端状态过期机制
4. 提供更细粒度的限流控制

## 错误码

限流相关的错误码：

```c
UVHTTP_ERROR_RATE_LIMIT_EXCEEDED = -550  // 超过限流
```

## 性能考虑

- **时间复杂度**: O(1) - 固定窗口算法
- **空间复杂度**: O(1) - 只维护一个计数器
- **开销**: 极小，只有简单的计数和比较操作

## 最佳实践

1. **合理配置限流参数**

   - 根据服务器性能设置 `max_requests`
   - 根据业务需求设置 `window_seconds`
   - 避免过于严格的限流影响正常用户

2. **使用白名单**

   - 将受信任的 IP 地址添加到白名单
   - 包括监控系统、内部服务等
   - 避免限流影响关键服务

3. **监控限流状态**

   - 定期检查 `remaining` 和 `reset_time`
   - 记录限流触发事件
   - 根据实际情况调整参数

4. **优雅降级**
   - 超过限流时返回清晰的错误信息
   - 提供 `Retry-After` 头
   - 考虑实现缓存层减轻服务器压力

## 相关文档

- [API 参考](API_REFERENCE.md)
- [架构设计](ARCHITECTURE.md)
- [错误码参考](ERROR_CODES.md)
- [功能模块系统](MIDDLEWARE_SYSTEM.md)
