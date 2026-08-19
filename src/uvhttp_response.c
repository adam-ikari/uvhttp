#include "uvhttp_response.h"

#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_features.h"
#include "uvhttp_gzip_cache.h"
#include "uvhttp_hash.h"
#include "uvhttp_logging.h"
#include "uvhttp_validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>

#if UVHTTP_FEATURE_COMPRESSION
#include <zlib.h>
#endif

/* ========== Compression Implementation ========== */

#if UVHTTP_FEATURE_COMPRESSION

/**
 * @brief Compress data using gzip
 * 
 * @param input Input data
 * @param input_len Input data length
 * @param output Output buffer (caller must free)
 * @param output_len Output data length
 * @return uvhttp_error_t UVHTTP_OK 成功，错误码失败
 * 
 * @note Uses zlib-style deflate (from qwrt's vendored miniz) at default level 6
 * @note Emits a real gzip stream (RFC 1952: gzip header + raw deflate + CRC32
 *   + ISIZE trailer), NOT zlib-wrapped deflate — the two formats are distinct
 *   and standard gzip decoders reject zlib streams (magic 0x78 vs 0x1f8b).
 * @note Caller is responsible for freeing output buffer
 */
static uvhttp_error_t uvhttp_compress_gzip(const char* input, size_t input_len,
                                           char** output, size_t* output_len) {
    if (!input || !output || !output_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (input_len == 0) {
        *output = NULL;
        *output_len = 0;
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* deflate raw (no zlib header) + gzip header (10) + CRC32 + ISIZE (8) */
    const size_t gzip_header_len = 10;
    const size_t trailer_len = 8;
    uLongf raw_size = compressBound(input_len);
    char* buf = uvhttp_alloc(gzip_header_len + raw_size + trailer_len);
    if (!buf) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* gzip header (RFC 1952): magic, method=deflate, flags=0, mtime=0,
     * xfl=0, OS=0xff (unknown; avoids OS-specific byte) */
    buf[0] = 0x1f;
    buf[1] = 0x8b;
    buf[2] = 0x08;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = 0xff;

    /* raw deflate stream */
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     -Z_DEFAULT_WINDOW_BITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        uvhttp_free(buf);
        return UVHTTP_ERROR_IO_ERROR;
    }
    zs.next_in = (Bytef*)input;
    zs.avail_in = (uInt)(input_len > 0xFFFFFFFFUL ? 0xFFFFFFFFUL : input_len);
    zs.next_out = (Bytef*)(buf + gzip_header_len);
    zs.avail_out = (uInt)raw_size;
    int result = deflate(&zs, Z_FINISH);
    if (result != Z_STREAM_END) {
        deflateEnd(&zs);
        uvhttp_free(buf);
        UVHTTP_LOG_ERROR("gzip deflate failed: %d\n", result);
        return UVHTTP_ERROR_IO_ERROR;
    }
    size_t deflate_len = zs.total_out;
    deflateEnd(&zs);

    /* CRC32 of the ORIGINAL input + ISIZE (input_len mod 2^32), little-endian */
    unsigned long crc = crc32(0L, (const Bytef*)input, input_len);
    char* tp = buf + gzip_header_len + deflate_len;
    tp[0] = (char)(crc & 0xff);
    tp[1] = (char)((crc >> 8) & 0xff);
    tp[2] = (char)((crc >> 16) & 0xff);
    tp[3] = (char)((crc >> 24) & 0xff);
    unsigned long isize = (unsigned long)(input_len & 0xffffffffUL);
    tp[4] = (char)(isize & 0xff);
    tp[5] = (char)((isize >> 8) & 0xff);
    tp[6] = (char)((isize >> 16) & 0xff);
    tp[7] = (char)((isize >> 24) & 0xff);

    size_t total = gzip_header_len + deflate_len + trailer_len;
    *output = buf;
    *output_len = total;
    return UVHTTP_OK;
}

#endif /* UVHTTP_FEATURE_COMPRESSION */

/* HTTP response header string constants */
#define HTTP_HEADER_CONNECTION_KEEPALIVE "Connection: keep-alive\r\n"
#define HTTP_HEADER_CONNECTION_CLOSE "Connection: close\r\n"

/* Function declaration */
static void uvhttp_free_write_data(uv_write_t* req, int status);

static const char* get_status_text(int status_code) {
    switch (status_code) {
    case UVHTTP_STATUS_CONTINUE:
        return "Continue";
    case UVHTTP_STATUS_SWITCHING_PROTOCOLS:
        return "Switching Protocols";
    case UVHTTP_STATUS_OK:
        return "OK";
    case UVHTTP_STATUS_CREATED:
        return "Created";
    case UVHTTP_STATUS_NO_CONTENT:
        return "No Content";
    case UVHTTP_STATUS_BAD_REQUEST:
        return "Bad Request";
    case UVHTTP_STATUS_UNAUTHORIZED:
        return "Unauthorized";
    case UVHTTP_STATUS_FORBIDDEN:
        return "Forbidden";
    case UVHTTP_STATUS_NOT_FOUND:
        return "Not Found";
    case UVHTTP_STATUS_METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case UVHTTP_STATUS_INTERNAL_ERROR:
        return "Internal Server Error";
    case UVHTTP_STATUS_NOT_IMPLEMENTED:
        return "Not Implemented";
    case UVHTTP_STATUS_BAD_GATEWAY:
        return "Bad Gateway";
    case UVHTTP_STATUS_SERVICE_UNAVAILABLE:
        return "Service Unavailable";
    default:
        return "Unknown";
    }
}

// auxiliary function: check if string contains control characters (including
// newline)
static int contains_control_chars(const char* str) {
    if (!str)
        return 0;

    for (const char* p = str; *p; p++) {
        unsigned char c = (unsigned char)*p;
        // check if contains control characters (0-31) but exclude tab (9) and
        // space (32)
        if (c < UVHTTP_SPACE_CHARACTER && c != UVHTTP_TAB_CHARACTER) {
            return 1;  // containcontrolcharacter
        }
        // clear check carriage return and newline, prevent HTTP response
        // splitting attack
        if (c == UVHTTP_CARRIAGE_RETURN || c == UVHTTP_LINE_FEED) {
            return 1;  // HTTP response splitting attempt
        }
        // checkdeletecharacter
        if (c == UVHTTP_DELETE_CHARACTER) {
            return 1;  // containdeletecharacter
        }
    }
    return 0;
}

static void build_response_headers(uvhttp_response_t* response, char* buffer,
                                   size_t* length) {
    size_t pos = 0;

    /* snprintf returns the number of bytes that *would* be written (even when
     * truncated), so accumulating its return value into pos lets pos exceed the
     * buffer capacity. Once that happens, (*length - pos) underflows (size_t is
     * unsigned) and the next snprintf writes past the buffer. CLAMP the size
     * argument to 0 once pos reaches the end: snprintf with size 0 writes
     * nothing but still returns the would-be length, so pos keeps tracking the
     * total bytes needed (reported back via *length for the caller's realloc). */
#define UVHTTP_SNAPPEND(fmt, ...)                                             \
    do {                                                                      \
        size_t _rem = (pos < *length) ? (*length - pos) : 0;                  \
        pos += snprintf(buffer + pos, _rem, (fmt), ##__VA_ARGS__);            \
    } while (0)

    // status line
    UVHTTP_SNAPPEND(UVHTTP_VERSION_1_1 " %d %s\r\n", response->status_code,
                    get_status_text(response->status_code));

    // defaultheaderscheck
    int has_content_type = 0;
    int has_content_length = 0;
    int has_connection = 0;

    // traverseexistingheaders
    for (size_t i = 0; i < response->header_count; i++) {
        uvhttp_header_t* header = uvhttp_response_get_header_at(response, i);
        if (!header) {
            continue;
        }

        // safe check: verify header value does not contain control characters,
        // prevent response splitting
        if (contains_control_chars(header->value)) {
            // if header value contains control characters, skip this header
            UVHTTP_LOG_ERROR("Invalid header value detected: header '%s' "
                             "contains control characters\n",
                             header->name);
            continue;
        }

        UVHTTP_SNAPPEND("%s: %s\r\n", header->name, header->value);
        if (strcasecmp(header->name, "Content-Type") == 0) {
            has_content_type = 1;
        }
        if (strcasecmp(header->name, "Content-Length") == 0) {
            has_content_length = 1;
        }
        if (strcasecmp(header->name, "Connection") == 0) {
            has_connection = 1;
        }
    }

    // adddefaultContent-Type
    if (!has_content_type) {
        UVHTTP_SNAPPEND("Content-Type: text/plain\r\n");
    }

    // HTTP/1.1 requirement: must have Content-Length or use chunked encoding
    // here we always add Content-Length to ensure protocol compliance
    if (!has_content_length) {
        if (response->body && response->body_length > 0) {
            UVHTTP_SNAPPEND("Content-Length: %zu\r\n", response->body_length);
        } else {
            // even if there is no body, still set Content-Length: 0
            UVHTTP_SNAPPEND("Content-Length: 0\r\n");
        }
    }

    // HTTP/1.1 optimization: set Connection header based on keep-alive
    if (!has_connection) {
        if (response->keepalive) {
            UVHTTP_SNAPPEND(HTTP_HEADER_CONNECTION_KEEPALIVE);
            UVHTTP_SNAPPEND("Keep-Alive: timeout=%d, max=%d\r\n",
                            UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT,
                            UVHTTP_DEFAULT_KEEP_ALIVE_MAX);
        } else {
            UVHTTP_SNAPPEND(HTTP_HEADER_CONNECTION_CLOSE);
        }
    }

    // endheaders
    UVHTTP_SNAPPEND("\r\n");

    *length = pos;

#undef UVHTTP_SNAPPEND
}

uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(response, 0, sizeof(uvhttp_response_t));

    // HTTP/1.1optimize: setdefaultvalue
    response->keepalive = 1;  // HTTP/1.1defaultkeepconnection
    response->status_code = UVHTTP_STATUS_OK;
    response->sent = 0;      // not sent
    response->finished = 0;  // not complete
    response->headers_capacity =
        UVHTTP_INLINE_HEADERS_CAPACITY; /* initial capacity: 32 inline headers
                                         */

    response->client = client;

    return UVHTTP_OK;
}

void uvhttp_response_cleanup(uvhttp_response_t* response) {
    if (!response) {
        return;
    }

    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }

    if (response->headers_extra) {
        uvhttp_free(response->headers_extra);
        response->headers_extra = NULL;
    }

    response->body_length = 0;
}

uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // verify status code range
    if (status_code < UVHTTP_STATUS_MIN_CONTINUE ||
        status_code > UVHTTP_STATUS_MAX) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    response->status_code = status_code;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                          const char* name, const char* value) {
    if (!response || !name || !value) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // verify header name and value
    if (uvhttp_validate_header_name(name) == 0 ||
        uvhttp_validate_header_value_safe(value) == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // additional verification: check if header value contains control
    // characters, prevent response splitting
    if (contains_control_chars(value)) {
        UVHTTP_LOG_ERROR(
            "Invalid header value '%s': contains control characters\n", value);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check if need to expand capacity */
    if (response->header_count >= response->headers_capacity) {
        /* calculate new capacity (maximum UVHTTP_MAX_HEADERS) */
        size_t new_capacity = response->headers_capacity * 2;
        if (new_capacity == 0) {
            new_capacity =
                UVHTTP_INLINE_HEADERS_CAPACITY; /* initial capacity */
        }
        if (new_capacity > UVHTTP_MAX_HEADERS) {
            new_capacity = UVHTTP_MAX_HEADERS;
        }

        /* if new capacity equals current capacity, it means maximum value
         * reached */
        if (new_capacity == response->headers_capacity) {
            return UVHTTP_ERROR_OUT_OF_MEMORY; /* full */
        }

        /* allocate or reallocate dynamic array */
        size_t old_extra_count =
            (response->headers_capacity > UVHTTP_INLINE_HEADERS_CAPACITY)
                ? response->headers_capacity - UVHTTP_INLINE_HEADERS_CAPACITY
                : 0;
        size_t new_extra_count = new_capacity - UVHTTP_INLINE_HEADERS_CAPACITY;

        uvhttp_header_t* new_extra;
        if (old_extra_count == 0) {
            /* first allocation, use malloc */
            new_extra = uvhttp_alloc(new_extra_count * sizeof(uvhttp_header_t));
            /* 释放可能残留的旧 headers_extra：当 headers_capacity 恰好等于
             * UVHTTP_INLINE_HEADERS_CAPACITY（例如之前 capacity 为 0 时首次扩展
             * 分配了一个 0 大小的占位块）时，old_extra_count 会被记为 0 走此分支，
             * 若不释放旧指针直接覆盖会造成内存泄漏。*/
            if (response->headers_extra) {
                uvhttp_free(response->headers_extra);
            }
        } else {
            /* reallocate, use realloc */
            new_extra =
                uvhttp_realloc(response->headers_extra,
                               new_extra_count * sizeof(uvhttp_header_t));
        }

        if (!new_extra) {
            return UVHTTP_ERROR_OUT_OF_MEMORY; /* memoryallocatefailure */
        }

        /* if first allocation, zero out newly allocated memory */
        if (old_extra_count == 0) {
            memset(new_extra, 0, new_extra_count * sizeof(uvhttp_header_t));
        }

        response->headers_extra = new_extra;
        response->headers_capacity = new_capacity;
    }

    /* get header pointer */
    uvhttp_header_t* header;
    if (response->header_count < UVHTTP_INLINE_HEADERS_CAPACITY) {
        header = &response->headers[response->header_count];
    } else {
        if (!response->headers_extra) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        header = &response->headers_extra[response->header_count -
                                          UVHTTP_INLINE_HEADERS_CAPACITY];
    }

    // use safe string copy function
    if (uvhttp_safe_strcpy(header->name, UVHTTP_MAX_HEADER_NAME_SIZE, name) !=
        0) {
        UVHTTP_LOG_ERROR("Failed to copy header name: %s\n", name);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (uvhttp_safe_strcpy(header->value, UVHTTP_MAX_HEADER_VALUE_SIZE,
                           value) != 0) {
        UVHTTP_LOG_ERROR("Failed to copy header value: %s\n", value);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    response->header_count++;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body, size_t length) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!body) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // check length limit - simplified version uses 1MB limit
    if (length > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // verifybodycontent - checkinvalidcharacter
    for (size_t i = 0; i < length; i++) {
        // allow all binary data, but record warning
        if (i < length - 1 && body[i] == 0) {
            // NULL byte is valid, no need to process
        }
    }

    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }

    response->body = uvhttp_alloc(length);
    if (!response->body) {
        response->body_length = 0;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memcpy(response->body, body, length);
    response->body_length = length;

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t* response,
                                         const char* data, size_t length) {
    if (!response || !data || length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check integer overflow */

    if (length > SIZE_MAX - sizeof(uvhttp_write_data_t)) {

        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* optimization: merge write_data and data buffer into one allocation,
     * reduce memory fragmentation */

    size_t total_size = sizeof(uvhttp_write_data_t) + length;

    uvhttp_write_data_t* write_data = uvhttp_alloc(total_size);

    if (!write_data) {

        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* use flexible array member, automatically handle memory alignment */

    memcpy(write_data->data, data, length);

    write_data->length = length;

    write_data->response = response;

    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);

    write_data->write_req.data = write_data;

    int result = uv_write(&write_data->write_req,
                          (uv_stream_t*)response->client, &buf, 1,

                          (uv_write_cb)uvhttp_free_write_data);

    if (result < 0) {

        /* fix memory leak: only need to release entire struct, no need to
         * separately release data */

        uvhttp_free(write_data);

        return UVHTTP_ERROR_RESPONSE_SEND;
    }

    return UVHTTP_OK;
}

/* single-thread safe write complete callback
 * executed in libuv event loop thread, safely release write related resources
 * single-thread advantage: no locks needed, resource release order is
 * predictable
 */
static void uvhttp_free_write_data(uv_write_t* req, int status) {
    (void)status;  // avoid unused parameter warning
    uvhttp_write_data_t* write_data = (uvhttp_write_data_t*)req->data;
    if (write_data) {
        /* check if need to close connection or restart read */
        if (write_data->response) {
            uv_tcp_t* client = (uv_tcp_t*)write_data->response->client;
            if (client) {
                uvhttp_connection_t* conn = (uvhttp_connection_t*)client->data;
                if (conn) {
                    if (!write_data->response->keepalive) {
                        /* closeconnection */
                        uvhttp_connection_close(conn);
                    } else if (!conn->is_websocket) {
                        /* keep-alive connection, restart read to receive next
                         * request (skip for websocket: its read callback was
                         * already set up by switch_to_websocket; restarting
                         * HTTP read here would override it) */
                        uvhttp_connection_schedule_restart_read(conn);
                    }
                }
            }
        }

        /* release write_data (data buffer is part of struct, no need to
         * separately release) */
        uvhttp_free(write_data);
    }
}

/* ============ pure function: build response data ============ */
/* pure function: build HTTP response data, no side effects, easy to test
 * response: response object
 * out_data: output parameter, return built response data
 * out_length: outputparameter, returnresponsedatalength
 * return: UVHTTP_OK success, other values indicate error
 *
 * note: caller is responsible for releasing returned *out_data memory
 */
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t* response,
                                          char** out_data, size_t* out_length) {
    if (!response || !out_data || !out_length) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* duplicate send check */
    if (response->sent) {
        *out_data = NULL;
        *out_length = 0;
        return UVHTTP_OK;
    }

    /* ========== Step 1: Compress body first (before building headers) ========== */
    const char* body_to_send = response->body;
    size_t body_length = response->body_length;
    size_t original_body_length = response->body_length;  /* save for restoration */
    char* compressed_body = NULL;  /* track for cleanup */
    
#if UVHTTP_FEATURE_COMPRESSION
    /* 零开销检查：编译期优化会完全移除这个分支 */
    if (response->compress && 
        response->body && 
        response->body_length >= (size_t)response->compress_threshold) {

        /* 先查 gzip LRU 缓存：相同 body 内容只压缩一次 */
        size_t cached_len = 0;
        const char* cached = NULL;
        if (response->gzip_cache) {
            cached = uvhttp_gzip_cache_find(
                (uvhttp_gzip_cache_t*)response->gzip_cache,
                uvhttp_hash_default(response->body, response->body_length),
                response->body_length, &cached_len);
        }

        if (cached) {
            /* 缓存命中：直接用缓存压缩结果（send 收尾只释放 compressed_body，安全） */
            body_to_send = cached;
            body_length = cached_len;
            response->body_length = cached_len;
            uvhttp_response_set_header(response, "Content-Encoding", "gzip");
            UVHTTP_LOG_DEBUG("Response compressed (cache hit): %zu -> %zu bytes\n",
                             original_body_length, cached_len);
        } else {
            /* 尝试压缩响应体 */
            size_t compressed_len = 0;

            uvhttp_error_t compress_result = uvhttp_compress_gzip(
                response->body, 
                response->body_length,
                &compressed_body, 
                &compressed_len
            );

            /* 如果压缩成功且有效（压缩后更小），使用压缩数据 */
            if (compress_result == UVHTTP_OK && 
                compressed_body && 
                compressed_len < response->body_length) {

                body_to_send = compressed_body;
                body_length = compressed_len;

                /* 临时更新 response->body_length，这样 build_response_headers 会使用压缩后的大小 */
                response->body_length = compressed_len;

                /* 添加 Content-Encoding 头 */
                uvhttp_response_set_header(response, "Content-Encoding", "gzip");

                /* 缓存压缩结果（内部拷贝，不受 response 释放影响） */
                if (response->gzip_cache) {
                    uvhttp_gzip_cache_put(
                        (uvhttp_gzip_cache_t*)response->gzip_cache,
                        uvhttp_hash_default(response->body,
                                            original_body_length),
                        original_body_length, compressed_body,
                        compressed_len);
                }

                UVHTTP_LOG_DEBUG("Response compressed: %zu -> %zu bytes (%.1f%% reduction)\n",
                                original_body_length, compressed_len,
                                (1.0 - (double)compressed_len / original_body_length) * 100);
            } else {
                /* 压缩失败或无效，使用原数据 */
                if (compressed_body) {
                    uvhttp_free(compressed_body);
                    compressed_body = NULL;
                }
            }
        }
    }
#endif /* UVHTTP_FEATURE_COMPRESSION */

    /* ========== Step 2: Build headers (after compression, so Content-Length is correct) ========== */
    /* optimization: increase initial buffer size, reduce reallocation */
    size_t headers_size =
        UVHTTP_INITIAL_BUFFER_SIZE * 2; /* increase from 512 to 1024 */
    char* headers_buffer = uvhttp_alloc(headers_size);
    if (!headers_buffer) {
        if (compressed_body) {
            uvhttp_free(compressed_body);
        }
        /* 恢复原始 body_length */
        response->body_length = original_body_length;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    size_t headers_length = headers_size;
    build_response_headers(response, headers_buffer, &headers_length);

    /* 恢复原始 body_length */
    response->body_length = original_body_length;

    /* check if buffer is too small, if so reallocate larger buffer */
    if (headers_length >= headers_size) {
        uvhttp_free(headers_buffer);
        headers_size =
            headers_length +
            UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN; /* add safety margin */
        headers_buffer = uvhttp_alloc(headers_size);
        if (!headers_buffer) {
            if (compressed_body) {
                uvhttp_free(compressed_body);
            }
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        headers_length = headers_size;
        build_response_headers(response, headers_buffer, &headers_length);
    }

    /* ========== Step 3: Calculate total size and allocate response data ========== */
    size_t total_size = headers_length + body_length;
    
    /* allocate complete response data */
    char* response_data =
        uvhttp_alloc(total_size + 1); /* +1 for null terminator */
    if (!response_data) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* copyheaders */
    memcpy(response_data, headers_buffer, headers_length);

    /* copybody */
    if (body_to_send && body_length > 0) {
        memcpy(response_data + headers_length, body_to_send, body_length);
    }
    
    /* 释放临时压缩缓冲区 */
#if UVHTTP_FEATURE_COMPRESSION
    if (compressed_body) {
        uvhttp_free(compressed_body);
    }
#endif
    
    /* ensure null-terminated (although HTTP does not need it, but for safety)
     */
    response_data[total_size] = '\0';

    /* immediately release headers_buffer, no longer needed */
    uvhttp_free(headers_buffer);

    *out_data = response_data;
    *out_length = total_size;

    return UVHTTP_OK;
}

/* ============ side-effect function: send raw data ============ */
/* side-effect function: send raw data, contains network I/O
 * data: data to send
 * length: data length
 * client: client connection
 * response: response object (used for callback processing)
 * return: UVHTTP_OK success, other values indicate error
 */
uvhttp_error_t uvhttp_response_send_raw(const char* data, size_t length,
                                        void* client,
                                        uvhttp_response_t* response) {
    if (!data || length == 0 || !client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check integer overflow */
    if (length > SIZE_MAX - sizeof(uvhttp_write_data_t)) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* create write data structure */
    size_t total_size = sizeof(uvhttp_write_data_t) + length -
                        1; /* -1 because data already has 1 byte */

    uvhttp_write_data_t* write_data = uvhttp_alloc(total_size);
    if (!write_data) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* copy data to data array */
    memcpy(write_data->data, data, length);
    write_data->length = length;
    write_data->response = response;

    /* initializewrite_req */
    memset(&write_data->write_req, 0, sizeof(uv_write_t));
    write_data->write_req.data = write_data;

    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);

    uv_stream_t* stream = (uv_stream_t*)client;

    /* check if stream is valid */
    if (stream->type != UV_TCP) {
        uvhttp_free(write_data);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check stream's loop pointer */
    if (!stream->loop) {
        uvhttp_free(write_data);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* For TLS connections, encrypt data before sending */
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (conn && conn->tls_enabled && conn->ssl) {
        /* Encrypt data using TLS write */
        uvhttp_error_t tls_result =
            uvhttp_connection_tls_write(conn, data, length);
        if (tls_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("TLS write failed: %d\n", tls_result);
            uvhttp_free(write_data);
            return tls_result;
        }
        /* TLS writes are synchronous (mbedtls_ssl_write + uv_try_write), so
         * the uv_write completion callback (uvhttp_free_write_data) that
         * normally schedules restart_read for keep-alive never fires.
         * Schedule it here or the llhttp parser stays at the completed state
         * and the next request on this connection is never parsed. */
        if (response) {
            if (!response->keepalive) {
                uvhttp_connection_close(conn);
            } else if (!conn->is_websocket && response->status_code != 101) {
                /* 101 = WebSocket upgrade: the handshake path calls
                 * uvhttp_connection_switch_to_websocket which starts the WS
                 * read callback; scheduling restart_read here would let the
                 * idle callback restart plain HTTP reads and override it. */
                uvhttp_connection_schedule_restart_read(conn);
            }
        }
        /* TLS write succeeded, data was sent through mbedtls_bio_send callback
         */
        uvhttp_free(write_data);
        return UVHTTP_OK;
    }

    /* For non-TLS connections, send data directly */
    int result = uv_write((uv_write_t*)write_data, stream, &buf, 1,
                          (uv_write_cb)uvhttp_free_write_data);

    if (result < 0) {
        /* write failure, immediately clean resources */
        uvhttp_free(write_data);
        return UVHTTP_ERROR_RESPONSE_SEND;
    }

    /* if response set Connection: close, need to close connection after send
     * complete */
    if (response && !response->keepalive) {

        /* get connection object and close connection */
        uv_tcp_t* client_tcp = (uv_tcp_t*)response->client;
        if (client_tcp) {
            uvhttp_connection_t* conn = (uvhttp_connection_t*)client_tcp->data;
            if (conn) {
                conn->keepalive = 0;
            }
        }
    }

    return UVHTTP_OK;
}

/* ============ responsesendfunction ============ */
/* single-threaded event-driven HTTP response send
 * ensure HTTP response format is correct
 *
 * this function combines data building and actual sending
 */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* debug output: show response send start */

    /* single-thread safe duplicate send check */
    if (response->sent) {
        return UVHTTP_OK;
    }

    /* call pure function build response data */
    char* response_data = NULL;
    size_t response_length = 0;
    uvhttp_error_t err =
        uvhttp_response_build_data(response, &response_data, &response_length);

    if (err != UVHTTP_OK) {
        return err;
    }

    /* mark response as sent */
    response->sent = 1;

    /* call side-effect function send data */
    err = uvhttp_response_send_raw(response_data, response_length,
                                   response->client, response);

    /* release memory allocated by pure function */
    uvhttp_free(response_data);

    if (err == UVHTTP_OK) {
        response->finished = 1;
    } else {
    }

    return err;
}

/* ========== Compression API Implementation ========== */

#if UVHTTP_FEATURE_COMPRESSION

uvhttp_error_t uvhttp_response_set_compress(uvhttp_response_t* response, 
                                            int enable) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->compress = enable ? 1 : 0;
    
    /* 重置压缩相关状态 */
    if (!enable) {
        response->compress_algorithm = 0;
        response->compress_threshold = 0;
    } else {
        /* 设置默认值 */
        if (response->compress_threshold == 0) {
            response->compress_threshold = 1024;  /* 默认 1KB */
        }
    }
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_compress_algorithm(uvhttp_response_t* response,
                                                     int algorithm) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 只有启用压缩时才能设置算法 */
    if (!response->compress) {
        return UVHTTP_ERROR_INVALID_PARAM;  /* Compression not enabled */
    }
    
    /* 验证算法类型 */
    if (algorithm < 0 || algorithm > 1) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->compress_algorithm = algorithm;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_compress_threshold(uvhttp_response_t* response,
                                                       size_t threshold) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 验证阈值范围 */
    if (threshold > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->compress_threshold = threshold;
    return UVHTTP_OK;
}

/* ========== Compression Helper Functions ========== */

/**
 * @brief 可压缩的文件扩展名列表
 */
static const char* const COMPRESSIBLE_EXTENSIONS[] = {
    /* 文本文件 */
    ".html", ".htm", ".css", ".js", ".json", ".xml", ".txt", ".md",
    /* 脚本文件 */
    ".php", ".py", ".rb", ".pl", ".sh", ".bat", ".cmd",
    /* 配置文件 */
    ".ini", ".cfg", ".conf", ".yaml", ".yml", ".toml",
    /* 数据文件 */
    ".csv", ".sql", ".log",
    /* Web 相关 */
    ".svg", ".woff", ".woff2", ".ttf", ".eot",
    NULL
};

/**
 * @brief 不压缩的文件扩展名列表
 */
static const char* const NON_COMPRESSIBLE_EXTENSIONS[] = {
    /* 图片 */
    ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".ico", ".tiff", ".tif",
    /* 视频 */
    ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".3gp",
    /* 音频 */
    ".mp3", ".wav", ".ogg", ".flac", ".aac", ".m4a", ".wma",
    /* 压缩文件 */
    ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2", ".xz", ".lzma", ".z",
    /* 二进制文件 */
    ".exe", ".dll", ".so", ".dylib", ".bin", ".elf", ".o", ".a", ".lib",
    /* 办公文档（已压缩） */
    ".docx", ".xlsx", ".pptx", ".pdf",
    NULL
};

int uvhttp_should_compress_by_extension(const char* filename) {
    if (!filename || !*filename) {
        return 0;
    }
    
    /* 查找最后一个点（文件扩展名） */
    const char* last_dot = NULL;
    const char* p = filename;
    while (*p) {
        if (*p == '.') {
            last_dot = p;
        }
        p++;
    }
    
    /* 没有扩展名，不压缩 */
    if (!last_dot) {
        return 0;
    }
    
    /* 检查是否在非压缩列表中 */
    for (size_t i = 0; NON_COMPRESSIBLE_EXTENSIONS[i] != NULL; i++) {
        if (strcasecmp(last_dot, NON_COMPRESSIBLE_EXTENSIONS[i]) == 0) {
            return 0;
        }
    }
    
    /* 检查是否在可压缩列表中 */
    for (size_t i = 0; COMPRESSIBLE_EXTENSIONS[i] != NULL; i++) {
        if (strcasecmp(last_dot, COMPRESSIBLE_EXTENSIONS[i]) == 0) {
            return 1;
        }
    }
    
    /* 未知扩展名，不压缩（保守策略） */
    return 0;
}

int uvhttp_should_compress_by_content_type(const char* content_type) {
    if (!content_type || !*content_type) {
        return 0;
    }
    
    /* 不压缩的类型 */
    if (strncasecmp(content_type, "image/", 6) == 0) return 0;
    if (strncasecmp(content_type, "video/", 6) == 0) return 0;
    if (strncasecmp(content_type, "audio/", 6) == 0) return 0;
    if (strcasecmp(content_type, "application/zip") == 0) return 0;
    if (strcasecmp(content_type, "application/gzip") == 0) return 0;
    if (strcasecmp(content_type, "application/x-gzip") == 0) return 0;
    if (strcasecmp(content_type, "application/x-compressed") == 0) return 0;
    if (strcasecmp(content_type, "application/pdf") == 0) return 0;
    if (strncasecmp(content_type, "application/vnd.", 16) == 0) return 0;
    
    /* 可压缩的类型 */
    if (strncasecmp(content_type, "text/", 5) == 0) return 1;
    if (strcasecmp(content_type, "application/json") == 0) return 1;
    if (strcasecmp(content_type, "application/xml") == 0) return 1;
    if (strcasecmp(content_type, "application/javascript") == 0) return 1;
    if (strcasecmp(content_type, "application/xhtml+xml") == 0) return 1;
    if (strcasecmp(content_type, "application/rss+xml") == 0) return 1;
    if (strcasecmp(content_type, "application/atom+xml") == 0) return 1;
    
    /* 未知类型，不压缩（保守策略） */
    return 0;
}

uvhttp_error_t uvhttp_response_set_compress_by_filename(uvhttp_response_t* response,
                                                        const char* filename) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 判断是否应该压缩 */
    int should_compress = uvhttp_should_compress_by_extension(filename);
    
    if (should_compress) {
        /* 启用压缩并设置默认阈值 */
        response->compress = 1;
        if (response->compress_threshold == 0) {
            response->compress_threshold = 1024;
        }
    } else {
        /* 禁用压缩 */
        response->compress = 0;
    }
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_compress_by_content_type(uvhttp_response_t* response,
                                                           const char* content_type) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 判断是否应该压缩 */
    int should_compress = uvhttp_should_compress_by_content_type(content_type);
    
    if (should_compress) {
        /* 启用压缩并设置默认阈值 */
        response->compress = 1;
        if (response->compress_threshold == 0) {
            response->compress_threshold = 1024;
        }
    } else {
        /* 禁用压缩 */
        response->compress = 0;
    }
    
    return UVHTTP_OK;
}

#endif /* UVHTTP_FEATURE_COMPRESSION */

void uvhttp_response_free(uvhttp_response_t* response) {
    if (!response) {
        return;
    }

    uvhttp_response_cleanup(response);
    uvhttp_free(response);
}

/* ========== Headers operation API implement ========== */

/* get header count */
size_t uvhttp_response_get_header_count(uvhttp_response_t* response) {
    if (!response) {
        return 0;
    }
    return response->header_count;
}

/* get header at specified index (internal use) */
uvhttp_header_t* uvhttp_response_get_header_at(uvhttp_response_t* response,
                                               size_t index) {
    if (!response || index >= response->header_count) {
        return NULL;
    }

    /* check if in inline array */
    if (index < UVHTTP_INLINE_HEADERS_CAPACITY) {
        return &response->headers[index];
    }

    /* in dynamically expanded array */
    if (response->headers_extra) {
        return &response->headers_extra[index - UVHTTP_INLINE_HEADERS_CAPACITY];
    }

    return NULL;
}

/* traverse all headers */
void uvhttp_response_foreach_header(uvhttp_response_t* response,
                                    uvhttp_header_callback_t callback,
                                    void* user_data) {
    if (!response || !callback) {
        return;
    }

    for (size_t i = 0; i < response->header_count; i++) {
        uvhttp_header_t* header = uvhttp_response_get_header_at(response, i);
        if (header) {
            callback(header->name, header->value, user_data);
        }
    }
}