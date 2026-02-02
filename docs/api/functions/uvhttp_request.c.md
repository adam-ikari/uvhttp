# uvhttp_request.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `is_websocket_handshake`

**Signature:**
```c
int is_websocket_handshake(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`int`



---
### `on_message_begin`

**Signature:**
```c
int on_message_begin(llhttp_t * parser)
```

**Parameters:**
- `parser` (llhttp_t *): TBD


**Returns:**
`int`



---
### `on_url`

**Signature:**
```c
int on_url(llhttp_t * parser, const char * at, size_t length)
```

**Parameters:**
- `parser` (llhttp_t *): TBD
- `at` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`int`



---
### `on_header_field`

**Signature:**
```c
int on_header_field(llhttp_t * parser, const char * at, size_t length)
```

**Parameters:**
- `parser` (llhttp_t *): TBD
- `at` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`int`



---
### `on_header_value`

**Signature:**
```c
int on_header_value(llhttp_t * parser, const char * at, size_t length)
```

**Parameters:**
- `parser` (llhttp_t *): TBD
- `at` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`int`



---
### `on_body`

**Signature:**
```c
int on_body(llhttp_t * parser, const char * at, size_t length)
```

**Parameters:**
- `parser` (llhttp_t *): TBD
- `at` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`int`



---
### `on_message_complete`

**Signature:**
```c
int on_message_complete(llhttp_t * parser)
```

**Parameters:**
- `parser` (llhttp_t *): TBD


**Returns:**
`int`



---
### `handle_websocket_handshake_request`

**Signature:**
```c
int handle_websocket_handshake_request(uvhttp_connection_t conn)
```

**Parameters:**
- `conn` (uvhttp_connection_t): TBD


**Returns:**
`int`



---
### `ensure_valid_url`

**Signature:**
```c
void ensure_valid_url(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`void`



---
### `uvhttp_request_init`

**Signature:**
```c
uvhttp_error_t uvhttp_request_init(uvhttp_request_t request, uv_tcp_t * client)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `client` (uv_tcp_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_request_cleanup`

**Signature:**
```c
void uvhttp_request_cleanup(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`void`



---
### `uvhttp_request_get_method`

**Signature:**
```c
const char * uvhttp_request_get_method(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_url`

**Signature:**
```c
const char * uvhttp_request_get_url(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_header`

**Signature:**
```c
const char * uvhttp_request_get_header(uvhttp_request_t request, const char * name)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `name` (const char *): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_body`

**Signature:**
```c
const char * uvhttp_request_get_body(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_body_length`

**Signature:**
```c
size_t uvhttp_request_get_body_length(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`size_t`



---
### `uvhttp_request_get_path`

**Signature:**
```c
const char * uvhttp_request_get_path(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_query_string`

**Signature:**
```c
const char * uvhttp_request_get_query_string(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_query_param`

**Signature:**
```c
const char * uvhttp_request_get_query_param(uvhttp_request_t request, const char * name)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `name` (const char *): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_get_client_ip`

**Signature:**
```c
const char * uvhttp_request_get_client_ip(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_request_free`

**Signature:**
```c
void uvhttp_request_free(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`void`



---
### `uvhttp_request_get_header_count`

**Signature:**
```c
size_t uvhttp_request_get_header_count(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`size_t`



---
### `uvhttp_request_get_header_at`

**Signature:**
```c
[`uvhttp_header_t`](./structs/uvhttp_header_t.md) uvhttp_request_get_header_at(uvhttp_request_t request, size_t index)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `index` (size_t): TBD


**Returns:**
`[`uvhttp_header_t`](./structs/uvhttp_header_t.md)`



---
### `uvhttp_request_add_header`

**Signature:**
```c
uvhttp_error_t uvhttp_request_add_header(uvhttp_request_t request, const char * name, const char * value)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `name` (const char *): TBD
- `value` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_request_foreach_header`

**Signature:**
```c
void uvhttp_request_foreach_header(uvhttp_request_t request, uvhttp_header_callback_t callback, void * user_data)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `callback` (uvhttp_header_callback_t): TBD
- `user_data` (void *): TBD


**Returns:**
`void`



---
