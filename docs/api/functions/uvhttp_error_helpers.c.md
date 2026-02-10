# uvhttp_error_helpers.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `contains_sensitive_info`

**Signature:**
```c
int contains_sensitive_info(const char * str)
```

**Parameters:**
- `str` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_handle_memory_failure`

**Signature:**
```c
void uvhttp_handle_memory_failure(const char * context, void(*)(void *) cleanup_func, void * cleanup_data)
```

**Parameters:**
- `context` (const char *): TBD
- `cleanup_func` (void(*)(void *)): TBD
- `cleanup_data` (void *): TBD


**Returns:**
`void`



---
### `uvhttp_handle_write_error`

**Signature:**
```c
void uvhttp_handle_write_error(uv_write_t * req, int status, const char * context)
```

**Parameters:**
- `req` (uv_write_t *): TBD
- `status` (int): TBD
- `context` (const char *): TBD


**Returns:**
`void`



---
### `uvhttp_log_safe_error`

**Signature:**
```c
void uvhttp_log_safe_error(int error_code, const char * context, const char * user_msg)
```

**Parameters:**
- `error_code` (int): TBD
- `context` (const char *): TBD
- `user_msg` (const char *): TBD


**Returns:**
`void`



---
### `uvhttp_sanitize_error_message`

**Signature:**
```c
uvhttp_error_t uvhttp_sanitize_error_message(const char * message, char * safe_buffer, size_t buffer_size)
```

**Parameters:**
- `message` (const char *): TBD
- `safe_buffer` (char *): TBD
- `buffer_size` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
