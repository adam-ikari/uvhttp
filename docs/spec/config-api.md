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

**Rule: a field is either authoritative or absent.** Every field below is read
by at least one execution path. v2.10 deleted the 14 fields that were set,
validated, printed — and then ignored by the whole library:

| Removed field (v2.10) | What actually owned that limit |
|---|---|
| `read_buffer_size` | compile-time `UVHTTP_READ_BUFFER_SIZE` (per-connection buffer, `connection.c`) |
| `max_body_size` | compile-time `UVHTTP_MAX_BODY_SIZE` (`request.c` / `response.c` / `utils.c`) |
| `max_header_size` | nothing enforced it at all |
| `max_url_size` | `MAX_URL_LEN` (2048) in `request.h` |
| `max_requests_per_connection` | nothing enforced it at all |
| `request_timeout`, `keepalive_timeout` | `connection_timeout` (`connection.c` idle timer) — these were two extra names for the same knob |
| `cache_default_max_entries`, `cache_default_ttl` | compile-time `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES` / `_TTL` |
| `lru_cache_batch_eviction_size` | compile-time `UVHTTP_LRU_CACHE_*_BATCH_EVICTION_SIZE` |

`uvhttp_set_timeout()`, `uvhttp_set_max_body_size()`,
`uvhttp_config_update_size_limits()` and
`uvhttp_config_update_read_buffer_size()` were removed with them: they wrote
those fields and reported success. If you need one of these limits to be
runtime-tunable, add the field back **together with the code that reads it** —
a config entry nobody reads is documentation, not control.

### Surviving fields

| Field | Type | Default | Valid range | Read by |
|-------|------|---------|-------------|---------|
| `max_connections` | int | 2048 | 1-65535 | `server.c` connection cap |
| `backlog` | int | 256 | validated only as `<= max_connections` (warn) | `server.c` listen |
| `connection_timeout` | int | 60 | 5-300 | `connection.c` idle timer |
| `max_file_size` | size_t | 10 MB | not range-checked | `static.c` file responses |
| `trust_proxy_headers` | int | 0 | 0 or 1 | `request.c` client-IP resolution |
| `websocket_max_frame_size` | int | 16 MB | 1 KB-256 MB | `websocket.c` |
| `websocket_max_message_size` | int | 64 MB | 1 KB-1 GB, `>= frame_size` | `websocket.c` |
| `websocket_ping_interval` | int | 30 | 1-3600 | `websocket.c` |
| `websocket_ping_timeout` | int | 10 | 1-3600 | `websocket.c` |
| `tcp_keepalive_timeout` | int | 60 | 1-7200 | `server.c` TCP keepalive |
| `sendfile_timeout_ms` | int | 30000 | 1000-300000 | `static.c` sendfile wait |
| `sendfile_max_retry` | int | 2 | `>= 0` | `static.c` sendfile retry |

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