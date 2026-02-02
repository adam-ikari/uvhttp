# uvhttp_response.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_free_write_data`

**Signature:**
```c
void uvhttp_free_write_data(uv_write_t * req, int status)
```

**Parameters:**
- `req` (uv_write_t *): TBD
- `status` (int): TBD


**Returns:**
`void`



---
### `get_status_text`

**Signature:**
```c
const char * get_status_text(int status_code)
```

**Parameters:**
- `status_code` (int): TBD


**Returns:**
`const char *`



---
### `contains_control_chars`

**Signature:**
```c
int contains_control_chars(const char * str)
```

**Parameters:**
- `str` (const char *): TBD


**Returns:**
`int`



---
### `build_response_headers`

**Signature:**
```c
void build_response_headers(uvhttp_response_t response, char * buffer, size_t * length)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `buffer` (char *): TBD
- `length` (size_t *): TBD


**Returns:**
`void`



---
### `uvhttp_response_init`

**Signature:**
```c
uvhttp_error_t uvhttp_response_init(uvhttp_response_t response, void * client)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `client` (void *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_cleanup`

**Signature:**
```c
void uvhttp_response_cleanup(uvhttp_response_t response)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD


**Returns:**
`void`



---
### `uvhttp_response_set_status`

**Signature:**
```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t response, int status_code)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `status_code` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_set_header`

**Signature:**
```c
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t response, const char * name, const char * value)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `name` (const char *): TBD
- `value` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_set_body`

**Signature:**
```c
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t response, const char * body, size_t length)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `body` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_send_response_data`

**Signature:**
```c
uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t response, const char * data, size_t length)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `data` (const char *): TBD
- `length` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_build_data`

**Signature:**
```c
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t response, char ** out_data, size_t * out_length)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `out_data` (char **): TBD
- `out_length` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_send_raw`

**Signature:**
```c
uvhttp_error_t uvhttp_response_send_raw(const char * data, size_t length, void * client, uvhttp_response_t response)
```

**Parameters:**
- `data` (const char *): TBD
- `length` (size_t): TBD
- `client` (void *): TBD
- `response` (uvhttp_response_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_send`

**Signature:**
```c
uvhttp_error_t uvhttp_response_send(uvhttp_response_t response)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_response_free`

**Signature:**
```c
void uvhttp_response_free(uvhttp_response_t response)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD


**Returns:**
`void`



---
### `uvhttp_response_get_header_count`

**Signature:**
```c
size_t uvhttp_response_get_header_count(uvhttp_response_t response)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD


**Returns:**
`size_t`



---
### `uvhttp_response_get_header_at`

**Signature:**
```c
[`uvhttp_header_t`](./structs/uvhttp_header_t.md) uvhttp_response_get_header_at(uvhttp_response_t response, size_t index)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `index` (size_t): TBD


**Returns:**
`[`uvhttp_header_t`](./structs/uvhttp_header_t.md)`



---
### `uvhttp_response_foreach_header`

**Signature:**
```c
void uvhttp_response_foreach_header(uvhttp_response_t response, uvhttp_header_callback_t callback, void * user_data)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `callback` (uvhttp_header_callback_t): TBD
- `user_data` (void *): TBD


**Returns:**
`void`



---
