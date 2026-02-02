# uvhttp_server.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `write_503_response_cb`

**Signature:**
```c
void write_503_response_cb(uv_write_t * req, int status)
```

**Parameters:**
- `req` (uv_write_t *): TBD
- `status` (int): TBD


**Returns:**
`void`



---
### `on_connection`

**Signature:**
```c
void on_connection(uv_stream_t * server_handle, int status)
```

**Parameters:**
- `server_handle` (uv_stream_t *): TBD
- `status` (int): TBD


**Returns:**
`void`



---
### `uvhttp_server_new`

**Signature:**
```c
uvhttp_error_t uvhttp_server_new(uv_loop_t * loop, uvhttp_server_t server)
```

**Parameters:**
- `loop` (uv_loop_t *): TBD
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_free`

**Signature:**
```c
uvhttp_error_t uvhttp_server_free(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_listen`

**Signature:**
```c
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t server, const char * host, int port)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `host` (const char *): TBD
- `port` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_set_handler`

**Signature:**
```c
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t server, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_set_router`

**Signature:**
```c
uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t server, uvhttp_router_t router)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `router` (uvhttp_router_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_set_context`

**Signature:**
```c
uvhttp_error_t uvhttp_server_set_context(uvhttp_server_t server, [`uvhttp_context`](./structs/uvhttp_context.md) context)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `context` ([`uvhttp_context`](./structs/uvhttp_context.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_stop`

**Signature:**
```c
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_enable_tls`

**Signature:**
```c
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t server, void * tls_ctx)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `tls_ctx` (void *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_disable_tls`

**Signature:**
```c
uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_is_tls_enabled`

**Signature:**
```c
int uvhttp_server_is_tls_enabled(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`int`



---
### `create_simple_server_internal`

**Signature:**
```c
uvhttp_error_t create_simple_server_internal(const char * host, int port, [`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server)
```

**Parameters:**
- `host` (const char *): TBD
- `port` (int): TBD
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_create`

**Signature:**
```c
uvhttp_error_t uvhttp_server_create(const char * host, int port, [`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server)
```

**Parameters:**
- `host` (const char *): TBD
- `port` (int): TBD
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `add_route_internal`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) add_route_internal([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_method_t method, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `method` (uvhttp_method_t): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_get`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_get([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_post`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_post([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_put`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_put([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_delete`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_delete([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_any`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_any([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_set_max_connections`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_set_max_connections([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, int max_conn)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `max_conn` (int): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_set_timeout`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_set_timeout([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, int timeout)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `timeout` (int): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_set_max_body_size`

**Signature:**
```c
[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) uvhttp_set_max_body_size([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server, size_t size)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD
- `size` (size_t): TBD


**Returns:**
`[`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)`



---
### `uvhttp_get_param`

**Signature:**
```c
const char * uvhttp_get_param(uvhttp_request_t request, const char * name)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `name` (const char *): TBD


**Returns:**
`const char *`



---
### `uvhttp_get_header`

**Signature:**
```c
const char * uvhttp_get_header(uvhttp_request_t request, const char * name)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `name` (const char *): TBD


**Returns:**
`const char *`



---
### `uvhttp_get_body`

**Signature:**
```c
const char * uvhttp_get_body(uvhttp_request_t request)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_server_run`

**Signature:**
```c
int uvhttp_server_run([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD


**Returns:**
`int`



---
### `uvhttp_server_stop_simple`

**Signature:**
```c
void uvhttp_server_stop_simple([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD


**Returns:**
`void`



---
### `uvhttp_server_simple_free`

**Signature:**
```c
void uvhttp_server_simple_free([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md) server)
```

**Parameters:**
- `server` ([`uvhttp_server_builder_t`](./structs/uvhttp_server_builder_t.md)): TBD


**Returns:**
`void`



---
### `default_handler`

**Signature:**
```c
int default_handler(uvhttp_request_t request, uvhttp_response_t response)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `response` (uvhttp_response_t): TBD


**Returns:**
`int`



---
### `uvhttp_serve`

**Signature:**
```c
int uvhttp_serve(const char * host, int port)
```

**Parameters:**
- `host` (const char *): TBD
- `port` (int): TBD


**Returns:**
`int`



---
### `uvhttp_tls_context_free`

**Signature:**
```c
void uvhttp_tls_context_free(void * ctx)
```

**Parameters:**
- `ctx` (void *): TBD


**Returns:**
`void`



---
### `uvhttp_server_set_timeout_callback`

**Signature:**
```c
uvhttp_error_t uvhttp_server_set_timeout_callback(uvhttp_server_t server, uvhttp_timeout_callback_t callback, void * user_data)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `callback` (uvhttp_timeout_callback_t): TBD
- `user_data` (void *): TBD


**Returns:**
`uvhttp_error_t`



---
