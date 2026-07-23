# Request API Spec

## Overview

The Request module parses incoming HTTP requests using llhttp, provides
access to request method, path, headers, query parameters, and body.

## Interfaces

### uvhttp_request_init
- **Signature**: `uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client)`
- **Purpose**: Initialize a request object for a client connection
- **Preconditions**: `request` must be a valid pointer to uninitialized memory. `client` must be a valid TCP handle.
- **Postconditions**: Request is initialized with empty state, ready for parsing.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: request or client is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_request_cleanup
- **Signature**: `void uvhttp_request_cleanup(uvhttp_request_t* request)`
- **Purpose**: Free request resources (body, headers)
- **Preconditions**: `request` must be valid.
- **Postconditions**: All allocated fields are freed. Pointers are NULLed.
- **Thread safety**: Not thread-safe.

### uvhttp_request_get_method
- **Signature**: `const char* uvhttp_request_get_method(const uvhttp_request_t* request)`
- **Purpose**: Get the HTTP method string
- **Preconditions**: `request` must be valid.
- **Returns**: Method string (e.g., "GET", "POST"), or NULL if not yet parsed.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_path
- **Signature**: `const char* uvhttp_request_get_path(const uvhttp_request_t* request)`
- **Purpose**: Get the request path
- **Preconditions**: `request` must be valid.
- **Returns**: Path string (e.g., "/api/users"), or NULL if not yet parsed.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_header
- **Signature**: `const char* uvhttp_request_get_header(const uvhttp_request_t* request, const char* name)`
- **Purpose**: Get a specific header value
- **Preconditions**: `request` must be valid. `name` must be non-NULL.
- **Returns**: Header value string, or NULL if not found.
- **Error conditions**: Returns NULL if request or name is NULL.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_query_param
- **Signature**: `const char* uvhttp_request_get_query_param(const uvhttp_request_t* request, const char* name)`
- **Purpose**: Get a specific query parameter value
- **Preconditions**: `request` must be valid. `name` must be non-NULL.
- **Returns**: Parameter value string, or NULL if not found.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_body
- **Signature**: `const char* uvhttp_request_get_body(const uvhttp_request_t* request)`
- **Purpose**: Get the request body
- **Preconditions**: `request` must be valid.
- **Returns**: Body pointer, or NULL if empty.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_body_length
- **Signature**: `size_t uvhttp_request_get_body_length(const uvhttp_request_t* request)`
- **Purpose**: Get the request body length
- **Preconditions**: `request` must be valid.
- **Returns**: Body length in bytes.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_get_client_ip
- **Signature**: `const char* uvhttp_request_get_client_ip(const uvhttp_request_t* request)`
- **Purpose**: Get the client IP address
- **Preconditions**: `request` must be valid.
- **Returns**: IP address string.
- **Thread safety**: Thread-safe for reads.

### uvhttp_request_foreach_header
- **Signature**: `void uvhttp_request_foreach_header(const uvhttp_request_t* request, void (*callback)(const char* name, const char* value, void* user_data), void* user_data)`
- **Purpose**: Iterate over all request headers
- **Preconditions**: `request` must be valid. `callback` must be non-NULL.
- **Thread safety**: Thread-safe for reads.

## Behavior Rules

1. **Parsing**: HTTP parsing is done via llhttp callbacks. The request object is populated incrementally as data arrives.

2. **Header limits**: Maximum header count is `UVHTTP_MAX_HEADERS` (64). Maximum header name length is 256 bytes. Maximum header value length is 4096 bytes.

3. **URL limits**: Maximum URL length is 2048 bytes. Path traversal (`../`) is checked and rejected.

4. **Body limits**: Maximum body size is configurable (default 10 MB). Larger bodies are rejected.

5. **Query parsing**: Query parameters are parsed from the URL. Parameter names and values are validated for dangerous characters.

6. **Header validation**: Header names must be alphanumeric with hyphens. Header values are checked for control characters.

## Test Requirements

- Method, path, header access
- Query parameter extraction
- Body access
- NULL parameter handling for all accessors
- Header iteration
- URL validation (path traversal rejection)
- Maximum limits enforcement
- Empty request handling