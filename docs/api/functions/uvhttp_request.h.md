# uvhttp_request.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , client , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (client): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , parser , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (parser): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , parser_settings , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (parser_settings): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , path , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (path): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , query , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (query): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , body , UVHTTP_POINTER_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (body): TBD
- `` (UVHTTP_POINTER_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , header_count , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (header_count): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_CHECK_ALIGNMENT`

**Signature:**
```c
 UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t , body_length , UVHTTP_SIZE_T_ALIGNMENT )
```

**Parameters:**
- `` (uvhttp_request_t): TBD
- `` (body_length): TBD
- `` (UVHTTP_SIZE_T_ALIGNMENT): TBD


**Returns:**
``



---
### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT(uvhttp_request_t 64, "url buffer should be after first 64 bytes" )
```

**Parameters:**
- `64` (uvhttp_request_t): TBD
- `` ("url buffer should be after first 64 bytes"): TBD


**Returns:**
``



---
### `UVHTTP_STATIC_ASSERT`

**Signature:**
```c
 UVHTTP_STATIC_ASSERT(uvhttp_request_t 64, "headers array should be after first 64 bytes" )
```

**Parameters:**
- `64` (uvhttp_request_t): TBD
- `` ("headers array should be after first 64 bytes"): TBD


**Returns:**
``



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
