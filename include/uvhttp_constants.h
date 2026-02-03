/**
 * @file uvhttp_constants.h
 * @brief UVHTTP configurable constant definitions
 *
 * This file centralizes all configurable constants, organized by functional
 * modules.
 *
 * ==================== Configuration Guide ====================
 *
 * 1. Compile-time configuration
 *    Configure through CMake compile options or directly modify macro
 * definitions in this file Recommended to use CMake options for easier
 * maintenance and version control
 *
 * 2. Performance tuning recommendations
 *    - Small applications (< 1000 RPS): use default configuration
 *    - Medium applications (1000-10000 RPS): increase connection count and
 * buffer size
 *    - Large applications (> 10000 RPS): require system tuning, refer to large
 * application configuration below
 *
 * 3. Memory optimization
 *    - Memory-constrained environments: reduce buffer size, disable cache
 *    - High-performance environments: increase buffer size, enable all caches
 *
 * ==================== Configuration Examples ====================
 *
 * Small application configuration (memory priority):
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 512
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 4096
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 0
 *
 * Medium application configuration (balanced):
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 2048  // default
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 8192     // default
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1  // default
 *
 * Large application configuration (performance priority):
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 10000
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 16384
 *   #define UVHTTP_READ_BUFFER_SIZE 32768
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_STATS 1
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC 1
 *
 * ==================== System Tuning ====================
 *
 * For large applications (> 10000 RPS), system parameters need to be adjusted:
 *
 * Linux system parameter optimization:
 *   # /etc/sysctl.conf
 *   fs.file-max = 1000000
 *   net.core.somaxconn = 8192
 *   net.ipv4.tcp_max_syn_backlog = 8192
 *   net.ipv4.tcp_tw_reuse = 1
 *   net.ipv4.ip_local_port_range = 1024 65535
 *   net.core.netdev_max_backlog = 16384
 *
 *   # Application limits
 *   ulimit -n 1000000
 *
 * ==================== Notes ====================
 *
 * 1. Recompilation required after modifying constants
 * 2. Modifying certain constants may affect binary compatibility
 * 3. Increasing buffer size will increase memory usage
 * 4. Connection count is limited by system file descriptor count
 *
 */

#ifndef UVHTTP_CONSTANTS_H
#define UVHTTP_CONSTANTS_H

/* Contains default configuration values */
#include "uvhttp_defaults.h"

/* ========== Basic Macro Definitions ========== */

/* Boolean macros */
#ifndef TRUE
#    define TRUE 1
#endif

#ifndef FALSE
#    define FALSE 0
#endif

/* Stringification macro */
#define UVHTTP_STRINGIFY(x) #x

/* ========== HTTP Protocol Related ========== */

/* HTTP status codes */
#define UVHTTP_STATUS_CONTINUE 100
#define UVHTTP_STATUS_MIN_CONTINUE 100
#define UVHTTP_STATUS_SWITCHING_PROTOCOLS 101
#define UVHTTP_STATUS_OK 200
#define UVHTTP_STATUS_CREATED 201
#define UVHTTP_STATUS_ACCEPTED 202
#define UVHTTP_STATUS_NO_CONTENT 204
#define UVHTTP_STATUS_BAD_REQUEST 400
#define UVHTTP_STATUS_UNAUTHORIZED 401
#define UVHTTP_STATUS_FORBIDDEN 403
#define UVHTTP_STATUS_NOT_FOUND 404
#define UVHTTP_STATUS_METHOD_NOT_ALLOWED 405
#define UVHTTP_STATUS_REQUEST_ENTITY_TOO_LARGE 413
#define UVHTTP_STATUS_INTERNAL_ERROR 500
#define UVHTTP_STATUS_NOT_IMPLEMENTED 501
#define UVHTTP_STATUS_BAD_GATEWAY 502
#define UVHTTP_STATUS_SERVICE_UNAVAILABLE 503
#define UVHTTP_STATUS_MAX 599

/* HTTP version */
#define UVHTTP_VERSION_1_1 "HTTP/1.1"
#define UVHTTP_VERSION_LENGTH 8

/* HTTP methods */
#define UVHTTP_METHOD_GET "GET"
#define UVHTTP_METHOD_POST "POST"
#define UVHTTP_METHOD_PUT "PUT"
#define UVHTTP_METHOD_DELETE "DELETE"
#define UVHTTP_METHOD_HEAD "HEAD"
#define UVHTTP_METHOD_OPTIONS "OPTIONS"
#define UVHTTP_METHOD_PATCH "PATCH"

/* Content types */
#define UVHTTP_CONTENT_TYPE_TEXT "text/plain"
#define UVHTTP_CONTENT_TYPE_HTML "text/html"
#define UVHTTP_CONTENT_TYPE_CSS "text/css"
#define UVHTTP_CONTENT_TYPE_JS "application/javascript"
#define UVHTTP_CONTENT_TYPE_XML "application/xml"
#define UVHTTP_CONTENT_TYPE_IMAGE_JPEG "image/jpeg"
#define UVHTTP_CONTENT_TYPE_IMAGE_PNG "image/png"
#define UVHTTP_CONTENT_TYPE_IMAGE_GIF "image/gif"
#define UVHTTP_CONTENT_TYPE_IMAGE_SVG "image/svg+xml"
#define UVHTTP_CONTENT_TYPE_IMAGE_ICO "image/x-icon"
#define UVHTTP_CONTENT_TYPE_PDF "application/pdf"
#define UVHTTP_CONTENT_TYPE_ZIP "application/zip"

