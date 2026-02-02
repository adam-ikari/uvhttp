# uvhttp_config.h Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_config_new`

**Signature:**
```c
uvhttp_error_t uvhttp_config_new([`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_config_free`

**Signature:**
```c
void uvhttp_config_free([`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`void`



---
### `uvhttp_config_set_defaults`

**Signature:**
```c
void uvhttp_config_set_defaults([`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`void`



---
### `uvhttp_config_validate`

**Signature:**
```c
int uvhttp_config_validate([`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`int`



---
### `uvhttp_config_print`

**Signature:**
```c
void uvhttp_config_print([`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`void`



---
### `uvhttp_config_update_max_connections`

**Signature:**
```c
int uvhttp_config_update_max_connections(uvhttp_context_t context, int max_connections)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `max_connections` (int): TBD


**Returns:**
`int`



---
### `uvhttp_config_update_read_buffer_size`

**Signature:**
```c
int uvhttp_config_update_read_buffer_size(uvhttp_context_t context, int buffer_size)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `buffer_size` (int): TBD


**Returns:**
`int`



---
### `uvhttp_config_update_size_limits`

**Signature:**
```c
int uvhttp_config_update_size_limits(uvhttp_context_t context, size_t max_body_size, size_t max_header_size)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `max_body_size` (size_t): TBD
- `max_header_size` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_config_get_current`

**Signature:**
```c
[`uvhttp_config_t`](./structs/uvhttp_config_t.md) uvhttp_config_get_current(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`[`uvhttp_config_t`](./structs/uvhttp_config_t.md)`



---
### `uvhttp_config_set_current`

**Signature:**
```c
void uvhttp_config_set_current(uvhttp_context_t context, [`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`void`



---
