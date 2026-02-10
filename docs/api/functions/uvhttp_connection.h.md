# uvhttp_connection.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , server , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (server): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , request , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (request): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , response , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (response): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , content_length , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (content_length): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , body_received , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (body_received): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t , read_buffer_size , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_connection_t): TBD
- `` (read_buffer_size): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT(uvhttp_connection_t 64, "current_header_field should be after first 64 bytes" )
```

**Parameters:**
- `64` (uvhttp_connection_t): TBD
- `` ("current_header_field should be after first 64 bytes"): TBD


**Returns:**
``



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
### `uvhttp_connection_start_tls_handshake`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_start_tls_handshake(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_connection_tls_read`

**Signature:**
```c
uvhttp_error_t uvhttp_connection_tls_read(uvhttp_connection_t conn)
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
### `uvhttp_connection_tls_cleanup`

**Signature:**
```c
void uvhttp_connection_tls_cleanup(uvhttp_connection_t conn)
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
### `uvhttp_connection_get_state_string`

**Signature:**
```c
const char * uvhttp_connection_get_state_string(uvhttp_connection_state_t state)
```

**Parameters:**
- `state` (uvhttp_connection_state_t): TBD


**Returns:**
`const char *`



---
