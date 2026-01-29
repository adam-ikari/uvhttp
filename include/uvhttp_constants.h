/**
 * @file uvhttp_constants.h
 * @brief UVHTTP 可配置常量定义
 * 
 * 本文件集中了所有可配置的常量，按功能模块分类。
 * 
 * ==================== 配置指南 ====================
 * 
 * 1. 编译时配置
 *    通过 CMake 编译选项或直接修改本文件中的宏定义来配置
 *    推荐使用 CMake 选项，便于维护和版本控制
 * 
 * 2. 性能调优建议
 *    - 小型应用（< 1000 RPS）：使用默认配置
 *    - 中型应用（1000-10000 RPS）：增加连接数和缓冲区大小
 *    - 大型应用（> 10000 RPS）：需要系统调优，参考下面的大型应用配置
 * 
 * 3. 内存优化
 *    - 内存受限环境：减小缓冲区大小，禁用缓存
 *    - 高性能环境：增加缓冲区大小，启用所有缓存
 * 
 * ==================== 配置示例 ====================
 * 
 * 小型应用配置（内存优先）：
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 512
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 4096
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 0
 * 
 * 中型应用配置（平衡）：
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 2048  // 默认
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 8192     // 默认
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1  // 默认
 * 
 * 大型应用配置（性能优先）：
 *   #define UVHTTP_MAX_CONNECTIONS_DEFAULT 10000
 *   #define UVHTTP_INITIAL_BUFFER_SIZE 16384
 *   #define UVHTTP_READ_BUFFER_SIZE 32768
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_STATS 1
 *   #define UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC 1
 * 
 * ==================== 系统调优 ====================
 * 
 * 对于大型应用（> 10000 RPS），需要调整系统参数：
 * 
 * Linux 系统参数优化：
 *   # /etc/sysctl.conf
 *   fs.file-max = 1000000
 *   net.core.somaxconn = 8192
 *   net.ipv4.tcp_max_syn_backlog = 8192
 *   net.ipv4.tcp_tw_reuse = 1
 *   net.ipv4.ip_local_port_range = 1024 65535
 *   net.core.netdev_max_backlog = 16384
 * 
 *   # 应用限制
 *   ulimit -n 1000000
 * 
 * ==================== 注意事项 ====================
 * 
 * 1. 修改常量后需要重新编译
 * 2. 某些常量的修改可能影响二进制兼容性
 * 3. 增加缓冲区大小会增加内存使用
 * 4. 连接数受限于系统文件描述符数量
 * 
 */

#ifndef UVHTTP_CONSTANTS_H
#define UVHTTP_CONSTANTS_H

/* ========== 基础宏定义 ========== */

/* 布尔值宏 */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* 字符串化宏 */
#define UVHTTP_STRINGIFY(x) #x

/* ========== HTTP 协议相关 ========== */

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

/* ========== 缓冲区大小配置（性能关键） ========== */

/**
 * 请求/响应缓冲区大小
 * 
 * 配置建议：
 * - 小型应用：4096 (4KB) - 节省内存
 * - 中型应用：8192 (8KB) - 平衡性能和内存（默认）
 * - 大型应用：16384 (16KB) - 高性能，更多内存
 * 
 * 影响：
 * - 较大的缓冲区减少内存重分配次数，提升性能
 * - 较小的缓冲区节省内存，但可能增加重分配开销
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_INITIAL_BUFFER_SIZE=16384 ..
 */
#ifndef UVHTTP_INITIAL_BUFFER_SIZE
#define UVHTTP_INITIAL_BUFFER_SIZE       8192
#endif

/**
 * 最大请求体大小
 * 
 * 配置建议：
 * - 小型应用：256KB - 防止大文件上传
 * - 中型应用：1MB - 默认值
 * - 大型应用：10MB - 支持大文件上传
 * 
 * 安全提示：
 * - 过大的值可能导致内存耗尽攻击
 * - 建议结合限流中间件使用
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_MAX_BODY_SIZE=10485760 ..
 */
#ifndef UVHTTP_MAX_BODY_SIZE
#define UVHTTP_MAX_BODY_SIZE              (1024 * 1024)  /* 1MB */
#endif

/**
 * 读取缓冲区大小
 * 
 * 配置建议：
 * - 小型应用：8192 (8KB)
 * - 中型应用：16384 (16KB) - 默认
 * - 大型应用：32768 (32KB) - 高吞吐量
 * 
 * 影响：
 * - 较大的缓冲区提升网络 I/O 性能
 * - 较小的缓冲区节省内存
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_READ_BUFFER_SIZE=32768 ..
 */
#ifndef UVHTTP_READ_BUFFER_SIZE
#define UVHTTP_READ_BUFFER_SIZE          16384
#endif

/* ========== HTTP 头部配置 ========== */

/**
 * Header 名称和值缓冲区大小
 * 
 * 配置建议：
 * - 默认值已经足够大，不建议减小
 * - 如果需要支持超长 header，可以增加这些值
 * 
 * 安全边距：
 * - UVHTTP_MAX_HEADER_NAME_SIZE: 256字节（RFC 7230 建议）
 * - UVHTTP_MAX_HEADER_VALUE_SIZE: 4096字节（支持 GitHub CSP）
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_MAX_HEADER_NAME_SIZE=512 ..
 */
#ifndef UVHTTP_MAX_HEADER_NAME_SIZE
#define UVHTTP_MAX_HEADER_NAME_SIZE       256
#endif

#ifndef UVHTTP_MAX_HEADER_VALUE_SIZE
#define UVHTTP_MAX_HEADER_VALUE_SIZE     4096
#endif

#define UVHTTP_MAX_HEADER_NAME_LENGTH     (UVHTTP_MAX_HEADER_NAME_SIZE - 1)
#define UVHTTP_MAX_HEADER_VALUE_LENGTH    (UVHTTP_MAX_HEADER_VALUE_SIZE - 1)

/**
 * 最大 header 数量
 * 
 * 配置建议：
 * - 默认值：64（基于真实网站分析）
 * - 大多数应用：32-64 足够
 * - 复杂应用：可以增加到 128
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_MAX_HEADERS=128 ..
 */
#ifndef UVHTTP_MAX_HEADERS
#define UVHTTP_MAX_HEADERS                64
#endif

/**
 * 内联 header 容量
 * 
 * 配置建议：
 * - 默认值：32（优化小 header 场景）
 * - 大多数应用：32-64 足够
 * - 复杂应用：可以增加到 128
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_INLINE_HEADERS_CAPACITY=64 ..
 */
#ifndef UVHTTP_INLINE_HEADERS_CAPACITY
#define UVHTTP_INLINE_HEADERS_CAPACITY     32
#endif

/**
 * URL、路径、方法长度限制
 * 
 * 配置建议：
 * - 默认值已经足够，不建议修改
 * - 过大的值可能影响性能
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_MAX_URL_SIZE=4096 ..
 */
#ifndef UVHTTP_MAX_URL_SIZE
#define UVHTTP_MAX_URL_SIZE              2048
#endif

#ifndef UVHTTP_MAX_PATH_SIZE
#define UVHTTP_MAX_PATH_SIZE             1024
#endif

#ifndef UVHTTP_MAX_METHOD_SIZE
#define UVHTTP_MAX_METHOD_SIZE           16
#endif

/* ========== 连接管理配置 ========== */

/**
 * 最大连接数配置
 * 
 * 配置建议：
 * - 小型应用（< 1000 RPS）：512-1024
 * - 中型应用（1000-10000 RPS）：2048-4096（默认 2048）
 * - 大型应用（> 10000 RPS）：8192-10000
 * 
 * 系统要求：
 * - 需要调整 ulimit -n
 * - 需要调整系统文件描述符限制
 * 
 * 注意：
 * - 过大的值可能导致内存不足
 * - 建议结合限流中间件使用
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_MAX_CONNECTIONS_DEFAULT=4096 ..
 */
#define UVHTTP_MAX_CONNECTIONS_HARD      65535  /* 硬限制 */

