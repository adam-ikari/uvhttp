# UVHTTP API Reference

**Version**: v2.4.4  
**Updated**: 2026-02-26  
**C Standard**: C11

## Overview

UVHTTP provides a concise, efficient C API for building HTTP/1.1 servers.

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
uvhttp_router_t* uvhttp_router_new(void);
```

Creates a new router object.

**Return Value**:
- Success: Router object pointer
- Failure: `NULL`

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
const char* uvhttp_request_get_body(uvhttp_request_t* request,
                                   size_t* len);
```

Gets the request body.

**Parameters**:
- `request`: Request object
- `len`: Output parameter, returns request body length

**Return Value**: Request body data pointer

## Response Handling API

### uvhttp_response_set_status

```c
void uvhttp_response_set_status(uvhttp_response_t* response,
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
void uvhttp_response_set_header(uvhttp_response_t* response,
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
void uvhttp_response_set_body(uvhttp_response_t* response,
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

### uvhttp_context_free

```c
void uvhttp_context_free(uvhttp_context_t* context);
```

Frees the context object.

## Error Handling API

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
void uvhttp_realloc(void* ptr, size_t size);
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
Safe string copy.

#### uvhttp_url_decode
```c
int uvhttp_url_decode(const char* src, char* dest, size_t dest_size);
```
URL decoding.

### Hash Functions

#### uvhttp_hash_string
```c
uint64_t uvhttp_hash_string(const char* str);
```
Calculates string hash value.

## Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| UVHTTP_OK | 0 | Success |
| UVHTTP_ERROR_INVALID_PARAM | -1 | Invalid parameter |
| UVHTTP_ERROR_OUT_OF_MEMORY | -2 | Out of memory |
| UVHTTP_ERROR_IO | -3 | I/O error |
| UVHTTP_ERROR_TLS | -4 | TLS error |
| UVHTTP_ERROR_WEBSOCKET | -5 | WebSocket error |
| UVHTTP_ERROR_ROUTER | -6 | Router error |
| UVHTTP_ERROR_STATIC_FILE | -7 | Static file error |

## Constants

### HTTP Methods

```c
#define UVHTTP_METHOD_GET "GET"
#define UVHTTP_METHOD_POST "POST"
#define UVHTTP_METHOD_PUT "PUT"
#define UVHTTP_METHOD_DELETE "DELETE"
#define UVHTTP_METHOD_HEAD "HEAD"
#define UVHTTP_METHOD_OPTIONS "OPTIONS"
```

### HTTP Status Codes

```c
#define UVHTTP_STATUS_OK 200
#define UVHTTP_STATUS_CREATED 201
#define UVHTTP_STATUS_BAD_REQUEST 400
#define UVHTTP_STATUS_NOT_FOUND 404
#define UVHTTP_STATUS_INTERNAL_SERVER_ERROR 500
```

### Constant Limits

```c
#define UVHTTP_MAX_HEADERS 64
#define UVHTTP_MAX_HEADER_NAME_SIZE 256
#define UVHTTP_MAX_HEADER_VALUE_SIZE 8192
#define UVHTTP_MAX_URL_SIZE 8192
```

## Compilation Options

### CMake Options

```cmake
BUILD_WITH_WEBSOCKET=ON          # Enable WebSocket support
BUILD_WITH_MIMALLOC=ON           # Enable mimalloc allocator
BUILD_WITH_HTTPS=ON              # Enable TLS support
ENABLE_DEBUG=OFF                 # Debug mode
ENABLE_COVERAGE=OFF              # Code coverage
BUILD_EXAMPLES=ON                # Build example programs
```

### Compilation Macros

```c
UVHTTP_FEATURE_WEBSOCKET          # WebSocket support
UVHTTP_FEATURE_STATIC_FILES       # Static file serving
UVHTTP_FEATURE_TLS                # TLS support
UVHTTP_FEATURE_LRU_CACHE          # LRU cache
UVHTTP_FEATURE_ROUTER_CACHE       # Router cache
UVHTTP_FEATURE_LOGGING            # Logging system
UVHTTP_ALLOCATOR_TYPE             # Allocator type (0=system, 1=mimalloc)
```

## Examples

### Basic HTTP Server

```c
#include "uvhttp.h"

void home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
}

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", home_handler);
    server->router = router;
    
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