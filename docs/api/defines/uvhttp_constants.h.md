# uvhttp_constants.h Defines

**Defined at:** `undefined:undefined`


## Macros

### `TRUE`

**Value:** `1`
**Description:** TBD


---
### `FALSE`

**Value:** `0`
**Description:** TBD


---
### `UVHTTP_STRINGIFY`

**Value:** `#x`
**Description:** TBD


---
### `UVHTTP_STATUS_CONTINUE`

**Value:** `100`
**Description:** TBD


---
### `UVHTTP_STATUS_MIN_CONTINUE`

**Value:** `100`
**Description:** TBD


---
### `UVHTTP_STATUS_SWITCHING_PROTOCOLS`

**Value:** `101`
**Description:** TBD


---
### `UVHTTP_STATUS_OK`

**Value:** `200`
**Description:** TBD


---
### `UVHTTP_STATUS_CREATED`

**Value:** `201`
**Description:** TBD


---
### `UVHTTP_STATUS_ACCEPTED`

**Value:** `202`
**Description:** TBD


---
### `UVHTTP_STATUS_NO_CONTENT`

**Value:** `204`
**Description:** TBD


---
### `UVHTTP_STATUS_BAD_REQUEST`

**Value:** `400`
**Description:** TBD


---
### `UVHTTP_STATUS_UNAUTHORIZED`

**Value:** `401`
**Description:** TBD


---
### `UVHTTP_STATUS_FORBIDDEN`

**Value:** `403`
**Description:** TBD


---
### `UVHTTP_STATUS_NOT_FOUND`

**Value:** `404`
**Description:** TBD


---
### `UVHTTP_STATUS_METHOD_NOT_ALLOWED`

**Value:** `405`
**Description:** TBD


---
### `UVHTTP_STATUS_REQUEST_ENTITY_TOO_LARGE`

**Value:** `413`
**Description:** TBD


---
### `UVHTTP_STATUS_INTERNAL_ERROR`

**Value:** `500`
**Description:** TBD


---
### `UVHTTP_STATUS_NOT_IMPLEMENTED`

**Value:** `501`
**Description:** TBD


---
### `UVHTTP_STATUS_BAD_GATEWAY`

**Value:** `502`
**Description:** TBD


---
### `UVHTTP_STATUS_SERVICE_UNAVAILABLE`

**Value:** `503`
**Description:** TBD


---
### `UVHTTP_STATUS_MAX`

**Value:** `599`
**Description:** TBD


---
### `UVHTTP_VERSION_1_1`

**Value:** `"HTTP/1.1"`
**Description:** TBD


---
### `UVHTTP_VERSION_LENGTH`

**Value:** `8`
**Description:** TBD


---
### `UVHTTP_METHOD_GET`

**Value:** `"GET"`
**Description:** TBD


---
### `UVHTTP_METHOD_POST`

**Value:** `"POST"`
**Description:** TBD


---
### `UVHTTP_METHOD_PUT`

**Value:** `"PUT"`
**Description:** TBD


---
### `UVHTTP_METHOD_DELETE`

**Value:** `"DELETE"`
**Description:** TBD


---
### `UVHTTP_METHOD_HEAD`

**Value:** `"HEAD"`
**Description:** TBD


---
### `UVHTTP_METHOD_OPTIONS`

**Value:** `"OPTIONS"`
**Description:** TBD


---
### `UVHTTP_METHOD_PATCH`

**Value:** `"PATCH"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_TEXT`

**Value:** `"text/plain"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_HTML`

**Value:** `"text/html"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_CSS`

**Value:** `"text/css"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_JS`

**Value:** `"application/javascript"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_XML`

**Value:** `"application/xml"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_IMAGE_JPEG`

**Value:** `"image/jpeg"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_IMAGE_PNG`

**Value:** `"image/png"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_IMAGE_GIF`

**Value:** `"image/gif"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_IMAGE_SVG`

**Value:** `"image/svg+xml"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_IMAGE_ICO`

**Value:** `"image/x-icon"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_PDF`

**Value:** `"application/pdf"`
**Description:** TBD


---
### `UVHTTP_CONTENT_TYPE_ZIP`

**Value:** `"application/zip"`
**Description:** TBD


---
### `UVHTTP_INITIAL_BUFFER_SIZE`

**Value:** `8192`
**Description:** TBD


---
### `UVHTTP_MAX_BODY_SIZE`

