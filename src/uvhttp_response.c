#include "uvhttp_response.h"

#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_features.h"
#include "uvhttp_logging.h"
#include "uvhttp_validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* HTTP response header string constants */
#define HTTP_HEADER_CONNECTION_KEEPALIVE "Connection: keep-alive\r\n"
#define HTTP_HEADER_CONNECTION_CLOSE "Connection: close\r\n"

/* Function declaration */
static void uvhttp_free_write_data(uv_write_t* req, int status);

static const char* get_status_text(int status_code) {
    switch (status_code) {
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

    // status line
    pos +=
        snprintf(buffer + pos, *length - pos, UVHTTP_VERSION_1_1 " %d %s\r\n",
                 response->status_code, get_status_text(response->status_code));

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

        pos += snprintf(buffer + pos, *length - pos, "%s: %s\r\n", header->name,
                        header->value);

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
        pos += snprintf(buffer + pos, *length - pos,
                        "Content-Type: text/plain\r\n");
    }

    // HTTP/1.1 requirement: must have Content-Length or use chunked encoding
    // here we always add Content-Length to ensure protocol compliance
    if (!has_content_length) {
        if (response->body && response->body_length > 0) {
            pos += snprintf(buffer + pos, *length - pos,
                            "Content-Length: %zu\r\n", response->body_length);
        } else {
            // even if there is no body, still set Content-Length: 0
            pos +=
                snprintf(buffer + pos, *length - pos, "Content-Length: 0\r\n");
        }
    }

    // HTTP/1.1 optimization: set Connection header based on keep-alive
    if (!has_connection) {
        if (response->keepalive) {
            pos += snprintf(buffer + pos, *length - pos,
                            HTTP_HEADER_CONNECTION_KEEPALIVE);
            pos += snprintf(buffer + pos, *length - pos,
                            "Keep-Alive: timeout=%d, max=%d\r\n",
                            UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT,
                            UVHTTP_DEFAULT_KEEP_ALIVE_MAX);
        } else {
            pos += snprintf(buffer + pos, *length - pos,
                            HTTP_HEADER_CONNECTION_CLOSE);
        }
    }

    // endheaders
    pos += snprintf(buffer + pos, *length - pos, "\r\n");

    *length = pos;
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
                    } else {
                        /* keep-alive connection, restart read to receive next
                         * request */
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

    /* build complete HTTP response - pure memory operation */
    /* optimization: increase initial buffer size, reduce reallocation */
    size_t headers_size =
        UVHTTP_INITIAL_BUFFER_SIZE * 2; /* increase from 512 to 1024 */
    char* headers_buffer = uvhttp_alloc(headers_size);
    if (!headers_buffer) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    size_t headers_length = headers_size;
    build_response_headers(response, headers_buffer, &headers_length);

    /* check if buffer is too small, if so reallocate larger buffer */
    if (headers_length >= headers_size) {
        uvhttp_free(headers_buffer);
        headers_size =
            headers_length +
            UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN; /* add safety margin */
        headers_buffer = uvhttp_alloc(headers_size);
        if (!headers_buffer) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        headers_length = headers_size;
        build_response_headers(response, headers_buffer, &headers_length);
    }

    /* calculate total size */
    size_t total_size = headers_length + response->body_length;
    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

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
    if (response->body && response->body_length > 0) {
        memcpy(response_data + headers_length, response->body,
               response->body_length);
    }

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

    /* directly call libuv, use same method as test/bench_server.c */
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