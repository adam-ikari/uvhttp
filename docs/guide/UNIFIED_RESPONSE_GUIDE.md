# UVHTTP 响应处理指南

## 概述

UVHTTP 提供简洁高效的响应处理机制，基于核心API构建。开发者通过直接操作响应对象，完全控制HTTP响应的各个方面，包括状态码、头部和内容。

## 核心特性

- **直接控制**：开发者直接设置响应状态码、头部和内容
- **类型安全**：编译时类型检查，运行时验证
- **灵活性**：支持任意内容类型和自定义头部
- **性能优化**：零开销抽象，直接映射到libuv操作
- **错误处理**：统一的错误响应格式和处理方式

## 核心 API 参考

### 响应对象操作

#### `uvhttp_response_set_status()`

设置HTTP状态码。

```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
```

**参数：**
- `response`: 响应对象
- `status_code`: HTTP状态码（如200、404等）

**返回值：**
- `UVHTTP_OK`: 成功
- 其他值：错误代码

#### `uvhttp_response_set_header()`

设置响应头。

```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, 
                                         const char* name, 
                                         const char* value);
```

**参数：**
- `response`: 响应对象
- `name`: 头部名称
- `value`: 头部值

#### `uvhttp_response_set_body()`

设置响应体。

```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, 
                                       const char* body, 
                                       size_t length);
```

**参数：**
- `response`: 响应对象
- `body`: 响应体内容
- `length`: 内容长度

#### `uvhttp_response_send()`

发送响应。

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

## 使用示例

### JSON 响应

```c
int json_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"Hello World\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}
```

### HTML 响应

```c
int html_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>Hello World</h1></body></html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}
```

### 完整示例

```c
#include "uvhttp.h"

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* accept_header = uvhttp_request_get_header(req, "Accept");
    
    if (accept_header && strstr(accept_header, "application/json")) {
        // JSON 响应
        const char* json = "{\"message\":\"Hello World\"}";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, json, strlen(json));
        return uvhttp_response_send(res);
    }
    else {
        // HTML 响应
        const char* html = "<html><body><h1>Hello World</h1></body></html>";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
        uvhttp_response_set_body(res, html, strlen(html));
        return uvhttp_response_send(res);
    }
}
```

### 错误处理

```c
uvhttp_result_t error_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 使用统一的错误响应函数
    uvhttp_error_t result = uvhttp_send_error_response(res, 400, "请求参数错误", "缺少必需的参数");
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}
```

## 设计原则

### 使用者控制原则

UVHTTP 统一响应处理遵循使用者控制原则：

1. **Content-Type 完全由使用者控制**：不进行任何自动检测或推断
2. **统一的发送接口**：简化响应发送流程，减少代码重复
3. **明确的意图表达**：使用者需要明确指定内容类型
4. **灵活性优先**：支持任意 Content-Type，不限制于预定义类型

### 推荐的 Content-Type

虽然不进行自动检测，但推荐使用标准的 Content-Type：

- JSON: `application/json`
- HTML: `text/html; charset=utf-8`
- XML: `application/xml`
- CSS: `text/css`
- JavaScript: `application/javascript`
- 纯文本: `text/plain; charset=utf-8`

## 最佳实践

1. **明确设置 Content-Type**：在使用 `uvhttp_send_unified_response()` 前务必设置正确的 Content-Type
2. **便捷函数用于明确场景**：当内容类型明确时，可以使用便捷函数如 `uvhttp_send_json_response()`
3. **错误处理统一化**：使用 `uvhttp_send_error_response()` 确保错误响应格式一致
4. **内容验证**：在发送响应前验证内容的正确性
5. **保持一致性**：在同一个项目中保持 Content-Type 设置的一致性

## 性能考虑

- 统一响应处理函数内部优化了内存使用
- 便捷函数减少了函数调用开销，适用于性能敏感的场景
- 不进行内容类型检测，避免了额外的计算开销

## 兼容性

- 新的统一响应处理 API 与现有的响应处理 API 完全兼容
- 可以在同一个项目中混合使用新旧 API
- 推荐新代码使用统一响应处理，现有代码可以逐步迁移

## 示例项目

- `examples/unified_response_demo.c` - 完整的统一响应处理演示
- `examples/json_api_demo.c` - 更新后的 JSON API 示例，展示统一响应处理的使用

## 总结

统一响应处理简化了 HTTP 响应的发送流程，开发者可以使用统一的 API 处理所有类型的响应内容，同时保持对 Content-Type 的完全控制。这种设计既提供了便利性，又保持了灵活性和明确性，提高了开发效率和代码的可维护性。