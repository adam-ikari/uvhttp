# uvhttp_defaults.h Defines

**Defined at:** `undefined:undefined`


## Macros

### `UVHTTP_MAX_CONNECTIONS_DEFAULT`

**Value:** `2048`
**Description:** TBD


---
### `UVHTTP_BACKLOG_DEFAULT`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_DEFAULT_HOST`

**Value:** `"0.0.0.0"`
**Description:** TBD


---
### `UVHTTP_DEFAULT_PORT`

**Value:** `8080`
**Description:** TBD


---
### `UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT`

**Value:** `30 /* 秒 */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_KEEP_ALIVE_MAX`

**Value:** `100    /* 每个连接最大请求数 */`
**Description:** TBD


---
### `UVHTTP_CONNECTION_TIMEOUT_DEFAULT`

**Value:** `60`
**Description:** TBD


---
### `UVHTTP_CONNECTION_TIMEOUT_MIN`

**Value:** `5`
**Description:** TBD


---
### `UVHTTP_CONNECTION_TIMEOUT_MAX`

**Value:** `300`
**Description:** TBD


---
### `UVHTTP_DEFAULT_REQUEST_TIMEOUT`

**Value:** `60`
**Description:** TBD


---
### `UVHTTP_DEFAULT_READ_BUFFER_SIZE`

**Value:** `        16384 /* 16KB - 优化：从 8KB 增加到 16KB */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_BODY_SIZE`

**Value:** `(1024 * 1024) /* 1MB */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_HEADER_SIZE`

**Value:** `8192`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_URL_SIZE`

**Value:** `2048`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_FILE_SIZE`

**Value:** `(10 * 1024 * 1024) /* 10MB */`
**Description:** TBD


---
### `UVHTTP_ASYNC_FILE_BUFFER_SIZE`

**Value:** `        131072 /* 128KB - 优化：从 64KB 增加到 128KB */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN`

**Value:** `100`
**Description:** TBD


---
### `UVHTTP_DEFAULT_RATE_LIMIT_WINDOW`

**Value:** `60`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE`

**Value:** `        (16 * 1024 * 1024) /* 16MB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE`

**Value:** `        (64 * 1024 * 1024) /* 64MB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE`

**Value:** `(64 * 1024) /* 64KB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL`

**Value:** `30`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MEMORY_POOL_SIZE`

**Value:** `(16 * 1024 * 1024) /* 16MB */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD`

**Value:** `0.8`
**Description:** TBD


---
### `UVHTTP_DEFAULT_LOG_LEVEL`

**Value:** `2 /* INFO */`
**Description:** TBD


---
### `UVHTTP_CORS_MAX_AGE_DEFAULT`

**Value:** `"86400"`
**Description:** TBD


---
### `UVHTTP_HASH_DEFAULT_SEED`

**Value:** `0x1A2B3C4D5E6F7089ULL`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_CONNECTIONS`

**Value:** `[object Object]`
**Description:** TBD


---
### `UVHTTP_DEFAULT_BACKLOG`

**Value:** `[object Object]`
**Description:** TBD


---
### `UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT`

**Value:** `[object Object]`
**Description:** TBD


---
