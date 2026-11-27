# Response API Spec

## Overview

The Response module builds and sends HTTP responses. It provides functions
to build a response in one validated call and send it, or prepare it without sending.

## Interfaces

### uvhttp_respond
- **Signature**: `uvhttp_error_t uvhttp_respond(uvhttp_response_t* response, int status_code, const char* content_type, const char* body, size_t body_length)`
- **Purpose**: Validate all inputs, set status + optional Content-Type + body, and send in one call
- **Preconditions**: `response` must be valid and unsent; `(body == NULL) == (body_length == 0)`
- **Postconditions**: On success the response is sent and marked sent; on failure status and header count are restored
- **Error conditions**: `UVHTTP_ERROR_INVALID_PARAM` for bad input; setter/send error codes otherwise
- **Thread safety**: Not thread-safe.

### uvhttp_respond_with_headers
- **Signature**: `uvhttp_error_t uvhttp_respond_with_headers(uvhttp_response_t* response, int status_code, const uvhttp_header_view_t* headers, size_t header_count, const char* body, size_t body_length)`
- **Purpose**: N-header form of `uvhttp_respond`; all headers are validated before the response is touched
- **Error conditions**: Same as `uvhttp_respond`
- **Thread safety**: Not thread-safe.

### uvhttp_response_prepare
- **Signature**: `uvhttp_error_t uvhttp_response_prepare(uvhttp_response_t* response, int status_code, const uvhttp_header_view_t* headers, size_t header_count, const char* body, size_t body_length)`
- **Purpose**: Same validated status + headers + body construction, **without sending** — for middleware that only prepares a response
- **Error conditions**: Same as `uvhttp_respond`
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

- Atomic status + headers + body validation and rollback
- Empty, small and large bodies
- Send and no-send (prepare) forms
- NULL parameter handling for all functions
- Response splitting guard (control character rejection)
- Cleanup (no leaks)