#ifndef UVHTTP_MAX_CONNECTIONS_DEFAULT
#define UVHTTP_MAX_CONNECTIONS_DEFAULT  2048   /* 默认值 */
#endif

#ifndef UVHTTP_MAX_CONNECTIONS_MAX
#define UVHTTP_MAX_CONNECTIONS_MAX      10000  /* 最大推荐值 */
#endif

/**
 * TCP backlog 配置
 * 
 * 配置建议：
 * - 小型应用：1024
 * - 中型应用：4096
 * - 大型应用：8192（默认）
 * 
 * 影响：
 * - 控制待处理连接队列长度
 * - 过小可能导致连接被拒绝
 * - 过大占用更多内存
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_BACKLOG=4096 ..
 */
#ifndef UVHTTP_BACKLOG
#define UVHTTP_BACKLOG                    8192
#endif

/**
 * Keep-Alive 配置
 * 
 * 配置建议：
 * - 默认值适合大多数应用
 * - 高并发场景可以增加 UVHTTP_DEFAULT_KEEP_ALIVE_MAX
 */
#define UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT 5   /* 秒 */
#define UVHTTP_DEFAULT_KEEP_ALIVE_MAX    100  /* 每个连接最大请求数 */

/**
 * 连接超时配置
 * 
 * 配置建议：
 * - 快速响应应用：30秒
 * - 普通应用：60秒（默认）
 * - 长时间处理应用：120-300秒
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_CONNECTION_TIMEOUT_DEFAULT=120 ..
 */
#ifndef UVHTTP_CONNECTION_TIMEOUT_DEFAULT
#define UVHTTP_CONNECTION_TIMEOUT_DEFAULT 60   /* 默认连接超时时间（秒） */
#endif

#ifndef UVHTTP_CONNECTION_TIMEOUT_MIN
#define UVHTTP_CONNECTION_TIMEOUT_MIN     5    /* 最小连接超时时间（秒） */
#endif

#ifndef UVHTTP_CONNECTION_TIMEOUT_MAX
#define UVHTTP_CONNECTION_TIMEOUT_MAX     300  /* 最大连接超时时间（秒） */
#endif

/* ========== 静态文件服务配置 ========== */

/**
 * 文件 I/O 缓冲区大小
 * 
 * 配置建议：
 * - 小文件为主：32KB
 * - 大文件为主：64KB（默认）
 * - 超大文件：128KB
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_ASYNC_FILE_BUFFER_SIZE=131072 ..
 */
#ifndef UVHTTP_ASYNC_FILE_BUFFER_SIZE
#define UVHTTP_ASYNC_FILE_BUFFER_SIZE     65536  /* 64KB */
#endif

/**
 * 最大并发文件读取数
 * 
 * 配置建议：
 * - 小型应用：16
 * - 中型应用：32-64（默认）
 * - 大型应用：128-256
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_ASYNC_FILE_MAX_CONCURRENT=128 ..
 */
#ifndef UVHTTP_ASYNC_FILE_MAX_CONCURRENT
#define UVHTTP_ASYNC_FILE_MAX_CONCURRENT  64
#endif

/**
 * 最大文件大小
 * 
 * 配置建议：
 * - 小型应用：1MB
 * - 中型应用：10MB（默认）
 * - 大型应用：100MB-1GB
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_ASYNC_FILE_MAX_SIZE=104857600 ..
 */
#ifndef UVHTTP_ASYNC_FILE_MAX_SIZE
#define UVHTTP_ASYNC_FILE_MAX_SIZE        (10 * 1024 * 1024)  /* 10MB */
#endif

/**
 * 静态文件缓存配置
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_STATIC_MAX_CACHE_SIZE=2097152 ..
 */
#ifndef UVHTTP_STATIC_MAX_CACHE_SIZE
#define UVHTTP_STATIC_MAX_CACHE_SIZE      (1024 * 1024)  /* 1MB */
#endif

#ifndef UVHTTP_STATIC_MAX_PATH_SIZE
#define UVHTTP_STATIC_MAX_PATH_SIZE       1024
#endif

