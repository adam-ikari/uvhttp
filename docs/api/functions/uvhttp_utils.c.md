# uvhttp_utils.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_safe_strcpy`

**Signature:**
```c
int uvhttp_safe_strcpy(char * dest, size_t dest_size, const char * src)
```

**Parameters:**
- `dest` (char *): TBD
- `dest_size` (size_t): TBD
- `src` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_safe_strncpy`

**Signature:**
```c
int uvhttp_safe_strncpy(char * dest, const char * src, size_t dest_size)
```

**Parameters:**
- `dest` (char *): TBD
- `src` (const char *): TBD
- `dest_size` (size_t): TBD


**Returns:**
`int`



---
### `is_valid_status_code`

**Signature:**
```c
int is_valid_status_code(int code)
```

**Parameters:**
- `code` (int): TBD


**Returns:**
`int`



---
### `is_valid_string_length`

**Signature:**
```c
int is_valid_string_length(const char * str, size_t max_len)
```

**Parameters:**
- `str` (const char *): TBD
- `max_len` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_send_unified_response`

**Signature:**
```c
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t response, const char * content, size_t length, int status_code)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `content` (const char *): TBD
- `length` (size_t): TBD
- `status_code` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_send_error_response`

**Signature:**
```c
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t response, int error_code, const char * error_message, const char * details)
```

**Parameters:**
- `response` (uvhttp_response_t): TBD
- `error_code` (int): TBD
- `error_message` (const char *): TBD
- `details` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_is_valid_status_code`

**Signature:**
```c
int uvhttp_is_valid_status_code(int status_code)
```

**Parameters:**
- `status_code` (int): TBD


**Returns:**
`int`



---
### `uvhttp_is_valid_ip_address`

**Signature:**
```c
int uvhttp_is_valid_ip_address(const char * ip)
```

**Parameters:**
- `ip` (const char *): TBD


**Returns:**
`int`



---
