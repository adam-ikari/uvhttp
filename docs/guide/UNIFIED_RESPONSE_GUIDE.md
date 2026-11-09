# UVHTTP Unified Response Guide

## Overview

A handler sets the status, the headers and the body, then sends. UVHTTP does
not infer the content type for you.

## Core Features

- The handler sets the status code, headers and body directly.
- Argument types are checked at compile time and validated at run time.
- Any content type and any custom header is allowed.
- The calls map straight to libuv operations with no layer in between.
- Errors share one response format.

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

The library never guesses a content type:

1. You set the Content-Type header; nothing is detected or inferred.
2. One send call covers every response, so the sending code lives in one place.
3. The content type is explicit at the call site.
4. Any content type works; there is no fixed list.

### Recommended Content-Types

Although no automatic detection is performed, standard Content-Types are the
safe choice:

- JSON: `application/json`
- HTML: `text/html; charset=utf-8`
- XML: `application/xml`
- CSS: `text/css`
- JavaScript: `application/javascript`
- Plain text: `text/plain; charset=utf-8`

## Best Practices

1. Set the Content-Type before you call `uvhttp_send_unified_response()`.
2. When the type is clear, a convenience function such as `uvhttp_send_unified_response()` does the work; set the `Content-Type` header first with `uvhttp_response_set_header(res, "Content-Type", "application/json")`.
3. Send errors through `uvhttp_send_error_response()` so the format stays the same everywhere.
4. Check the content before sending it.
5. Keep the Content-Type values consistent across the project.

## Performance Considerations

- The response functions manage memory internally
- Convenience functions cut the number of calls per response, which matters in performance-sensitive paths
- Skipping content type detection leaves that work out of every response

## Compatibility

- The unified response APIs stay compatible with the existing response APIs
- Old and new calls can be mixed in the same project
- New code should use the unified response handling; existing code can migrate one handler at a time

## Example Projects

- `examples/unified_response_demo.c` - shows the unified response API end to end
- `examples/json_api_demo.c` - the JSON API example, updated to use it

## Summary

One send path handles every response. The caller still sets the Content-Type,
so nothing changes behind your back, and every handler follows the same
sequence: status, headers, body, send.
