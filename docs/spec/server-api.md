# Server API Spec

## Overview

The Server module manages the HTTP server lifecycle: creation, binding,
listening, connection acceptance, and graceful shutdown. It is the top-level
object that ties together the event loop, router, TLS context, and WebSocket
connection management.

## Interfaces

### uvhttp_server_new
- **Signature**: `uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server)`
- **Purpose**: Create a new HTTP server instance
- **Preconditions**: `loop` must be a valid, initialized `uv_loop_t`. `server` must be a non-NULL pointer to a `uvhttp_server_t*` that will receive the result.
- **Postconditions**: On success, `*server` points to a valid server with `is_listening=0`, `freed=0`, `active_connections=0`, `handler=NULL`, `router=NULL`, `config=NULL`, `context=NULL`, `tls_ctx=NULL`.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `loop` or `server` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
- **Thread safety**: Not thread-safe. Must be called from the event loop thread.

### uvhttp_server_listen
- **Signature**: `uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port)`
- **Purpose**: Bind to a host:port and start accepting connections
- **Preconditions**: `server` must be valid (created by `uvhttp_server_new`), not already listening. A handler or router must have been set.
- **Postconditions**: On success, `server->is_listening=1`, the TCP handle is bound and listening. On failure, server state is unchanged.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` or `host` is NULL
  - `UVHTTP_ERROR_SERVER_LISTEN`: bind or listen syscall failed
- **Thread safety**: Not thread-safe.

### uvhttp_server_stop
- **Signature**: `uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server)`
- **Purpose**: Stop accepting new connections. Existing connections continue.
- **Preconditions**: `server` must be listening.
- **Postconditions**: `server->is_listening=0`. The TCP handle is closed.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` is NULL
  - `UVHTTP_ERROR_NOT_FOUND`: server is not listening
- **Thread safety**: Not thread-safe.

### uvhttp_server_free
- **Signature**: `uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server)`
- **Purpose**: Free all server resources. Must be called after stop.
- **Preconditions**: `server` must be valid. Should not be listening (call `stop` first).
- **Postconditions**: All server memory is freed. The `freed` flag prevents double-free. Any remaining connections are cleaned up.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` is NULL
  - Double-free is handled gracefully (returns UVHTTP_OK on second call)
- **Thread safety**: Not thread-safe.

### uvhttp_server_set_handler
- **Signature**: `uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server, uvhttp_request_handler_t handler)`
- **Purpose**: Set the default request handler for all requests
- **Preconditions**: `server` must be valid. `handler` must be non-NULL.
- **Postconditions**: `server->handler` is set to the provided handler.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` or `handler` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_server_set_router
- **Signature**: `uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server, uvhttp_router_t* router)`
- **Purpose**: Attach a router for path-based request dispatching
- **Preconditions**: `server` must be valid. `router` must be a valid router.
- **Postconditions**: `server->router` is set. The server does not own the router; the caller must free it after the server.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` or `router` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_server_set_context
- **Signature**: `uvhttp_error_t uvhttp_server_set_context(uvhttp_server_t* server, struct uvhttp_context* context)`
- **Purpose**: Attach a context object for shared state
- **Preconditions**: `server` must be valid. `context` must be a valid context.
- **Postconditions**: `server->context` is set.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` or `context` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_server_enable_tls / uvhttp_server_disable_tls
- **Signature**: `uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server, uvhttp_tls_context_t* tls_ctx)` / `uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server)`
- **Purpose**: Enable or disable TLS on the server
- **Preconditions**: `server` must be valid. TLS must be compiled in (`UVHTTP_FEATURE_TLS`).
- **Postconditions**: `server->tls_enabled` is set accordingly.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` is NULL
  - `UVHTTP_ERROR_TLS_INIT`: TLS context is invalid
- **Thread safety**: Not thread-safe.
- **Feature gate**: `#if UVHTTP_FEATURE_TLS`

## Behavior Rules

1. **Server-request binding**: Each incoming connection creates a `uvhttp_request_t` and `uvhttp_response_t` pair. The handler is called once per request.

2. **Handler dispatch priority**: If a router is set, the router is consulted first. If the router finds a matching handler, it is used. Otherwise, the default handler is used.

3. **Connection limit**: The server enforces `max_connections`. When the limit is reached, new connections receive a 503 response.

4. **Graceful shutdown**: `uvhttp_server_stop` stops accepting new connections. Existing connections are allowed to complete. `uvhttp_server_free` cleans up all resources.

5. **Double-free protection**: The `freed` flag prevents double-free. Calling `uvhttp_server_free` twice is safe.

6. **Rate limiting**: When enabled, the server tracks request count per time window. When the limit is exceeded, new requests receive a 429 response.

## Performance Requirements

- Connection acceptance: O(1) per new connection
- Handler dispatch: O(1) when router cache is used
- Memory: ~256 bytes per server instance (plus per-connection allocations)

## Test Requirements

- Server creation and destruction (with and without router, config, context)
- Listen on valid and invalid hosts/ports
- Stop and restart
- Multiple server instances on the same loop
- Connection limit enforcement
- Double-free protection
- Rate limiting enable/disable/check
- TLS enable/disable
- WebSocket connection management enable/disable
- Handler dispatch with router and without
- Server configuration via builder API