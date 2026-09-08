# UVHTTP API Reference

**Version**: v2.7.2  
**Updated**: 2026-09-08  
**C Standard**: C11

## Overview

UVHTTP provides a concise, efficient C API for building HTTP/1.1 servers.

> This is a curated, tutorial-style reference covering the most common APIs.
> The complete, auto-generated reference for all ~290 public functions lives in
> [docs/api/generated/](./generated/index.html) (Doxygen).

## Core Types

### uvhttp_server_t

Server object that manages the entire HTTP server lifecycle.

```c
typedef struct uvhttp_server uvhttp_server_t;
```

### uvhttp_router_t

Router object that manages URL path and handler mappings.

```c
typedef struct uvhttp_router uvhttp_router_t;
```

### uvhttp_context_t

Context object that stores server runtime state.

```c
typedef struct uvhttp_context uvhttp_context_t;
```

### uvhttp_request_t

Request object that encapsulates HTTP request information.

```c
typedef struct uvhttp_request uvhttp_request_t;
```

### uvhttp_response_t

Response object that encapsulates HTTP response information.

```c
typedef struct uvhttp_response uvhttp_response_t;
```

## Server API

### uvhttp_server_new

```c
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server);
```

Creates a new server object.

**Parameters**:
- `loop`: libuv event loop
- `server`: Output parameter, returns server object pointer

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code (use `uvhttp_error_string()` to get error description)

**Example**:
```c
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server;
uvhttp_error_t result = uvhttp_server_new(loop, &server);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
    return 1;
}
```

### uvhttp_server_free

```c
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
```

Frees the server object.

**Parameters**:
- `server`: Server object

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code

**Example**:
```c
uvhttp_error_t result = uvhttp_server_free(server);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to free server: %s\n", uvhttp_error_string(result));
}
```

### uvhttp_server_listen

```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server,
                                   const char* host,
                                   int port);
```

Starts the server listening on the specified address and port.

**Parameters**:
- `server`: Server object
- `host`: Listen address (e.g., "0.0.0.0")
- `port`: Listen port

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code

**Example**:
```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to listen: %d\n", result);
    return 1;
}
```

## Router API

### uvhttp_router_new

```c
uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router);
```

Creates a new router object.

**Parameters**:
- `router`: Output parameter, returns router object pointer

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code (use `uvhttp_error_string()` to get error description)

### uvhttp_router_free

```c
void uvhttp_router_free(uvhttp_router_t* router);
```

Frees the router object.

### uvhttp_router_add_route

```c
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
```

Adds a routing rule.

**Parameters**:
- `router`: Router object
- `path`: URL path (e.g., "/api")
- `handler`: Handler function

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code (use `uvhttp_error_string()` to get error description)

**Example**:
```c
uvhttp_error_t result = uvhttp_router_add_route(router, "/", home_handler);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to add route: %s\n", uvhttp_error_string(result));
    return;
}

result = uvhttp_router_add_route(router, "/api", api_handler);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to add route: %s\n", uvhttp_error_string(result));
    return;
}
```

## Request Handling API

### uvhttp_request_get_method

```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
```

Gets the HTTP method.

**Return Value**: HTTP method string (e.g., "GET", "POST")

### uvhttp_request_get_path

```c
const char* uvhttp_request_get_path(uvhttp_request_t* request);
```

Gets the request path.

**Return Value**: URL path string

### uvhttp_request_get_header

```c
const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                     const char* name);
```

Gets a request header.

**Parameters**:
- `request`: Request object
- `name`: Header name

**Return Value**: Header value, returns `NULL` if not exists

**Example**:
```c
const char* content_type = uvhttp_request_get_header(request, "Content-Type");
```

### uvhttp_request_get_body

```c
const char* uvhttp_request_get_body(uvhttp_request_t* request);
```

Gets the request body (NULL-terminated). The body length is obtained
separately via `uvhttp_request_get_body_length()`.

**Parameters**:
- `request`: Request object

**Return Value**: Request body data pointer, or `NULL` when there is no body

### uvhttp_request_get_body_length

```c
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);
```

Gets the request body length in bytes.

**Parameters**:
- `request`: Request object

**Return Value**: Body length (0 when there is no body)


