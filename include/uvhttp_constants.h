/* UVHTTP 常量定义 */

#ifndef UVHTTP_CONSTANTS_H
#define UVHTTP_CONSTANTS_H

/* HTTP 状态码 */
#define UVHTTP_STATUS_CONTINUE           100
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

/* HTTP 版本 */
#define UVHTTP_VERSION_1_1 "HTTP/1.1"
#define UVHTTP_VERSION_LENGTH 8

/* 缓冲区大小 */
#define UVHTTP_INITIAL_BUFFER_SIZE       1024
#define UVHTTP_MAX_BODY_SIZE              (1024 * 1024)  /* 1MB */
#define UVHTTP_MAX_HEADERS                64
#define UVHTTP_MAX_HEADER_NAME_SIZE       256
#define UVHTTP_MAX_HEADER_VALUE_SIZE     1024
#define UVHTTP_MAX_HEADER_NAME_LENGTH     256
#define UVHTTP_MAX_HEADER_VALUE_LENGTH    1024
#define UVHTTP_MAX_URL_SIZE              2048
#define UVHTTP_MAX_METHOD_SIZE           16
#define UVHTTP_MAX_PATH_SIZE             1024

/* WebSocket 常量 */
#define UVHTTP_WEBSOCKET_VERSION          13
#define UVHTTP_WEBSOCKET_MAGIC_KEY        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define UVHTTP_WEBSOCKET_MAGIC_KEY_LENGTH 36
#define UVHTTP_WEBSOCKET_ACCEPT_KEY_SIZE   29
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
#define UVHTTP_MAX_CONNECTIONS           512   /* 从128增加到512，经过负载测试验证 */
#define UVHTTP_READ_BUFFER_SIZE          8192  /* 8KB缓冲区，优化内存使用 */
#define UVHTTP_BACKLOG                    256   /* 增加backlog以处理突发连接 */



/* TLS 相关 */
#define UVHTTP_TLS_VERIFY_DEPTH           1
#define UVHTTP_TLS_DH_MIN_BITLEN          2048
#define UVHTTP_TLS_MAX_SESSIONS           1024
#define UVHTTP_TLS_ERROR_BUFFER_SIZE      256
#define UVHTTP_TLS_CERT_BUFFER_SIZE       256
#define UVHTTP_TLS_CN_BUFFER_SIZE         256
#define UVHTTP_TLS_SAN_BUFFER_SIZE       256
#define UVHTTP_TLS_PATH_MAX_SIZE          256

/* JSON 相关 */
#define UVHTTP_JSON_ESCAPE_BUFFER_SIZE    512
#define UVHTTP_JSON_VALUE_BUFFER_SIZE     1024
#define UVHTTP_JSON_BUFFER_SIZE          2048
#define UVHTTP_JSON_ERROR_BUFFER_SIZE     1280
#define UVHTTP_JSON_MAX_ESCAPE_LENGTH     6

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

/* UTF-8 编码 */
#define UVHTTP_UTF8_2BYTE_MASK           0xE0
#define UVHTTP_UTF8_3BYTE_MASK           0xF0
#define UVHTTP_UTF8_4BYTE_MASK           0xF8

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
#define UVHTTP_CONTENT_TYPE_JSON         "application/json"
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

/* 解析器相关 */
#define UVHTTP_PARSER_FIELD_COUNT        8
#define UVHTTP_PARSER_INTERNAL_SIZE      100

#endif /* UVHTTP_CONSTANTS_H */