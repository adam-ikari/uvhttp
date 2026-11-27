#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"
#include "uvhttp_platform.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_response uvhttp_response_t;

#define MAX_RESPONSE_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    uv_write_t write_req;
    size_t length;
    uvhttp_response_t* response;
    char data[1]; /* Buffer, 1bytes */
} uvhttp_write_data_t;

typedef struct {
    char* data;
    size_t length;
    uvhttp_response_t* response;
    uvhttp_connection_t* connection;
    size_t offset;
} uvhttp_tls_write_data_t;

struct uvhttp_response {
    /* ========== Cache1(0-63bytes): hot pathField - frequently accessed
     * ========== */
    /* Response、sendfrequently accessed */
    int status_code;        /* 4 bytes - HTTP status code */
    int headers_sent;       /* 4 bytes - headerusesend */
    int sent;               /* 4 bytes - Responseusesend */
    int finished;           /* 4 bytes - Responseusecompleted */
    int keepalive;          /* 4 bytes - usekeepConnection */
    int compress;           /* 4 bytes - useEnablecompress */
    int cache_ttl;          /* 4 bytes - Cache TTL(seconds) */
    int compress_algorithm; /* 4 bytes - Compression algorithm (0=auto, 1=gzip)
                             */
    int compress_threshold; /* 4 bytes - Compression threshold (bytes) */
    int head_request;       /* 4 bytes - 1 when the request method was HEAD;
                             * headers (incl. Content-Length) are built as
                             * usual but the body bytes must not be sent */
    size_t header_count;    /* 8 bytes - header quantity */
    size_t body_length;     /* 8 bytes - body length */
    uv_tcp_t* client;       /* 8 bytes - TCP Client */
    char* body;             /* 8 bytes - Response */
    /* Cache line 1 total: 64 bytes */

    /* ========== Cache line 2 (64-127 bytes): Pointer and counter fields -
     * Secondary frequent access ========== */
    /* Frequently accessed during response building and cache management */
    time_t cache_expires; /* 8 bytes - Cache expiration time */
    uvhttp_header_t*
        headers_extra;       /* 8 bytes - Extra headers (dynamic expansion) */
    size_t headers_capacity; /* 8 bytes - Total headers capacity */
    void* gzip_cache;        /* 8 bytes - Gzip compression cache
                                (uvhttp_gzip_cache_t*), borrowed from server */
    int _padding2[8];        /* 32 bytes - Padding to 64 bytes */
    /* Cache line 2 total: 64 bytes */

    /* ========== Cache line 3+ (128+ bytes): Headers array ========== */
    /* Placed at the end to avoid affecting cache locality of hot path fields */
    /* Headers - Hybrid allocation: inline + dynamic expansion (optimized for
     * cache locality) */
    uvhttp_header_t
        headers[UVHTTP_INLINE_HEADERS_CAPACITY]; /* Inline, reduce dynamic
                                                    allocation */
};

/* ========== Memory layout verification static assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, client, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, body, UVHTTP_POINTER_ALIGNMENT);

/* Verify size_t alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, header_count,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, body_length, UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, cache_expires,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* Verify large buffers at end of structure */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, headers) >= 64,
                     "headers array should be after first 64 bytes");

/* ============ Core API Functions ============ */
uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client);

/* State mutators and the raw send are library-internal building blocks; use
 * uvhttp_respond / uvhttp_respond_with_headers / uvhttp_response_prepare. */

/* ============ Refactored functions: Separating pure functions and side effects
 * ============ */

/* Pure functions: Build HTTP response data with no side effects, easy to test
 * Caller responsible for freeing returned *out_data memory
 */
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t* response,
                                          char** out_data, size_t* out_length);

/* Side effect functions: Send raw data with network I/O */
uvhttp_error_t uvhttp_response_send_raw(const char* data, size_t length,
                                        void* client,
                                        uvhttp_response_t* response);

