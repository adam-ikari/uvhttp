#include "uvhttp_error.h"

const char* uvhttp_error_string(uvhttp_error_t error) {
    switch (error) {
        case UVHTTP_OK:
            return "Success";
        
        /* General errors */
        case UVHTTP_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case UVHTTP_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case UVHTTP_ERROR_NOT_FOUND:
            return "Not found";
        case UVHTTP_ERROR_ALREADY_EXISTS:
            return "Already exists";
        
        /* Server errors */
        case UVHTTP_ERROR_SERVER_INIT:
            return "Server initialization failed";
        case UVHTTP_ERROR_SERVER_LISTEN:
            return "Server listen failed";
        case UVHTTP_ERROR_SERVER_STOP:
            return "Server stop failed";
        case UVHTTP_ERROR_CONNECTION_LIMIT:
            return "Connection limit reached";
        
        /* Connection errors */
        case UVHTTP_ERROR_CONNECTION_INIT:
            return "Connection initialization failed";
        case UVHTTP_ERROR_CONNECTION_ACCEPT:
            return "Connection accept failed";
        case UVHTTP_ERROR_CONNECTION_START:
            return "Connection start failed";
        case UVHTTP_ERROR_CONNECTION_CLOSE:
            return "Connection close failed";
        
        /* Request/Response errors */
        case UVHTTP_ERROR_REQUEST_INIT:
            return "Request initialization failed";
        case UVHTTP_ERROR_RESPONSE_INIT:
            return "Response initialization failed";
        case UVHTTP_ERROR_RESPONSE_SEND:
            return "Response send failed";
        
        /* TLS errors */
        case UVHTTP_ERROR_TLS_INIT:
            return "TLS initialization failed";
        case UVHTTP_ERROR_TLS_CONTEXT:
            return "TLS context creation failed";
        case UVHTTP_ERROR_TLS_HANDSHAKE:
            return "TLS handshake failed";
        
        /* Router errors */
        case UVHTTP_ERROR_ROUTER_INIT:
            return "Router initialization failed";
        case UVHTTP_ERROR_ROUTER_ADD:
            return "Router add failed";
        
        /* Allocator errors */
        case UVHTTP_ERROR_ALLOCATOR_INIT:
            return "Allocator initialization failed";
        case UVHTTP_ERROR_ALLOCATOR_SET:
            return "Allocator set failed";
        
        /* WebSocket errors */
        case UVHTTP_ERROR_WEBSOCKET_INIT:
            return "WebSocket initialization failed";
        case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
            return "WebSocket handshake failed";
        case UVHTTP_ERROR_WEBSOCKET_FRAME:
            return "WebSocket frame processing failed";
        
        /* HTTP/2 errors */
        case UVHTTP_ERROR_HTTP2_INIT:
            return "HTTP/2 initialization failed";
        case UVHTTP_ERROR_HTTP2_STREAM:
            return "HTTP/2 stream processing failed";
        case UVHTTP_ERROR_HTTP2_SESSION:
            return "HTTP/2 session failed";
        
        default:
            return "Unknown error";
    }
}