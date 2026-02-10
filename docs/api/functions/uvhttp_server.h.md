# uvhttp_server.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t , loop , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_server_t): TBD
- `` (loop): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t , router , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_server_t): TBD
- `` (router): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t , config , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_server_t): TBD
- `` (config): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t , active_connections , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_server_t): TBD
- `` (active_connections): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t , max_connections , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_server_t): TBD
- `` (max_connections): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



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
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t server, uvhttp_tls_context_t tls_ctx)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `tls_ctx` (uvhttp_tls_context_t): TBD


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
### `uvhttp_server_enable_rate_limit`

**Signature:**
```c
uvhttp_error_t uvhttp_server_enable_rate_limit(uvhttp_server_t server, int max_requests, int window_seconds)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `max_requests` (int): TBD
- `window_seconds` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_disable_rate_limit`

**Signature:**
```c
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_check_rate_limit`

**Signature:**
```c
uvhttp_error_t uvhttp_server_check_rate_limit(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_add_rate_limit_whitelist`

**Signature:**
```c
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(uvhttp_server_t server, const char * client_ip)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `client_ip` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_get_rate_limit_status`

**Signature:**
```c
uvhttp_error_t uvhttp_server_get_rate_limit_status(uvhttp_server_t server, const char * client_ip, int * remaining, uint64_t * reset_time)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `client_ip` (const char *): TBD
- `remaining` (int *): TBD
- `reset_time` (uint64_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_reset_rate_limit_client`

**Signature:**
```c
uvhttp_error_t uvhttp_server_reset_rate_limit_client(uvhttp_server_t server, const char * client_ip)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `client_ip` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_clear_rate_limit_all`

**Signature:**
```c
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


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
### `uvhttp_server_register_ws_handler`

**Signature:**
```c
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t server, const char * path, [`uvhttp_ws_handler_t`](./structs/uvhttp_ws_handler_t.md) handler)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `path` (const char *): TBD
- `handler` ([`uvhttp_ws_handler_t`](./structs/uvhttp_ws_handler_t.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_send`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t ws_conn, const char * data, size_t len)
```

**Parameters:**
- `ws_conn` (uvhttp_ws_connection_t): TBD
- `data` (const char *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_close`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t ws_conn, int code, const char * reason)
```

**Parameters:**
- `ws_conn` (uvhttp_ws_connection_t): TBD
- `code` (int): TBD
- `reason` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_find_ws_handler`

**Signature:**
```c
[`uvhttp_ws_handler_t`](./structs/uvhttp_ws_handler_t.md) uvhttp_server_find_ws_handler(uvhttp_server_t server, const char * path)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `path` (const char *): TBD


**Returns:**
`[`uvhttp_ws_handler_t`](./structs/uvhttp_ws_handler_t.md)`



---
### `uvhttp_server_ws_enable_connection_management`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_enable_connection_management(uvhttp_server_t server, int timeout_seconds, int heartbeat_interval)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `timeout_seconds` (int): TBD
- `heartbeat_interval` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_disable_connection_management`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_disable_connection_management(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_get_connection_count`

**Signature:**
```c
int uvhttp_server_ws_get_connection_count(uvhttp_server_t server)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD


**Returns:**
`int`



---
### `uvhttp_server_ws_get_connection_count_by_path`

**Signature:**
```c
int uvhttp_server_ws_get_connection_count_by_path(uvhttp_server_t server, const char * path)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `path` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_server_ws_broadcast`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t server, const char * path, const char * data, size_t len)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `path` (const char *): TBD
- `data` (const char *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_close_all`

**Signature:**
```c
uvhttp_error_t uvhttp_server_ws_close_all(uvhttp_server_t server, const char * path)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `path` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_server_ws_add_connection`

**Signature:**
```c
void uvhttp_server_ws_add_connection(uvhttp_server_t server, uvhttp_ws_connection_t ws_conn, const char * path)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `ws_conn` (uvhttp_ws_connection_t): TBD
- `path` (const char *): TBD


**Returns:**
`void`



---
### `uvhttp_server_ws_remove_connection`

**Signature:**
```c
void uvhttp_server_ws_remove_connection(uvhttp_server_t server, uvhttp_ws_connection_t ws_conn)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `ws_conn` (uvhttp_ws_connection_t): TBD


**Returns:**
`void`



---
### `uvhttp_server_ws_update_activity`

**Signature:**
```c
void uvhttp_server_ws_update_activity(uvhttp_server_t server, uvhttp_ws_connection_t ws_conn)
```

**Parameters:**
- `server` (uvhttp_server_t): TBD
- `ws_conn` (uvhttp_ws_connection_t): TBD


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