## Response Handling API

### uvhttp_response_set_status

```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code);
```

Sets the response status code.

**Parameters**:
- `response`: Response object
- `status_code`: HTTP status code (e.g., 200, 404)

**Example**:
```c
uvhttp_response_set_status(response, 200);
```

### uvhttp_response_set_header

```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                          const char* name,
                                          const char* value);
```

Sets a response header.

**Parameters**:
- `response`: Response object
- `name`: Header name
- `value`: Header value

**Example**:
```c
uvhttp_response_set_header(response, "Content-Type", "application/json");
```

### uvhttp_response_set_body

```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body,
                                        size_t len);
```

Sets the response body.

**Parameters**:
- `response`: Response object
- `body`: Response body data
- `len`: Response body length

**Example**:
```c
const char* body = "Hello, World!";
uvhttp_response_set_body(response, body, strlen(body));
```

### uvhttp_response_send

```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

Sends the response.

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code

## Context API

### uvhttp_context_create

```c
uvhttp_error_t uvhttp_context_create(uv_loop_t* loop,
                                    uvhttp_context_t** context);
```

Creates a context object.

**Parameters**:
- `loop`: libuv event loop
- `context`: Output parameter, returns context object

**Return Value**:
- `UVHTTP_OK`: Success
- Other values: Error code

### uvhttp_context_destroy

```c
void uvhttp_context_destroy(uvhttp_context_t* context);
```

Frees the context object.

**Parameters**:
- `context`: Context object

### uvhttp_error_string

```c
const char* uvhttp_error_string(uvhttp_error_t error);
```

Gets the error name.

**Return Value**: Error name string

### uvhttp_error_description

```c
const char* uvhttp_error_description(uvhttp_error_t error);
```

Gets the error description.

**Return Value**: Error description string

### uvhttp_error_suggestion

```c
const char* uvhttp_error_suggestion(uvhttp_error_t error);
```

Gets the fix suggestion.

**Return Value**: Fix suggestion string

### uvhttp_error_is_recoverable

```c
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```

Checks if the error is recoverable.

**Return Value**:
- `1`: Recoverable
- `0`: Not recoverable

## Memory Management API

### Basic Operations

UVHTTP provides a unified memory management interface with compile-time allocator selection.

```c
void* uvhttp_alloc(size_t size);
void* uvhttp_realloc(void* ptr, size_t size);
void uvhttp_free(void* ptr);
void* uvhttp_calloc(size_t nmemb, size_t size);
```

#### uvhttp_alloc

```c
void* uvhttp_alloc(size_t size);
```

Allocates memory.

**Parameters**:
- `size`: Number of bytes to allocate

**Return Value**:
- Success: Memory pointer
- Failure: `NULL`

**Example**:
```c
void* ptr = uvhttp_alloc(1024);
if (!ptr) {
    // Handle out of memory
}
```

#### uvhttp_free

```c
void uvhttp_free(void* ptr);
```

Frees memory.

**Parameters**:
- `ptr`: Memory pointer to free

**Example**:
```c
uvhttp_free(ptr);
```

#### uvhttp_realloc

```c
void* uvhttp_realloc(void* ptr, size_t size);
```

Reallocates memory.

**Parameters**:
- `ptr`: Original memory pointer
- `size`: New size

**Return Value**:
- Success: New memory pointer
- Failure: `NULL`

**Example**:
```c
ptr = uvhttp_realloc(ptr, 2048);
if (!ptr) {
    // Handle out of memory
}
```

#### uvhttp_calloc

```c
void* uvhttp_calloc(size_t nmemb, size_t size);
```

Allocates and initializes memory to zero.

**Parameters**:
- `nmemb`: Number of elements
- `size`: Size of each element

**Return Value**:
- Success: Memory pointer
- Failure: `NULL`

**Example**:
```c
int* array = uvhttp_calloc(100, sizeof(int));
if (!array) {
    // Handle out of memory
}
```

### Allocator Information

#### uvhttp_allocator_name

```c
const char* uvhttp_allocator_name(void);
```

Gets the current allocator name.

**Return Value**: Allocator name string ("system" or "mimalloc")

**Example**:
```c
printf("Using allocator: %s\n", uvhttp_allocator_name());
```

### Compilation Configuration

Select allocator type via CMake compilation macro:

```cmake
# System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc allocator
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### Performance Characteristics

