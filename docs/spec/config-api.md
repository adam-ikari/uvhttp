# Config API Spec

## Overview

The Config module provides server configuration options. Configuration is
set at the server level and can be shared across multiple server instances.

## Interfaces

### uvhttp_config_new
- **Signature**: `uvhttp_error_t uvhttp_config_new(uvhttp_config_t** config)`
- **Purpose**: Create a new config object with default values
- **Preconditions**: `config` must be non-NULL.
- **Postconditions**: On success, `*config` points to a config with all default values.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: config is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
- **Thread safety**: Not thread-safe.

### uvhttp_config_free
- **Signature**: `void uvhttp_config_free(uvhttp_config_t* config)`
- **Purpose**: Free a config object
- **Preconditions**: `config` can be NULL (no-op).
- **Postconditions**: All memory is freed.
- **Thread safety**: Not thread-safe.

### uvhttp_config_set_defaults
- **Signature**: `void uvhttp_config_set_defaults(uvhttp_config_t* config)`
- **Purpose**: Reset config to default values
- **Preconditions**: `config` must be valid.
- **Postconditions**: All config fields are set to their default values.
- **Thread safety**: Not thread-safe.

### uvhttp_config_validate
- **Signature**: `int uvhttp_config_validate(const uvhttp_config_t* config)`
- **Purpose**: Validate config values against min/max bounds
- **Preconditions**: `config` must be valid.
- **Returns**: 0 if valid, non-zero (error code) if invalid.
- **Thread safety**: Thread-safe for reads.

### uvhttp_config_get_current / uvhttp_config_set_current
- **Signature**: `uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context)` / `void uvhttp_config_set_current(uvhttp_context_t* context, uvhttp_config_t* config)`
- **Purpose**: Get/set the current config on a context
- **Preconditions**: `context` must be valid.
- **Thread safety**: Not thread-safe.

## Configurable Options

| Field | Type | Default | Range | Description |
|-------|------|---------|-------|-------------|
| `max_connections` | size_t | 2048 | 1-65535 | Max concurrent connections |
| `keepalive_timeout` | int | 60 | 0-3600 | Keep-alive timeout (seconds) |
| `request_timeout` | int | 30 | 1-3600 | Request timeout (seconds) |
| `max_header_size` | size_t | 8192 | 256-65536 | Max header size (bytes) |
| `max_body_size` | size_t | 10485760 | 1024-1073741824 | Max body size (bytes) |
| `max_headers` | int | 64 | 8-256 | Max header count |
| `read_buffer_size` | size_t | 4096 | 256-65536 | Read buffer size (bytes) |
| `backlog` | int | 128 | 0-65535 | TCP listen backlog |
| `tcp_nodelay` | int | 1 | 0-1 | TCP_NODELAY |
| `tcp_keepalive` | int | 1 | 0-1 | TCP keepalive |
| `websocket_max_frame_size` | size_t | 65536 | 1024-1048576 | Max WebSocket frame (bytes) |
| `websocket_ping_interval` | int | 30 | 5-300 | Ping interval (seconds) |
| `websocket_ping_timeout` | int | 10 | 3-60 | Ping timeout (seconds) |

## Behavior Rules

1. **Defaults**: All fields have sensible defaults. Callers can create a config and only change the fields they need.

2. **Validation**: `uvhttp_config_validate` checks all fields against their valid ranges. Out-of-range values return `UVHTTP_ERROR_INVALID_PARAM`.

3. **Immutability**: Config should not be modified while the server is listening. Changes take effect on the next listen.

## Test Requirements

- Config creation with defaults
- Individual field modification
- Validation of valid and invalid values
- Boundary testing (min/max for each field)
- NULL parameter handling
- Config free (no-op on NULL)
- Config set/get on context
- Complete workflow (create, modify, validate, use, free)