# uvhttp_connection.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT(uvhttp_request_t 65536, "uvhttp_request_t size too small" )
```

**Parameters:**
- `65536` (uvhttp_request_t): TBD
- `` ("uvhttp_request_t size too small"): TBD


**Returns:**
``



---
### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT()
```

**Parameters:**
None


**Returns:**
``



---
### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT(uvhttp_response_t 65536, "uvhttp_response_t size too small" )
```

**Parameters:**
- `65536` (uvhttp_response_t): TBD
- `` ("uvhttp_response_t size too small"): TBD


**Returns:**
``



---
### `on_idle_restart_read`

**Signature:**
```c
void on_idle_restart_read(uv_idle_t * handle)
```

**Parameters:**
- `handle` (uv_idle_t *): TBD


**Returns:**
`void`



---
### `on_alloc_buffer`

**Signature:**
```c
void on_alloc_buffer(uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf)
```

**Parameters:**
- `handle` (uv_handle_t *): TBD
- `suggested_size` (size_t): TBD
- `buf` (uv_buf_t *): TBD


**Returns:**
`void`



---
### `on_read`

**Signature:**
```c
void on_read(uv_stream_t * stream, ssize_t nread, const uv_buf_t * buf)
```

**Parameters:**
- `stream` (uv_stream_t *): TBD
- `nread` (ssize_t): TBD
- `buf` (const uv_buf_t *): TBD


**Returns:**
`void`



---
### `uvhttp_connection_restart_read`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_restart_read(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_new`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_new([`uvhttp_server`](./structs/uvhttp_server.md) server, uvhttp_connection_t conn)
```

**Parameters:**
- `server` ([`uvhttp_server`](./structs/uvhttp_server.md)): TBD
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_free`

**Signature:**
```c
void uvhttp_connection_free(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`void`



---
### `uvhttp_connection_start`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `on_handle_close`

**Signature:**
```c
void on_handle_close(uv_handle_t * handle)
```

**Parameters:**
- `handle` (uv_handle_t *): TBD


**Returns:**
`void`



---
### `uvhttp_connection_close`

**Signature:**
```c
void uvhttp_connection_close(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`void`



---
### `uvhttp_connection_set_state`

**Signature:**
```c
void uvhttp_connection_set_state(uvhttp_connection_t conn, uvhttp_connection_state_t state)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD
- `state` (uvhttp_connection_state_t): TBD


**Returns:**
`void`



---
### `uvhttp_connection_tls_handshake_func`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_tls_handshake_func(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_tls_write`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_tls_write(uvhttp_connection_t conn, const void * data, size_t len)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD
- `data` (const void *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_schedule_restart_read`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_schedule_restart_read(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `connection_timeout_cb`

**Signature:**
```c
void connection_timeout_cb(uv_timer_t * handle)
```

**Parameters:**
- `handle` (uv_timer_t *): TBD


**Returns:**
`void`



---
### `uvhttp_connection_start_timeout`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_start_timeout(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_start_timeout_custom`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_start_timeout_custom(uvhttp_connection_t conn, int timeout_seconds)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD
- `timeout_seconds` (int): TBD


**Returns:**
`uvhttp_error_t`



---
