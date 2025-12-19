#ifndef UVHTTP_RESPONSE_SIMPLE_H
#define UVHTTP_RESPONSE_SIMPLE_H

#include <stdlib.h>
#include <string.h>
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
typedef struct uvhttp_response uvhttp_response_t;



// 简化的响应结构
struct uvhttp_response {
    int status_code;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    char* body;
    size_t body_length;
    
    int finished;
    void* client;
};

// 简化的响应函数
int uvhttp_response_init(uvhttp_response_t* response, void* client);
void uvhttp_response_cleanup(uvhttp_response_t* response);
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
void uvhttp_response_send(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif