#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include <uv.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_response uvhttp_response_t;

#define MAX_RESPONSE_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    uv_write_t write_req;
    char* data;
    size_t length;
    uvhttp_response_t* response;
} uvhttp_write_data_t;

typedef struct {
    char* data;
    size_t length;
    uvhttp_response_t* response;
    uvhttp_connection_t* connection;
    size_t offset;
} uvhttp_tls_write_data_t;

struct uvhttp_response {
    uv_tcp_t* client;
    int status_code;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    char* body;
    size_t body_length;
    
    int headers_sent;
    int finished;
};

/* API functions */
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
void uvhttp_response_send(uvhttp_response_t* response);
void uvhttp_response_cleanup(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif