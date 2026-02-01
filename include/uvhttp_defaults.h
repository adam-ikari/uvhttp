/**
 * @file uvhttp_defaults.h
 * @brief UVHTTP 默认配置值
 *
 * 本文件集中管理所有可配置的默认值，便于统一管理和修改
 * 所有默认值都可以通过 CMake 编译选项覆盖
 */

#ifndef UVHTTP_DEFAULTS_H
#define UVHTTP_DEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 服务器配置默认值 ========== */

/**
 * 最大连接数
 *
 * 配置建议：
 * - 小型应用：512
 * - 中型应用：2048（默认）
 * - 大型应用：10000
 *
 * CMake 配置：
 * cmake -DUVHTTP_MAX_CONNECTIONS_DEFAULT=4096 ..
 */
#ifndef UVHTTP_MAX_CONNECTIONS_DEFAULT
#    define UVHTTP_MAX_CONNECTIONS_DEFAULT 2048
#endif

/**
 * TCP backlog（待处理连接队列长度）
 *
 * 配置建议：
 * - 小型应用：1024
 * - 中型应用：256（默认）
 * - 大型应用：8192
 *
 * CMake 配置：
 * cmake -DUVHTTP_BACKLOG_DEFAULT=4096 ..
 */
#ifndef UVHTTP_BACKLOG_DEFAULT
#    define UVHTTP_BACKLOG_DEFAULT 256
#endif

/**
 * 默认监听地址和端口
 */
#define UVHTTP_DEFAULT_HOST "0.0.0.0"
#define UVHTTP_DEFAULT_PORT 8080

/**
 * Keep-Alive 配置
 *
 * 配置建议：
 * - 默认值适合大多数应用
 * - 高并发场景可以增加 UVHTTP_DEFAULT_KEEP_ALIVE_MAX
 */
#define UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT 30 /* 秒 */
#define UVHTTP_DEFAULT_KEEP_ALIVE_MAX 100    /* 每个连接最大请求数 */

/* ========== 超时配置默认值 ========== */

/**
 * 连接超时时间
 *
 * 配置建议：
 * - 快速响应应用：30秒
 * - 普通应用：60秒（默认）
 * - 长时间处理应用：120-300秒
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
 * 请求超时时间（秒）
 */
#define UVHTTP_DEFAULT_REQUEST_TIMEOUT 60

/* ========== 缓冲区和大小限制默认值 ========== */

/**
 * 读取缓冲区大小（字节）- 性能优化
 *
 * 配置建议：
 * - 小文件为主：16KB（默认）
 * - 大文件为主：32KB
 * - 超大文件：64KB
 *
 * 优化说明：增大缓冲区可以减少系统调用次数，提升性能
 */
#ifndef UVHTTP_DEFAULT_READ_BUFFER_SIZE
#    define UVHTTP_DEFAULT_READ_BUFFER_SIZE \
        16384 /* 16KB - 优化：从 8KB 增加到 16KB */
#endif

/**
 * 请求体最大大小（字节）
 *
 * 配置建议：
 * - 小型应用：1MB（默认）
 * - 文件上传应用：10-100MB
 */
#define UVHTTP_DEFAULT_MAX_BODY_SIZE (1024 * 1024) /* 1MB */

/**
 * 请求头最大大小（字节）
 */
#define UVHTTP_DEFAULT_MAX_HEADER_SIZE 8192

/**
 * URL 最大长度（字节）
 */
#define UVHTTP_DEFAULT_MAX_URL_SIZE 2048

/**
 * 文件响应最大文件大小（字节）
 */
#define UVHTTP_DEFAULT_MAX_FILE_SIZE (10 * 1024 * 1024) /* 10MB */

/**
 * 异步文件 I/O 缓冲区大小（字节）- 性能优化
 *
 * 配置建议：
 * - 小文件为主：64KB
 * - 大文件为主：128KB（默认）
 * - 超大文件：256KB
 *
 * 优化说明：增大缓冲区可以减少 I/O 操作次数，提升大文件传输性能
 */
#ifndef UVHTTP_ASYNC_FILE_BUFFER_SIZE
#    define UVHTTP_ASYNC_FILE_BUFFER_SIZE \
        131072 /* 128KB - 优化：从 64KB 增加到 128KB */
#endif

/* ========== 安全配置默认值 ========== */

/**
 * 每个连接的最大请求数
 */
#define UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN 100

/**
 * 限流时间窗口（秒）
 */
#define UVHTTP_DEFAULT_RATE_LIMIT_WINDOW 60

/* ========== WebSocket 配置默认值 ========== */

/**
 * WebSocket 最大帧大小（字节）
 *
 * 配置建议：
 * - 小消息应用：1MB
 * - 普通应用：16MB（默认）
 * - 大消息应用：32MB+
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE \
        (16 * 1024 * 1024) /* 16MB */
#endif

/**
 * WebSocket 最大消息大小（字节）
 *
 * 配置建议：
 * - 小消息应用：8MB
 * - 普通应用：64MB（默认）
 * - 大消息应用：128MB+
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE \
        (64 * 1024 * 1024) /* 64MB */
#endif

/**
 * WebSocket 接收缓冲区大小（字节）
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE
#    define UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE (64 * 1024) /* 64KB */
#endif

/**
 * WebSocket Ping 间隔（秒）
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL
#    define UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL 30
#endif

/**
 * WebSocket Ping 超时（秒）
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT
#    define UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT 10
#endif

/* ========== 内存配置默认值 ========== */

/**
 * 内存池大小（字节）
 */
#define UVHTTP_DEFAULT_MEMORY_POOL_SIZE (16 * 1024 * 1024) /* 16MB */

/**
 * 内存警告阈值
 */
#define UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD 0.8

/* ========== 功能开关默认值 ========== */

/* UVHTTP_DEFAULT_ENABLE_COMPRESSION 已删除 - 未使用 */
/* UVHTTP_DEFAULT_ENABLE_TLS 已删除 - 未使用 */
/* UVHTTP_DEFAULT_ENABLE_MEMORY_DEBUG 已删除 - 未使用 */

/**
 * 日志级别
 *
 * 0: FATAL
 * 1: ERROR
 * 2: WARN
 * 3: INFO
 * 4: DEBUG
 * 5: TRACE
 */
#define UVHTTP_DEFAULT_LOG_LEVEL 2 /* INFO */

/* UVHTTP_DEFAULT_ENABLE_ACCESS_LOG 已删除 - 未使用 */

/* ========== CORS 配置默认值 ========== */

/**
 * CORS 最大缓存时间（秒）
 */
#define UVHTTP_CORS_MAX_AGE_DEFAULT "86400"

/* ========== 哈希配置默认值 ========== */

/**
 * 哈希种子
 */
#define UVHTTP_HASH_DEFAULT_SEED 0x1A2B3C4D5E6F7089ULL

/* ========== 向后兼容的宏别名 ========== */

/* 保持向后兼容的别名 */
#define UVHTTP_DEFAULT_MAX_CONNECTIONS UVHTTP_MAX_CONNECTIONS_DEFAULT
#define UVHTTP_DEFAULT_BACKLOG UVHTTP_BACKLOG_DEFAULT
#define UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_DEFAULTS_H */