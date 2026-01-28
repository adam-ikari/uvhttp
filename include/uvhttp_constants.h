/* UVHTTP 常量定义 */

#ifndef UVHTTP_CONSTANTS_H
#define UVHTTP_CONSTANTS_H

/* 布尔值宏 */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* 字符串化宏 */
#define UVHTTP_STRINGIFY(x) #x

/* HTTP 状态码 */
#define UVHTTP_STATUS_CONTINUE           100
#define UVHTTP_STATUS_MIN_CONTINUE       100
#define UVHTTP_STATUS_SWITCHING_PROTOCOLS 101
#define UVHTTP_STATUS_OK                 200
#define UVHTTP_STATUS_CREATED            201
#define UVHTTP_STATUS_ACCEPTED           202
#define UVHTTP_STATUS_NO_CONTENT          204
#define UVHTTP_STATUS_BAD_REQUEST        400
#define UVHTTP_STATUS_UNAUTHORIZED       401
#define UVHTTP_STATUS_FORBIDDEN           403
#define UVHTTP_STATUS_NOT_FOUND           404
#define UVHTTP_STATUS_METHOD_NOT_ALLOWED   405
#define UVHTTP_STATUS_REQUEST_ENTITY_TOO_LARGE 413
#define UVHTTP_STATUS_INTERNAL_ERROR      500
#define UVHTTP_STATUS_NOT_IMPLEMENTED     501
#define UVHTTP_STATUS_BAD_GATEWAY         502
#define UVHTTP_STATUS_SERVICE_UNAVAILABLE 503
#define UVHTTP_STATUS_MAX                 599

/* HTTP 版本 */
#define UVHTTP_VERSION_1_1 "HTTP/1.1"
#define UVHTTP_VERSION_LENGTH 8

/* 缓冲区大小 - 优化高并发性能 */
#define UVHTTP_INITIAL_BUFFER_SIZE       8192  /* 增加到 8KB 减少重分配 */
#define UVHTTP_MAX_BODY_SIZE              (1024 * 1024)  /* 1MB */

/* 文件 I/O 优化 */
#define UVHTTP_ASYNC_FILE_BUFFER_SIZE     65536  /* 64KB 文件读取缓冲区 */
#define UVHTTP_ASYNC_FILE_MAX_CONCURRENT  64  /* 最大并发文件读取数 */
#define UVHTTP_ASYNC_FILE_MAX_SIZE        (10 * 1024 * 1024)  /* 10MB 最大文件 */
#define UVHTTP_MAX_HEADERS                32  /* 基于真实网站分析（最大23个），提供1.39x安全边距 */
#define UVHTTP_MAX_HEADER_NAME_SIZE       256  /* 足够，安全边距6.74x */
#define UVHTTP_MAX_HEADER_VALUE_SIZE     4096  /* 支持GitHub CSP（3680字符），安全边距1.11x */
#define UVHTTP_MAX_HEADER_NAME_LENGTH     256
#define UVHTTP_MAX_HEADER_VALUE_LENGTH    4096
#define UVHTTP_MAX_URL_SIZE              2048
#define UVHTTP_MAX_METHOD_SIZE           16
#define UVHTTP_MAX_PATH_SIZE             1024

/* WebSocket 相关 */
#define UVHTTP_WEBSOCKET_VERSION          13
#define UVHTTP_WEBSOCKET_MAGIC_KEY        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define UVHTTP_WEBSOCKET_MAGIC_KEY_LENGTH 36
#define UVHTTP_WEBSOCKET_ACCEPT_KEY_SIZE   40  /* Base64编码的SHA1哈希长度 */
#define UVHTTP_WEBSOCKET_MAX_FRAME_SIZE    4096
#define UVHTTP_WEBSOCKET_FRAME_HEADER_SIZE 10
#define UVHTTP_WEBSOCKET_CLOSE_CODE_SIZE   4
#define UVHTTP_WEBSOCKET_OPCODE_TEXT       0x1
#define UVHTTP_WEBSOCKET_OPCODE_BINARY     0x2
#define UVHTTP_WEBSOCKET_OPCODE_CLOSE      0x8
#define UVHTTP_WEBSOCKET_FIN              0x80
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_126   126
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_65536 65536

/* 连接相关 - 基于生产环境测试的保守值 */
#define UVHTTP_MAX_CONNECTIONS_HARD      65535  /* 硬限制，基于系统文件描述符 */
#define UVHTTP_MAX_CONNECTIONS_DEFAULT  2048   /* 默认值，适合大多数应用 */
#define UVHTTP_MAX_CONNECTIONS_MAX      10000  /* 最大推荐值，需要系统调优 */
#define UVHTTP_READ_BUFFER_SIZE          16384 /* 16KB缓冲区，提升吞吐量 */

/* HTTP响应头安全边距 */
#define UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN 256  /* 响应头安全边距，防止缓冲区溢出 */

/* 静态文件服务常量 */
#define UVHTTP_DIR_LISTING_BUFFER_SIZE 4096     /* 目录列表HTML缓冲区大小 */
#define UVHTTP_DIR_ENTRY_HTML_OVERHEAD 200      /* 每个目录条目的HTML开销 */

/* 默认网络配置 */
#define UVHTTP_DEFAULT_HOST "0.0.0.0"           /* 默认监听地址 */
#define UVHTTP_DEFAULT_PORT 8080                /* 默认端口 */
#define UVHTTP_BACKLOG                    8192  /* 从2048增加到8192，支持更高并发 */

/* 服务器清理配置 */
#define UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS 10  /* 服务器关闭时运行循环的迭代次数，确保所有关闭回调被执行 */



/* TLS 相关 */
#define UVHTTP_TLS_VERIFY_DEPTH           1
#define UVHTTP_TLS_DH_MIN_BITLEN          2048
#define UVHTTP_TLS_MAX_SESSIONS           1024
#define UVHTTP_TLS_ERROR_BUFFER_SIZE      256
#define UVHTTP_TLS_CERT_BUFFER_SIZE       256
#define UVHTTP_TLS_CN_BUFFER_SIZE         256
#define UVHTTP_TLS_SAN_BUFFER_SIZE       256
#define UVHTTP_TLS_PATH_MAX_SIZE          256



/* 中间件相关 */
#define UVHTTP_CORS_MAX_AGE_DEFAULT       "86400"
#define UVHTTP_RATE_LIMIT_WINDOW          60
#define UVHTTP_RATE_LIMIT_MAX_AGE        17
#define UVHTTP_STATIC_MAX_CACHE_SIZE      (1024 * 1024)
#define UVHTTP_STATIC_MAX_PATH_SIZE       1024
#define UVHTTP_STATIC_MAX_FILE_SIZE       (10 * 1024 * 1024)
#define UVHTTP_STATIC_MAX_CONTENT_LENGTH  32



/* 错误消息长度 */
#define UVHTTP_ERROR_MESSAGE_LENGTH      256

/* 文件路径相关 */
#define UVHTTP_MAX_FILE_PATH_SIZE        2048
#define UVHTTP_DECODED_PATH_SIZE        1024

/* 字符串处理 */
#define UVHTTP_NULL_BYTE                 '\0'
#define UVHTTP_CARRIAGE_RETURN           '\r'
#define UVHTTP_LINE_FEED                 '\n'
#define UVHTTP_TAB                       '\t'
#define UVHTTP_BACKSLASH                 '\\'
#define UVHTTP_QUOTE                     '"'
#define UVHTTP_ESCAPE_SEQUENCE_LENGTH    6

/* 位掩码 */
#define UVHTTP_WEBSOCKET_FIN_MASK         0x80
#define UVHTTP_WEBSOCKET_OPCODE_MASK      0x0F
#define UVHTTP_WEBSOCKET_PAYLOAD_MASK     0x7F

/* HTTP 方法 */
#define UVHTTP_METHOD_GET                 "GET"
#define UVHTTP_METHOD_POST                "POST"
#define UVHTTP_METHOD_PUT                 "PUT"
#define UVHTTP_METHOD_DELETE              "DELETE"
#define UVHTTP_METHOD_HEAD                "HEAD"
#define UVHTTP_METHOD_OPTIONS             "OPTIONS"
#define UVHTTP_METHOD_PATCH               "PATCH"

/* 内容类型 */
#define UVHTTP_CONTENT_TYPE_TEXT         "text/plain"
#define UVHTTP_CONTENT_TYPE_HTML         "text/html"
#define UVHTTP_CONTENT_TYPE_CSS          "text/css"
#define UVHTTP_CONTENT_TYPE_JS           "application/javascript"

#define UVHTTP_CONTENT_TYPE_XML          "application/xml"
#define UVHTTP_CONTENT_TYPE_IMAGE_JPEG   "image/jpeg"
#define UVHTTP_CONTENT_TYPE_IMAGE_PNG    "image/png"
#define UVHTTP_CONTENT_TYPE_IMAGE_GIF    "image/gif"
#define UVHTTP_CONTENT_TYPE_IMAGE_SVG    "image/svg+xml"
#define UVHTTP_CONTENT_TYPE_IMAGE_ICO    "image/x-icon"
#define UVHTTP_CONTENT_TYPE_PDF          "application/pdf"
#define UVHTTP_CONTENT_TYPE_ZIP          "application/zip"

/* 响应头 */
#define UVHTTP_HEADER_CONTENT_TYPE        "Content-Type"
#define UVHTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define UVHTTP_HEADER_CACHE_CONTROL      "Cache-Control"
#define UVHTTP_HEADER_CONNECTION         "Connection"
#define UVHTTP_HEADER_UPGRADE           "Upgrade"
#define UVHTTP_HEADER_WEBSOCKET_KEY      "Sec-WebSocket-Key"
#define UVHTTP_HEADER_WEBSOCKET_ACCEPT   "Sec-WebSocket-Accept"

