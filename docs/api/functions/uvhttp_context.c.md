# uvhttp_context.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_context_create`

**Signature:**
```c
uvhttp_error_t uvhttp_context_create(uv_loop_t * loop, uvhttp_context_t context)
```

**Parameters:**
- `loop` (uv_loop_t *): TBD
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_context_destroy`

**Signature:**
```c
void uvhttp_context_destroy(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`void`



---
### `uvhttp_context_init`

**Signature:**
```c
uvhttp_error_t uvhttp_context_init(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_context_init_tls`

**Signature:**
```c
uvhttp_error_t uvhttp_context_init_tls(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_context_cleanup_tls`

**Signature:**
```c
void uvhttp_context_cleanup_tls(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`void`



---
### `uvhttp_context_init_websocket`

**Signature:**
```c
uvhttp_error_t uvhttp_context_init_websocket(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_context_cleanup_websocket`

**Signature:**
```c
void uvhttp_context_cleanup_websocket(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`void`



---
### `uvhttp_context_init_config`

**Signature:**
```c
uvhttp_error_t uvhttp_context_init_config(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_context_cleanup_config`

**Signature:**
```c
void uvhttp_context_cleanup_config(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`void`



---
