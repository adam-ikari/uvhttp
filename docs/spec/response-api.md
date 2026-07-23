# Response API Spec

## Overview

The Response module builds and sends HTTP responses. It provides functions
to set response status, headers, body, and control response sending.

## Interfaces

### uvhttp_response_set_status
- **Signature**: `uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code)`
- **Purpose**: Set the HTTP response status code
- **Preconditions**: `response` must be valid. `status_code` should be a valid HTTP status code.
- **Postconditions**: `response->status_code` is set.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: response is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_response_set_header
- **Signature**: `uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)`
- **Purpose**: Set a response header
- **Preconditions**: `response` must be valid. `name` and `value` must be non-NULL.
- **Postconditions**: The header is added to the response. If the header already exists, it is overwritten.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: response, name, or value is NULL
  - Header value contains control characters (response splitting guard)
- **Thread safety**: Not thread-safe.

### uvhttp_response_set_body
- **Signature**: `uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)`
- **Purpose**: Set the response body
- **Preconditions**: `response` must be valid. `body` can be NULL (empty body).
- **Postconditions**: The body is copied into the response. Content-Length is NOT set automatically.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: response is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
- **Thread safety**: Not thread-safe.

### uvhttp_response_send
- **Signature**: `uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response)`
- **Purpose**: Send the response to the client
- **Preconditions**: `response` must be valid. Status code must be set.
- **Postconditions**: The response is sent. Headers are flushed, then the body is sent.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: response is NULL
  - `UVHTTP_ERROR_RESPONSE_SEND`: write failure
- **Thread safety**: Not thread-safe.

### uvhttp_response_cleanup
- **Signature**: `void uvhttp_response_cleanup(uvhttp_response_t* response)`
- **Purpose**: Free response resources
- **Preconditions**: `response` must be valid.
- **Postconditions**: All allocated fields are freed. Pointers are NULLed.
- **Thread safety**: Not thread-safe.

## Behavior Rules

1. **Response ownership**: The framework creates the response object. The handler does not need to allocate or free it.

2. **Content-Type**: The handler must set Content-Type explicitly. The framework does not auto-detect.

3. **Content-Length**: The framework sets Content-Length from the body length. If no body is set, Content-Length is 0.

4. **Response splitting guard**: All header values are checked for control characters (`\r`, `\n`, `\0`) before emission. If detected, the header is rejected.

5. **Error responses**: When an error handler is invoked, it should set an appropriate status code (4xx/5xx) and return an error body.

6. **Large responses**: For large bodies (>1MB), the handler should use chunked encoding or sendfile for static files.

## Performance Requirements

- Header emission: O(n) where n = header count
- Body sending: O(body size) for small bodies, zero-copy for large files
- Response header buffer: 4096 bytes (configurable)

## Test Requirements

- Status code setting
- Header setting and overwriting
- Body setting (empty, small, large)
- Response sending
- NULL parameter handling for all functions
- Response splitting guard (control character rejection)
- Cleanup (no leaks)