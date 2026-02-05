# uvhttp_validation.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_validate_string_length`

**Signature:**
```c
int uvhttp_validate_string_length(const char * str, size_t min_len, size_t max_len)
```

**Parameters:**
- `str` (const char *): TBD
- `min_len` (size_t): TBD
- `max_len` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_validate_url_path`

**Signature:**
```c
int uvhttp_validate_url_path(const char * path)
```

**Parameters:**
- `path` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_validate_header_name`

**Signature:**
```c
int uvhttp_validate_header_name(const char * name)
```

**Parameters:**
- `name` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_validate_header_value_safe`

**Signature:**
```c
int uvhttp_validate_header_value_safe(const char * value)
```

**Parameters:**
- `value` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_validate_query_string`

**Signature:**
```c
int uvhttp_validate_query_string(const char * query)
```

**Parameters:**
- `query` (const char *): TBD


**Returns:**
`int`



---
