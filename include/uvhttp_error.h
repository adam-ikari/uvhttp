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
    
    /* Server errors */
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_SERVER_LISTEN = -101,
    UVHTTP_ERROR_SERVER_STOP = -102,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,
    
    /* Connection errors */
    UVHTTP_ERROR_CONNECTION_INIT = -200,
    UVHTTP_ERROR_CONNECTION_ACCEPT = -201,
    UVHTTP_ERROR_CONNECTION_START = -202,
    UVHTTP_ERROR_CONNECTION_CLOSE = -203,
    
    /* Request/Response errors */
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_INIT = -301,
    UVHTTP_ERROR_RESPONSE_SEND = -302,
    
    /* TLS errors */
    UVHTTP_ERROR_TLS_INIT = -400,
    UVHTTP_ERROR_TLS_CONTEXT = -401,
    UVHTTP_ERROR_TLS_HANDSHAKE = -402,
    
    /* Router errors */
    UVHTTP_ERROR_ROUTER_INIT = -500,
    UVHTTP_ERROR_ROUTER_ADD = -501,
    
    /* Allocator errors */
    UVHTTP_ERROR_ALLOCATOR_INIT = -600,
    UVHTTP_ERROR_ALLOCATOR_SET = -601,
    
    /* WebSocket errors */
    UVHTTP_ERROR_WEBSOCKET_INIT = -700,
    UVHTTP_ERROR_WEBSOCKET_HANDSHAKE = -701,
    UVHTTP_ERROR_WEBSOCKET_FRAME = -702,
    
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
    
    UVHTTP_ERROR_MAX /* 动态计算最大值 */
} uvhttp_error_t;

/* Error code to string */
const char* uvhttp_error_string(uvhttp_error_t error);

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

/* Maximum error code for statistics */
#define UVHTTP_ERROR_MAX 1000

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_H */