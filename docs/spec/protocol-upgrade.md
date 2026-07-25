# Protocol Upgrade Spec

## Overview

The Protocol Upgrade module provides a framework for upgrading HTTP connections to custom protocols (such as IPPS, gRPC-Web, WebSocket, etc.). It manages a registry of protocol detectors and handlers, handles connection ownership transfer to external libraries, and supports lifecycle callbacks for cleanup after upgrade. The module is the core mechanism behind the HTTP Upgrade and WebSocket handshake paths.

## Interfaces

### uvhttp_server_register_protocol_upgrade
- **Signature**: `uvhttp_error_t uvhttp_server_register_protocol_upgrade(uvhttp_server_t* server, const char* protocol_name, const char* upgrade_header, uvhttp_protocol_detector_t detector, uvhttp_protocol_upgrade_handler_t handler, void* user_data)`
- **Purpose**: Register a protocol upgrade handler for a server
- **Preconditions**: `server` must be valid. `protocol_name`, `detector`, and `handler` must be non-NULL. `protocol_name` must be < 32 characters. `upgrade_header` (if provided) must be < 64 characters.
- **Postconditions**: On success, the protocol is registered in the server's protocol registry (inserted at head for LIFO detection order). The protocol name and upgrade header are normalized to lowercase. `server->protocol_registry` is created if it did not exist.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server`, `protocol_name`, `detector`, or `handler` is NULL; protocol name or upgrade header exceeds max length; protocol count exceeds 10 (max)
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure for registry or protocol info
  - `UVHTTP_ERROR_ALREADY_EXISTS`: a protocol with the same name is already registered
- **Thread safety**: Not thread-safe. Must be called before the server starts listening.
- **Detection order**: Newly registered protocols are checked first (LIFO due to head insertion).

### uvhttp_server_unregister_protocol_upgrade
- **Signature**: `uvhttp_error_t uvhttp_server_unregister_protocol_upgrade(uvhttp_server_t* server, const char* protocol_name)`
- **Purpose**: Unregister a protocol upgrade handler
- **Preconditions**: `server` must be valid. `protocol_name` must be non-NULL.
- **Postconditions**: On success, the protocol is removed from the registry and its memory is freed. The protocol count is decremented.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `server` or `protocol_name` is NULL
  - `UVHTTP_ERROR_NOT_FOUND`: protocol is not registered, or no registry exists
- **Thread safety**: Not thread-safe. Must not be called while connections are being upgraded.

### uvhttp_protocol_find
- **Signature** (internal): The framework's detection logic iterates the protocol registry linked list. For each registered protocol, the `Upgrade` and `Connection` headers are pre-fetched from the request. If the protocol has an `upgrade_header` string set, a fast lowercase comparison is performed first. If that matches, or if no upgrade_header was set, the detector function is called.
- **Purpose**: Find a matching protocol for an incoming HTTP upgrade request
- **Preconditions**: The server must have a non-NULL protocol registry. The request must be a valid HTTP request with Upgrade and Connection headers.
- **Postconditions**: Returns the first matching protocol info, or NULL if no match is found.
- **Thread safety**: Not thread-safe. Must be called from the request handler thread.

### uvhttp_connection_transfer_ownership
- **Signature**: `uvhttp_error_t uvhttp_connection_transfer_ownership(uvhttp_connection_t* conn, uvhttp_connection_ownership_callback_t callback, void* user_data)`
- **Purpose**: Transfer ownership of a TCP connection to an external library
- **Preconditions**: `conn` must be valid and in `UVHTTP_CONN_STATE_HTTP_PROCESSING` state. `callback` must be non-NULL. The connection must not already be upgraded.
- **Postconditions**: The connection's HTTP reading is stopped, the timeout timer is stopped, the file descriptor is obtained, and the connection state is set to `UVHTTP_CONN_STATE_PROTOCOL_UPGRADED`. The callback is invoked with the TCP handle, fd, and user data.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `conn` or `callback` is NULL
  - `UVHTTP_ERROR_CONNECTION_INIT`: connection is in an invalid state, or already upgraded, or fd is invalid
  - `UVHTTP_ERROR_IO_ERROR`: failed to get file descriptor
- **Thread safety**: Not thread-safe. Must be called from the event loop thread.

### uvhttp_connection_set_lifecycle
- **Signature**: `uvhttp_error_t uvhttp_connection_set_lifecycle(uvhttp_connection_t* conn, uvhttp_connection_lifecycle_t* lifecycle)`
- **Purpose**: Set a lifecycle callback for when the connection is closed (used for cleanup by external libraries)
- **Preconditions**: `conn` and `lifecycle` must be non-NULL. `lifecycle->on_close` must be set.
- **Postconditions**: The lifecycle callback structure is stored in the connection. The callback will be invoked when the connection is closed.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `conn` or `lifecycle` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure for lifecycle structure
- **Thread safety**: Not thread-safe.

### uvhttp_connection_get_fd
- **Signature**: `uvhttp_error_t uvhttp_connection_get_fd(uvhttp_connection_t* conn, int* fd)`
- **Purpose**: Get the file descriptor of a connection
- **Preconditions**: `conn` and `fd` must be non-NULL.
- **Postconditions**: `*fd` is set to the socket file descriptor.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `conn` or `fd` is NULL
  - `UVHTTP_ERROR_IO_ERROR`: `uv_fileno` failed
- **Thread safety**: Not thread-safe.

### uvhttp_connection_get_peer_address
- **Signature**: `uvhttp_error_t uvhttp_connection_get_peer_address(uvhttp_connection_t* conn, struct sockaddr_storage* addr, socklen_t* addr_len)`
- **Purpose**: Get the peer address of a connection
- **Preconditions**: `conn`, `addr`, and `addr_len` must be non-NULL.
- **Postconditions**: `*addr` is filled with the peer's socket address. `*addr_len` is set to the actual address length.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM**: `conn`, `addr`, or `addr_len` is NULL
  - `UVHTTP_ERROR_IO_ERROR`: `uv_tcp_getpeername` failed
- **Thread safety**: Not thread-safe.

## Type Definitions

### uvhttp_protocol_detector_t
- **Signature**: `int (*detector)(uvhttp_request_t* request, char* protocol_name, size_t protocol_name_len, const char* upgrade_header, const char* connection_header)`
- **Purpose**: Detect whether an HTTP request matches a specific protocol. Pre-fetched headers (`Upgrade`, `Connection`) are passed to avoid repeated lookups.
- **Preconditions**: `request` must be valid. `protocol_name` must be a buffer of at least `protocol_name_len` bytes.
- **Postconditions**: Returns 1 if the protocol is detected, 0 otherwise. On detection, `protocol_name` may be filled with the protocol name.
- **Thread safety**: Thread-safe for reads.

### uvhttp_protocol_upgrade_handler_t
- **Signature**: `uvhttp_error_t (*handler)(uvhttp_connection_t* conn, const char* protocol_name, void* user_data)`
- **Purpose**: Handle the protocol upgrade logic after detection
- **Preconditions**: `conn` must be valid and in HTTP processing state. `protocol_name` must be non-NULL.
- **Postconditions**: The handler performs the upgrade (e.g., sends 101 Switching Protocols, transfers connection ownership).
- **Thread safety**: Not thread-safe. Must be called from the event loop thread.

### uvhttp_connection_ownership_callback_t
- **Signature**: `void (*callback)(uv_tcp_t* tcp_handle, int fd, void* user_data)`
- **Purpose**: Receive ownership of a TCP connection for an external library
- **Preconditions**: `tcp_handle` is valid and the connection is stopped. `fd` is a valid socket descriptor.
- **Postconditions**: The external library now owns the connection and is responsible for all I/O and cleanup.
- **Thread safety**: Not thread-safe.

## WebSocket Upgrade Path

The WebSocket protocol follows the standard HTTP upgrade mechanism:

1. The client sends an HTTP GET request with headers: `Upgrade: websocket`, `Connection: Upgrade`, `Sec-WebSocket-Key: <base64-key>`, `Sec-WebSocket-Version: 13`.
2. The server's protocol detector identifies the WebSocket upgrade request.
3. The WebSocket upgrade handler validates the `Sec-WebSocket-Key` and generates the `Sec-WebSocket-Accept` response.
4. The server responds with HTTP 101 Switching Protocols.
5. The server transfers connection ownership via `uvhttp_connection_transfer_ownership`.
6. The WebSocket module takes over the raw socket for frame-level communication.

## Behavior Rules

1. **Protocol registration**: Protocols are stored in a linked list with LIFO detection order. Newly registered protocols are checked first.

2. **Fast matching**: If an `upgrade_header` string is provided during registration, the framework performs a fast lowercase comparison against the `Upgrade` header before calling the detector function.

3. **Protocol name normalization**: Protocol names and upgrade header values are normalized to lowercase during registration. Detection comparisons are case-insensitive.

4. **Maximum protocols**: At most 10 protocols can be registered. Attempting to register more returns `UVHTTP_ERROR_INVALID_PARAM`.

5. **Duplicate prevention**: Registering a protocol with the same name as an existing one returns `UVHTTP_ERROR_ALREADY_EXISTS`.

6. **Connection state validation**: Ownership transfer requires the connection to be in `UVHTTP_CONN_STATE_HTTP_PROCESSING` state. The state transitions to `UVHTTP_CONN_STATE_PROTOCOL_UPGRADED` after transfer.

7. **Ownership transfer sequence**: Stop HTTP reading, stop timeout timer, get file descriptor, validate fd, mark connection as upgraded, call the callback.

8. **Lifecycle callbacks**: After ownership transfer, an external library can set a lifecycle callback to be notified when the connection is closed, allowing cleanup of external resources.

9. **Detector function contract**: The detector function receives pre-fetched header values to avoid redundant header lookups. It should only return 1 if it is certain the protocol matches.

## Performance Requirements

- Protocol registration: O(1) (head insertion into linked list)
- Protocol unregistration: O(n) where n is the number of registered protocols
- Protocol detection: O(n) worst-case (linear scan of registry), with fast string comparison optimization
- Ownership transfer: O(1) (stop handles, get fd, set state, invoke callback)
- Memory: ~128 bytes per registered protocol info
- Protocol count limit: 10 (to bound detection time)

## Test Requirements

- Protocol registration and unregistration
- Duplicate protocol registration returns UVHTTP_ERROR_ALREADY_EXISTS
- Protocol detection with exact match, case-insensitive match
- Protocol detection with fast matching (upgrade_header provided)
- Protocol detection when no upgrade_header is provided (detector-only)
- Protocol not found returns NULL
- Unregister a non-existent protocol returns UVHTTP_ERROR_NOT_FOUND
- Maximum protocol count enforcement (10)
- Protocol name too long (> 32 chars) rejected
- Upgrade header too long (> 64 chars) rejected
- Connection ownership transfer (valid state, invalid state, already upgraded)
- Connection lifecycle callback invocation
- Connection get_fd and get_peer_address
- NULL parameter handling for all public functions
- Memory cleanup (no leaks on free, registry cleanup on server free)
- WebSocket upgrade path (handshake, ownership transfer, lifecycle)