**Value:** `(1024 * 1024) /* 1MB */`
**Description:** TBD


---
### `UVHTTP_READ_BUFFER_SIZE`

**Value:** `16384`
**Description:** TBD


---
### `UVHTTP_MAX_HEADER_NAME_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_MAX_HEADER_VALUE_SIZE`

**Value:** `4096`
**Description:** TBD


---
### `UVHTTP_MAX_HEADER_NAME_LENGTH`

**Value:** `[object Object]`
**Description:** TBD


---
### `UVHTTP_MAX_HEADER_VALUE_LENGTH`

**Value:** `[object Object]`
**Description:** TBD


---
### `UVHTTP_MAX_HEADERS`

**Value:** `64`
**Description:** TBD


---
### `UVHTTP_INLINE_HEADERS_CAPACITY`

**Value:** `32`
**Description:** TBD


---
### `UVHTTP_MAX_URL_SIZE`

**Value:** `2048`
**Description:** TBD


---
### `UVHTTP_MAX_PATH_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_MAX_METHOD_SIZE`

**Value:** `16`
**Description:** TBD


---
### `UVHTTP_MAX_CONNECTIONS_HARD`

**Value:** `65535 /* 硬限制 */`
**Description:** TBD


---
### `UVHTTP_MAX_CONNECTIONS_MAX`

**Value:** `10000 /* 最大推荐值 */`
**Description:** TBD


---
### `UVHTTP_BACKLOG`

**Value:** `8192`
**Description:** TBD


---
### `UVHTTP_ASYNC_FILE_MAX_CONCURRENT`

**Value:** `64`
**Description:** TBD


---
### `UVHTTP_ASYNC_FILE_MAX_SIZE`

**Value:** `(10 * 1024 * 1024) /* 10MB */`
**Description:** TBD


---
### `UVHTTP_STATIC_MAX_CACHE_SIZE`

**Value:** `(1024 * 1024) /* 1MB */`
**Description:** TBD


---
### `UVHTTP_STATIC_MAX_PATH_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_STATIC_MAX_CONTENT_LENGTH`

**Value:** `32`
**Description:** TBD


---
### `UVHTTP_STATIC_MAX_FILE_SIZE`

**Value:** `(10 * 1024 * 1024) /* 10MB */`
**Description:** TBD


---
### `UVHTTP_STATIC_SMALL_FILE_THRESHOLD`

**Value:** `4096 /* 4KB */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE`

**Value:** `(256 * 1024) /* 256KB */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_MIN_FILE_SIZE`

**Value:** `(64 * 1024) /* 64KB */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_DEFAULT_TIMEOUT_MS`

**Value:** `30000 /* 30 秒 */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_DEFAULT_MAX_RETRY`

**Value:** `2`
**Description:** TBD


---
### `UVHTTP_FILE_SIZE_SMALL`

**Value:** `(1024 * 1024) /* 1MB */`
**Description:** TBD


---
### `UVHTTP_FILE_SIZE_MEDIUM`

**Value:** `(10 * 1024 * 1024) /* 10MB */`
**Description:** TBD


---
### `UVHTTP_FILE_SIZE_LARGE`

**Value:** `(100 * 1024 * 1024) /* 100MB */`
**Description:** TBD


---
### `UVHTTP_CHUNK_SIZE_SMALL`

**Value:** `(64 * 1024) /* 64KB */`
**Description:** TBD


---
### `UVHTTP_CHUNK_SIZE_MEDIUM`

**Value:** `(256 * 1024) /* 256KB */`
**Description:** TBD


---
### `UVHTTP_CHUNK_SIZE_LARGE`

**Value:** `(1024 * 1024) /* 1MB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MIN_FRAME_SIZE`

**Value:** `1024 /* 1KB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MAX_FRAME_SIZE`

**Value:** `        (256 * 1024 * 1024) /* 256MB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MIN_MESSAGE_SIZE`

**Value:** `1024 /* 1KB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MAX_MESSAGE_SIZE`

**Value:** `        (1024 * 1024 * 1024) /* 1GB */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MIN_PING_INTERVAL`

**Value:** `1 /* 1秒 */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MAX_PING_INTERVAL`

**Value:** `3600 /* 1小时 */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MIN_PING_TIMEOUT`