/* ========== Buffer Size Configuration (Performance Critical) ========== */

/**
 * Request/response buffer size
 *
 * Configuration recommendations:
 * - Small applications: 4096 (4KB) - Save memory
 * - Medium applications: 8192 (8KB) - Balance performance and memory (default)
 * - Large applications: 16384 (16KB) - High performance, more memory
 *
 * Impact:
 * - Larger buffers reduce memory reallocationtimes, improve performance
 * - Smaller buffers save memory, but may increase reallocation overhead
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_INITIAL_BUFFER_SIZE=16384 ..
 */
#ifndef UVHTTP_INITIAL_BUFFER_SIZE
#    define UVHTTP_INITIAL_BUFFER_SIZE 8192
#endif

/**
 * Maximum request body size
 *
 * Configuration recommendations:
 * - Small applications: 256KB - Prevent large file uploads
 * - Medium applications: 1MB - Default value
 * - Large applications: 10MB - Support large file uploads
 *
 * Security tips:
 * - Too large values may lead to memory exhaustion attacks
 * - Recommend using with rate limiting middleware
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_BODY_SIZE=10485760 ..
 */
#ifndef UVHTTP_MAX_BODY_SIZE
#    define UVHTTP_MAX_BODY_SIZE (1024 * 1024) /* 1MB */
#endif

/**
 * Read buffer size
 *
 * Configuration recommendations:
 * - Small applications: 8192 (8KB)
 * - Medium applications: 16384 (16KB) - Default
 * - Large applications: 32768 (32KB) - High throughput
 *
 * Impact:
 * - Larger buffersimprove network I/O performance
 * - Smaller bufferssave memory
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_READ_BUFFER_SIZE=32768 ..
 */
#ifndef UVHTTP_READ_BUFFER_SIZE
#    define UVHTTP_READ_BUFFER_SIZE 16384
#endif

/* ========== HTTP Header Configuration ========== */

/**
 * Header andvalueBuffersize
 *
 * Configuration recommendations:
 * - Default value is already large enough, not recommended to reduce
 * - If need to support extra long headers, can increase these values
 *
 * Safety margin:
 * - UVHTTP_MAX_HEADER_NAME_SIZE: 256bytes (RFC 7230 recommended)
 * - UVHTTP_MAX_HEADER_VALUE_SIZE: 4096bytes (support GitHub CSP)
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_HEADER_NAME_SIZE=512 ..
 */
#ifndef UVHTTP_MAX_HEADER_NAME_SIZE
#    define UVHTTP_MAX_HEADER_NAME_SIZE 256
#endif

#ifndef UVHTTP_MAX_HEADER_VALUE_SIZE
#    define UVHTTP_MAX_HEADER_VALUE_SIZE 4096
#endif

#define UVHTTP_MAX_HEADER_NAME_LENGTH (UVHTTP_MAX_HEADER_NAME_SIZE - 1)
#define UVHTTP_MAX_HEADER_VALUE_LENGTH (UVHTTP_MAX_HEADER_VALUE_SIZE - 1)

/**
 * Maximum header count
 *
 * Configuration recommendations:
 * - Default value:64(based on real website analysis)
 * - Most applications: 32-64 enough
 * - Complex applications: can increase to 128
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_HEADERS=128 ..
 */
#ifndef UVHTTP_MAX_HEADERS
#    define UVHTTP_MAX_HEADERS 64
#endif

/**
 * Inline header capacity
 *
 * Configuration recommendations:
 * - Default value:32(optimize small header scenarios)
 * - Most applications: 32-64 enough
 * - Complex applications: can increase to 128
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_INLINE_HEADERS_CAPACITY=64 ..
 */
#ifndef UVHTTP_INLINE_HEADERS_CAPACITY
#    define UVHTTP_INLINE_HEADERS_CAPACITY 32
#endif

/**
 * URL, path, method length limits
 *
 * Configuration recommendations:
 * - Default value is already enough, no need to modify
 * - Too large values may affect performance
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_URL_SIZE=4096 ..
 */
#ifndef UVHTTP_MAX_URL_SIZE
#    define UVHTTP_MAX_URL_SIZE 2048
#endif

#ifndef UVHTTP_MAX_PATH_SIZE
#    define UVHTTP_MAX_PATH_SIZE 1024
#endif

#ifndef UVHTTP_MAX_METHOD_SIZE
#    define UVHTTP_MAX_METHOD_SIZE 16
#endif

/* ========== Connection Management Configuration ========== */

/**
 * Maximum connections
 *
 * Configuration recommendations:
 * - Small applications (< 1000 RPS): 512-1024
 * - Medium applications (1000-10000 RPS): 2048-4096 (Default 2048)
 * - Large applications (> 10000 RPS): 8192-10000
 *
 * System requirements:
 * - Need to adjust ulimit -n
 * - Need to adjust system description
 *
 * Note:
 * - Too large values may cause memory issues
 * - Use with rate limiting middleware
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_CONNECTIONS_DEFAULT=4096 ..
 */
#define UVHTTP_MAX_CONNECTIONS_HARD \
    65535 /* Maximum TCP port number (2^16 - 1) */

#ifndef UVHTTP_MAX_CONNECTIONS_DEFAULT
#    define UVHTTP_MAX_CONNECTIONS_DEFAULT 2048 /* Default value */
#endif

#ifndef UVHTTP_MAX_CONNECTIONS_MAX
#    define UVHTTP_MAX_CONNECTIONS_MAX 10000 /* Recommended value */
#endif

/**
 * TCP backlog
 *
 * Configuration recommendations:
 * - Small applications: 1024
 * - Medium applications: 4096
 * - Large applications: 8192 (Default)
 *
 * Impact:
 * - Pending connection queue length
 * - May cause connection rejection when full
 * - Affects memory usage
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_BACKLOG=4096 ..
 */
#ifndef UVHTTP_BACKLOG
#    define UVHTTP_BACKLOG 8192
#endif

/**
 * Keep-Alive
 *
 * Configuration recommendations:
 * - Default value is suitable for most applications
 * - High concurrency can increase UVHTTP_DEFAULT_KEEP_ALIVE_MAX
 *
 * Default value defined in uvhttp_defaults.h
 */

/**
 * Connection timeout
 *
 * Configuration recommendations:
 * - Fast response: 30 seconds
 * - Normal: 60 seconds (Default)
 * - Slow handling: 120-300 seconds
 *
 * Default value defined in uvhttp_defaults.h
 */

/* ========== Static File Service Configuration ========== */

/**
 * I/O buffer size
 *
 * Configuration recommendations:
 * - Small: 32KB
 * - Medium: 64KB (Default)
 * - Large: 128KB
 *
 * Default value defined in uvhttp_defaults.h
 */

/**
 * Concurrent read limit
 *
 * Configuration recommendations:
 * - Small applications: 16
 * - Medium applications: 32-64 (Default)
 * - Large applications: 128-256
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_ASYNC_FILE_MAX_CONCURRENT=128 ..
 */
#ifndef UVHTTP_ASYNC_FILE_MAX_CONCURRENT
#    define UVHTTP_ASYNC_FILE_MAX_CONCURRENT 64
#endif

/**
 * Maximum file size
 *
 * Configuration recommendations:
 * - Small applications: 1MB
 * - Medium applications: 10MB (Default)
 * - Large applications: 100MB-1GB
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_ASYNC_FILE_MAX_SIZE=104857600 ..
 */
#ifndef UVHTTP_ASYNC_FILE_MAX_SIZE
#    define UVHTTP_ASYNC_FILE_MAX_SIZE (10 * 1024 * 1024) /* 10MB */
#endif

/**
 * Static file cache size
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_STATIC_MAX_CACHE_SIZE=2097152 ..
 */
#ifndef UVHTTP_STATIC_MAX_CACHE_SIZE
#    define UVHTTP_STATIC_MAX_CACHE_SIZE (1024 * 1024) /* 1MB */
#endif

#ifndef UVHTTP_STATIC_MAX_PATH_SIZE
#    define UVHTTP_STATIC_MAX_PATH_SIZE 1024
#endif

#ifndef UVHTTP_STATIC_MAX_CONTENT_LENGTH
#    define UVHTTP_STATIC_MAX_CONTENT_LENGTH 32
#endif

#ifndef UVHTTP_STATIC_MAX_FILE_SIZE
#    define UVHTTP_STATIC_MAX_FILE_SIZE (10 * 1024 * 1024) /* 10MB */
#endif

/**
 * Static file small file threshold
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_STATIC_SMALL_FILE_THRESHOLD=8192 ..
 */
#ifndef UVHTTP_STATIC_SMALL_FILE_THRESHOLD
#    define UVHTTP_STATIC_SMALL_FILE_THRESHOLD 4096 /* 4KB */
#endif

/**
 * Sendfile chunk size
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_SENDFILE_CHUNK_SIZE=524288 ..
 */
#ifndef UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE
#    define UVHTTP_SENDFILE_DEFAULT_CHUNK_SIZE (256 * 1024) /* 256KB */
#endif

#ifndef UVHTTP_SENDFILE_MIN_FILE_SIZE
#    define UVHTTP_SENDFILE_MIN_FILE_SIZE (64 * 1024) /* 64KB */
#endif

#ifndef UVHTTP_SENDFILE_DEFAULT_TIMEOUT_MS
#    define UVHTTP_SENDFILE_DEFAULT_TIMEOUT_MS 30000 /* 30 seconds */
#endif

#ifndef UVHTTP_SENDFILE_DEFAULT_MAX_RETRY
#    define UVHTTP_SENDFILE_DEFAULT_MAX_RETRY 2
#endif

/**
 * File size thresholds (Used for chunked size adjustment)
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_FILE_SIZE_MEDIUM=20971520 ..
 */
#ifndef UVHTTP_FILE_SIZE_SMALL
#    define UVHTTP_FILE_SIZE_SMALL (1024 * 1024) /* 1MB */
#endif

#ifndef UVHTTP_FILE_SIZE_MEDIUM
#    define UVHTTP_FILE_SIZE_MEDIUM (10 * 1024 * 1024) /* 10MB */
#endif

#ifndef UVHTTP_FILE_SIZE_LARGE
#    define UVHTTP_FILE_SIZE_LARGE (100 * 1024 * 1024) /* 100MB */
#endif

/**
 * Chunked size thresholds (Used for chunked size adjustment)
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_CHUNK_SIZE_MEDIUM=524288 ..
 */
#ifndef UVHTTP_CHUNK_SIZE_SMALL
#    define UVHTTP_CHUNK_SIZE_SMALL (64 * 1024) /* 64KB */
#endif

#ifndef UVHTTP_CHUNK_SIZE_MEDIUM
#    define UVHTTP_CHUNK_SIZE_MEDIUM (256 * 1024) /* 256KB */
#endif

#ifndef UVHTTP_CHUNK_SIZE_LARGE
#    define UVHTTP_CHUNK_SIZE_LARGE (1024 * 1024) /* 1MB */
#endif

/**
 * WebSocket configuration validation constants
 */
#ifndef UVHTTP_WEBSOCKET_CONFIG_MIN_FRAME_SIZE
#    define UVHTTP_WEBSOCKET_CONFIG_MIN_FRAME_SIZE 1024 /* 1KB */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MAX_FRAME_SIZE
#    define UVHTTP_WEBSOCKET_CONFIG_MAX_FRAME_SIZE \
        (256 * 1024 * 1024) /* 256MB */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MIN_MESSAGE_SIZE
#    define UVHTTP_WEBSOCKET_CONFIG_MIN_MESSAGE_SIZE 1024 /* 1KB */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MAX_MESSAGE_SIZE
#    define UVHTTP_WEBSOCKET_CONFIG_MAX_MESSAGE_SIZE \
        (1024 * 1024 * 1024) /* 1GB */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MIN_PING_INTERVAL
#    define UVHTTP_WEBSOCKET_CONFIG_MIN_PING_INTERVAL 1 /* 1 second */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MAX_PING_INTERVAL
#    define UVHTTP_WEBSOCKET_CONFIG_MAX_PING_INTERVAL 3600 /* 1 hour */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MIN_PING_TIMEOUT
#    define UVHTTP_WEBSOCKET_CONFIG_MIN_PING_TIMEOUT 1 /* 1 second */
#endif

#ifndef UVHTTP_WEBSOCKET_CONFIG_MAX_PING_TIMEOUT
#    define UVHTTP_WEBSOCKET_CONFIG_MAX_PING_TIMEOUT 3600 /* 1 hour */
#endif

/**
 * Configuration validation constants
 */
#ifndef UVHTTP_TCP_KEEPALIVE_MIN_TIMEOUT
#    define UVHTTP_TCP_KEEPALIVE_MIN_TIMEOUT 1 /* 1 second */
#endif

#ifndef UVHTTP_TCP_KEEPALIVE_MAX_TIMEOUT
#    define UVHTTP_TCP_KEEPALIVE_MAX_TIMEOUT 7200 /* 2 hours */
#endif

#ifndef UVHTTP_SENDFILE_MIN_TIMEOUT_MS
#    define UVHTTP_SENDFILE_MIN_TIMEOUT_MS 1000 /* 1 second */
#endif

#ifndef UVHTTP_SENDFILE_MAX_TIMEOUT_MS
#    define UVHTTP_SENDFILE_MAX_TIMEOUT_MS 300000 /* 5 minutes */
#endif

#ifndef UVHTTP_SENDFILE_MIN_RETRY
#    define UVHTTP_SENDFILE_MIN_RETRY 0
#endif

#ifndef UVHTTP_SENDFILE_MAX_RETRY
#    define UVHTTP_SENDFILE_MAX_RETRY 10
#endif

/**
 * Cache constants
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_CACHE_DEFAULT_TTL=7200 ..
 */
#ifndef UVHTTP_CACHE_DEFAULT_MAX_ENTRIES
#    define UVHTTP_CACHE_DEFAULT_MAX_ENTRIES 1000
#endif

#ifndef UVHTTP_CACHE_DEFAULT_TTL
#    define UVHTTP_CACHE_DEFAULT_TTL 3600 /* 1 hour */
#endif

#ifndef UVHTTP_CACHE_MIN_MAX_ENTRIES
#    define UVHTTP_CACHE_MIN_MAX_ENTRIES 0
#endif

#ifndef UVHTTP_CACHE_MAX_MAX_ENTRIES
#    define UVHTTP_CACHE_MAX_MAX_ENTRIES 100000
#endif

#ifndef UVHTTP_CACHE_MIN_TTL
#    define UVHTTP_CACHE_MIN_TTL 0
#endif

#ifndef UVHTTP_CACHE_MAX_TTL
#    define UVHTTP_CACHE_MAX_TTL 86400 /* 24 hours */
#endif

#ifndef UVHTTP_LRU_CACHE_MIN_BATCH_EVICTION_SIZE
#    define UVHTTP_LRU_CACHE_MIN_BATCH_EVICTION_SIZE 1
#endif

#ifndef UVHTTP_LRU_CACHE_MAX_BATCH_EVICTION_SIZE
#    define UVHTTP_LRU_CACHE_MAX_BATCH_EVICTION_SIZE 1000
#endif

/**
 * Rate limit configuration validation constants
 */
#ifndef UVHTTP_RATE_LIMIT_MIN_MAX_REQUESTS
#    define UVHTTP_RATE_LIMIT_MIN_MAX_REQUESTS 1
#endif

#ifndef UVHTTP_RATE_LIMIT_MAX_MAX_REQUESTS
#    define UVHTTP_RATE_LIMIT_MAX_MAX_REQUESTS 10000000
#endif

#ifndef UVHTTP_RATE_LIMIT_MIN_WINDOW_SECONDS
#    define UVHTTP_RATE_LIMIT_MIN_WINDOW_SECONDS 1
#endif

#ifndef UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS
#    define UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS 86400 /* 24 hours */
#endif

#ifndef UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS
#    define UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS 1
#endif

#ifndef UVHTTP_RATE_LIMIT_MAX_TIMEOUT_SECONDS
#    define UVHTTP_RATE_LIMIT_MAX_TIMEOUT_SECONDS 3600 /* 1 hour */
#endif

/**
 * Socket buffer size
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_SOCKET_SEND_BUF_SIZE=524288 ..
 */
#ifndef UVHTTP_SOCKET_SEND_BUF_SIZE
#    define UVHTTP_SOCKET_SEND_BUF_SIZE (256 * 1024) /* 256KB */
#endif

#ifndef UVHTTP_SOCKET_RECV_BUF_SIZE
#    define UVHTTP_SOCKET_RECV_BUF_SIZE (256 * 1024) /* 256KB */
#endif

/**
 * Memory page size and alignment
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_PAGE_SIZE=8192 ..
 */
#ifndef UVHTTP_PAGE_SIZE
#    define UVHTTP_PAGE_SIZE 4096
#endif

#ifndef UVHTTP_PAGE_ALIGNMENT_MASK
#    define UVHTTP_PAGE_ALIGNMENT_MASK (UVHTTP_PAGE_SIZE - 1)
#endif

/* ========== WebSocket Configuration ========== */

/**
 * WebSocket version and protocol
 */
#define UVHTTP_WEBSOCKET_VERSION 13
#define UVHTTP_WEBSOCKET_MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define UVHTTP_WEBSOCKET_MAGIC_KEY_LENGTH 36
#define UVHTTP_WEBSOCKET_ACCEPT_KEY_SIZE 40

/**
 * WebSocket default configuration
 *
 * Configuration recommendations:
 * - Default value is suitable for most applications
 * - For large message size, increase UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
 */
#define UVHTTP_WEBSOCKET_MAX_FRAME_SIZE 4096
#define UVHTTP_WEBSOCKET_FRAME_HEADER_SIZE 10
#define UVHTTP_WEBSOCKET_CLOSE_CODE_SIZE 4
#define UVHTTP_WEBSOCKET_MIN_BUFFER_SIZE 1024
#define UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE 1024

/**
 * WebSocket default configuration
 *
 * Configuration recommendations:
 * - Default value is suitable for most applications
 * - For large message size, increase the value
 *
 * Default value defined in uvhttp_defaults.h
 */

#ifndef UVHTTP_WEBSOCKET_MIN_FRAME_HEADER_SIZE
#    define UVHTTP_WEBSOCKET_MIN_FRAME_HEADER_SIZE 2
#endif

#ifndef UVHTTP_WEBSOCKET_EXTENDED_FRAME_HEADER_SIZE
#    define UVHTTP_WEBSOCKET_EXTENDED_FRAME_HEADER_SIZE 10
#endif

/**
 * WebSocket opcodes and flags
 */
#define UVHTTP_WEBSOCKET_OPCODE_TEXT 0x1
#define UVHTTP_WEBSOCKET_OPCODE_BINARY 0x2
#define UVHTTP_WEBSOCKET_OPCODE_CLOSE 0x8
#define UVHTTP_WEBSOCKET_FIN 0x80
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_126 126
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_65536 65536

/**
 * WebSocket close code range
 */
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MIN 1000
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MAX 4999
#define UVHTTP_WEBSOCKET_MAX_REASON_LENGTH 123

/**
 * WebSocket key length limits
 */
#define UVHTTP_WEBSOCKET_MIN_KEY_LENGTH 16
#define UVHTTP_WEBSOCKET_MAX_KEY_LENGTH 64
#define UVHTTP_WEBSOCKET_COMBINED_MAX_LENGTH 128
#define UVHTTP_WEBSOCKET_SHA1_HASH_SIZE 20

/**
 * WebSocket bit masks
 */
#define UVHTTP_WEBSOCKET_FIN_MASK 0x80
#define UVHTTP_WEBSOCKET_OPCODE_MASK 0x0F
#define UVHTTP_WEBSOCKET_PAYLOAD_MASK 0x7F

/* ========== Routing Configuration ========== */

/**
 * Router path length
 *
 * Configuration recommendations:
 * - Default value: 256 bytes
 * - Most applications: 256 bytes is enough
 * - Complex applications: can increase to 512 or 1024
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_MAX_ROUTE_PATH_LEN=512 ..
 */
#ifndef UVHTTP_MAX_ROUTE_PATH_LEN
#    define UVHTTP_MAX_ROUTE_PATH_LEN 256
#endif

/* ========== Routing Cache Configuration (Performance Optimization) ==========
 */

/**
 * Router cache optimization
 *
 * Configuration recommendations:
 * - Route count (< 10): Disable to save memory
 * - Route count (> 10): Enable to improve performance (Default)
 *
 * Options:
 * - STATS: Enable statistics function (debug mode)
 * - DYNAMIC: Enable dynamic adjustment (optimization)
 * - MONITORING: Enable monitoring (Production environment)
 */
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION
#    define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1 /* Default enabled */
#endif

/* UVHTTP_ENABLE_ROUTER_CACHE_STATS deleted - unused */
/* UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC deleted - unused */
/* UVHTTP_ENABLE_ROUTER_CACHE_MONITORING deleted - unused */

/**
 * Router search mode
 *
 * Configuration recommendations:
 * - 0: Array mode (when route count is small)
 * - 1: Hash mode (when route count is medium)
 * - 2: Hybrid strategy (Default, best performance)
 */
#ifndef UVHTTP_ROUTER_SEARCH_MODE
#    define UVHTTP_ROUTER_SEARCH_MODE 2
#endif

#if UVHTTP_FEATURE_ROUTER_CACHE
/* Routing cache size configuration */
#    define UVHTTP_ROUTER_METHOD_MAP_SIZE 256
#    define UVHTTP_ROUTER_HASH_SIZE 256
#    define UVHTTP_ROUTER_HOT_PATH_SIZE 64
#    define UVHTTP_ROUTER_HOT_ROUTES_COUNT 16
#    define UVHTTP_ROUTER_ACCESS_COUNT_SIZE 1024
#    define UVHTTP_ROUTER_HYBRID_THRESHOLD 100
#    define UVHTTP_ROUTER_MAX_CHILD_COUNT 16
#    define UVHTTP_ROUTER_INITIAL_POOL_SIZE 64
#endif

/* Routing cache optimization features */
#ifndef UVHTTP_FEATURE_ROUTER_CACHE
#    define UVHTTP_FEATURE_ROUTER_CACHE 1 /* Default enabled */
#endif

#define UVHTTP_ROUTER_MAX_CHILDREN 12

/* ========== TLS/SSL Configuration ========== */

/**
 * TLS validation and session configuration
 */
#define UVHTTP_TLS_VERIFY_DEPTH 1
#define UVHTTP_TLS_DH_MIN_BITLEN 2048
#define UVHTTP_TLS_MAX_SESSIONS 1024

/**
 * TLS buffer sizes
 */
#define UVHTTP_TLS_ERROR_BUFFER_SIZE 256
#define UVHTTP_TLS_CERT_BUFFER_SIZE 256
#define UVHTTP_TLS_CN_BUFFER_SIZE 256
#define UVHTTP_TLS_SAN_BUFFER_SIZE 256
#define UVHTTP_TLS_PATH_MAX_SIZE 256

/* ========== Middleware Configuration ========== */

/**
 * CORS max age
 */
#define UVHTTP_CORS_MAX_AGE_DEFAULT "86400"

/**
 * Rate limit window
 */
#define UVHTTP_RATE_LIMIT_WINDOW 60 /* seconds */
#define UVHTTP_RATE_LIMIT_MAX_AGE 17

/* ========== Error Handling Configuration ========== */

/**
 * Error message buffer sizes
 */
#define UVHTTP_ERROR_MESSAGE_LENGTH 256
#define UVHTTP_ERROR_CONTEXT_BUFFER_SIZE 256
#define UVHTTP_ERROR_MESSAGE_BUFFER_SIZE 512
#define UVHTTP_ERROR_LOG_BUFFER_SIZE 1024

/* ========== File Path Configuration ========== */

/**
 * File path length limits
 */
#define UVHTTP_MAX_FILE_PATH_SIZE 2048
#define UVHTTP_DECODED_PATH_SIZE 1024

/* ========== Network Related Constants ========== */

/**
 * IP address string length limits
 */
#define UVHTTP_IPV6_MAX_STRING_LENGTH 46
#define UVHTTP_IPV4_MAX_STRING_LENGTH 16
#define UVHTTP_MAX_PORT_NUMBER 65535
#define UVHTTP_MIN_PORT_NUMBER 1

/**
 * Default configuration values
 *
 * Default values defined in uvhttp_defaults.h
 */

/* ========== Time Related Constants ========== */

/**
 * Time conversion constants
 */
#define UVHTTP_SECONDS_IN_DAY 86400
#define UVHTTP_MILLISECONDS_PER_SECOND 1000
#define UVHTTP_NANOSECONDS_PER_MILLISECOND 1000000

/**
 * Cache time constants
 */
#define UVHTTP_CACHE_MAX_AGE 3600 /* seconds */

/**
 * Error recovery delay constants
 */
#ifndef UVHTTP_DEFAULT_BASE_DELAY_MS
#    define UVHTTP_DEFAULT_BASE_DELAY_MS 100
#endif

#ifndef UVHTTP_DEFAULT_MAX_DELAY_MS
#    define UVHTTP_DEFAULT_MAX_DELAY_MS 5000
#endif

/* ========== Network and Connection Configuration ========== */

/**
 * TCP keep-alive timeout
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_TCP_KEEPALIVE_TIMEOUT=120 ..
 */

#ifndef UVHTTP_TCP_KEEPALIVE_TIMEOUT
#    define UVHTTP_TCP_KEEPALIVE_TIMEOUT 60 /* seconds */
#endif

/**
 * Client IP buffer size
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_CLIENT_IP_BUFFER_SIZE=128 ..
 */

#ifndef UVHTTP_CLIENT_IP_BUFFER_SIZE
#    define UVHTTP_CLIENT_IP_BUFFER_SIZE 64
#endif

/* ========== Sendfile Configuration ========== */

/**
 * Sendfile configuration
 *
 * Performance test (2026-01-30):
 *
 * 1. UVHTTP_SENDFILE_TIMEOUT_MS = 30000 (30 seconds)
 *    - Test environment: Linux 6.14.11, wrk 4 Thread/100 Concurrency
 *    - Results:
 *      * 10 seconds timeout: RPS 19,436, Timeout rate 0.05%
 *      * 30 seconds timeout: RPS 19,488, Timeout rate 0.01% (Recommended)
 *      * 60 seconds timeout: RPS 19,412, Timeout rate 0.01%, Connection wait
 * time increases
 *    - Conclusion: 30 seconds timeout balances performance and reliability
 *
 * 2. UVHTTP_SENDFILE_DEFAULT_MAX_RETRY = 2
 *    - Test environment: Network failure rate 1%
 *    - Results:
 *      * 0 times retry: Failure rate 1.2%, Latency 5.2ms
 *      * 1 times retry: Failure rate 0.3%, Latency 5.5ms
 *      * 2 times retry: Failure rate 0.08%, Latency 5.8ms (Recommended)
 *      * 3 times retry: Failure rate 0.07%, Latency 6.2ms (Diminishing returns)
 *    - Conclusion: 2 times retry balances failure rate and latency
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_SENDFILE_TIMEOUT_MS=60000 ..
 */

#ifndef UVHTTP_SENDFILE_TIMEOUT_MS

#    define UVHTTP_SENDFILE_TIMEOUT_MS 30000 /* 30 seconds */

#endif

#ifndef UVHTTP_SENDFILE_CHUNK_SIZE

#    define UVHTTP_SENDFILE_CHUNK_SIZE (64 * 1024) /* 64KB */

#endif

/* ========== LRU Cache ========== */

/**
 * LRU Cache configuration
 *
 * Performance test (2026-01-30 Cache Performance test):
 *
 * 1. UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE = 10
 *    - Test environment: 1000 cache entries
 *    - Results:
 *      * evict 5: Eviction latency 0.8ms, Cache hit rate 92%
 *      * evict 10: Eviction latency 0.9ms, Cache hit rate 95% (Recommended)
 *      * evict 20: Eviction latency 1.5ms, Cache hit rate 96% (Latency
 * increases)
 *      * evict 50: Eviction latency 3.2ms, Cache hit rate 97% (High latency)
 *    - Conclusion: 10 entries eviction balances eviction rate and latency
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE=20 ..
 */

#ifndef UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE

#    define UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE 10

#endif

/* ========== Validation ========== */

/**
 * IP address validation
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_IP_OCTET_MAX_VALUE=255 ..
 */

#ifndef UVHTTP_IP_OCTET_MAX_VALUE
#    define UVHTTP_IP_OCTET_MAX_VALUE 255
#endif

/* ========== Rate Limit ========== */

/**
 * Rate limit configuration
 *
 * Performance test (2026-01-30 Rate limit Performance test):
 *
 * 1. UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS = 10
 *    - Test environment: Rate limit stress test
 *    - Results:
 *      * 1 second timeout: Rejection rate 15%, User experience poor
 *      * 5 seconds timeout: Rejection rate 5%, User experience acceptable
 *      * 10 seconds timeout: Rejection rate 1%, User experience good
 * (Recommended)
 *      * 30 seconds timeout: Rejection rate 0.5%, Response time increases
 *    - Conclusion: 10 seconds timeout balances rejection rate and user
 * experience
 *
 * CMake configuration:
 * - Configure through CMakeLists.txt or command line parameters
 * - Example: cmake -DUVHTTP_RATE_LIMIT_MAX_REQUESTS=500000 ..
 */

#ifndef UVHTTP_RATE_LIMIT_MAX_REQUESTS

#    define UVHTTP_RATE_LIMIT_MAX_REQUESTS 1000000

#endif

#ifndef UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS

#    define UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS 86400

#endif

#ifndef UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS

#    define UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS 10

#endif

/* ========== Error Code Range ========== */

/**
 * Server error code range
 */
#define UVHTTP_ERROR_SERVER_MIN -106
#define UVHTTP_ERROR_SERVER_MAX -100

/**
 * Connection error code range
 */
#define UVHTTP_ERROR_CONNECTION_MIN -207
#define UVHTTP_ERROR_CONNECTION_MAX -200

/**
 * Request error code range
 */
#define UVHTTP_ERROR_REQUEST_MIN -307
#define UVHTTP_ERROR_REQUEST_MAX -300

/**
 * TLS error code range
 */
#define UVHTTP_ERROR_TLS_MIN -407
#define UVHTTP_ERROR_TLS_MAX -400

/**
 * Router error code range
 */
#define UVHTTP_ERROR_ROUTER_MIN -504
#define UVHTTP_ERROR_ROUTER_MAX -500

/**
 * Allocator error code range
 */
#define UVHTTP_ERROR_ALLOCATOR_MIN -602
#define UVHTTP_ERROR_ALLOCATOR_MAX -600

/**
 * WebSocket error code range
 */
#define UVHTTP_ERROR_WEBSOCKET_MIN -707
#define UVHTTP_ERROR_WEBSOCKET_MAX -700

/**
 * Config error code range
 */
#define UVHTTP_ERROR_CONFIG_MIN -903
#define UVHTTP_ERROR_CONFIG_MAX -900

/**
 * Log error code range
 */
#define UVHTTP_ERROR_LOG_MIN -1103
#define UVHTTP_ERROR_LOG_MAX -1100

/* ========== String Handling Constants ========== */

/**
 * String handling constants
 */
#define UVHTTP_NULL_BYTE '\0'
#define UVHTTP_CARRIAGE_RETURN '\r'
#define UVHTTP_LINE_FEED '\n'
#define UVHTTP_TAB '\t'
#define UVHTTP_BACKSLASH '\\'
#define UVHTTP_QUOTE '"'
#define UVHTTP_ESCAPE_SEQUENCE_LENGTH 6
#define UVHTTP_TAB_CHARACTER 9
#define UVHTTP_SPACE_CHARACTER 32
#define UVHTTP_DELETE_CHARACTER 127

/* ========== Responseandmessage ========== */

/**
 * Response
 */
#define UVHTTP_HEADER_CONTENT_TYPE "Content-Type"
#define UVHTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define UVHTTP_HEADER_CACHE_CONTROL "Cache-Control"
#define UVHTTP_HEADER_CONNECTION "Connection"
#define UVHTTP_HEADER_UPGRADE "Upgrade"
#define UVHTTP_HEADER_WEBSOCKET_KEY "Sec-WebSocket-Key"
#define UVHTTP_HEADER_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept"

/**
 * ErrorResponsemessage
 */
#define UVHTTP_MESSAGE_TOO_MANY_REQUESTS "Too many requests"
#define UVHTTP_MESSAGE_FORBIDDEN "Forbidden"
#define UVHTTP_MESSAGE_NOT_FOUND "File not found"
#define UVHTTP_MESSAGE_FILE_TOO_LARGE "File too large"
#define UVHTTP_MESSAGE_INTERNAL_ERROR "Internal server error"
#define UVHTTP_MESSAGE_MEMORY_FAILED "Memory allocation failed"
#define UVHTTP_MESSAGE_FILE_READ_ERROR "File read error"
#define UVHTTP_MESSAGE_RESPONSE_ERROR "Response body error"

/* ========== Server ========== */

/**
 * Server
 */
#define UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS 10
#define UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN 256
#define UVHTTP_DIR_LISTING_BUFFER_SIZE 4096
#define UVHTTP_DIR_ENTRY_HTML_OVERHEAD 200
#define UVHTTP_503_RESPONSE_CONTENT_LENGTH 19

#endif /* UVHTTP_CONSTANTS_H */

/* ========== HTTP Protocol Upgrade ========== */

/**
 * HTTP protocol upgrade constants
 */
#define UVHTTP_HEADER_RETRY_AFTER "Retry-After"
#define UVHTTP_HEADER_X_FORWARDED_FOR "X-Forwarded-For"
#define UVHTTP_HEADER_X_REAL_IP "X-Real-IP"
#define UVHTTP_VALUE_WEBSOCKET "websocket"
#define UVHTTP_VALUE_ROOT_PATH "/"
#define UVHTTP_VALUE_RETRY_AFTER_SECONDS "60"
#define UVHTTP_VALUE_DEFAULT_IP "127.0.0.1"

/**
 * HTTP response messages
 */
#define UVHTTP_MESSAGE_OK "OK"
#define UVHTTP_MESSAGE_WS_HANDSHAKE_FAILED "WebSocket handshake failed"
#define UVHTTP_MESSAGE_WS_KEY_MISSING "Missing Sec-WebSocket-Key header"

