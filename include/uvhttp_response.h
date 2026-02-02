#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
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
    int status_code;     /* 4 bytes - HTTP status code */
    int headers_sent;    /* 4 bytes - headerusesend */
    int sent;            /* 4 bytes - Responseusesend */
    int finished;        /* 4 bytes - Responseusecompleted */
    int keepalive;       /* 4 bytes - usekeepConnection */
    int compress;        /* 4 bytes - useEnablecompress */
    int cache_ttl;       /* 4 bytes - Cache TTL(seconds) */
    int _padding1;       /* 4 bytes - paddingto32bytes */
    size_t header_count; /* 8 bytes - header quantity */
    size_t body_length;  /* 8 bytes - body length */
    uv_tcp_t* client;    /* 8 bytes - TCP Client */
    char* body;          /* 8 bytes - Response */
    /* Cache line 1 total: 64 bytes */

    /* ========== Cache line 2 (64-127 bytes): Pointer and counter fields -
     * Secondary frequent access ========== */
    /* Frequently accessed during response building and cache management */
    time_t cache_expires; /* 8 bytes - Cache expiration time */
    uvhttp_header_t*
        headers_extra;       /* 8 bytes - Extra headers (dynamic expansion) */
    size_t headers_capacity; /* 8 bytes - Total headers capacity */
    int _padding2[10];       /* 40 bytes - Padding to 64 bytes */
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
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code);
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                          const char* name, const char* value);
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body, size_t length);

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

/* Response sending functions */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);

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