/* ============ Compression API ============ */
#if UVHTTP_FEATURE_COMPRESSION
/**
 * @brief 启用响应压缩
 *
 * @param response 响应对象
 * @param enable 是否启用压缩 (0=禁用, 1=启用)
 * @return uvhttp_error_t UVHTTP_OK 成功，错误码失败
 *
 * @note 零开销：未启用时无任何性能损失
 * @note 自动选择最佳压缩算法
 * @note 压缩仅对 >= compress_threshold 的响应生效
 * @note 默认阈值: 1024 字节
 */
uvhttp_error_t uvhttp_response_set_compress(uvhttp_response_t* response,
                                            int enable);

/**
 * @brief 设置压缩算法
 *
 * @param response 响应对象
 * @param algorithm 压缩算法 (0=auto, 1=gzip)
 * @return uvhttp_error_t UVHTTP_OK 成功，错误码失败
 *
 * @note 编译期选择，未启用的算法零成本
 * @note 0=auto (自动选择最佳算法)
 * @note 1=gzip (gzip 压缩)
 */
uvhttp_error_t uvhttp_response_set_compress_algorithm(
    uvhttp_response_t* response, int algorithm);

/**
 * @brief 设置压缩阈值
 *
 * @param response 响应对象
 * @param threshold 压缩阈值（字节），默认 1024
 * @return uvhttp_error_t UVHTTP_OK 成功，错误码失败
 *
 * @note 小于阈值的响应不会被压缩
 * @note 避免小文件压缩反而增大的问题
 * @note 建议值: 512-2048 字节
 */
uvhttp_error_t uvhttp_response_set_compress_threshold(
    uvhttp_response_t* response, size_t threshold);
#else
/* 零开销空实现：编译期优化完全移除压缩相关代码 */
static inline uvhttp_error_t uvhttp_response_set_compress(
    uvhttp_response_t* response, int enable) {
    (void)response;
    (void)enable;
    return UVHTTP_OK;
}

static inline uvhttp_error_t uvhttp_response_set_compress_algorithm(
    uvhttp_response_t* response, int algorithm) {
    (void)response;
    (void)algorithm;
    return UVHTTP_OK;
}

static inline uvhttp_error_t uvhttp_response_set_compress_threshold(
    uvhttp_response_t* response, size_t threshold) {
    (void)response;
    (void)threshold;
    return UVHTTP_OK;
}

#endif

/* ============ Compression Helper Functions ============ */
/* These functions are always available for checking compression eligibility */

/**
 * @brief 根据文件扩展名判断是否应该压缩
 *
 * @param filename 文件名或路径
 * @return int 1=应该压缩, 0=不应该压缩
 *
 * @note 零开销：简单的字符串比较
 * @note 基于常见文件扩展名的最佳实践
 * @note 应用层可覆盖此函数实现自定义逻辑
 *
 * 可压缩的文件扩展名:
 * - 文本文件: .html, .htm, .css, .js, .json, .xml, .txt, .md
 * - 脚本文件: .php, .py, .rb, .pl, .sh, .bat, .cmd
 * - 配置文件: .ini, .cfg, .conf, .yaml, .yml, .toml
 * - 数据文件: .csv, .sql, .log
 *
 * 不压缩的文件扩展名:
 * - 图片: .jpg, .jpeg, .png, .gif, .bmp, .webp, .svg, .ico
 * - 视频: .mp4, .avi, .mkv, .mov, .wmv, .flv, .webm
 * - 音频: .mp3, .wav, .ogg, .flac, .aac, .m4a
 * - 压缩文件: .zip, .rar, .7z, .tar, .gz, .bz2, .xz
 * - 二进制文件: .exe, .dll, .so, .dylib, .bin
 */
int uvhttp_should_compress_by_extension(const char* filename);

