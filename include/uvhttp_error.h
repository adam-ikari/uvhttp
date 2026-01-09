#ifndef UVHTTP_ERROR_H
#define UVHTTP_ERROR_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UVHTTP error codes */
typedef enum {
    UVHTTP_OK = 0,
    
    /* General errors */
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NOT_FOUND = -3,
    UVHTTP_ERROR_ALREADY_EXISTS = -4,
    UVHTTP_ERROR_NULL_POINTER = -5,
    UVHTTP_ERROR_BUFFER_TOO_SMALL = -6,
    UVHTTP_ERROR_TIMEOUT = -7,
    UVHTTP_ERROR_CANCELLED = -8,
    
    /* Server errors */
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_SERVER_LISTEN = -101,
    UVHTTP_ERROR_SERVER_STOP = -102,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,
    UVHTTP_ERROR_SERVER_ALREADY_RUNNING = -104,
    UVHTTP_ERROR_SERVER_NOT_RUNNING = -105,
    UVHTTP_ERROR_SERVER_INVALID_CONFIG = -106,
    
    /* Connection errors */
    UVHTTP_ERROR_CONNECTION_INIT = -200,
    UVHTTP_ERROR_CONNECTION_ACCEPT = -201,
    UVHTTP_ERROR_CONNECTION_START = -202,
    UVHTTP_ERROR_CONNECTION_CLOSE = -203,
    UVHTTP_ERROR_CONNECTION_RESET = -204,
    UVHTTP_ERROR_CONNECTION_TIMEOUT = -205,
    UVHTTP_ERROR_CONNECTION_REFUSED = -206,
    UVHTTP_ERROR_CONNECTION_BROKEN = -207,
    
    /* Request/Response errors */
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_INIT = -301,
    UVHTTP_ERROR_RESPONSE_SEND = -302,
    UVHTTP_ERROR_INVALID_HTTP_METHOD = -303,
    UVHTTP_ERROR_INVALID_HTTP_VERSION = -304,
    UVHTTP_ERROR_HEADER_TOO_LARGE = -305,
    UVHTTP_ERROR_BODY_TOO_LARGE = -306,
    UVHTTP_ERROR_MALFORMED_REQUEST = -307,
    
    /* TLS errors */
    UVHTTP_ERROR_TLS_INIT = -400,
    UVHTTP_ERROR_TLS_CONTEXT = -401,
    UVHTTP_ERROR_TLS_HANDSHAKE = -402,
    UVHTTP_ERROR_TLS_CERT_LOAD = -403,
    UVHTTP_ERROR_TLS_KEY_LOAD = -404,
    UVHTTP_ERROR_TLS_VERIFY_FAILED = -405,
    UVHTTP_ERROR_TLS_EXPIRED = -406,
    UVHTTP_ERROR_TLS_NOT_YET_VALID = -407,
    
    /* Router errors */
    UVHTTP_ERROR_ROUTER_INIT = -500,
    UVHTTP_ERROR_ROUTER_ADD = -501,
    UVHTTP_ERROR_ROUTE_NOT_FOUND = -502,
    UVHTTP_ERROR_ROUTE_ALREADY_EXISTS = -503,
    UVHTTP_ERROR_INVALID_ROUTE_PATTERN = -504,
    
    /* Rate limit errors */
    UVHTTP_ERROR_RATE_LIMIT_EXCEEDED = -550,
    
    /* Allocator errors */
    UVHTTP_ERROR_ALLOCATOR_INIT = -600,
    UVHTTP_ERROR_ALLOCATOR_SET = -601,
    UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED = -602,
    
    /* WebSocket errors */
    UVHTTP_ERROR_WEBSOCKET_INIT = -700,
    UVHTTP_ERROR_WEBSOCKET_HANDSHAKE = -701,
    UVHTTP_ERROR_WEBSOCKET_FRAME = -702,
    UVHTTP_ERROR_WEBSOCKET_TOO_LARGE = -703,
    UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE = -704,
    UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED = -705,
    UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED = -706,
    UVHTTP_ERROR_WEBSOCKET_CLOSED = -707,
    
    /* HTTP/2 errors */
    UVHTTP_ERROR_HTTP2_INIT = -800,
    UVHTTP_ERROR_HTTP2_STREAM = -801,
    UVHTTP_ERROR_HTTP2_SETTINGS = -802,
    UVHTTP_ERROR_HTTP2_FLOW_CONTROL = -803,
    UVHTTP_ERROR_HTTP2_HEADER_COMPRESS = -804,
    UVHTTP_ERROR_HTTP2_PRIORITY = -805,
    
    /* Configuration errors */
    UVHTTP_ERROR_CONFIG_PARSE = -900,
    UVHTTP_ERROR_CONFIG_INVALID = -901,
    UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND = -902,
    UVHTTP_ERROR_CONFIG_MISSING_REQUIRED = -903,
    
    /* Middleware errors */
    UVHTTP_ERROR_MIDDLEWARE_INIT = -1000,
    UVHTTP_ERROR_MIDDLEWARE_REGISTER = -1001,
    UVHTTP_ERROR_MIDDLEWARE_EXECUTE = -1002,
    UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND = -1003,
    
    /* Logging errors */
    UVHTTP_ERROR_LOG_INIT = -1100,
    UVHTTP_ERROR_LOG_WRITE = -1101,
    UVHTTP_ERROR_LOG_FILE_OPEN = -1102,
    UVHTTP_ERROR_LOG_NOT_INITIALIZED = -1103,
    
    UVHTTP_ERROR_MAX /* 动态计算最大值 */
} uvhttp_error_t;

/* 统一的返回类型别名，用于所有API函数 */
typedef uvhttp_error_t uvhttp_result_t;

/* Error code to string */
const char* uvhttp_error_string(uvhttp_error_t error);

/* Error code to category */
const char* uvhttp_error_category_string(uvhttp_error_t error);

/* Error code to description */
const char* uvhttp_error_description(uvhttp_error_t error);

/* Error code to suggestion */
const char* uvhttp_error_suggestion(uvhttp_error_t error);

/* Check if error is recoverable */
int uvhttp_error_is_recoverable(uvhttp_error_t error);

/* Error recovery and retry mechanism */
void uvhttp_set_error_recovery_config(int max_retries, int base_delay_ms, 
                                     int max_delay_ms, double backoff_multiplier);
uvhttp_error_t uvhttp_retry_operation(uvhttp_error_t (*operation)(void*), 
                                     void* context, const char* operation_name);

/* Error logging and statistics */
void uvhttp_log_error(uvhttp_error_t error, const char* context);
void uvhttp_get_error_stats(size_t* error_counts, time_t* last_error_time, 
                           const char** last_error_context);
void uvhttp_reset_error_stats(void);
uvhttp_error_t uvhttp_get_most_frequent_error(void);

/* Error code count for statistics array */
#define UVHTTP_ERROR_COUNT 120

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_H */