/* 响应消息 */
#define UVHTTP_MESSAGE_TOO_MANY_REQUESTS  "Too many requests"
#define UVHTTP_MESSAGE_FORBIDDEN          "Forbidden"
#define UVHTTP_MESSAGE_NOT_FOUND          "File not found"
#define UVHTTP_MESSAGE_FILE_TOO_LARGE     "File too large"
#define UVHTTP_MESSAGE_INTERNAL_ERROR     "Internal server error"
#define UVHTTP_MESSAGE_MEMORY_FAILED      "Memory allocation failed"
#define UVHTTP_MESSAGE_FILE_READ_ERROR    "File read error"
#define UVHTTP_MESSAGE_RESPONSE_ERROR     "Response body error"

/* 时间相关 */
#define UVHTTP_SECONDS_IN_DAY            86400
#define UVHTTP_CACHE_MAX_AGE             3600

/* 网络相关常量 */
#define UVHTTP_IPV6_MAX_STRING_LENGTH    46
#define UVHTTP_IPV4_MAX_STRING_LENGTH    16
#define UVHTTP_MAX_PORT_NUMBER           65535
#define UVHTTP_MIN_PORT_NUMBER           1
#define UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT 5
#define UVHTTP_DEFAULT_KEEP_ALIVE_MAX    100
#define UVHTTP_503_RESPONSE_CONTENT_LENGTH 19

/* WebSocket 协议常量 */
#define UVHTTP_WEBSOCKET_MIN_KEY_LENGTH  16
#define UVHTTP_WEBSOCKET_MAX_KEY_LENGTH  64
#define UVHTTP_WEBSOCKET_COMBINED_MAX_LENGTH 128
#define UVHTTP_WEBSOCKET_SHA1_HASH_SIZE  20
#define UVHTTP_WEBSOCKET_MIN_BUFFER_SIZE 1024
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MIN  1000
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MAX  4999
#define UVHTTP_WEBSOCKET_MAX_REASON_LENGTH 123

/* 控制字符常量 */
#define UVHTTP_TAB_CHARACTER            9
#define UVHTTP_SPACE_CHARACTER          32
#define UVHTTP_DELETE_CHARACTER         127

/* 时间常量 */
#define UVHTTP_MILLISECONDS_PER_SECOND   1000
#define UVHTTP_NANOSECONDS_PER_MILLISECOND 1000000
#define UVHTTP_DEFAULT_BASE_DELAY_MS     100
#define UVHTTP_DEFAULT_MAX_DELAY_MS      5000

/* 错误处理常量 */
#define UVHTTP_ERROR_CONTEXT_BUFFER_SIZE 256
#define UVHTTP_ERROR_MESSAGE_BUFFER_SIZE 512
#define UVHTTP_ERROR_LOG_BUFFER_SIZE     1024

/* 路由缓存常量 */


/* 路由查找模式选择 */
#ifndef UVHTTP_ROUTER_SEARCH_MODE
#define UVHTTP_ROUTER_SEARCH_MODE 2  /* 0=纯线性, 1=纯哈希, 2=混合策略(默认) */
#endif
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION
#define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1
#endif

/* 路由缓存统计功能开关（默认禁用） */
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_STATS
#define UVHTTP_ENABLE_ROUTER_CACHE_STATS 0
#endif

/* 路由缓存动态调整功能开关（默认禁用） */
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC
#define UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC 0
#endif

/* 路由缓存性能监控功能开关（默认禁用） */
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_MONITORING
#define UVHTTP_ENABLE_ROUTER_CACHE_MONITORING 0
#endif

#if UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION
/* ========== 路由缓存相关常量 ========== */
#define UVHTTP_ROUTER_METHOD_MAP_SIZE    256
#define UVHTTP_ROUTER_HASH_SIZE          256
#define UVHTTP_ROUTER_HOT_PATH_SIZE      64
#define UVHTTP_ROUTER_HOT_ROUTES_COUNT   16
#define UVHTTP_ROUTER_ACCESS_COUNT_SIZE  1024
#define UVHTTP_ROUTER_HYBRID_THRESHOLD   100
#define UVHTTP_ROUTER_MAX_CHILD_COUNT    16
#define UVHTTP_ROUTER_INITIAL_POOL_SIZE  64
#endif /* UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION */

/* ========== 路由配置 ========== */
#define UVHTTP_ROUTER_MAX_CHILDREN       16

/* ========== WebSocket配置 ========== */
#define UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE 1024

/* ========== 错误恢复配置（如果未定义）========== */
#ifndef UVHTTP_DEFAULT_BASE_DELAY_MS
#define UVHTTP_DEFAULT_BASE_DELAY_MS     100
#endif

#ifndef UVHTTP_DEFAULT_MAX_DELAY_MS
#define UVHTTP_DEFAULT_MAX_DELAY_MS      5000
#endif

#endif /* UVHTTP_CONSTANTS_H */
/* 连接超时相关常量 */
#define UVHTTP_CONNECTION_TIMEOUT_DEFAULT 60  /* 默认连接超时时间（秒） */
#define UVHTTP_CONNECTION_TIMEOUT_MIN     5   /* 最小连接超时时间（秒） */
#define UVHTTP_CONNECTION_TIMEOUT_MAX     300 /* 最大连接超时时间（秒） */
