#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include <uv.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_connection uvhttp_connection_t;

#define MAX_RESPONSE_HEADERS 32
#define MAX_RESPONSE_HEADER_NAME_LEN 128
#define MAX_RESPONSE_HEADER_VALUE_LEN 4096
#define MAX_RESPONSE_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    char name[MAX_RESPONSE_HEADER_NAME_LEN];
    char value[MAX_RESPONSE_HEADER_VALUE_LEN];
} uvhttp_response_header_t;

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
    
    uvhttp_response_header_t headers[MAX_RESPONSE_HEADERS];
    size_t header_count;
    
    char* body;
    size_t body_length;
    
    int headers_sent;
    int finished;
};

#ifdef __cplusplus
}
#endif

#endif