#ifndef UVHTTP_STATIC_MAX_CONTENT_LENGTH
#define UVHTTP_STATIC_MAX_CONTENT_LENGTH  32
#endif

#ifndef UVHTTP_STATIC_MAX_FILE_SIZE
#define UVHTTP_STATIC_MAX_FILE_SIZE       (10 * 1024 * 1024)  /* 10MB */
#endif

/**
 * 静态文件大小阈值
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_STATIC_SMALL_FILE_THRESHOLD=8192 ..
 */
#ifndef UVHTTP_STATIC_SMALL_FILE_THRESHOLD
#define UVHTTP_STATIC_SMALL_FILE_THRESHOLD   4096  /* 4KB */
#endif

/* ========== WebSocket 配置 ========== */

/**
 * WebSocket 版本和协议
 */
#define UVHTTP_WEBSOCKET_VERSION          13
#define UVHTTP_WEBSOCKET_MAGIC_KEY        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define UVHTTP_WEBSOCKET_MAGIC_KEY_LENGTH 36
#define UVHTTP_WEBSOCKET_ACCEPT_KEY_SIZE   40

/**
 * WebSocket 默认配置
 * 
 * 配置建议：
 * - 默认值适合大多数应用
 * - 大消息应用可以增加 UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
 */
#define UVHTTP_WEBSOCKET_MAX_FRAME_SIZE    4096
#define UVHTTP_WEBSOCKET_FRAME_HEADER_SIZE 10
#define UVHTTP_WEBSOCKET_CLOSE_CODE_SIZE   4
#define UVHTTP_WEBSOCKET_MIN_BUFFER_SIZE   1024
#define UVHTTP_WEBSOCKET_MIN_BUFFER_EXPANSION_SIZE 1024

/**
 * WebSocket 默认运行时配置
 * 
 * 配置建议：
 * - 默认值适合大多数应用
 * - 大消息应用可以增加这些值
 * 
 * CMake 配置：
 * - 通过 CMakeLists.txt 或命令行参数配置
 * - 示例：cmake -DUVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE=33554432 ..
 */
#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
#define UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE      (16 * 1024 * 1024)  /* 16MB */
#endif

#ifndef UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE
#define UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE    (64 * 1024 * 1024)  /* 64MB */
#endif

#ifndef UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE
#define UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE    (64 * 1024)         /* 64KB */
#endif

#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL
#define UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL       30                 /* 秒 */
#endif

#ifndef UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT
#define UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT        10                 /* 秒 */
#endif

#ifndef UVHTTP_WEBSOCKET_MIN_FRAME_HEADER_SIZE
#define UVHTTP_WEBSOCKET_MIN_FRAME_HEADER_SIZE       2
#endif

#ifndef UVHTTP_WEBSOCKET_EXTENDED_FRAME_HEADER_SIZE
#define UVHTTP_WEBSOCKET_EXTENDED_FRAME_HEADER_SIZE  10
#endif

/**
 * WebSocket 操作码
 */
#define UVHTTP_WEBSOCKET_OPCODE_TEXT       0x1
#define UVHTTP_WEBSOCKET_OPCODE_BINARY     0x2
#define UVHTTP_WEBSOCKET_OPCODE_CLOSE      0x8
#define UVHTTP_WEBSOCKET_FIN              0x80
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_126   126
#define UVHTTP_WEBSOCKET_PAYLOAD_LEN_65536 65536

/**
 * WebSocket 关闭码范围
 */
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MIN  1000
#define UVHTTP_WEBSOCKET_CLOSE_CODE_MAX  4999
#define UVHTTP_WEBSOCKET_MAX_REASON_LENGTH 123

/**
 * WebSocket 密钥长度限制
 */
#define UVHTTP_WEBSOCKET_MIN_KEY_LENGTH  16
#define UVHTTP_WEBSOCKET_MAX_KEY_LENGTH  64
#define UVHTTP_WEBSOCKET_COMBINED_MAX_LENGTH 128
#define UVHTTP_WEBSOCKET_SHA1_HASH_SIZE  20

