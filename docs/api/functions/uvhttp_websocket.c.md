# uvhttp_websocket.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `uvhttp_ws_random_bytes`

**Signature:**
```c
int uvhttp_ws_random_bytes(uvhttp_context_t context, unsigned char * buf, size_t len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `buf` (unsigned char *): TBD
- `len` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_ws_connection_create`

**Signature:**
```c
struct uvhttp_ws_connection * uvhttp_ws_connection_create(int fd, mbedtls_ssl_context * ssl, int is_server, [`uvhttp_config_t`](./structs/uvhttp_config_t.md) config)
```

**Parameters:**
- `fd` (int): TBD
- `ssl` (mbedtls_ssl_context *): TBD
- `is_server` (int): TBD
- `config` ([`uvhttp_config_t`](./structs/uvhttp_config_t.md)): TBD


**Returns:**
`struct uvhttp_ws_connection *`



---
### `uvhttp_ws_connection_free`

**Signature:**
```c
void uvhttp_ws_connection_free(struct uvhttp_ws_connection * conn)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD


**Returns:**
`void`



---
### `uvhttp_ws_parse_frame_header`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_parse_frame_header(const uint8_t * data, size_t len, uvhttp_ws_frame_header_t * header, size_t * header_size)
```

**Parameters:**
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD
- `header` (uvhttp_ws_frame_header_t *): TBD
- `header_size` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_apply_mask`

**Signature:**
```c
void uvhttp_ws_apply_mask(uint8_t * data, size_t len, const uint8_t * masking_key)
```

**Parameters:**
- `data` (uint8_t *): TBD
- `len` (size_t): TBD
- `masking_key` (const uint8_t *): TBD


**Returns:**
`void`



---
### `uvhttp_ws_build_frame`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_build_frame(uvhttp_context_t context, uint8_t * buffer, size_t buffer_size, const uint8_t * payload, size_t payload_len, uvhttp_ws_opcode_t opcode, int mask, int fin)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `buffer` (uint8_t *): TBD
- `buffer_size` (size_t): TBD
- `payload` (const uint8_t *): TBD
- `payload_len` (size_t): TBD
- `opcode` (uvhttp_ws_opcode_t): TBD
- `mask` (int): TBD
- `fin` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_generate_accept`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_generate_accept(const char * key, char * accept, size_t accept_len)
```

**Parameters:**
- `key` (const char *): TBD
- `accept` (char *): TBD
- `accept_len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_verify_accept`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_verify_accept(const char * key, const char * accept)
```

**Parameters:**
- `key` (const char *): TBD
- `accept` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_handshake_server`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_handshake_server(struct uvhttp_ws_connection * conn, const char * request, size_t request_len, char * response, size_t * response_len)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD
- `request` (const char *): TBD
- `request_len` (size_t): TBD
- `response` (char *): TBD
- `response_len` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_handshake_client`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_handshake_client(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const char * host, const char * path, char * request, size_t * request_len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `host` (const char *): TBD
- `path` (const char *): TBD
- `request` (char *): TBD
- `request_len` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_verify_handshake_response`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_verify_handshake_response(struct uvhttp_ws_connection * conn, const char * response, size_t response_len)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD
- `response` (const char *): TBD
- `response_len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_send_frame`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_send_frame(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const uint8_t * data, size_t len, uvhttp_ws_opcode_t opcode)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD
- `opcode` (uvhttp_ws_opcode_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_send_text`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_send_text(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const char * text, size_t len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `text` (const char *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_send_binary`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_send_binary(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const uint8_t * data, size_t len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_send_ping`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_send_ping(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const uint8_t * data, size_t len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_send_pong`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_send_pong(uvhttp_context_t context, struct uvhttp_ws_connection * conn, const uint8_t * data, size_t len)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_close`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_close(uvhttp_context_t context, struct uvhttp_ws_connection * conn, int code, const char * reason)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD
- `conn` (struct uvhttp_ws_connection *): TBD
- `code` (int): TBD
- `reason` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_recv_frame`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_recv_frame(struct uvhttp_ws_connection * conn, uvhttp_ws_frame_t * frame)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD
- `frame` (uvhttp_ws_frame_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_process_data`

**Signature:**
```c
uvhttp_error_t uvhttp_ws_process_data(struct uvhttp_ws_connection * conn, const uint8_t * data, size_t len)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD
- `data` (const uint8_t *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_ws_set_callbacks`

**Signature:**
```c
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection * conn, uvhttp_ws_on_message_callback on_message, uvhttp_ws_on_close_callback on_close, uvhttp_ws_on_error_callback on_error)
```

**Parameters:**
- `conn` (struct uvhttp_ws_connection *): TBD
- `on_message` (uvhttp_ws_on_message_callback): TBD
- `on_close` (uvhttp_ws_on_close_callback): TBD
- `on_error` (uvhttp_ws_on_error_callback): TBD


**Returns:**
`void`



---
### `__attribute__`

**Signature:**
```c
 __attribute__((unused) )
```

**Parameters:**
- `` ((unused)): TBD


**Returns:**
``



---
