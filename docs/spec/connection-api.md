# Connection API Spec

## Overview

The Connection module manages TCP connections, HTTP parsing via llhttp,
request/response lifecycle, and connection cleanup. Each connection is tied
to a single TCP socket and handles one or more HTTP requests (keep-alive).

## Interfaces

### uvhttp_connection_new
- **Signature**: `uvhttp_error_t uvhttp_connection_new(uvhttp_server_t* server, uvhttp_connection_t** conn)`
- **Purpose**: Create a new connection object for an accepted TCP socket
- **Preconditions**: `server` must be valid. `conn` must be non-NULL.
- **Postconditions**: On success, `*conn` points to a valid connection with `state=UVHTTP_CONN_STATE_NEW`, `server` set, `tcp_handle` initialized.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: server or conn is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
  - `UVHTTP_ERROR_CONNECTION_LIMIT`: server is at max connections
- **Thread safety**: Not thread-safe.

### uvhttp_connection_start
- **Signature**: `uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn)`
- **Purpose**: Start reading from the connection
- **Preconditions**: `conn` must be valid, in NEW state.
- **Postconditions**: State transitions to `UVHTTP_CONN_STATE_HTTP_READING`. Read callback is registered.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: conn is NULL
  - `UVHTTP_ERROR_CONNECTION_INIT`: failed to start reading
- **Thread safety**: Not thread-safe.

### uvhttp_connection_close
- **Signature**: `void uvhttp_connection_close(uvhttp_connection_t* conn)`
- **Purpose**: Initiate connection close. Resources are freed in the close callback.
- **Preconditions**: `conn` must be valid. Can be NULL (no-op).
- **Postconditions**: TCP handle close is initiated. Resources are freed in `on_handle_close` callback.
- **Thread safety**: Not thread-safe.

### uvhttp_connection_free
- **Signature**: `void uvhttp_connection_free(uvhttp_connection_t* conn)`
- **Purpose**: Free connection resources immediately
- **Preconditions**: `conn` must be valid. Should only be called after close is complete.
- **Postconditions**: All connection memory is freed. The `freed` flag prevents double-free.
- **Error conditions**: NULL conn is a no-op.
- **Thread safety**: Not thread-safe.

### uvhttp_connection_set_state
- **Signature**: `void uvhttp_connection_set_state(uvhttp_connection_t* conn, uvhttp_connection_state_t state)`
- **Purpose**: Update connection state
- **Preconditions**: `conn` must be valid.
- **Postconditions**: `conn->state` is updated.
- **Thread safety**: Not thread-safe.

## Connection States

```
NEW → HTTP_READING → (request complete) → HTTP_WRITING → (response sent) → HTTP_READING
NEW → HTTP_READING → (error) → CLOSING → CLOSED
NEW → HTTP_READING → (upgrade) → WEBSOCKET_OPEN → WEBSOCKET_CLOSING → CLOSED
```

## Behavior Rules

1. **Keep-Alive**: After a response is sent, the connection returns to HTTP_READING state for the next request. The keep-alive timeout is configurable.

2. **Request/response lifecycle**: Each request creates a `uvhttp_request_t` and `uvhttp_response_t` pair. These are recycled for keep-alive connections.

3. **Error handling**: Parse errors, timeout, or connection reset transition to CLOSING state. The error is logged and the connection is cleaned up.

4. **Double-free protection**: The `freed` flag prevents double-free. All cleanup functions check this flag before proceeding.

5. **TLS support**: When TLS is enabled, read/write operations go through mbedtls SSL functions instead of raw TCP.

6. **WebSocket upgrade**: When a WebSocket upgrade request is detected, the connection transitions to WebSocket mode.

## Test Requirements

- Connection creation and destruction
- State transitions
- Keep-Alive request/response cycle
- Error handling (parse errors, timeout)
- Double-free protection
- TLS connection lifecycle
- WebSocket upgrade path
- NULL parameter handling for all public functions
- Connection limit enforcement