/**
 * WebSocket 位掩码
 */
#define UVHTTP_WEBSOCKET_FIN_MASK         0x80
#define UVHTTP_WEBSOCKET_OPCODE_MASK      0x0F
#define UVHTTP_WEBSOCKET_PAYLOAD_MASK     0x7F

/* ========== 路由缓存配置（性能优化） ========== */

/**
 * 路由缓存开关
 * 
 * 配置建议：
 * - 路由数量少（< 10）：可以禁用以节省内存
 * - 路由数量多（> 10）：启用以提升性能（默认）
 * 
 * 高级选项：
 * - STATS: 启用统计功能（调试用）
 * - DYNAMIC: 启用动态调整（自适应优化）
 * - MONITORING: 启用性能监控（生产环境慎用）
 */
#ifndef UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION
#define UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION 1  /* 默认启用 */
#endif

#ifndef UVHTTP_ENABLE_ROUTER_CACHE_STATS
#define UVHTTP_ENABLE_ROUTER_CACHE_STATS 0  /* 默认禁用 */
#endif

#ifndef UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC
#define UVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC 0  /* 默认禁用 */
#endif

#ifndef UVHTTP_ENABLE_ROUTER_CACHE_MONITORING
#define UVHTTP_ENABLE_ROUTER_CACHE_MONITORING 0  /* 默认禁用 */
#endif

/**
 * 路由查找模式
 * 
 * 配置建议：
 * - 0: 纯线性查找（路由少时最快）
 * - 1: 纯哈希查找（路由多时最快）
 * - 2: 混合策略（自适应，默认）
 */
#ifndef UVHTTP_ROUTER_SEARCH_MODE
#define UVHTTP_ROUTER_SEARCH_MODE 2
#endif

#if UVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION
/* 路由缓存大小配置 */
#define UVHTTP_ROUTER_METHOD_MAP_SIZE    256
#define UVHTTP_ROUTER_HASH_SIZE          256
#define UVHTTP_ROUTER_HOT_PATH_SIZE      64
#define UVHTTP_ROUTER_HOT_ROUTES_COUNT   16
#define UVHTTP_ROUTER_ACCESS_COUNT_SIZE  1024
#define UVHTTP_ROUTER_HYBRID_THRESHOLD   100
#define UVHTTP_ROUTER_MAX_CHILD_COUNT    16
#define UVHTTP_ROUTER_INITIAL_POOL_SIZE  64
#endif

#define UVHTTP_ROUTER_MAX_CHILDREN       16

/* ========== TLS/SSL 配置 ========== */

/**
 * TLS 验证和会话配置
 */
#define UVHTTP_TLS_VERIFY_DEPTH           1
#define UVHTTP_TLS_DH_MIN_BITLEN          2048
#define UVHTTP_TLS_MAX_SESSIONS           1024

/**
 * TLS 缓冲区大小
 */
#define UVHTTP_TLS_ERROR_BUFFER_SIZE      256
#define UVHTTP_TLS_CERT_BUFFER_SIZE       256
#define UVHTTP_TLS_CN_BUFFER_SIZE         256
#define UVHTTP_TLS_SAN_BUFFER_SIZE       256
#define UVHTTP_TLS_PATH_MAX_SIZE          256

/* ========== 中间件配置 ========== */

/**
 * CORS 配置
 */
#define UVHTTP_CORS_MAX_AGE_DEFAULT       "86400"

/**
 * 限流配置
 */
#define UVHTTP_RATE_LIMIT_WINDOW          60   /* 秒 */
#define UVHTTP_RATE_LIMIT_MAX_AGE        17

/* ========== 错误处理配置 ========== */

/**
 * 错误消息缓冲区大小
 */
#define UVHTTP_ERROR_MESSAGE_LENGTH      256
#define UVHTTP_ERROR_CONTEXT_BUFFER_SIZE 256
#define UVHTTP_ERROR_MESSAGE_BUFFER_SIZE 512
#define UVHTTP_ERROR_LOG_BUFFER_SIZE     1024

