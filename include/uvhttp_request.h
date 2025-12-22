#ifndef UVHTTP_REQUEST_H
#define UVHTTP_REQUEST_H

#include <uv.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "llhttp.h"
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_URL_LEN 2048
#define MAX_BODY_LEN (1024 * 1024)  // 1MB

// HTTP方法枚举
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
    uv_tcp_t* client;
    llhttp_t* parser;
    llhttp_settings_t* parser_settings;
    
    uvhttp_method_t method;
    char url[MAX_URL_LEN];
    char* path;
    char* query;
    char* body;
    size_t body_length;
    size_t body_capacity;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    int parsing_complete;
    void* user_data;
};

/* API functions */
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
void uvhttp_request_free(uvhttp_request_t* request);
void uvhttp_request_cleanup(uvhttp_request_t* request);
const char* uvhttp_request_get_path(uvhttp_request_t* request);
const char* uvhttp_request_get_query_string(uvhttp_request_t* request);
const char* uvhttp_request_get_query_param(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_client_ip(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

#ifdef __cplusplus
}
#endif

#endif