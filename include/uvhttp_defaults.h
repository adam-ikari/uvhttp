/**
 * @file uvhttp_defaults.h
 * @brief UVHTTP DefaultConfiguration value
 *
 * manageofDefault value, onemanageandmodify
 * ofDefault value CMake Compile option
 */

#ifndef UVHTTP_DEFAULTS_H
#define UVHTTP_DEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Server Configuration Default Values ========== */

/**
 * Maximum connections
 *
 * Configuration recommendation:
 * - : 512
 * - : 2048(Default)
 * - : 10000
 *
 * CMake configuration:
 * cmake -DUVHTTP_MAX_CONNECTIONS_DEFAULT=4096 ..
 */
#ifndef UVHTTP_MAX_CONNECTIONS_DEFAULT
#    define UVHTTP_MAX_CONNECTIONS_DEFAULT 2048
#endif

/**
 * TCP backlog(pendinghandleConnectionqueuelength)
 *
 * Configuration recommendation:
 * - : 1024
 * - : 256(Default)
 * - : 8192
 *
 * CMake configuration:
 * cmake -DUVHTTP_BACKLOG_DEFAULT=4096 ..
 */
#ifndef UVHTTP_BACKLOG_DEFAULT
#    define UVHTTP_BACKLOG_DEFAULT 256
#endif

/**
 * Defaultlistenaddressand
 */
#define UVHTTP_DEFAULT_HOST "0.0.0.0"
#define UVHTTP_DEFAULT_PORT 8080

/**
 * Keep-Alive
 *
 * Configuration recommendation:
 * - Default value
 * - Concurrencyincrease UVHTTP_DEFAULT_KEEP_ALIVE_MAX
 */
#define UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT 30 /* seconds */
#define UVHTTP_DEFAULT_KEEP_ALIVE_MAX      \
    100 /* Maximum requests per connection \
         */

/* ========== Timeout Configuration Default Values ========== */

/**
 * ConnectionTimeoutwhen
 *
 * Configuration recommendation:
 * - Response: 30seconds
 * - : 60seconds(Default)
 * - whenhandle: 120-300seconds
 */
#ifndef UVHTTP_CONNECTION_TIMEOUT_DEFAULT
#    define UVHTTP_CONNECTION_TIMEOUT_DEFAULT 60
#endif

#ifndef UVHTTP_CONNECTION_TIMEOUT_MIN
#    define UVHTTP_CONNECTION_TIMEOUT_MIN 5
#endif

#ifndef UVHTTP_CONNECTION_TIMEOUT_MAX
#    define UVHTTP_CONNECTION_TIMEOUT_MAX 300
#endif

/**
 * Request timeoutwhen(seconds)
 */
#define UVHTTP_DEFAULT_REQUEST_TIMEOUT 60

/* ========== Buffer and Size Limit Default Values ========== */

/**
 * Read buffer size(bytes)- Performance optimization
 *
 * Configuration recommendation:
 * - to: 16KB(Default)
 * - to: 32KB
 * - : 64KB
 *
 * optimizationdescription: BufferreduceSystemtimes, improve
 */
#ifndef UVHTTP_DEFAULT_READ_BUFFER_SIZE
#    define UVHTTP_DEFAULT_READ_BUFFER_SIZE \
        16384 /* 16KB - optimization:  8KB increaseto 16KB */
#endif

/**
 * Requestsize(bytes)
 *
 * Configuration recommendation:
 * - : 1MB(Default)
 * - : 10-100MB
 */
#define UVHTTP_DEFAULT_MAX_BODY_SIZE (1024 * 1024) /* 1MB */

/**
 * Requestsize(bytes)
 */
#define UVHTTP_DEFAULT_MAX_HEADER_SIZE 8192

/**
 * URL length(bytes)
 */
#define UVHTTP_DEFAULT_MAX_URL_SIZE 2048

/**
 * ResponseFile size(bytes)
 */
#define UVHTTP_DEFAULT_MAX_FILE_SIZE (10 * 1024 * 1024) /* 10MB */

/**
 * Asynchronous I/O Buffersize(bytes)- Performance optimization
 *
 * Configuration recommendation:
 * - to: 64KB
 * - to: 128KB(Default)
 * - : 256KB
 *
 * optimizationdescription: Bufferreduce I/O times, improvetransfer
 */
#ifndef UVHTTP_ASYNC_FILE_BUFFER_SIZE
#    define UVHTTP_ASYNC_FILE_BUFFER_SIZE \
        131072 /* 128KB - optimization:  64KB increaseto 128KB */
#endif

/* ========== Security Configuration Default Values ========== */

/**
 * ConnectionRequest
 */
#define UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN 100

/**
 * Rate limiting time window(seconds)
 */
#define UVHTTP_DEFAULT_RATE_LIMIT_WINDOW 60

/* ========== WebSocket Configuration Default Values ========== */

/**
 * WebSocket Maximum frame size(bytes)
 *
 * Configuration recommendation:
 * - message: 1MB
 * - : 16MB(Default)
 * - message: 32MB+
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE \
        (16 * 1024 * 1024) /* 16MB */
#endif

/**
 * WebSocket Maximum message size(bytes)
 *
 * Configuration recommendation:
 * - message: 8MB
 * - : 64MB(Default)
 * - message: 128MB+
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE \
        (64 * 1024 * 1024) /* 64MB */
#endif

/**
 * WebSocket receiveBuffersize(bytes)
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE (64 * 1024) /* 64KB */
#endif

/**
 * WebSocket Ping interval(seconds)
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL
#    define UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL 30
#endif

/**
 * WebSocket Ping Timeout(seconds)
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT
#    define UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT 10
#endif

/* ========== Memory Configuration Default Values ========== */

/**
 * memorysize(bytes)
 */
#define UVHTTP_DEFAULT_MEMORY_POOL_SIZE (16 * 1024 * 1024) /* 16MB */

/**
 * memoryWarningvalue
 */
#define UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD 0.8

/* ========== Feature Toggle Default Values ========== */

/* UVHTTP_DEFAULT_ENABLE_COMPRESSION deleted - unused */
/* UVHTTP_DEFAULT_ENABLE_TLS deleted - unused */
/* UVHTTP_DEFAULT_ENABLE_MEMORY_DEBUG deleted - unused */

/**
 * Log level
 *
 * 0: FATAL
 * 1: ERROR
 * 2: WARN
 * 3: INFO
 * 4: DEBUG
 * 5: TRACE
 */
#define UVHTTP_DEFAULT_LOG_LEVEL 2 /* INFO */

/* UVHTTP_DEFAULT_ENABLE_ACCESS_LOG delete - Use */

/* ========== CORS Configuration Default Values ========== */

/**
 * CORS Cachewhen(seconds)
 */
#define UVHTTP_CORS_MAX_AGE_DEFAULT "86400"

/* ========== Hash Configuration Default Values ========== */

/**
 * hash
 */
#define UVHTTP_HASH_DEFAULT_SEED 0x1A2B3C4D5E6F7089ULL

/* ========== Backward Compatible Macro Aliases ========== */

/* Maintained for backward compatibility */
#define UVHTTP_DEFAULT_MAX_CONNECTIONS UVHTTP_MAX_CONNECTIONS_DEFAULT
#define UVHTTP_DEFAULT_BACKLOG UVHTTP_BACKLOG_DEFAULT
#define UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_DEFAULTS_H */