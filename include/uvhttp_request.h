#ifndef UVHTTP_REQUEST_H
#define UVHTTP_REQUEST_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"

#include "llhttp.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_URL_LEN 2048
#define MAX_BODY_LEN (1024 * 1024)  // 1MB

// HTTP method enumeration
typedef enum {
    UVHTTP_ANY = 0,
    UVHTTP_GET,
    UVHTTP_POST,
    UVHTTP_PUT,
    UVHTTP_DELETE,
    UVHTTP_HEAD,
    UVHTTP_OPTIONS,
    UVHTTP_PATCH
} uvhttp_method_t;

typedef struct uvhttp_request uvhttp_request_t;

struct uvhttp_request {
    /* ========== Cache line 1 (0-63 bytes): Hot path fields - Most frequently
     * accessed ========== */
    /* Frequently accessed during HTTP parsing and route matching */
    uvhttp_method_t method; /* 4 bytes - HTTP method */
    int parsing_complete;   /* 4 bytes - parsing complete */
    int _padding1[2];       /* 8 bytes - padding to 16 bytes */
    size_t header_count;    /* 8 bytes - header count */
    size_t body_length;     /* 8 bytes - body length */
    size_t body_capacity;   /* 8 bytes - body capacity */
    uv_tcp_t* client;       /* 8 bytes - TCP client handle */
    llhttp_t* parser;       /* 8 bytes - HTTP parser */
    /* Cache line 1 total: 56 bytes (remaining 8 bytes padding) */

    /* ========== Cache line 2 (64-127 bytes): Pointer fields - Secondary
     * frequent access ========== */
    /* Frequently accessed during request processing and response building */
    llhttp_settings_t* parser_settings; /* 8 bytes - parser settings */
    char* path;                         /* 8 bytes - request path */
    char* query;                        /* 8 bytes - query string */
    char* body;                         /* 8 bytes - request body */
    void* user_data;                    /* 8 bytes - user data */
    uvhttp_header_t*
        headers_extra;       /* 8 bytes - extra headers (dynamic expansion) */
    size_t headers_capacity; /* 8 bytes - headers total capacity */
    int _padding2[2];        /* 8 bytes - padding to 64 bytes */
    /* Cache line 2 total: 64 bytes */

    /* ========== Cache line 3+ (128+ bytes): Large buffers ========== */
    /* Placed at the end to avoid affecting cache locality of hot path fields */
    char url[MAX_URL_LEN]; /* 2048 bytes - URL buffer */

    /* Headers - Hybrid allocation: inline + dynamic expansion (optimized for
     * cache locality) */
    uvhttp_header_t
        headers[UVHTTP_INLINE_HEADERS_CAPACITY]; /* inline, reduce dynamic
                                                    allocation */
};

/* ========== Memory layout verification static assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, client, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, parser, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, parser_settings,
                       UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, path, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, query, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, body, UVHTTP_POINTER_ALIGNMENT);

/* Verify size_t alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, header_count, UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, body_length, UVHTTP_SIZE_T_ALIGNMENT);

/* Verify large buffers at end of structure */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_request_t, url) >= 64,
                     "url buffer should be after first 64 bytes");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_request_t, headers) >= 64,
                     "headers array should be after first 64 bytes");

/* API functions */
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
void uvhttp_request_free(uvhttp_request_t* request);
void uvhttp_request_cleanup(uvhttp_request_t* request);
const char* uvhttp_request_get_path(uvhttp_request_t* request);
const char* uvhttp_request_get_query_string(uvhttp_request_t* request);
const char* uvhttp_request_get_query_param(uvhttp_request_t* request,
                                           const char* name);
const char* uvhttp_request_get_client_ip(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                      const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

/* ========== Headers Operation API ========== */

/* get header quantity */
size_t uvhttp_request_get_header_count(uvhttp_request_t* request);

/* getspecifiedindex header(internalUse) */
uvhttp_header_t* uvhttp_request_get_header_at(uvhttp_request_t* request,
                                              size_t index);

/* add header(internalUse, Automaticexpand) */
uvhttp_error_t uvhttp_request_add_header(uvhttp_request_t* request,
                                         const char* name, const char* value);

/* traverseof headers */
typedef void (*uvhttp_header_callback_t)(const char* name, const char* value,
                                         void* user_data);
void uvhttp_request_foreach_header(uvhttp_request_t* request,
                                   uvhttp_header_callback_t callback,
                                   void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_REQUEST_H */