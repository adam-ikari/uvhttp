# UVHTTP Unified Response Guide

## Overview

UVHTTP unified response handling.

## Core Features

- **Direct Control**: application developers directly set the response status code, headers, and body
- **Type Safety**: compile-time type checking and runtime validation
- **Flexibility**: supports arbitrary content types and custom headers
- **Performance Optimization**: zero-overhead abstraction that maps directly to libuv operations
- **Error Handling**: unified error response format and handling

## Core API Reference

### Response Object Operations

#### `uvhttp_response_set_status()`

Sets the HTTP status code.

```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
```

**Parameters:**
- `response`: the response object
- `status_code`: the HTTP status code (e.g., 200, 404)

**Return value:**
- `UVHTTP_OK`: success
- Other values: error codes

#### `uvhttp_response_set_header()`

Sets a response header.

```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                         const char* name,
                                         const char* value);
```

**Parameters:**
- `response`: the response object
- `name`: the header name
- `value`: the header value

#### `uvhttp_response_set_body()`

Sets the response body.

```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                       const char* body,
                                       size_t length);
```

**Parameters:**
- `response`: the response object
- `body`: the response body content
- `length`: the content length

#### `uvhttp_response_send()`

Sends the response.

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

## Usage Examples

### JSON Response

```c
int json_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"Hello World\"}";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));

    return uvhttp_response_send(res);
}
```

### HTML Response

```c
int html_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>Hello World</h1></body></html>";

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));

    return uvhttp_response_send(res);
}
```

### Complete Example

```c
#include "uvhttp.h"

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* accept_header = uvhttp_request_get_header(req, "Accept");

    if (accept_header && strstr(accept_header, "application/json")) {
        const char* json = "{\"message\":\"Hello World\"}";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, json, strlen(json));
        return uvhttp_response_send(res);
    } else {
        const char* html = "<html><body><h1>Hello World</h1></body></html>";
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
        uvhttp_response_set_body(res, html, strlen(html));
        return uvhttp_response_send(res);
    }
}
```

### Error Handling

```c
uvhttp_result_t error_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_error_t result = uvhttp_send_error_response(res, 400, "invalid_parameters", "Missing required parameters");
    return (result == UVHTTP_OK) ? UVHTTP_OK : UVHTTP_ERROR_RESPONSE_SEND;
}
```

## Design Principles

### Caller Control Principle

UVHTTP unified response handling follows the caller control principle:

1. **Content-Type is fully controlled by the caller**: no automatic detection or inference
2. **Unified send interface**: simplifies the response sending flow and reduces code duplication
3. **Explicit intent**: the caller must explicitly specify the content type
4. **Flexibility first**: supports any Content-Type, not limited to predefined types

### Recommended Content-Types

Although no automatic detection is performed, the use of standard Content-Types is recommended:

- JSON: `application/json`
- HTML: `text/html; charset=utf-8`
- XML: `application/xml`
- CSS: `text/css`
- JavaScript: `application/javascript`
- Plain text: `text/plain; charset=utf-8`

## Best Practices

1. **Set Content-Type explicitly**: always set the correct Content-Type before using `uvhttp_send_unified_response()`
2. **Use convenience functions for unambiguous scenarios**: when the content type is clear, use convenience functions such as `uvhttp_send_json_response()`
3. **Unified error handling**: use `uvhttp_send_error_response()` for consistent error response format
4. **Content validation**: validate content correctness before sending the response
5. **Maintain consistency**: keep Content-Type settings consistent within the same project

## Performance Considerations

- Unified response handling functions optimize memory usage internally
- Convenience functions reduce function call overhead, making them suitable for performance-sensitive scenarios
- No content type detection avoids extra computational overhead

## Compatibility

- The new unified response handling APIs are fully compatible with the existing response handling APIs
- New and old APIs can be mixed within the same project
- New code is recommended to use unified response handling; existing code can migrate incrementally

## Example Projects

- `examples/unified_response_demo.c` - a complete unified response handling demonstration
- `examples/json_api_demo.c` - the updated JSON API example demonstrating unified response handling

## Summary

Unified response handling simplifies the HTTP response sending flow. Application developers can use a unified API to handle all types of response content while maintaining full control over Content-Type. This design provides both convenience and flexibility with explicitness, improving development efficiency and code maintainability.
