#include "uvhttp_error.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_features.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Log error */
void uvhttp_log_error(uvhttp_error_t error, const char* context) {
    (void)error;
    (void)context;
    /* Use logging system to record errors */
    /* Note: statistics feature removed, only logging remains */
}

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

    /* Configuration errors */ case UVHTTP_ERROR_CONFIG_PARSE:
        return "Configuration parse error";
    case UVHTTP_ERROR_CONFIG_INVALID:
        return "Invalid configuration";

    default:

        return "Unknown error";
    }
}

/* Get error category string */

const char* uvhttp_error_category_string(uvhttp_error_t error) {

    if (error == UVHTTP_OK) {

        return "Success";
    }

    /* General errors */

    if (error >= -8 && error <= -1) {

        return "General Error";
    }

    /* Server errors */

    if (error >= -106 && error <= -100) {

        return "Server Error";
    }

    /* Connection errors */

    if (error >= -207 && error <= -200) {

        return "Connection Error";
    }

    /* Request/Response errors */

    if (error >= -307 && error <= -300) {

        return "Request/Response Error";
    }

    /* TLS errors */

    if (error >= -407 && error <= -400) {

        return "TLS Error";
    }

    /* Routing errors */

    if (error >= -504 && error <= -500) {

        return "Router Error";
    }

    /* Allocator errors */

    if (error >= -602 && error <= -600) {

        return "Allocator Error";
    }

    /* WebSocket errors */

    if (error >= -707 && error <= -700) {

        return "WebSocket Error";
    }

    /* HTTP/2 errors */

    if (error >= -805 && error <= -800) {

        return "HTTP/2 Error";
    }

    /* Configuration errors */

    if (error >= -903 && error <= -900) {

        return "Configuration Error";
    }

    /* Middleware errors */

    if (error >= -1003 && error <= -1000) {

        return "Middleware Error";
    }

    /* Logging errors */

    if (error >= -1103 && error <= -1100) {

        return "Logging Error";
    }

    return "Unknown Error";
}

/* Get error description */

const char* uvhttp_error_description(uvhttp_error_t error) {

    switch (error) {

    case UVHTTP_OK:

        return "Operation completed successfully";

        /* General errors */

    case UVHTTP_ERROR_INVALID_PARAM:

        return "One or more parameters are invalid";

    case UVHTTP_ERROR_OUT_OF_MEMORY:

        return "Failed to allocate memory";

    case UVHTTP_ERROR_NOT_FOUND:

        return "Requested resource was not found";

    case UVHTTP_ERROR_ALREADY_EXISTS:

        return "Resource already exists";

    case UVHTTP_ERROR_NULL_POINTER:

        return "Null pointer encountered";

    case UVHTTP_ERROR_BUFFER_TOO_SMALL:

        return "Buffer is too small to hold the data";

    case UVHTTP_ERROR_TIMEOUT:

        return "Operation timed out";

    case UVHTTP_ERROR_CANCELLED:

        return "Operation was cancelled";

        /* Server errors */

    case UVHTTP_ERROR_SERVER_INIT:

        return "Failed to initialize server";

    case UVHTTP_ERROR_SERVER_LISTEN:

        return "Failed to listen on the specified port";

    case UVHTTP_ERROR_SERVER_STOP:

        return "Failed to stop server";

    case UVHTTP_ERROR_CONNECTION_LIMIT:

        return "Maximum connection limit reached";

    case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:

        return "Server is already running";

    case UVHTTP_ERROR_SERVER_NOT_RUNNING:

        return "Server is not running";

    case UVHTTP_ERROR_SERVER_INVALID_CONFIG:

        return "Invalid server configuration";

        /* Connection errors */

    case UVHTTP_ERROR_CONNECTION_INIT:

        return "Failed to initialize connection";

    case UVHTTP_ERROR_CONNECTION_ACCEPT:

        return "Failed to accept incoming connection";

    case UVHTTP_ERROR_CONNECTION_START:

        return "Failed to start connection";

    case UVHTTP_ERROR_CONNECTION_CLOSE:

        return "Failed to close connection";

    case UVHTTP_ERROR_CONNECTION_RESET:

        return "Connection was reset by peer";

    case UVHTTP_ERROR_CONNECTION_TIMEOUT:

        return "Connection timed out";

    case UVHTTP_ERROR_CONNECTION_REFUSED:

        return "Connection was refused";

    case UVHTTP_ERROR_CONNECTION_BROKEN:

        return "Connection is broken";

        /* Request/Response errors */

    case UVHTTP_ERROR_REQUEST_INIT:

        return "Failed to initialize request";

    case UVHTTP_ERROR_RESPONSE_INIT:

        return "Failed to initialize response";

    case UVHTTP_ERROR_RESPONSE_SEND:

        return "Failed to send response";

    case UVHTTP_ERROR_INVALID_HTTP_METHOD:

        return "Invalid HTTP method";

    case UVHTTP_ERROR_INVALID_HTTP_VERSION:

        return "Invalid HTTP version";

    case UVHTTP_ERROR_HEADER_TOO_LARGE:

        return "HTTP headers are too large";

    case UVHTTP_ERROR_BODY_TOO_LARGE:

        return "Request body is too large";

    case UVHTTP_ERROR_MALFORMED_REQUEST:

        return "Malformed HTTP request";

    case UVHTTP_ERROR_FILE_TOO_LARGE:

        return "File is too large";

    case UVHTTP_ERROR_IO_ERROR:

        return "I/O error";

        /* TLS errors */

    case UVHTTP_ERROR_TLS_INIT:

        return "Failed to initialize TLS";

    case UVHTTP_ERROR_TLS_CONTEXT:

        return "Failed to create TLS context";

    case UVHTTP_ERROR_TLS_HANDSHAKE:

        return "TLS handshake failed";

    case UVHTTP_ERROR_TLS_CERT_LOAD:

        return "Failed to load TLS certificate";

    case UVHTTP_ERROR_TLS_KEY_LOAD:

        return "Failed to load TLS private key";

    case UVHTTP_ERROR_TLS_VERIFY_FAILED:

        return "TLS certificate verification failed";

    case UVHTTP_ERROR_TLS_EXPIRED:

        return "TLS certificate has expired";

    case UVHTTP_ERROR_TLS_NOT_YET_VALID:

        return "TLS certificate is not yet valid";

        /* Router errors */

    case UVHTTP_ERROR_ROUTER_INIT:

        return "Failed to initialize router";

    case UVHTTP_ERROR_ROUTER_ADD:

        return "Failed to add route";

    case UVHTTP_ERROR_ROUTE_NOT_FOUND:

        return "No matching route found";

    case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:

        return "Route already exists";

    case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:

        return "Invalid route pattern";

        /* Allocator errors */

    case UVHTTP_ERROR_ALLOCATOR_INIT:

        return "Failed to initialize allocator";

    case UVHTTP_ERROR_ALLOCATOR_SET:

        return "Failed to set allocator";

    case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:

        return "Allocator is not initialized";

        /* WebSocket errors */

    case UVHTTP_ERROR_WEBSOCKET_INIT:

        return "Failed to initialize WebSocket";

    case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:

        return "WebSocket handshake failed";

    case UVHTTP_ERROR_WEBSOCKET_FRAME:

        return "WebSocket frame processing failed";

    case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:

        return "WebSocket message is too large";

    case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:

        return "Invalid WebSocket opcode";

    case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:

        return "WebSocket is not connected";

    case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:

        return "WebSocket is already connected";

    case UVHTTP_ERROR_WEBSOCKET_CLOSED:

        return "WebSocket connection is closed";

        /* Configuration errors */

    case UVHTTP_ERROR_CONFIG_PARSE:
        return "Failed to parse configuration";

    case UVHTTP_ERROR_CONFIG_INVALID:

        return "Invalid configuration";

    case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:

        return "Configuration file not found";

    case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:

        return "Missing required configuration";

        /* Middleware errors */

        /* Logging errors */

    case UVHTTP_ERROR_LOG_INIT:

        return "Failed to initialize logging";

    case UVHTTP_ERROR_LOG_WRITE:

        return "Failed to write log";

    case UVHTTP_ERROR_LOG_FILE_OPEN:

        return "Failed to open log file";

    case UVHTTP_ERROR_LOG_NOT_INITIALIZED:

        return "Logging is not initialized";

    default:

        return "Unknown error";
    }
}

/* get error fix suggestion */

const char* uvhttp_error_suggestion(uvhttp_error_t error) {

    switch (error) {

    case UVHTTP_OK:

        return "No action needed";

        /* General errors */

    case UVHTTP_ERROR_INVALID_PARAM:

        return "Check the parameters passed to the function";

    case UVHTTP_ERROR_OUT_OF_MEMORY:

        return "Free up memory or increase available memory";

    case UVHTTP_ERROR_NOT_FOUND:

        return "Verify the resource exists and the path is correct";

    case UVHTTP_ERROR_ALREADY_EXISTS:

        return "Use a different name or remove the existing resource";

    case UVHTTP_ERROR_NULL_POINTER:

        return "Ensure all pointers are properly initialized";

    case UVHTTP_ERROR_BUFFER_TOO_SMALL:

        return "Allocate a larger buffer";

    case UVHTTP_ERROR_TIMEOUT:

        return "Increase timeout or optimize operation";

    case UVHTTP_ERROR_CANCELLED:

        return "Check if operation was intentionally cancelled";

        /* Server errors */

    case UVHTTP_ERROR_SERVER_INIT:

        return "Check system resources and configuration";

    case UVHTTP_ERROR_SERVER_LISTEN:

        return "Check if port is available and you have permissions";

    case UVHTTP_ERROR_SERVER_STOP:

        return "Ensure server is running before stopping";

    case UVHTTP_ERROR_CONNECTION_LIMIT:

        return "Increase connection limit or reduce concurrent connections";

    case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:

        return "Stop the existing server before starting a new one";

    case UVHTTP_ERROR_SERVER_NOT_RUNNING:

        return "Start the server before performing this operation";

    case UVHTTP_ERROR_SERVER_INVALID_CONFIG:

        return "Review and correct server configuration";

        /* Connection errors */

    case UVHTTP_ERROR_CONNECTION_INIT:

        return "Check network configuration and availability";

    case UVHTTP_ERROR_CONNECTION_ACCEPT:

        return "Retry the connection attempt";

    case UVHTTP_ERROR_CONNECTION_START:

        return "Verify server is running and accessible";

    case UVHTTP_ERROR_CONNECTION_CLOSE:

        return "Ensure connection is still active";

    case UVHTTP_ERROR_CONNECTION_RESET:

        return "Check network stability and retry";

    case UVHTTP_ERROR_CONNECTION_TIMEOUT:

        return "Increase timeout or check network connectivity";

    case UVHTTP_ERROR_CONNECTION_REFUSED:

        return "Verify server is running and accepting connections";

    case UVHTTP_ERROR_CONNECTION_BROKEN:

        return "Re-establish the connection";

        /* Request/Response errors */

    case UVHTTP_ERROR_REQUEST_INIT:

        return "Check request parameters and format";

    case UVHTTP_ERROR_RESPONSE_INIT:

        return "Verify response configuration";

    case UVHTTP_ERROR_RESPONSE_SEND:

        return "Retry sending the response";

    case UVHTTP_ERROR_INVALID_HTTP_METHOD:

        return "Use a valid HTTP method (GET, POST, PUT, DELETE, etc.)";

    case UVHTTP_ERROR_INVALID_HTTP_VERSION:

        return "Use HTTP/1.1 or HTTP/2";

    case UVHTTP_ERROR_HEADER_TOO_LARGE:

        return "Reduce header size or increase limit";

    case UVHTTP_ERROR_BODY_TOO_LARGE:

        return "Reduce body size or increase limit";

    case UVHTTP_ERROR_MALFORMED_REQUEST:

        return "Check request format and syntax";

    case UVHTTP_ERROR_FILE_TOO_LARGE:

        return "Use a smaller file or increase max_file_size limit";

    case UVHTTP_ERROR_IO_ERROR:

        return "Check file permissions and disk space";

        /* TLS errors */

    case UVHTTP_ERROR_TLS_INIT:

        return "Check TLS library installation";

    case UVHTTP_ERROR_TLS_CONTEXT:

        return "Verify TLS configuration";

    case UVHTTP_ERROR_TLS_HANDSHAKE:

        return "Check certificates and TLS configuration";

    case UVHTTP_ERROR_TLS_CERT_LOAD:

        return "Verify certificate file exists and is readable";

    case UVHTTP_ERROR_TLS_KEY_LOAD:

        return "Verify key file exists and is readable";

    case UVHTTP_ERROR_TLS_VERIFY_FAILED:

        return "Check certificate chain and validity";

    case UVHTTP_ERROR_TLS_EXPIRED:

        return "Renew the certificate";

    case UVHTTP_ERROR_TLS_NOT_YET_VALID:

        return "Check system time or wait for certificate validity";

        /* Router errors */

    case UVHTTP_ERROR_ROUTER_INIT:

        return "Check router configuration";

    case UVHTTP_ERROR_ROUTER_ADD:

        return "Verify route pattern is valid";

    case UVHTTP_ERROR_ROUTE_NOT_FOUND:

        return "Register the route or check URL";

    case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:

        return "Use a different route pattern";

    case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:

        return "Use a valid route pattern";

        /* Allocator errors */

    case UVHTTP_ERROR_ALLOCATOR_INIT:

        return "Check allocator configuration";

    case UVHTTP_ERROR_ALLOCATOR_SET:

        return "Verify allocator type is supported";

    case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:

        return "Initialize the allocator before use";

        /* WebSocket errors */

    case UVHTTP_ERROR_WEBSOCKET_INIT:

        return "Check WebSocket configuration";

    case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:

        return "Verify WebSocket URL and protocol";

    case UVHTTP_ERROR_WEBSOCKET_FRAME:

        return "Check WebSocket message format";

    case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:

        return "Reduce message size or increase limit";

    case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:

        return "Use valid WebSocket opcode";

    case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:

        return "Establish WebSocket connection first";

    case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:

        return "Close existing connection before opening new one";

    case UVHTTP_ERROR_WEBSOCKET_CLOSED:

        return "Re-establish WebSocket connection";

        /* Configuration errors */

    case UVHTTP_ERROR_CONFIG_PARSE:
        return "Check configuration file syntax";

    case UVHTTP_ERROR_CONFIG_INVALID:

        return "Review and correct configuration values";

    case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:

        return "Ensure configuration file exists";

    case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:

        return "Add missing required configuration";

        /* Middleware errors */

        /* Logging errors */

    case UVHTTP_ERROR_LOG_INIT:

        return "Check logging configuration";

    case UVHTTP_ERROR_LOG_WRITE:

        return "Check disk space and permissions";

    case UVHTTP_ERROR_LOG_FILE_OPEN:

        return "Verify log file path and permissions";

    case UVHTTP_ERROR_LOG_NOT_INITIALIZED:

        return "Initialize logging before use";

    default:

        return "Refer to error code documentation";
    }
}

