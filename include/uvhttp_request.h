#ifndef UVHTTP_REQUEST_H
#define UVHTTP_REQUEST_H

#include <uv.h>
#include <llhttp.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HEADERS 32
#define MAX_HEADER_NAME_LEN 128
#define MAX_HEADER_VALUE_LEN 4096
#define MAX_URL_LEN 2048
#define MAX_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    char name[MAX_HEADER_NAME_LEN];
    char value[MAX_HEADER_VALUE_LEN];
} uvhttp_header_t;

struct uvhttp_request {
    uv_tcp_t* client;
    llhttp_t parser;
    llhttp_settings_t parser_settings;
    
    char url[MAX_URL_LEN];
    char* body;
    size_t body_length;
    size_t body_capacity;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    int parsing_complete;
    void* user_data;
};

#ifdef __cplusplus
}
#endif

#endif