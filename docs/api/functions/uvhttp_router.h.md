# uvhttp_router.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , root , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (root): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , node_pool , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (node_pool): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , array_routes , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (array_routes): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , static_prefix , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (static_prefix): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , static_context , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (static_context): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , route_count , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (route_count): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t , node_pool_size , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_router_t): TBD
- `` (node_pool_size): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `uvhttp_router_new`

**Signature:**
```c
uvhttp_error_t uvhttp_router_new(uvhttp_router_t router)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_router_free`

**Signature:**
```c
void uvhttp_router_free(uvhttp_router_t router)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD


**Returns:**
`void`



---
### `uvhttp_router_add_route`

**Signature:**
```c
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t router, const char * path, uvhttp_request_handler_t handler)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_router_add_route_method`

**Signature:**
```c
uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t router, const char * path, uvhttp_method_t method, uvhttp_request_handler_t handler)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `method` (uvhttp_method_t): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_router_find_handler`

**Signature:**
```c
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t router, const char * path, const char * method)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `method` (const char *): TBD


**Returns:**
`uvhttp_request_handler_t`



---
### `uvhttp_router_match`

**Signature:**
```c
uvhttp_error_t uvhttp_router_match(uvhttp_router_t router, const char * path, const char * method, [`uvhttp_route_match_t`](./structs/uvhttp_route_match_t.md) match)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `method` (const char *): TBD
- `match` ([`uvhttp_route_match_t`](./structs/uvhttp_route_match_t.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_parse_path_params`

**Signature:**
```c
uvhttp_error_t uvhttp_parse_path_params(const char * path, [`uvhttp_param_t`](./structs/uvhttp_param_t.md) params, size_t * param_count)
```

**Parameters:**
- `path` (const char *): TBD
- `params` ([`uvhttp_param_t`](./structs/uvhttp_param_t.md)): TBD
- `param_count` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_method_from_string`

**Signature:**
```c
uvhttp_method_t uvhttp_method_from_string(const char * method)
```

**Parameters:**
- `method` (const char *): TBD


**Returns:**
`uvhttp_method_t`



---
### `uvhttp_method_to_string`

**Signature:**
```c
const char * uvhttp_method_to_string(uvhttp_method_t method)
```

**Parameters:**
- `method` (uvhttp_method_t): TBD


**Returns:**
`const char *`



---
### `uvhttp_router_add_static_route`

**Signature:**
```c
uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t router, const char * prefix_path, void * static_context)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `prefix_path` (const char *): TBD
- `static_context` (void *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_router_add_fallback_route`

**Signature:**
```c
uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t router, void * static_context)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `static_context` (void *): TBD


**Returns:**
`uvhttp_error_t`



---
