# UVHTTP 统一响应处理指南

## 概述

UVHTTP 现在提供统一的响应处理机制，简化响应发送流程，由使用者自行控制 Content-Type 设置。这样开发者可以使用统一的 API 处理各种类型的响应内容，同时保持对 Content-Type 的完全控制。

## 核心特性

- **使用者控制 Content-Type**：由使用者自行设置 Content-Type，不进行自动检测
- **统一响应接口**：使用相同的函数处理不同类型的响应
- **便捷函数**：提供针对特定内容类型的便捷函数
- **错误处理**：统一的错误响应格式和处理方式
- **类型安全**：编译时类型检查，运行时验证

## API 参考

### 主要函数

#### `uvhttp_send_unified_response()`

统一的响应发送函数，由使用者设置 Content-Type。

```c
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response, 
                                          const char* content, 
                                          size_t length, 
                                          int status_code);
```

**参数：**
- `response`: 响应对象
- `content`: 内容数据
- `length`: 内容长度（如果为0，会自动计算字符串长度）
- `status_code`: HTTP状态码（如果为0，使用响应对象中已有的状态码）

**返回值：**
- `UVHTTP_OK`: 成功
- 其他值：错误代码

**使用说明：**
- 使用者需要在使用此函数前设置 Content-Type
- 不进行自动内容类型检测
- 提供统一的响应发送接口

#### 便捷函数

```c
// 发送 JSON 响应
uvhttp_error_t uvhttp_send_json_response(uvhttp_response_t* response, 
                                        const char* json_content, 
                                        int status_code);

// 发送 HTML 响应
uvhttp_error_t uvhttp_send_html_response(uvhttp_response_t* response, 
                                        const char* html_content, 
                                        int status_code);

// 发送文本响应
uvhttp_error_t uvhttp_send_text_response(uvhttp_response_t* response, 
                                        const char* text_content, 
                                        int status_code);

// 发送错误响应（JSON格式）
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response, 
                                         int error_code, 
                                         const char* error_message, 
                                         const char* details);
```

## 使用示例

#### 旧方式 vs 新方式

##### 旧方式（多个步骤）

```c
// JSON 响应
uvhttp_response_set_status(res, 200);
uvhttp_response_set_header(res, "Content-Type", "application/json");
uvhttp_response_set_body(res, json_string, strlen(json_string));
uvhttp_response_send(res);

// HTML 响应
uvhttp_response_set_status(res, 200);
uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
uvhttp_response_set_body(res, html_string, strlen(html_string));
uvhttp_response_send(res);
```

##### 新方式（统一处理，由使用者控制 Content-Type）

```c
// JSON 响应
uvhttp_response_set_header(res, "Content-Type", "application/json");
uvhttp_send_unified_response(res, json_string, strlen(json_string), 200);

// HTML 响应
uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
uvhttp_send_unified_response(res, html_string, strlen(html_string), 200);
```

### 完整示例

```c
#include "uvhttp.h"
#include "uvhttp_utils.h"

uvhttp_result_t handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* accept_header = uvhttp_request_get_header(req, "Accept");
    
    if (accept_header && strstr(accept_header, "application/json")) {
        // JSON 响应
        const char* json = "{\"message\":\"Hello World\"}";
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        return uvhttp_send_unified_response(res, json, strlen(json), 200) == UVHTTP_OK 
               ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
    }
    else {
        // HTML 响应
        const char* html = "<html><body><h1>Hello World</h1></body></html>";
        uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
        return uvhttp_send_unified_response(res, html, strlen(html), 200) == UVHTTP_OK 
               ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
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