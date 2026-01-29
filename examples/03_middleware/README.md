# UVHTTP 中间件示例

本目录包含 UVHTTP 中间件系统的使用示例。

## 设计理念

UVHTTP 的中间件系统遵循以下原则：

- **零开销抽象**：使用编译期宏，无运行时动态分配
- **固定管线**：中间件顺序在编译时确定
- **简洁清晰**：避免链表、优先级等复杂概念
- **应用层主导**：所有中间件逻辑由应用层实现

## 示例程序

### 1. middleware_compile_time_demo.c
编译期中间件配置示例，演示如何：
- 使用 `UVHTTP_EXECUTE_MIDDLEWARE` 宏配置中间件管线
- 实现日志、认证、CORS 等中间件
- 不同路由使用不同中间件管线

**运行方式**：
```bash
cd build
./dist/bin/middleware_compile_time_demo
```

**测试**：
```bash
curl http://localhost:8082/public
curl http://localhost:8082/protected
curl -H 'Authorization: Bearer secret-token' http://localhost:8082/protected
```

### 2. middleware_chain_demo.c
中间件链复用示例，演示如何：
- 使用 `UVHTTP_DEFINE_MIDDLEWARE_CHAIN` 定义中间件链
- 使用 `UVHTTP_EXECUTE_MIDDLEWARE_CHAIN` 执行预定义的中间件链
- 在多个处理器中复用中间件链

**运行方式**：
```bash
cd build
./dist/bin/middleware_chain_demo
```

**测试**：
```bash
curl http://localhost:8083/health
curl http://localhost:8083/api/user
curl -H 'Authorization: Bearer secret-token' http://localhost:8083/api/user
curl -H 'Authorization: Bearer secret-token' http://localhost:8083/api/admin
```

### 3. rate_limit_demo.c
限流中间件示例，演示如何：
- 由应用层实现限流功能
- 使用令牌桶算法
- 返回 429 状态码和 Retry-After 头

**运行方式**：
```bash
cd build
./dist/bin/rate_limit_demo
```

**测试**：
```bash
curl http://localhost:8085/api
for i in {1..10}; do curl http://localhost:8085/api; done
```

### 4. test_middleware.c
中间件测试示例，用于验证中间件系统是否正常工作。

**运行方式**：
```bash
cd build
./dist/bin/test_middleware
```

## 中间件 API

### UVHTTP_EXECUTE_MIDDLEWARE

执行中间件链：

```c
UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
    middleware1,
    middleware2,
    middleware3
);
```

### UVHTTP_DEFINE_MIDDLEWARE_CHAIN

定义中间件链（供复用）：

```c
UVHTTP_DEFINE_MIDDLEWARE_CHAIN(api_chain,
    logging_middleware,
    auth_middleware,
    cors_middleware
);
```

### UVHTTP_EXECUTE_MIDDLEWARE_CHAIN

执行预定义的中间件链：

```c
UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, api_chain);
```

## 中间件函数签名

所有中间件函数必须遵循以下签名：

```c
typedef int (*uvhttp_middleware_handler_t)(
    uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
);
```

返回值：
- `UVHTTP_MIDDLEWARE_CONTINUE` - 继续执行下一个中间件
- `UVHTTP_MIDDLEWARE_STOP` - 停止执行中间件链

## 注意事项

1. **中间件返回 STOP 时应已发送响应**：如果中间件返回 `UVHTTP_MIDDLEWARE_STOP`，表示中间件已经处理了请求并发送了响应
2. **避免在中间件中阻塞**：中间件应该快速执行，避免阻塞事件循环
3. **线程安全**：如果中间件需要访问共享状态，确保线程安全
4. **资源清理**：如果中间件分配了资源，确保在适当的时候释放

## 应用层实现限流

限流功能完全由应用层实现，UVHTTP 库不提供限流功能。应用层可以根据需求选择：

- 令牌桶算法
- 漏桶算法
- 固定窗口算法
- 滑动窗口算法

参考 `rate_limit_demo.c` 了解如何实现限流中间件。