/**
 * @brief 根据内容类型判断是否应该压缩
 *
 * @param content_type MIME 类型 (例如: "text/html", "application/json")
 * @return int 1=应该压缩, 0=不应该压缩
 *
 * @note 零开销：简单的字符串比较
 * @note 基于 MIME 类型的最佳实践
 *
 * 可压缩的内容类型:
 * - text/ (所有文本类型)
 * - application/json
 * - application/xml
 * - application/javascript
 * - application/xhtml+xml
 *
 * 不压缩的内容类型:
 * - image/ (所有图片)
 * - video/ (所有视频)
 * - audio/ (所有音频)
 * - application/zip
 * - application/gzip
 * - application/x-gzip
 * - application/x-compressed
 * - application/pdf
 * - application/vnd.*
 *
 * 未知类型，不压缩（保守策略）
 */
int uvhttp_should_compress_by_content_type(const char* content_type);

/* Compression policy: the pure predicates uvhttp_should_compress_by_*
 * are the single decision point. To apply one, call uvhttp_response_set_compress
 * with the predicate result. The former set_compress_by_filename /
 * set_compress_by_content_type convenience wrappers were pure overlays on
 * that same decision and have been removed (strategy B: no duplicate entry
 * points). */

/**
 * @brief Atomic "set status + content type + body + send"
 *
 * Validates all inputs before touching @p response. On a validation or setter
 * failure the status code and the visible header count are restored, so the
 * response looks exactly as it did before the call. A body length of 0 (with
 * @p body NULL) is allowed: Content-Length: 0 is emitted and no body is sent —
 * use it for 204/304. Once every setter succeeded the response is sent; a send
 * error is returned as-is and is not retried (send is idempotent).
 *
 * @param response Response to fill and send (must be non-NULL, unsent)
 * @param status_code HTTP status code
 * @param content_type Value for Content-Type, or NULL to keep the default
 * @param body Body bytes, or NULL when @p body_length is 0
 * @param body_length Number of body bytes
 * @return UVHTTP_OK, or the failing setter's / send's error code
 */
uvhttp_error_t uvhttp_respond(uvhttp_response_t* response, int status_code,
                              const char* content_type, const char* body,
                              size_t body_length);

/** @brief One header for uvhttp_respond_with_headers (borrowed for the call). */
typedef struct uvhttp_header_view {
    const char* name;
    const char* value;
} uvhttp_header_view_t;

/**
 * @brief Atomic status + N headers + body + send
 *
 * The multi-header form of uvhttp_respond. All inputs are validated before
 * the response is touched; a setter failure restores status and the visible
 * header count. An empty headers array (or NULL with count 0) is allowed.
 */
/**
 * @brief Atomic status + headers + body WITHOUT sending
 *
 * The incremental counterpart of uvhttp_respond: lets a caller build a
 * complete response in one validated call without transmitting it, so a
 * middleware or embedding layer can inspect the result before the library
 * sends it. Same validation and rollback guarantees as uvhttp_respond.
 */
uvhttp_error_t uvhttp_response_prepare(
    uvhttp_response_t* response, int status_code,
    const uvhttp_header_view_t* headers, size_t header_count, const char* body,
    size_t body_length);

uvhttp_error_t uvhttp_respond_with_headers(
    uvhttp_response_t* response, int status_code,
    const uvhttp_header_view_t* headers, size_t header_count, const char* body,
    size_t body_length);

/* ============ Legacy functions ============ */
void uvhttp_response_cleanup(uvhttp_response_t* response);
void uvhttp_response_free(uvhttp_response_t* response);

/* ========== Headers  API ========== */

/* get header quantity */
size_t uvhttp_response_get_header_count(uvhttp_response_t* response);

/* getspecifiedindex header(internalUse) */
uvhttp_header_t* uvhttp_response_get_header_at(uvhttp_response_t* response,
                                               size_t index);

/* traverseof headers */
typedef void (*uvhttp_header_callback_t)(const char* name, const char* value,
                                         void* user_data);
void uvhttp_response_foreach_header(uvhttp_response_t* response,
                                    uvhttp_header_callback_t callback,
                                    void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_RESPONSE_H */