**Value:** `1 /* 1秒 */`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CONFIG_MAX_PING_TIMEOUT`

**Value:** `3600 /* 1小时 */`
**Description:** TBD


---
### `UVHTTP_TCP_KEEPALIVE_MIN_TIMEOUT`

**Value:** `1 /* 1秒 */`
**Description:** TBD


---
### `UVHTTP_TCP_KEEPALIVE_MAX_TIMEOUT`

**Value:** `7200 /* 2小时 */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_MIN_TIMEOUT_MS`

**Value:** `1000 /* 1秒 */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_MAX_TIMEOUT_MS`

**Value:** `300000 /* 5分钟 */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_MIN_RETRY`

**Value:** `0`
**Description:** TBD


---
### `UVHTTP_SENDFILE_MAX_RETRY`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES`

**Value:** `1000`
**Description:** TBD


---
### `UVHTTP_CACHE_DEFAULT_TTL`

**Value:** `3600 /* 1 小时 */`
**Description:** TBD


---
### `UVHTTP_CACHE_MIN_MAX_ENTRIES`

**Value:** `0`
**Description:** TBD


---
### `UVHTTP_CACHE_MAX_MAX_ENTRIES`

**Value:** `100000`
**Description:** TBD


---
### `UVHTTP_CACHE_MIN_TTL`

**Value:** `0`
**Description:** TBD


---
### `UVHTTP_CACHE_MAX_TTL`

**Value:** `86400 /* 24小时 */`
**Description:** TBD


---
### `UVHTTP_LRU_CACHE_MIN_BATCH_EVICTION_SIZE`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_LRU_CACHE_MAX_BATCH_EVICTION_SIZE`

**Value:** `1000`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MIN_MAX_REQUESTS`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MAX_MAX_REQUESTS`

**Value:** `10000000`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MIN_WINDOW_SECONDS`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS`

**Value:** `86400 /* 24小时 */`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MAX_TIMEOUT_SECONDS`

**Value:** `3600 /* 1小时 */`
**Description:** TBD


---
### `UVHTTP_SOCKET_SEND_BUF_SIZE`

**Value:** `(256 * 1024) /* 256KB */`
**Description:** TBD


---
### `UVHTTP_SOCKET_RECV_BUF_SIZE`

**Value:** `(256 * 1024) /* 256KB */`
**Description:** TBD


---
### `UVHTTP_PAGE_SIZE`

**Value:** `4096`
**Description:** TBD


---
### `UVHTTP_PAGE_ALIGNMENT_MASK`

**Value:** `[object Object]`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_VERSION`

**Value:** `13`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MAGIC_KEY`

**Value:** `"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MAGIC_KEY_LENGTH`

**Value:** `36`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_ACCEPT_KEY_SIZE`

**Value:** `40`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MAX_FRAME_SIZE`

**Value:** `4096`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_FRAME_HEADER_SIZE`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CLOSE_CODE_SIZE`

**Value:** `4`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MIN_BUFFER_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MIN_FRAME_HEADER_SIZE`

**Value:** `2`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_EXTENDED_FRAME_HEADER_SIZE`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_OPCODE_TEXT`

**Value:** `0x1`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_OPCODE_BINARY`

**Value:** `0x2`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_OPCODE_CLOSE`

**Value:** `0x8`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_FIN`

**Value:** `0x80`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_PAYLOAD_LEN_126`

**Value:** `126`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_PAYLOAD_LEN_65536`

**Value:** `65536`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CLOSE_CODE_MIN`

**Value:** `1000`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_CLOSE_CODE_MAX`

**Value:** `4999`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MAX_REASON_LENGTH`

**Value:** `123`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MIN_KEY_LENGTH`

**Value:** `16`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_MAX_KEY_LENGTH`

**Value:** `64`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_COMBINED_MAX_LENGTH`

**Value:** `128`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_SHA1_HASH_SIZE`

**Value:** `20`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_FIN_MASK`

**Value:** `0x80`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_OPCODE_MASK`

**Value:** `0x0F`
**Description:** TBD


---
### `UVHTTP_WEBSOCKET_PAYLOAD_MASK`

**Value:** `0x7F`
**Description:** TBD


---
### `UVHTTP_MAX_ROUTE_PATH_LEN`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION`

**Value:** `1 /* 默认启用 */`
**Description:** TBD


---
### `UVHTTP_ROUTER_SEARCH_MODE`

**Value:** `2`
**Description:** TBD


