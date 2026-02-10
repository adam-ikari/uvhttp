# uvhttp_router.c Functions

**Defined at:** `undefined:undefined`


## Functions

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
### `create_route_node`

**Signature:**
```c
uvhttp_route_node_t create_route_node(uvhttp_router_t router)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD


**Returns:**
`uvhttp_route_node_t`



---
### `find_or_create_child`

**Signature:**
```c
uvhttp_route_node_t find_or_create_child(uvhttp_router_t router, uvhttp_route_node_t parent, const char * segment, int is_param)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `parent` (uvhttp_route_node_t): TBD
- `segment` (const char *): TBD
- `is_param` (int): TBD


**Returns:**
`uvhttp_route_node_t`



---
### `parse_path_params`

**Signature:**
```c
int parse_path_params(const char * path, [`uvhttp_param_t`](./structs/uvhttp_param_t.md) params, size_t * param_count)
```

**Parameters:**
- `path` (const char *): TBD
- `params` ([`uvhttp_param_t`](./structs/uvhttp_param_t.md)): TBD
- `param_count` (size_t *): TBD


**Returns:**
`int`



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
### `add_array_route`

**Signature:**
```c
uvhttp_error_t add_array_route(uvhttp_router_t router, const char * path, uvhttp_method_t method, uvhttp_request_handler_t handler)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `method` (uvhttp_method_t): TBD
- `handler` (uvhttp_request_handler_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `find_array_route`

**Signature:**
```c
uvhttp_request_handler_t find_array_route(uvhttp_router_t router, const char * path, uvhttp_method_t method)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD
- `path` (const char *): TBD
- `method` (uvhttp_method_t): TBD


**Returns:**
`uvhttp_request_handler_t`



---
### `migrate_to_trie`

**Signature:**
```c
uvhttp_error_t migrate_to_trie(uvhttp_router_t router)
```

**Parameters:**
- `router` (uvhttp_router_t): TBD


**Returns:**
`uvhttp_error_t`



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
### `match_route_node`

**Signature:**
```c
int match_route_node(uvhttp_route_node_t node, const char ** segments, int segment_count, int segment_index, uvhttp_method_t method, [`uvhttp_route_match_t`](./structs/uvhttp_route_match_t.md) match)
```

**Parameters:**
- `node` (uvhttp_route_node_t): TBD
- `segments` (const char **): TBD
- `segment_count` (int): TBD
- `segment_index` (int): TBD
- `method` (uvhttp_method_t): TBD
- `match` ([`uvhttp_route_match_t`](./structs/uvhttp_route_match_t.md)): TBD


**Returns:**
`int`



---
### `static_file_handler_wrapper`

**Signature:**
```c
int static_file_handler_wrapper(uvhttp_request_t request, uvhttp_response_t response)
```

**Parameters:**
- `request` (uvhttp_request_t): TBD
- `response` (uvhttp_response_t): TBD


**Returns:**
`int`



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