/* ========== 文件路径配置 ========== */

/**
 * 文件路径长度限制
 */
#define UVHTTP_MAX_FILE_PATH_SIZE        2048
#define UVHTTP_DECODED_PATH_SIZE        1024

/* ========== 网络相关常量 ========== */

/**
 * IP 地址和端口配置
 */
#define UVHTTP_IPV6_MAX_STRING_LENGTH    46
#define UVHTTP_IPV4_MAX_STRING_LENGTH    16
#define UVHTTP_MAX_PORT_NUMBER           65535
#define UVHTTP_MIN_PORT_NUMBER           1

/**
 * 默认网络配置
 */
#define UVHTTP_DEFAULT_HOST "0.0.0.0"
#define UVHTTP_DEFAULT_PORT 8080

/* ========== 时间相关常量 ========== */

/**
 * 时间单位转换
 */
#define UVHTTP_SECONDS_IN_DAY            86400
#define UVHTTP_MILLISECONDS_PER_SECOND   1000
#define UVHTTP_NANOSECONDS_PER_MILLISECOND 1000000

/**
 * 缓存时间配置
 */
#define UVHTTP_CACHE_MAX_AGE             3600  /* 秒 */

/**
 * 错误恢复配置
 */
#ifndef UVHTTP_DEFAULT_BASE_DELAY_MS
#define UVHTTP_DEFAULT_BASE_DELAY_MS     100
#endif

#ifndef UVHTTP_DEFAULT_MAX_DELAY_MS
#define UVHTTP_DEFAULT_MAX_DELAY_MS      5000
#endif

/* ========== 网络和连接配置 ========== */



/**

 * TCP 配置

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_TCP_KEEPALIVE_TIMEOUT=120 ..

 */

#ifndef UVHTTP_TCP_KEEPALIVE_TIMEOUT

#define UVHTTP_TCP_KEEPALIVE_TIMEOUT     60    /* 秒 */

#endif



/**

 * 客户端 IP 缓冲区大小

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_CLIENT_IP_BUFFER_SIZE=128 ..

 */

#ifndef UVHTTP_CLIENT_IP_BUFFER_SIZE

#define UVHTTP_CLIENT_IP_BUFFER_SIZE     64

#endif



/* ========== Sendfile 配置 ========== */



/**

 * Sendfile 配置

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_SENDFILE_TIMEOUT_MS=60000 ..

 */

#ifndef UVHTTP_SENDFILE_TIMEOUT_MS

#define UVHTTP_SENDFILE_TIMEOUT_MS       30000 /* 30 秒 */

#endif



#ifndef UVHTTP_SENDFILE_CHUNK_SIZE

#define UVHTTP_SENDFILE_CHUNK_SIZE       (64 * 1024)  /* 64KB */

#endif



/* ========== LRU 缓存配置 ========== */



/**

 * LRU 缓存配置

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE=20 ..

 */

#ifndef UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE

#define UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE 10

#endif



/* ========== 验证配置 ========== */



/**

 * IP 地址验证配置

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_IP_OCTET_MAX_VALUE=255 ..

 */

#ifndef UVHTTP_IP_OCTET_MAX_VALUE

#define UVHTTP_IP_OCTET_MAX_VALUE        255

#endif



/* ========== 限流配置 ========== */



/**

 * 限流配置

 * 

 * CMake 配置：

 * - 通过 CMakeLists.txt 或命令行参数配置

 * - 示例：cmake -DUVHTTP_RATE_LIMIT_MAX_REQUESTS=500000 ..

 */

#ifndef UVHTTP_RATE_LIMIT_MAX_REQUESTS

#define UVHTTP_RATE_LIMIT_MAX_REQUESTS   1000000

#endif



#ifndef UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS

#define UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS 86400

#endif



#ifndef UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS

#define UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS 10

#endif

/* ========== 错误码范围 ========== */

/**
 * 服务器错误码范围
 */
#define UVHTTP_ERROR_SERVER_MIN          -106
#define UVHTTP_ERROR_SERVER_MAX          -100

