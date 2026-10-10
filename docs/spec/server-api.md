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

### uvhttp_server_take_router
- **Signature**: `uvhttp_error_t uvhttp_server_take_router(uvhttp_server_t* server, uvhttp_router_t* router)`
- **Purpose**: Hand a router over to the server for path-based request dispatching. Ownership transfers.
- **Preconditions**: `server` must be valid. `router` must be a valid router, or NULL when the server holds no router.
- **Postconditions**: `server->router` is set. **The server owns the router from this point and releases it in `uvhttp_server_free`; the caller must NOT call `uvhttp_router_free` on it** (doing so is a double free). There is no detach operation: a taken router can neither be replaced nor cleared. Passing the same router again is idempotent (returns `UVHTTP_OK`).
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` is NULL, or the server already owns a different router (overwriting would leak the previous one)
- **Thread safety**: Not thread-safe.
- **History**: Renamed from `uvhttp_server_set_router` in v2.10. "set" implied a borrow while the behavior was a transfer - that mismatch made the official embedding example double-free on its listen-failure path.

### uvhttp_server_take_context
- **Signature**: `uvhttp_error_t uvhttp_server_take_context(uvhttp_server_t* server, struct uvhttp_context* context)`
- **Purpose**: Hand a context object over to the server for shared state. Ownership transfers.
- **Preconditions**: `server` must be valid. `context` must be a valid context, or NULL when the server holds none.
- **Postconditions**: `server->context` is set. The server owns the context and releases it in `uvhttp_server_free`; the caller must NOT free it. No detach operation exists; passing the same context again is idempotent.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` is NULL, or the server already owns a different context
- **Thread safety**: Not thread-safe.
- **History**: Renamed from `uvhttp_server_set_context` in v2.10, for the same reason as `uvhttp_server_take_router`.

### uvhttp_server_listen_routes
- **Signature**: `uvhttp_error_t uvhttp_server_listen_routes(uv_loop_t* loop, const uvhttp_route_t* routes, size_t route_count, const char* host, int port, uvhttp_server_t** server)`
- **Purpose**: Atomic construction: create a server, install a route table, and start listening in one call - all-or-nothing.
- **Preconditions**: `loop` must be a valid, initialized `uv_loop_t`. `host` must be non-NULL. `routes` must be non-NULL when `route_count > 0`. Each entry needs a non-NULL `path` and `handler` and a method within `UVHTTP_ANY..UVHTTP_PATCH`. `server` must be a non-NULL out pointer.
- **Postconditions**: On success, `*server` points to a fully listening server with all routes installed. On any failure, `*server` is NULL and nothing is allocated - there is no partial state for the caller to roll back. Route entries are copied (the `routes` array may be freed by the caller afterwards); the router is internal to the server and its handle is not reachable afterwards, so later routes must be registered on a router before calling this (or via the five-step sequence).
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `loop`, `host`, or `server` is NULL; `route_count > 0` with NULL `routes`; an entry with NULL `path`/`handler` or an out-of-range method
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: server or router allocation failed, or a route failed to register
  - `UVHTTP_ERROR_SERVER_LISTEN`: bind or listen syscall failed
- **Thread safety**: Not thread-safe. Must be called from the event loop thread.

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