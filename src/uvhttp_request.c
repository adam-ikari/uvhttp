#include "uvhttp_request_simple.h"
#include "uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int uvhttp_request_init(uvhttp_request_t* request, void* client) {
    if (!request || !client) {
        return -1;
    }
    
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->client = client;
    request->method = UVHTTP_GET; // 默认方法
    
    // 初始化body缓冲区
    request->body_capacity = 1024;
    request->body = malloc(request->body_capacity);
    if (!request->body) {
        return -1;
    }
    request->body_length = 0;
    
    return 0;
}

void uvhttp_request_cleanup(uvhttp_request_t* request) {
    if (request->body) {
        free(request->body);
    }
}

const char* uvhttp_request_get_method(uvhttp_request_t* request) {
    switch (request->method) {
        case UVHTTP_GET: return "GET";
        case UVHTTP_POST: return "POST";
        case UVHTTP_PUT: return "PUT";
        case UVHTTP_DELETE: return "DELETE";
        case UVHTTP_HEAD: return "HEAD";
        case UVHTTP_OPTIONS: return "OPTIONS";
        case UVHTTP_PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}

const char* uvhttp_request_get_url(uvhttp_request_t* request) {
    return request->url;
}

const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name) {
    for (size_t i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }
    return NULL;
}

const char* uvhttp_request_get_body(uvhttp_request_t* request) {
    return request->body;
}

size_t uvhttp_request_get_body_length(uvhttp_request_t* request) {
    return request->body_length;
}