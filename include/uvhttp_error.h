#ifndef UVHTTP_ERROR_H
#define UVHTTP_ERROR_H

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
    UVHTTP_ERROR_HTTP2_SESSION = -802
} uvhttp_error_t;

/* Error code to string */
const char* uvhttp_error_string(uvhttp_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_H */