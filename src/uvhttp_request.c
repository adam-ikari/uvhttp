#include "uvhttp_request.h"
#include "uvhttp_utils.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

// Include the internal llhttp structure definition
struct llhttp__internal_s {
  llhttp_type_t type;
  const llhttp_settings_t* settings;
  llhttp_errno_t error;
  const char* error_pos;
  llhttp_method_t method;
  llhttp_status_t status_code;
  uint16_t http_major;
  uint16_t http_minor;
  uint8_t finish;
  uint8_t flags;
  uint8_t upgrade;
  uint8_t lenient_flags;
};

int uvhttp_request_init(uvhttp_request_t* request, void* client) {
    if (!request || !client) {
        return -1;
    }
    
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->client = client;
    request->method = UVHTTP_GET; // 默认方法
    
    // 初始化HTTP解析器
    request->parser_settings = uvhttp_malloc(sizeof(llhttp_settings_t));
    if (!request->parser_settings) {
        return -1;
    }
    llhttp_settings_init(request->parser_settings);
    
    request->parser = uvhttp_malloc(sizeof(struct llhttp__internal_s));
    if (!request->parser) {
        uvhttp_free(request->parser_settings);
        return -1;
    }
    llhttp_init(request->parser, HTTP_REQUEST, request->parser_settings);
    
    // 初始化body缓冲区
    request->body_capacity = UVHTTP_INITIAL_BUFFER_SIZE;
    request->body = uvhttp_malloc(request->body_capacity);
    if (!request->body) {
        uvhttp_free(request->parser);
        uvhttp_free(request->parser_settings);
        return -1;
    }
    request->body_length = 0;
    
    return 0;
}

void uvhttp_request_cleanup(uvhttp_request_t* request) {
    if (request->body) {
        uvhttp_free(request->body);
    }
    if (request->parser) {
        uvhttp_free(request->parser);
    }
    if (request->parser_settings) {
        uvhttp_free(request->parser_settings);
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