- **Zero runtime overhead**: All functions are inline
- **Compile-time optimization**: Fully optimizable by compiler
- **Type safety**: Compile-time type checking
- **Predictability**: No dynamic dispatch

### Best Practices

1. **Unified usage**: Always use `uvhttp_alloc/uvhttp_free`, don't mix with `malloc/free`
2. **Paired allocation**: Every allocation has a corresponding free
3. **Check return values**: Check if allocation succeeded
4. **Avoid leaks**: Ensure all paths free memory

### Complete Example

```c
#include "uvhttp_allocator.h"

void example_memory_usage(void) {
    // Allocate memory
    char* buffer = uvhttp_alloc(1024);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }

    // Use memory
    strcpy(buffer, "Hello, World!");

    // Reallocate
    buffer = uvhttp_realloc(buffer, 2048);
    if (!buffer) {
        fprintf(stderr, "Failed to reallocate memory\n");
        return;
    }

    // Free memory
    uvhttp_free(buffer);
}
```

## Utility Functions API

### String Processing

#### uvhttp_safe_strcpy
```c
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);
```
Safe string copy (declared in `uvhttp_common.h`).

### Hash Functions

#### uvhttp_hash_string
```c
uint64_t uvhttp_hash_string(const char* str);
```
Calculates string hash value (declared in `uvhttp_hash.h`).

> **Note**: The `uvhttp_url_decode` function listed in earlier versions of this
> document has been removed — it never existed in the public API.

## Error Codes

Error codes are defined in `uvhttp_error.h` (`uvhttp_error_t`, alias
`uvhttp_result_t`). Use `uvhttp_error_string()` / `uvhttp_error_description()`
to format an error at runtime.

### General errors (all modules)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_OK | 0 | Success |
| UVHTTP_ERROR_INVALID_PARAM | -1 | Invalid parameter |
| UVHTTP_ERROR_OUT_OF_MEMORY | -2 | Out of memory |
| UVHTTP_ERROR_NOT_FOUND | -3 | Not found |
| UVHTTP_ERROR_ALREADY_EXISTS | -4 | Already exists |
| UVHTTP_ERROR_NULL_POINTER | -5 | NULL pointer |
| UVHTTP_ERROR_BUFFER_TOO_SMALL | -6 | Buffer too small |
| UVHTTP_ERROR_TIMEOUT | -7 | Timeout |
| UVHTTP_ERROR_CANCELLED | -8 | Cancelled |
| UVHTTP_ERROR_NOT_SUPPORTED | -9 | Not supported |

### Server (-100s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_SERVER_INIT | -100 | Server init failed |
| UVHTTP_ERROR_SERVER_LISTEN | -101 | Listen failed |
| UVHTTP_ERROR_SERVER_STOP | -102 | Stop failed |
| UVHTTP_ERROR_CONNECTION_LIMIT | -103 | Connection limit reached |
| UVHTTP_ERROR_SERVER_ALREADY_RUNNING | -104 | Server already running |
| UVHTTP_ERROR_SERVER_NOT_RUNNING | -105 | Server not running |
| UVHTTP_ERROR_SERVER_INVALID_CONFIG | -106 | Invalid server config |

### Connection (-200s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_CONNECTION_INIT | -200 | Connection init failed |
| UVHTTP_ERROR_CONNECTION_ACCEPT | -201 | Accept failed |
| UVHTTP_ERROR_CONNECTION_START | -202 | Connection start failed |
| UVHTTP_ERROR_CONNECTION_CLOSE | -203 | Connection close failed |
| UVHTTP_ERROR_CONNECTION_RESET | -204 | Connection reset |
| UVHTTP_ERROR_CONNECTION_TIMEOUT | -205 | Connection timeout |
| UVHTTP_ERROR_CONNECTION_REFUSED | -206 | Connection refused |
| UVHTTP_ERROR_CONNECTION_BROKEN | -207 | Connection broken |

### Request / Response (-300s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_REQUEST_INIT | -300 | Request init failed |
| UVHTTP_ERROR_RESPONSE_INIT | -301 | Response init failed |
| UVHTTP_ERROR_RESPONSE_SEND | -302 | Response send failed |
| UVHTTP_ERROR_INVALID_HTTP_METHOD | -303 | Invalid HTTP method |
| UVHTTP_ERROR_INVALID_HTTP_VERSION | -304 | Invalid HTTP version |
| UVHTTP_ERROR_HEADER_TOO_LARGE | -305 | Header too large |
| UVHTTP_ERROR_BODY_TOO_LARGE | -306 | Body too large |
| UVHTTP_ERROR_MALFORMED_REQUEST | -307 | Malformed request |
| UVHTTP_ERROR_FILE_TOO_LARGE | -308 | File too large |
| UVHTTP_ERROR_IO_ERROR | -309 | I/O error |

### TLS (-400s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_TLS_INIT | -400 | TLS init failed |
| UVHTTP_ERROR_TLS_CONTEXT | -401 | TLS context error |
| UVHTTP_ERROR_TLS_HANDSHAKE | -402 | TLS handshake failed |
| UVHTTP_ERROR_TLS_CERT_LOAD | -403 | Certificate load failed |
| UVHTTP_ERROR_TLS_KEY_LOAD | -404 | Key load failed |
| UVHTTP_ERROR_TLS_VERIFY_FAILED | -405 | Certificate verify failed |
| UVHTTP_ERROR_TLS_EXPIRED | -406 | Certificate expired |
| UVHTTP_ERROR_TLS_NOT_YET_VALID | -407 | Certificate not yet valid |
| UVHTTP_ERROR_TLS_CERT | -408 | Certificate error |
| UVHTTP_ERROR_TLS_KEY | -409 | Key error |
| UVHTTP_ERROR_TLS_CA | -410 | CA error |
| UVHTTP_ERROR_TLS_VERIFY | -411 | Verify error |
| UVHTTP_ERROR_TLS_READ | -412 | TLS read error |
| UVHTTP_ERROR_TLS_WRITE | -413 | TLS write error |
| UVHTTP_ERROR_TLS_INVALID_PARAM | -414 | Invalid TLS parameter |
| UVHTTP_ERROR_TLS_MEMORY | -415 | TLS memory error |
| UVHTTP_ERROR_TLS_NOT_IMPLEMENTED | -416 | TLS feature not implemented |
| UVHTTP_ERROR_TLS_PARSE | -417 | TLS parse error |
| UVHTTP_ERROR_TLS_NO_CERT | -418 | No certificate |

TLS also defines two positive non-blocking states: `UVHTTP_ERROR_TLS_WANT_READ
= 1` and `UVHTTP_ERROR_TLS_WANT_WRITE = 2`.

### Router (-500s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_ROUTER_INIT | -500 | Router init failed |
| UVHTTP_ERROR_ROUTER_ADD | -501 | Route add failed |
| UVHTTP_ERROR_ROUTE_NOT_FOUND | -502 | Route not found |
| UVHTTP_ERROR_ROUTE_ALREADY_EXISTS | -503 | Route already exists |
| UVHTTP_ERROR_INVALID_ROUTE_PATTERN | -504 | Invalid route pattern |

### Rate limit

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_RATE_LIMIT_EXCEEDED | -550 | Rate limit exceeded |

### Allocator (-600s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_ALLOCATOR_INIT | -600 | Allocator init failed |
| UVHTTP_ERROR_ALLOCATOR_SET | -601 | Allocator set failed |
| UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED | -602 | Allocator not initialized |

### WebSocket (-700s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_WEBSOCKET_INIT | -700 | WebSocket init failed |
| UVHTTP_ERROR_WEBSOCKET_HANDSHAKE | -701 | WebSocket handshake failed |
| UVHTTP_ERROR_WEBSOCKET_FRAME | -702 | Invalid frame |
| UVHTTP_ERROR_WEBSOCKET_TOO_LARGE | -703 | Frame too large |
| UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE | -704 | Invalid opcode |
| UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED | -705 | Not connected |
| UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED | -706 | Already connected |
| UVHTTP_ERROR_WEBSOCKET_CLOSED | -707 | Connection closed |

### Configuration (-900s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_CONFIG_PARSE | -900 | Config parse failed |
| UVHTTP_ERROR_CONFIG_INVALID | -901 | Invalid config |
| UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND | -902 | Config file not found |
| UVHTTP_ERROR_CONFIG_MISSING_REQUIRED | -903 | Missing required setting |

### Middleware (-1000s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_MIDDLEWARE_STOPPED | -1000 | Middleware chain stopped |
| UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY | -1001 | Middleware chain empty |
| UVHTTP_ERROR_MIDDLEWARE_INVALID | -1002 | Invalid middleware |

### Logging (-1100s)

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_ERROR_LOG_INIT | -1100 | Log init failed |
| UVHTTP_ERROR_LOG_WRITE | -1101 | Log write failed |
| UVHTTP_ERROR_LOG_FILE_OPEN | -1102 | Log file open failed |
| UVHTTP_ERROR_LOG_NOT_INITIALIZED | -1103 | Log not initialized |

## Constants

### HTTP Methods

```c
#define UVHTTP_METHOD_GET "GET"
#define UVHTTP_METHOD_POST "POST"
#define UVHTTP_METHOD_PUT "PUT"
#define UVHTTP_METHOD_DELETE "DELETE"
#define UVHTTP_METHOD_HEAD "HEAD"
#define UVHTTP_METHOD_OPTIONS "OPTIONS"
#define UVHTTP_METHOD_PATCH "PATCH"
```

### HTTP Status Codes

```c
#define UVHTTP_STATUS_OK 200
#define UVHTTP_STATUS_CREATED 201
#define UVHTTP_STATUS_BAD_REQUEST 400
#define UVHTTP_STATUS_NOT_FOUND 404
#define UVHTTP_STATUS_INTERNAL_ERROR 500
```

### Constant Limits

```c
#define UVHTTP_MAX_HEADERS 64
#define UVHTTP_MAX_HEADER_NAME_SIZE 256
#define UVHTTP_MAX_HEADER_VALUE_SIZE 4096
#define UVHTTP_MAX_URL_SIZE 2048
```

## Compilation Options

### CMake Options

```cmake
BUILD_WITH_WEBSOCKET=ON          # Enable WebSocket support
BUILD_WITH_HTTPS=ON              # Enable TLS support
UVHTTP_ALLOCATOR_TYPE=0          # 0=system, 1=mimalloc, 2=custom
BUILD_EXAMPLES=ON                # Build example programs
BUILD_BENCHMARKS=ON              # Build performance benchmarks
BUILD_TESTS=ON                   # Build unit/integration tests
ENABLE_DEBUG=OFF                 # Debug mode
ENABLE_COVERAGE=OFF              # Code coverage
```

### Compilation Macros

```c
UVHTTP_FEATURE_WEBSOCKET          # WebSocket support
UVHTTP_FEATURE_STATIC_FILES       # Static file serving
UVHTTP_FEATURE_TLS                # TLS support
UVHTTP_FEATURE_LRU_CACHE          # LRU cache
UVHTTP_FEATURE_ROUTER_CACHE       # Router cache
UVHTTP_FEATURE_RATE_LIMIT         # Rate limiting
UVHTTP_FEATURE_COMPRESSION        # Response compression
UVHTTP_FEATURE_LOGGING            # Logging system
UVHTTP_ALLOCATOR_TYPE             # Allocator type (0=system, 1=mimalloc, 2=custom)
```

## Examples

### Basic HTTP Server

```c
#include "uvhttp.h"

int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    return uvhttp_response_send(response);
}

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_error_t r = uvhttp_server_new(loop, &server);
    if (r != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(r));
        return 1;
    }
    
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_server_set_router(server, router);
    
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %d\n", result);
        return 1;
    }
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    uvhttp_server_free(server);
    return 0;
}
```

## References

- [Architecture Documentation](../dev/ARCHITECTURE.md)
- [Developer Guide](../guide/DEVELOPER_GUIDE.md)
- [Tutorial](../guide/TUTORIAL.md)
- [Security Policy](../SECURITY.md)
- [libuv Documentation](https://docs.libuv.org/)
- [HTTP/1.1 Specification](https://tools.ietf.org/html/rfc7230)