/* Check if error is recoverable */
int uvhttp_error_is_recoverable(uvhttp_error_t error) {
    switch (error) {
    /* Retriable connection errors */
    case UVHTTP_ERROR_CONNECTION_ACCEPT:
    case UVHTTP_ERROR_CONNECTION_START:
    case UVHTTP_ERROR_CONNECTION_RESET:
    case UVHTTP_ERROR_CONNECTION_TIMEOUT:
    case UVHTTP_ERROR_CONNECTION_REFUSED:
    case UVHTTP_ERROR_CONNECTION_BROKEN:

    /* Retriable protocol errors */
    case UVHTTP_ERROR_RESPONSE_SEND:
    case UVHTTP_ERROR_TLS_HANDSHAKE:
    case UVHTTP_ERROR_WEBSOCKET_HANDSHAKE:
    case UVHTTP_ERROR_WEBSOCKET_FRAME:
    case UVHTTP_ERROR_WEBSOCKET_TOO_LARGE:
    case UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE:

    /* Retriable errors */
    case UVHTTP_ERROR_LOG_WRITE:
        return TRUE;

    /* Non-retriable errors */
    case UVHTTP_ERROR_INVALID_PARAM:
    case UVHTTP_ERROR_OUT_OF_MEMORY:
    case UVHTTP_ERROR_NOT_FOUND:
    case UVHTTP_ERROR_ALREADY_EXISTS:
    case UVHTTP_ERROR_NULL_POINTER:
    case UVHTTP_ERROR_BUFFER_TOO_SMALL:
    case UVHTTP_ERROR_SERVER_INIT:
    case UVHTTP_ERROR_SERVER_LISTEN:
    case UVHTTP_ERROR_SERVER_ALREADY_RUNNING:
    case UVHTTP_ERROR_SERVER_NOT_RUNNING:
    case UVHTTP_ERROR_SERVER_INVALID_CONFIG:
    case UVHTTP_ERROR_CONNECTION_INIT:
    case UVHTTP_ERROR_CONNECTION_CLOSE:
    case UVHTTP_ERROR_REQUEST_INIT:
    case UVHTTP_ERROR_RESPONSE_INIT:
    case UVHTTP_ERROR_INVALID_HTTP_METHOD:
    case UVHTTP_ERROR_INVALID_HTTP_VERSION:
    case UVHTTP_ERROR_HEADER_TOO_LARGE:
    case UVHTTP_ERROR_BODY_TOO_LARGE:
    case UVHTTP_ERROR_MALFORMED_REQUEST:
    case UVHTTP_ERROR_FILE_TOO_LARGE:
    case UVHTTP_ERROR_IO_ERROR:
    case UVHTTP_ERROR_TLS_INIT:
    case UVHTTP_ERROR_TLS_CONTEXT:
    case UVHTTP_ERROR_TLS_CERT_LOAD:
    case UVHTTP_ERROR_TLS_KEY_LOAD:
    case UVHTTP_ERROR_TLS_VERIFY_FAILED:
    case UVHTTP_ERROR_TLS_EXPIRED:
    case UVHTTP_ERROR_TLS_NOT_YET_VALID:
    case UVHTTP_ERROR_ROUTER_INIT:
    case UVHTTP_ERROR_ROUTER_ADD:
    case UVHTTP_ERROR_ROUTE_NOT_FOUND:
    case UVHTTP_ERROR_ROUTE_ALREADY_EXISTS:
    case UVHTTP_ERROR_INVALID_ROUTE_PATTERN:
    case UVHTTP_ERROR_ALLOCATOR_INIT:
    case UVHTTP_ERROR_ALLOCATOR_SET:
    case UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED:
    case UVHTTP_ERROR_WEBSOCKET_INIT:
    case UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED:
    case UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED:
    case UVHTTP_ERROR_WEBSOCKET_CLOSED:
    case UVHTTP_ERROR_CONFIG_PARSE:
    case UVHTTP_ERROR_CONFIG_INVALID:
    case UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND:
    case UVHTTP_ERROR_CONFIG_MISSING_REQUIRED:
    case UVHTTP_ERROR_LOG_INIT:
    case UVHTTP_ERROR_LOG_FILE_OPEN:
    case UVHTTP_ERROR_LOG_NOT_INITIALIZED:
        return FALSE;

    default:
        return FALSE;
    }
}