---
### `UVHTTP_FEATURE_ROUTER_CACHE`

**Value:** `1 /* 默认启用 */`
**Description:** TBD


---
### `UVHTTP_ROUTER_MAX_CHILDREN`

**Value:** `16`
**Description:** TBD


---
### `UVHTTP_TLS_VERIFY_DEPTH`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_TLS_DH_MIN_BITLEN`

**Value:** `2048`
**Description:** TBD


---
### `UVHTTP_TLS_MAX_SESSIONS`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_TLS_ERROR_BUFFER_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_TLS_CERT_BUFFER_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_TLS_CN_BUFFER_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_TLS_SAN_BUFFER_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_TLS_PATH_MAX_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_CORS_MAX_AGE_DEFAULT`

**Value:** `"86400"`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_WINDOW`

**Value:** `60 /* 秒 */`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MAX_AGE`

**Value:** `17`
**Description:** TBD


---
### `UVHTTP_ERROR_MESSAGE_LENGTH`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_ERROR_CONTEXT_BUFFER_SIZE`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_ERROR_MESSAGE_BUFFER_SIZE`

**Value:** `512`
**Description:** TBD


---
### `UVHTTP_ERROR_LOG_BUFFER_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_MAX_FILE_PATH_SIZE`

**Value:** `2048`
**Description:** TBD


---
### `UVHTTP_DECODED_PATH_SIZE`

**Value:** `1024`
**Description:** TBD


---
### `UVHTTP_IPV6_MAX_STRING_LENGTH`

**Value:** `46`
**Description:** TBD


---
### `UVHTTP_IPV4_MAX_STRING_LENGTH`

**Value:** `16`
**Description:** TBD


---
### `UVHTTP_MAX_PORT_NUMBER`

**Value:** `65535`
**Description:** TBD


---
### `UVHTTP_MIN_PORT_NUMBER`

**Value:** `1`
**Description:** TBD


---
### `UVHTTP_SECONDS_IN_DAY`

**Value:** `86400`
**Description:** TBD


---
### `UVHTTP_MILLISECONDS_PER_SECOND`

**Value:** `1000`
**Description:** TBD


---
### `UVHTTP_NANOSECONDS_PER_MILLISECOND`

**Value:** `1000000`
**Description:** TBD


---
### `UVHTTP_CACHE_MAX_AGE`

**Value:** `3600 /* 秒 */`
**Description:** TBD


---
### `UVHTTP_DEFAULT_BASE_DELAY_MS`

**Value:** `100`
**Description:** TBD


---
### `UVHTTP_DEFAULT_MAX_DELAY_MS`

**Value:** `5000`
**Description:** TBD


---
### `UVHTTP_TCP_KEEPALIVE_TIMEOUT`

**Value:** `60 /* 秒 */`
**Description:** TBD


---
### `UVHTTP_CLIENT_IP_BUFFER_SIZE`

**Value:** `64`
**Description:** TBD


---
### `UVHTTP_SENDFILE_TIMEOUT_MS`

**Value:** `30000 /* 30 秒 */`
**Description:** TBD


---
### `UVHTTP_SENDFILE_CHUNK_SIZE`

**Value:** `(64 * 1024) /* 64KB */`
**Description:** TBD


---
### `UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_IP_OCTET_MAX_VALUE`

**Value:** `255`
**Description:** TBD


---
### `UVHTTP_RATE_LIMIT_MAX_REQUESTS`

**Value:** `1000000`
**Description:** TBD


---
### `UVHTTP_ERROR_SERVER_MIN`

**Value:** `-106`
**Description:** TBD


---
### `UVHTTP_ERROR_SERVER_MAX`

**Value:** `-100`
**Description:** TBD


---
### `UVHTTP_ERROR_CONNECTION_MIN`

**Value:** `-207`
**Description:** TBD


---
### `UVHTTP_ERROR_CONNECTION_MAX`

**Value:** `-200`
**Description:** TBD


---
### `UVHTTP_ERROR_REQUEST_MIN`

**Value:** `-307`
**Description:** TBD


---
### `UVHTTP_ERROR_REQUEST_MAX`

**Value:** `-300`
**Description:** TBD


---
### `UVHTTP_ERROR_TLS_MIN`

**Value:** `-407`
**Description:** TBD


---
### `UVHTTP_ERROR_TLS_MAX`

**Value:** `-400`
**Description:** TBD


---
### `UVHTTP_ERROR_ROUTER_MIN`

**Value:** `-504`
**Description:** TBD


---
### `UVHTTP_ERROR_ROUTER_MAX`

**Value:** `-500`
**Description:** TBD


---
### `UVHTTP_ERROR_ALLOCATOR_MIN`

**Value:** `-602`
**Description:** TBD


---
### `UVHTTP_ERROR_ALLOCATOR_MAX`

**Value:** `-600`
**Description:** TBD


---
### `UVHTTP_ERROR_WEBSOCKET_MIN`

**Value:** `-707`
**Description:** TBD


---
### `UVHTTP_ERROR_WEBSOCKET_MAX`

**Value:** `-700`
**Description:** TBD


---
### `UVHTTP_ERROR_CONFIG_MIN`

**Value:** `-903`
**Description:** TBD


---
### `UVHTTP_ERROR_CONFIG_MAX`

**Value:** `-900`
**Description:** TBD


---
### `UVHTTP_ERROR_LOG_MIN`

**Value:** `-1103`
**Description:** TBD


---
### `UVHTTP_ERROR_LOG_MAX`

**Value:** `-1100`
**Description:** TBD


---
### `UVHTTP_NULL_BYTE`

**Value:** `'\0'`
**Description:** TBD


---
### `UVHTTP_CARRIAGE_RETURN`

**Value:** `'\r'`
**Description:** TBD


---
### `UVHTTP_LINE_FEED`

**Value:** `'\n'`
**Description:** TBD


---
### `UVHTTP_TAB`

**Value:** `'\t'`
**Description:** TBD


---
### `UVHTTP_BACKSLASH`

**Value:** `'\\'`
**Description:** TBD


---
### `UVHTTP_QUOTE`

**Value:** `'"'`
**Description:** TBD


---
### `UVHTTP_ESCAPE_SEQUENCE_LENGTH`

**Value:** `6`
**Description:** TBD


---
### `UVHTTP_TAB_CHARACTER`

**Value:** `9`
**Description:** TBD


---
### `UVHTTP_SPACE_CHARACTER`

**Value:** `32`
**Description:** TBD


---
### `UVHTTP_DELETE_CHARACTER`

**Value:** `127`
**Description:** TBD


---
### `UVHTTP_HEADER_CONTENT_TYPE`

**Value:** `"Content-Type"`
**Description:** TBD


---
### `UVHTTP_HEADER_CONTENT_LENGTH`

**Value:** `"Content-Length"`
**Description:** TBD


---
### `UVHTTP_HEADER_CACHE_CONTROL`

**Value:** `"Cache-Control"`
**Description:** TBD


---
### `UVHTTP_HEADER_CONNECTION`

**Value:** `"Connection"`
**Description:** TBD


---
### `UVHTTP_HEADER_UPGRADE`

**Value:** `"Upgrade"`
**Description:** TBD


---
### `UVHTTP_HEADER_WEBSOCKET_KEY`

**Value:** `"Sec-WebSocket-Key"`
**Description:** TBD


---
### `UVHTTP_HEADER_WEBSOCKET_ACCEPT`

**Value:** `"Sec-WebSocket-Accept"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_TOO_MANY_REQUESTS`

**Value:** `"Too many requests"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_FORBIDDEN`

**Value:** `"Forbidden"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_NOT_FOUND`

**Value:** `"File not found"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_FILE_TOO_LARGE`

**Value:** `"File too large"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_INTERNAL_ERROR`

**Value:** `"Internal server error"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_MEMORY_FAILED`

**Value:** `"Memory allocation failed"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_FILE_READ_ERROR`

**Value:** `"File read error"`
**Description:** TBD


---
### `UVHTTP_MESSAGE_RESPONSE_ERROR`

**Value:** `"Response body error"`
**Description:** TBD


---
### `UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS`

**Value:** `10`
**Description:** TBD


---
### `UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN`

**Value:** `256`
**Description:** TBD


---
### `UVHTTP_DIR_LISTING_BUFFER_SIZE`

**Value:** `4096`
**Description:** TBD


---
### `UVHTTP_DIR_ENTRY_HTML_OVERHEAD`

**Value:** `200`
**Description:** TBD


---
### `UVHTTP_503_RESPONSE_CONTENT_LENGTH`

**Value:** `19`
**Description:** TBD


---
