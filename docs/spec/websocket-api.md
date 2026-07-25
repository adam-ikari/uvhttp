# WebSocket API Spec

## Overview

The WebSocket module implements RFC 6455 WebSocket protocol support. It
handles the HTTP upgrade handshake, frame encoding/decoding, ping/pong,
and connection management.

## Interfaces

### uvhttp_ws_connection_create
- **Signature**: `struct uvhttp_ws_connection* uvhttp_ws_connection_create(int fd, mbedtls_ssl_context* ssl, int is_server, const uvhttp_config_t* config)`
- **Purpose**: Create a WebSocket connection
- **Preconditions**: `fd` must be a valid socket. `is_server` indicates if this is the server side.
- **Postconditions**: Returns a valid ws_connection with CONNECTING state.
- **Error conditions**: Returns NULL on allocation failure.
- **Thread safety**: Not thread-safe.
- **Feature gate**: `UVHTTP_FEATURE_WEBSOCKET`

### uvhttp_ws_connection_free
- **Signature**: `void uvhttp_ws_connection_free(struct uvhttp_ws_connection* conn)`
- **Purpose**: Free a WebSocket connection
- **Preconditions**: `conn` can be NULL (no-op).
- **Postconditions**: All resources are freed.
- **Thread safety**: Not thread-safe.

### uvhttp_ws_send_frame
- **Signature**: `uvhttp_error_t uvhttp_ws_send_frame(uvhttp_context_t* context, struct uvhttp_ws_connection* conn, const uint8_t* data, size_t len, uvhttp_ws_opcode_t opcode)`
- **Purpose**: Send a raw WebSocket frame
- **Preconditions**: `conn` must be valid and in OPEN state.
- **Postconditions**: Frame is sent to the client.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: conn is NULL or state is not OPEN
  - Write error from underlying socket/SSL
- **Thread safety**: Not thread-safe.

### uvhttp_ws_send_text / uvhttp_ws_send_binary
- **Signature**: `uvhttp_error_t uvhttp_ws_send_text(uvhttp_context_t* context, struct uvhttp_ws_connection* conn, const char* text, size_t len)` / `uvhttp_error_t uvhttp_ws_send_binary(uvhttp_context_t* context, struct uvhttp_ws_connection* conn, const uint8_t* data, size_t len)`
- **Purpose**: Send a text or binary message
- **Preconditions**: Same as `uvhttp_ws_send_frame`
- **Postconditions**: Message is sent as a complete WebSocket frame.
- **Thread safety**: Not thread-safe.

### uvhttp_ws_close
- **Signature**: `uvhttp_error_t uvhttp_ws_close(uvhttp_context_t* context, struct uvhttp_ws_connection* conn, int code, const char* reason)`
- **Purpose**: Initiate WebSocket close handshake
- **Preconditions**: `conn` must be valid.
- **Postconditions**: Close frame is sent. State transitions to CLOSING.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: conn is NULL or stale
- **Thread safety**: Not thread-safe.

### uvhttp_server_ws_broadcast
- **Signature**: `uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server, const char* path, const char* data, size_t len)`
- **Purpose**: Send a message to all WebSocket clients on a path
- **Preconditions**: `server` must be valid with connection management enabled.
- **Postconditions**: Message is sent to all matching connections.
- **Thread safety**: Not thread-safe.

## WebSocket States

```
CONNECTING → OPEN → CLOSING → CLOSED
```

## Frame Types

| Opcode | Type | Description |
|--------|------|-------------|
| 0x0 | Continuation | Continuation of fragmented message |
| 0x1 | Text | UTF-8 text message |
| 0x2 | Binary | Binary message |
| 0x8 | Close | Close handshake |
| 0x9 | Ping | Ping frame |
| 0xA | Pong | Pong frame |

## Behavior Rules

1. **Handshake**: WebSocket upgrade is initiated via HTTP Upgrade request. The server responds with 101 Switching Protocols.

2. **Frame masking**: Client frames must be masked. Server frames are not masked (per RFC 6455).

3. **Fragmentation**: Large messages can be fragmented into multiple frames. The FIN bit indicates the last frame.

4. **Ping/pong**: The server sends pings periodically (configurable interval). If no pong is received within the timeout, the connection is closed.

5. **Connection management**: The server tracks WebSocket connections in a linked list. Connections are checked for timeout periodically.

6. **Broadcast**: Messages can be broadcast to all connections on a specific path.

## Test Requirements

- Connection creation and destruction
- Frame encoding/decoding
- Text and binary message send
- Close handshake
- Broadcast to multiple connections
- Connection timeout detection
- Ping/pong cycle
- NULL parameter handling
- Memory cleanup (no leaks on free)