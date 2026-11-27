# UVHTTP 统一响应指南

## 概述

这个库不猜 Content-Type。状态码、头、body 由 handler 直接设置，发送只走一个入口。

## 核心特性

响应对象由调用方自己填：状态码、头、body 各一个 setter，最后 send。中间没有序列化层。头里写什么，发出去就是什么，出错也是自己的。

## 核心 API 参考

### 原子响应 API

逐步 setter 已不再公开，改用三个经过校验的入口：

```c
uvhttp_error_t uvhttp_respond(uvhttp_response_t* response, int status_code,
                              const char* content_type, const char* body,
                              size_t body_length);

uvhttp_error_t uvhttp_respond_with_headers(
    uvhttp_response_t* response, int status_code,
    const uvhttp_header_view_t* headers, size_t header_count,
    const char* body, size_t body_length);

uvhttp_error_t uvhttp_response_prepare(
    uvhttp_response_t* response, int status_code,
    const uvhttp_header_view_t* headers, size_t header_count,
    const char* body, size_t body_length);
```

`uvhttp_respond` 设置状态码、可选的 `Content-Type` 和响应体，然后发送；`uvhttp_respond_with_headers` 是多头部版本；`uvhttp_response_prepare` 做同样的校验和填充但**不发送**，适合只准备头部的中间件。三者都会在改动响应前完成校验，失败时恢复状态码和头部数量。

## 使用示例

### JSON 响应

```c
int json_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"Hello World\"}";





    return uvhttp_respond(res, 200, "application/json; charset=utf-8", json, strlen(json));
}
```

### HTML 响应

```c
int html_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>Hello World</h1></body></html>";





    return uvhttp_respond(res, 200, "text/html; charset=utf-8", html, strlen(html));
}
```

### 完整示例

```c
#include "uvhttp.h"

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* accept_header = uvhttp_request_get_header(req, "Accept");

    if (accept_header && strstr(accept_header, "application/json")) {
        const char* json = "{\"message\":\"Hello World\"}";



        return uvhttp_respond(res, 200, "application/json; charset=utf-8", json, strlen(json));
    } else {
        const char* html = "<html><body><h1>Hello World</h1></body></html>";



        return uvhttp_respond(res, 200, "text/html; charset=utf-8", html, strlen(html));
    }
}
```

### 错误处理

```c
uvhttp_result_t error_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_error_t result = uvhttp_send_error_response(res, 400, "invalid_parameters", "Missing required parameters");
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}
```

## 设计原则

### 使用者控制原则

响应怎么发由使用者决定，库不判断内容。具体是：

1. **Content-Type 完全由使用者控制**：不进行任何自动检测或推断
2. **统一的发送接口**：简化响应发送流程，减少代码重复
3. **明确的意图表达**：使用者需要明确指定内容类型
4. **灵活性优先**：支持任意 Content-Type，不限制于预定义类型

### 推荐的 Content-Type

库不检测类型，头里的值原样发出去。常用值：

- JSON: `application/json`
- HTML: `text/html; charset=utf-8`
- XML: `application/xml`
- CSS: `text/css`
- JavaScript: `application/javascript`
- 纯文本: `text/plain; charset=utf-8`

## 最佳实践

1. **先设 Content-Type**：头要在 `uvhttp_send_unified_response()` 之前设置，写错了没有兜底
2. **便捷函数用于明确场景**：当内容类型明确时，可以使用便捷函数如 `uvhttp_send_unified_response()`（`Content-Type` 直接传给 `uvhttp_respond`，或先用 `uvhttp_response_prepare` 准备头部）
3. **错误响应走统一入口**：`uvhttp_send_error_response()`，这样格式才一致
4. **发送前检查内容**：长度参数要和实际数据一致，否则发出去的内容不受控
5. **同一项目里统一 Content-Type 写法**：带不带 charset、用哪种别名，定下来就别改

## 性能考虑

发送路径就是几个 setter 加一次写操作，没有类型检测或格式转换。省下的开销来自不做的事：不检测类型、不转换格式。

## 兼容性

新的统一响应 API 和旧的响应 API 可以混在同一个项目里，不必一次改完。新代码用新的，旧代码遇到了再改。

## 示例项目

- `examples/unified_response_demo.c` - 完整的统一响应处理演示
- `examples/json_api_demo.c` - 更新后的 JSON API 示例，展示统一响应处理的使用

## 总结

约定只有一条：库不猜内容类型，头由你写。写对了照发，写错了也照发。