/**
 * 连接错误码范围
 */
#define UVHTTP_ERROR_CONNECTION_MIN      -207
#define UVHTTP_ERROR_CONNECTION_MAX      -200

/**
 * 请求错误码范围
 */
#define UVHTTP_ERROR_REQUEST_MIN         -307
#define UVHTTP_ERROR_REQUEST_MAX         -300

/**
 * TLS 错误码范围
 */
#define UVHTTP_ERROR_TLS_MIN             -407
#define UVHTTP_ERROR_TLS_MAX             -400

/**
 * 路由错误码范围
 */
#define UVHTTP_ERROR_ROUTER_MIN          -504
#define UVHTTP_ERROR_ROUTER_MAX          -500

/**
 * 分配器错误码范围
 */
#define UVHTTP_ERROR_ALLOCATOR_MIN       -602
#define UVHTTP_ERROR_ALLOCATOR_MAX       -600

/**
 * WebSocket 错误码范围
 */
#define UVHTTP_ERROR_WEBSOCKET_MIN       -707
#define UVHTTP_ERROR_WEBSOCKET_MAX       -700

/**
 * 配置错误码范围
 */
#define UVHTTP_ERROR_CONFIG_MIN          -903
#define UVHTTP_ERROR_CONFIG_MAX          -900

/**
 * 日志错误码范围
 */
#define UVHTTP_ERROR_LOG_MIN             -1103
#define UVHTTP_ERROR_LOG_MAX             -1100

/* ========== 字符串处理常量 ========== */

/**
 * 控制字符
 */
#define UVHTTP_NULL_BYTE                 '\0'
#define UVHTTP_CARRIAGE_RETURN           '\r'
#define UVHTTP_LINE_FEED                 '\n'
#define UVHTTP_TAB                       '\t'
#define UVHTTP_BACKSLASH                 '\\'
#define UVHTTP_QUOTE                     '"'
#define UVHTTP_ESCAPE_SEQUENCE_LENGTH    6
#define UVHTTP_TAB_CHARACTER            9
#define UVHTTP_SPACE_CHARACTER          32
#define UVHTTP_DELETE_CHARACTER         127

/* ========== 响应头和消息 ========== */

/**
 * 常用响应头名称
 */
#define UVHTTP_HEADER_CONTENT_TYPE        "Content-Type"
#define UVHTTP_HEADER_CONTENT_LENGTH      "Content-Length"
#define UVHTTP_HEADER_CACHE_CONTROL      "Cache-Control"
#define UVHTTP_HEADER_CONNECTION         "Connection"
#define UVHTTP_HEADER_UPGRADE           "Upgrade"
#define UVHTTP_HEADER_WEBSOCKET_KEY      "Sec-WebSocket-Key"
#define UVHTTP_HEADER_WEBSOCKET_ACCEPT   "Sec-WebSocket-Accept"

/**
 * 错误响应消息
 */
#define UVHTTP_MESSAGE_TOO_MANY_REQUESTS  "Too many requests"
#define UVHTTP_MESSAGE_FORBIDDEN          "Forbidden"
#define UVHTTP_MESSAGE_NOT_FOUND          "File not found"
#define UVHTTP_MESSAGE_FILE_TOO_LARGE     "File too large"
#define UVHTTP_MESSAGE_INTERNAL_ERROR     "Internal server error"
#define UVHTTP_MESSAGE_MEMORY_FAILED      "Memory allocation failed"
#define UVHTTP_MESSAGE_FILE_READ_ERROR    "File read error"
#define UVHTTP_MESSAGE_RESPONSE_ERROR     "Response body error"

/* ========== 服务器清理配置 ========== */

/**
 * 服务器清理配置
 */
#define UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS 10
#define UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN 256
#define UVHTTP_DIR_LISTING_BUFFER_SIZE 4096
#define UVHTTP_DIR_ENTRY_HTML_OVERHEAD 200
#define UVHTTP_503_RESPONSE_CONTENT_LENGTH 19

#endif /* UVHTTP_CONSTANTS_H */
