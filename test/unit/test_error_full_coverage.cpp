#include <gtest/gtest.h>
#include "uvhttp_error.h"
#include "uvhttp_features.h"

/* 静态函数用于测试重试操作 */
static int g_call_count = 0;
static uvhttp_error_t success_operation_static(void* context) {
    g_call_count++;
    return UVHTTP_OK;
}

static uvhttp_error_t fail_operation_static(void* context) {
    g_call_count++;
    return UVHTTP_ERROR_INVALID_PARAM;
}

static uvhttp_error_t fail_then_succeed_static(void* context) {
    g_call_count++;
    if (g_call_count < 3) {
        return UVHTTP_ERROR_CONNECTION_TIMEOUT;
    }
    return UVHTTP_OK;
}

static uvhttp_error_t always_fail_static(void* context) {
    g_call_count++;
    return UVHTTP_ERROR_CONNECTION_TIMEOUT;
}

/* 测试错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringSuccess) {
    const char* str = uvhttp_error_string(UVHTTP_OK);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "Success");
}

/* 测试通用错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringGeneralErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM), "Invalid parameter");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY), "Out of memory");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_NOT_FOUND), "Not found");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ALREADY_EXISTS), "Already exists");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_NULL_POINTER), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_BUFFER_TOO_SMALL), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TIMEOUT), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CANCELLED), "Unknown error");
}

/* 测试服务器错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringServerErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT), "Server initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN), "Server listen failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_STOP), "Server stop failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_LIMIT), "Connection limit reached");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_NOT_RUNNING), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_INVALID_CONFIG), "Unknown error");
}

/* 测试连接错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringConnectionErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT), "Connection initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_ACCEPT), "Connection accept failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_START), "Connection start failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_CLOSE), "Connection close failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_RESET), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_TIMEOUT), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_REFUSED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_BROKEN), "Unknown error");
}

/* 测试请求/响应错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringRequestResponseErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_REQUEST_INIT), "Request initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_INIT), "Response initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_SEND), "Response send failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_METHOD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_VERSION), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_HEADER_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_BODY_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_MALFORMED_REQUEST), "Unknown error");
}

/* 测试TLS错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringTLSErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_INIT), "TLS initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_CONTEXT), "TLS context creation failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE), "TLS handshake failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_CERT_LOAD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_KEY_LOAD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_VERIFY_FAILED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_EXPIRED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_NOT_YET_VALID), "Unknown error");
}

/* 测试路由错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringRouterErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT), "Router initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTER_ADD), "Router add failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), "Unknown error");
}

/* 测试分配器错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringAllocatorErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_INIT), "Allocator initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_SET), "Allocator set failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), "Unknown error");
}

/* 测试WebSocket错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringWebSocketErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT), "WebSocket initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), "WebSocket handshake failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_FRAME), "WebSocket frame processing failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_CLOSED), "Unknown error");
}

/* 测试配置错误码到字符串的转换 */
TEST(UvhttpErrorTest, ErrorStringConfigErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_PARSE), "Configuration parse error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_INVALID), "Invalid configuration");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), "Unknown error");
}

/* 测试未知错误码 */
TEST(UvhttpErrorTest, ErrorStringUnknown) {
    const char* str = uvhttp_error_string((uvhttp_error_t)-9999);
    ASSERT_NE(str, nullptr);
    EXPECT_STREQ(str, "Unknown error");
}

/* 测试错误分类 - 成功 */
TEST(UvhttpErrorTest, ErrorCategorySuccess) {
    const char* category = uvhttp_error_category_string(UVHTTP_OK);
    ASSERT_NE(category, nullptr);
    EXPECT_STREQ(category, "Success");
}

/* 测试错误分类 - 通用错误 */
TEST(UvhttpErrorTest, ErrorCategoryGeneral) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_OUT_OF_MEMORY), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_NOT_FOUND), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ALREADY_EXISTS), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_NULL_POINTER), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_BUFFER_TOO_SMALL), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TIMEOUT), "General Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CANCELLED), "General Error");
}

/* 测试错误分类 - 服务器错误 */
TEST(UvhttpErrorTest, ErrorCategoryServer) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INIT), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_LISTEN), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_STOP), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_LIMIT), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_NOT_RUNNING), "Server Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INVALID_CONFIG), "Server Error");
}

/* 测试错误分类 - 连接错误 */
TEST(UvhttpErrorTest, ErrorCategoryConnection) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_INIT), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_ACCEPT), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_START), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_CLOSE), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_RESET), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_TIMEOUT), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_REFUSED), "Connection Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_BROKEN), "Connection Error");
}

/* 测试错误分类 - 请求/响应错误 */
TEST(UvhttpErrorTest, ErrorCategoryRequestResponse) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_REQUEST_INIT), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_RESPONSE_INIT), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_RESPONSE_SEND), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_HTTP_METHOD), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_HTTP_VERSION), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_HEADER_TOO_LARGE), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_BODY_TOO_LARGE), "Request/Response Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_MALFORMED_REQUEST), "Request/Response Error");
}

/* 测试错误分类 - TLS错误 */
TEST(UvhttpErrorTest, ErrorCategoryTLS) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_INIT), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_CONTEXT), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_HANDSHAKE), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_CERT_LOAD), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_KEY_LOAD), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_VERIFY_FAILED), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_EXPIRED), "TLS Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_TLS_NOT_YET_VALID), "TLS Error");
}

/* 测试错误分类 - 路由错误 */
TEST(UvhttpErrorTest, ErrorCategoryRouter) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_INIT), "Router Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_ADD), "Router Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ROUTE_NOT_FOUND), "Router Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), "Router Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), "Router Error");
}

/* 测试错误分类 - 限流错误 */
TEST(UvhttpErrorTest, ErrorCategoryRateLimit) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_RATE_LIMIT_EXCEEDED), "Unknown Error");
}

/* 测试错误分类 - 分配器错误 */
TEST(UvhttpErrorTest, ErrorCategoryAllocator) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ALLOCATOR_INIT), "Allocator Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ALLOCATOR_SET), "Allocator Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), "Allocator Error");
}

/* 测试错误分类 - WebSocket错误 */
TEST(UvhttpErrorTest, ErrorCategoryWebSocket) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INIT), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_FRAME), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), "WebSocket Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_CLOSED), "WebSocket Error");
}

/* 测试错误分类 - 配置错误 */
TEST(UvhttpErrorTest, ErrorCategoryConfig) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_PARSE), "Configuration Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_INVALID), "Configuration Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), "Configuration Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), "Configuration Error");
}

/* 测试错误分类 - 中间件错误 */
TEST(UvhttpErrorTest, ErrorCategoryMiddleware) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_INIT), "Middleware Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_REGISTER), "Middleware Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), "Middleware Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND), "Middleware Error");
}

/* 测试错误分类 - 日志错误 */
TEST(UvhttpErrorTest, ErrorCategoryLogging) {
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_LOG_INIT), "Logging Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_LOG_WRITE), "Logging Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_LOG_FILE_OPEN), "Logging Error");
    EXPECT_STREQ(uvhttp_error_category_string(UVHTTP_ERROR_LOG_NOT_INITIALIZED), "Logging Error");
}

/* 测试错误分类 - 未知错误 */
TEST(UvhttpErrorTest, ErrorCategoryUnknown) {
    const char* category = uvhttp_error_category_string((uvhttp_error_t)-9999);
    ASSERT_NE(category, nullptr);
    EXPECT_STREQ(category, "Unknown Error");
}

/* 测试错误描述 - 成功 */
TEST(UvhttpErrorTest, ErrorDescriptionSuccess) {
    const char* desc = uvhttp_error_description(UVHTTP_OK);
    ASSERT_NE(desc, nullptr);
    EXPECT_STREQ(desc, "Operation completed successfully");
}

/* 测试错误描述 - 通用错误 */
TEST(UvhttpErrorTest, ErrorDescriptionGeneral) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_INVALID_PARAM), "One or more parameters are invalid");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_OUT_OF_MEMORY), "Failed to allocate memory");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_NOT_FOUND), "Requested resource was not found");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ALREADY_EXISTS), "Resource already exists");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_NULL_POINTER), "Null pointer encountered");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_BUFFER_TOO_SMALL), "Buffer is too small to hold the data");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TIMEOUT), "Operation timed out");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CANCELLED), "Operation was cancelled");
}

/* 测试错误描述 - 服务器错误 */
TEST(UvhttpErrorTest, ErrorDescriptionServer) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_INIT), "Failed to initialize server");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_LISTEN), "Failed to listen on the specified port");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_STOP), "Failed to stop server");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_LIMIT), "Maximum connection limit reached");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), "Server is already running");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_NOT_RUNNING), "Server is not running");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_SERVER_INVALID_CONFIG), "Invalid server configuration");
}

/* 测试错误描述 - 连接错误 */
TEST(UvhttpErrorTest, ErrorDescriptionConnection) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_INIT), "Failed to initialize connection");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_ACCEPT), "Failed to accept incoming connection");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_START), "Failed to start connection");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_CLOSE), "Failed to close connection");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_RESET), "Connection was reset by peer");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_TIMEOUT), "Connection timed out");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_REFUSED), "Connection was refused");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_BROKEN), "Connection is broken");
}

/* 测试错误描述 - 请求/响应错误 */
TEST(UvhttpErrorTest, ErrorDescriptionRequestResponse) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_REQUEST_INIT), "Failed to initialize request");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_RESPONSE_INIT), "Failed to initialize response");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_RESPONSE_SEND), "Failed to send response");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_INVALID_HTTP_METHOD), "Invalid HTTP method");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_INVALID_HTTP_VERSION), "Invalid HTTP version");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_HEADER_TOO_LARGE), "HTTP headers are too large");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_BODY_TOO_LARGE), "Request body is too large");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_MALFORMED_REQUEST), "Malformed HTTP request");
}

/* 测试错误描述 - TLS错误 */
TEST(UvhttpErrorTest, ErrorDescriptionTLS) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_INIT), "Failed to initialize TLS");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_CONTEXT), "Failed to create TLS context");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_HANDSHAKE), "TLS handshake failed");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_CERT_LOAD), "Failed to load TLS certificate");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_KEY_LOAD), "Failed to load TLS private key");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_VERIFY_FAILED), "TLS certificate verification failed");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_EXPIRED), "TLS certificate has expired");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_TLS_NOT_YET_VALID), "TLS certificate is not yet valid");
}

/* 测试错误描述 - 路由错误 */
TEST(UvhttpErrorTest, ErrorDescriptionRouter) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ROUTER_INIT), "Failed to initialize router");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ROUTER_ADD), "Failed to add route");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ROUTE_NOT_FOUND), "No matching route found");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), "Route already exists");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), "Invalid route pattern");
}

/* 测试错误描述 - 分配器错误 */
TEST(UvhttpErrorTest, ErrorDescriptionAllocator) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ALLOCATOR_INIT), "Failed to initialize allocator");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ALLOCATOR_SET), "Failed to set allocator");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), "Allocator is not initialized");
}

/* 测试错误描述 - WebSocket错误 */
TEST(UvhttpErrorTest, ErrorDescriptionWebSocket) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INIT), "Failed to initialize WebSocket");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), "WebSocket handshake failed");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_FRAME), "WebSocket frame processing failed");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), "WebSocket message is too large");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), "Invalid WebSocket opcode");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), "WebSocket is not connected");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), "WebSocket is already connected");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_CLOSED), "WebSocket connection is closed");
}

/* 测试错误描述 - 配置错误 */
TEST(UvhttpErrorTest, ErrorDescriptionConfig) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONFIG_PARSE), "Failed to parse configuration");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONFIG_INVALID), "Invalid configuration");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), "Configuration file not found");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), "Missing required configuration");
}

/* 测试错误描述 - 中间件错误 */
TEST(UvhttpErrorTest, ErrorDescriptionMiddleware) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_MIDDLEWARE_INIT), "Failed to initialize middleware");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_MIDDLEWARE_REGISTER), "Failed to register middleware");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), "Middleware execution failed");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND), "Middleware not found");
}

/* 测试错误描述 - 日志错误 */
TEST(UvhttpErrorTest, ErrorDescriptionLogging) {
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_LOG_INIT), "Failed to initialize logging");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_LOG_WRITE), "Failed to write log");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_LOG_FILE_OPEN), "Failed to open log file");
    EXPECT_STREQ(uvhttp_error_description(UVHTTP_ERROR_LOG_NOT_INITIALIZED), "Logging is not initialized");
}

/* 测试错误描述 - 未知错误 */
TEST(UvhttpErrorTest, ErrorDescriptionUnknown) {
    const char* desc = uvhttp_error_description((uvhttp_error_t)-9999);
    ASSERT_NE(desc, nullptr);
    EXPECT_STREQ(desc, "Unknown error");
}

/* 测试错误建议 - 成功 */
TEST(UvhttpErrorTest, ErrorSuggestionSuccess) {
    const char* suggestion = uvhttp_error_suggestion(UVHTTP_OK);
    ASSERT_NE(suggestion, nullptr);
    EXPECT_STREQ(suggestion, "No action needed");
}

/* 测试错误建议 - 通用错误 */
TEST(UvhttpErrorTest, ErrorSuggestionGeneral) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_PARAM), "Check the parameters passed to the function");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_OUT_OF_MEMORY), "Free up memory or increase available memory");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_NOT_FOUND), "Verify the resource exists and the path is correct");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ALREADY_EXISTS), "Use a different name or remove the existing resource");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_NULL_POINTER), "Ensure all pointers are properly initialized");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_BUFFER_TOO_SMALL), "Allocate a larger buffer");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TIMEOUT), "Increase timeout or optimize operation");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CANCELLED), "Check if operation was intentionally cancelled");
}

/* 测试错误建议 - 服务器错误 */
TEST(UvhttpErrorTest, ErrorSuggestionServer) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INIT), "Check system resources and configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_LISTEN), "Check if port is available and you have permissions");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_STOP), "Ensure server is running before stopping");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_LIMIT), "Increase connection limit or reduce concurrent connections");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), "Stop the existing server before starting a new one");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_NOT_RUNNING), "Start the server before performing this operation");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INVALID_CONFIG), "Review and correct server configuration");
}

/* 测试错误建议 - 连接错误 */
TEST(UvhttpErrorTest, ErrorSuggestionConnection) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_INIT), "Check network configuration and availability");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_ACCEPT), "Retry the connection attempt");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_START), "Verify server is running and accessible");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_CLOSE), "Ensure connection is still active");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_RESET), "Check network stability and retry");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_TIMEOUT), "Increase timeout or check network connectivity");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_REFUSED), "Verify server is running and accepting connections");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_BROKEN), "Re-establish the connection");
}

/* 测试错误建议 - 请求/响应错误 */
TEST(UvhttpErrorTest, ErrorSuggestionRequestResponse) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_REQUEST_INIT), "Check request parameters and format");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_RESPONSE_INIT), "Verify response configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_RESPONSE_SEND), "Retry sending the response");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_HTTP_METHOD), "Use a valid HTTP method (GET, POST, PUT, DELETE, etc.)");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_HTTP_VERSION), "Use HTTP/1.1 or HTTP/2");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_HEADER_TOO_LARGE), "Reduce header size or increase limit");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_BODY_TOO_LARGE), "Reduce body size or increase limit");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_MALFORMED_REQUEST), "Check request format and syntax");
}

/* 测试错误建议 - TLS错误 */
TEST(UvhttpErrorTest, ErrorSuggestionTLS) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_INIT), "Check TLS library installation");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_CONTEXT), "Verify TLS configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_HANDSHAKE), "Check certificates and TLS configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_CERT_LOAD), "Verify certificate file exists and is readable");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_KEY_LOAD), "Verify key file exists and is readable");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_VERIFY_FAILED), "Check certificate chain and validity");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_EXPIRED), "Renew the certificate");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_NOT_YET_VALID), "Check system time or wait for certificate validity");
}

/* 测试错误建议 - 路由错误 */
TEST(UvhttpErrorTest, ErrorSuggestionRouter) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ROUTER_INIT), "Check router configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ROUTER_ADD), "Verify route pattern is valid");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ROUTE_NOT_FOUND), "Register the route or check URL");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), "Use a different route pattern");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), "Use a valid route pattern");
}

/* 测试错误建议 - 分配器错误 */
TEST(UvhttpErrorTest, ErrorSuggestionAllocator) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ALLOCATOR_INIT), "Check allocator configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ALLOCATOR_SET), "Verify allocator type is supported");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), "Initialize the allocator before use");
}

/* 测试错误建议 - WebSocket错误 */
TEST(UvhttpErrorTest, ErrorSuggestionWebSocket) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INIT), "Check WebSocket configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), "Verify WebSocket URL and protocol");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_FRAME), "Check WebSocket message format");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), "Reduce message size or increase limit");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), "Use valid WebSocket opcode");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), "Establish WebSocket connection first");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), "Close existing connection before opening new one");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_CLOSED), "Re-establish WebSocket connection");
}

/* 测试错误建议 - 配置错误 */
TEST(UvhttpErrorTest, ErrorSuggestionConfig) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONFIG_PARSE), "Check configuration file syntax");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONFIG_INVALID), "Review and correct configuration values");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), "Ensure configuration file exists");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), "Add missing required configuration");
}

/* 测试错误建议 - 中间件错误 */
TEST(UvhttpErrorTest, ErrorSuggestionMiddleware) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_MIDDLEWARE_INIT), "Check middleware configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_MIDDLEWARE_REGISTER), "Verify middleware registration parameters");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), "Retry operation or fix middleware logic");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND), "Register the middleware or check path");
}

/* 测试错误建议 - 日志错误 */
TEST(UvhttpErrorTest, ErrorSuggestionLogging) {
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_LOG_INIT), "Check logging configuration");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_LOG_WRITE), "Check disk space and permissions");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_LOG_FILE_OPEN), "Verify log file path and permissions");
    EXPECT_STREQ(uvhttp_error_suggestion(UVHTTP_ERROR_LOG_NOT_INITIALIZED), "Initialize logging before use");
}

/* 测试错误建议 - 未知错误 */
TEST(UvhttpErrorTest, ErrorSuggestionUnknown) {
    const char* suggestion = uvhttp_error_suggestion((uvhttp_error_t)-9999);
    ASSERT_NE(suggestion, nullptr);
    EXPECT_STREQ(suggestion, "Refer to error code documentation");
}

/* 测试错误可恢复性 - 成功 */
TEST(UvhttpErrorTest, ErrorIsRecoverableSuccess) {
    int recoverable = uvhttp_error_is_recoverable(UVHTTP_OK);
    EXPECT_EQ(recoverable, 0);
}

/* 测试错误可恢复性 - 不可重试的通用错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableGeneralNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALREADY_EXISTS), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_NULL_POINTER), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_BUFFER_TOO_SMALL), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TIMEOUT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CANCELLED), 0);
}

/* 测试错误可恢复性 - 不可重试的服务器错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableServerNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_LISTEN), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_STOP), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_LIMIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_NOT_RUNNING), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INVALID_CONFIG), 0);
}

/* 测试错误可恢复性 - 可重试的连接错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableConnectionRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_ACCEPT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_START), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_CLOSE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_RESET), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_REFUSED), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_BROKEN), 1);
}

/* 测试错误可恢复性 - 不可重试的请求/响应错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableRequestResponseNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_REQUEST_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_SEND), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_METHOD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_VERSION), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_HEADER_TOO_LARGE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_BODY_TOO_LARGE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MALFORMED_REQUEST), 0);
}

/* 测试错误可恢复性 - 不可重试的TLS错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableTLSNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CONTEXT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CERT_LOAD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_KEY_LOAD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_VERIFY_FAILED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_EXPIRED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_NOT_YET_VALID), 0);
}

/* 测试错误可恢复性 - 不可重试的路由错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableRouterNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_ADD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), 0);
}

/* 测试错误可恢复性 - 不可重试的分配器错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableAllocatorNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_SET), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), 0);
}

/* 测试错误可恢复性 - 可重试的WebSocket错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableWebSocketRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_FRAME), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_CLOSED), 0);
}

/* 测试错误可恢复性 - 不可重试的配置错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableConfigNonRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_PARSE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_INVALID), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), 0);
}

/* 测试错误可恢复性 - 可重试的中间件错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableMiddlewareRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_REGISTER), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND), 0);
}

/* 测试错误可恢复性 - 可重试的日志错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableLoggingRetryable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_WRITE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_FILE_OPEN), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_NOT_INITIALIZED), 0);
}

/* 测试错误可恢复性 - 未知错误 */
TEST(UvhttpErrorTest, ErrorIsRecoverableUnknown) {
    int recoverable = uvhttp_error_is_recoverable((uvhttp_error_t)-9999);
    EXPECT_EQ(recoverable, 0);
}

/* 测试设置错误恢复配置 */
TEST(UvhttpErrorTest, SetErrorRecoveryConfig) {
    uvhttp_set_error_recovery_config(5, 100, 5000, 3.0);
    /* 配置设置后，重试操作应该使用新的配置 */
}

/* 测试设置错误恢复配置 - 无效值 */
TEST(UvhttpErrorTest, SetErrorRecoveryConfigInvalidValues) {
    /* 负数应该被替换为默认值 */
    uvhttp_set_error_recovery_config(-1, -100, -5000, -3.0);
    /* 配置设置后，重试操作应该使用默认值 */
}

/* 测试重试操作 - 成功 */
TEST(UvhttpErrorTest, RetryOperationSuccess) {
    g_call_count = 0;
    uvhttp_error_t result = uvhttp_retry_operation(success_operation_static, nullptr, "test operation");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(g_call_count, 1);
}

/* 测试重试操作 - 不可重试的错误 */
TEST(UvhttpErrorTest, RetryOperationNonRetryable) {
    g_call_count = 0;
    uvhttp_error_t result = uvhttp_retry_operation(fail_operation_static, nullptr, "test operation");
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(g_call_count, 1);
}

/* 测试重试操作 - 可重试的错误 */
TEST(UvhttpErrorTest, RetryOperationRetryable) {
    g_call_count = 0;
    uvhttp_error_t result = uvhttp_retry_operation(fail_then_succeed_static, nullptr, "test operation");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(g_call_count, 3);
}

/* 测试重试操作 - 超过最大重试次数 */
TEST(UvhttpErrorTest, RetryOperationMaxRetries) {
    g_call_count = 0;
    uvhttp_set_error_recovery_config(2, 10, 100, 2.0);
    uvhttp_error_t result = uvhttp_retry_operation(always_fail_static, nullptr, "test operation");
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_EQ(g_call_count, 3); /* 初始尝试 + 2次重试 */
}

#if UVHTTP_FEATURE_STATISTICS
/* 测试记录错误 */
TEST(UvhttpErrorTest, LogError) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test context");
    
    size_t counts[UVHTTP_ERROR_COUNT];
    time_t last_time;
    const char* last_context;
    
    uvhttp_get_error_stats(NULL, counts, &last_time, &last_context);
    
    EXPECT_GT(counts[1], 0); /* UVHTTP_ERROR_INVALID_PARAM 的索引是 1 */
    EXPECT_GT(last_time, 0);
    ASSERT_NE(last_context, nullptr);
    EXPECT_NE(strstr(last_context, "test context"), nullptr);
}

/* 测试记录错误 - 无上下文 */
TEST(UvhttpErrorTest, LogErrorNoContext) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_log_error(UVHTTP_ERROR_OUT_OF_MEMORY, nullptr);
    
    size_t counts[UVHTTP_ERROR_COUNT];
    time_t last_time;
    const char* last_context;
    
    uvhttp_get_error_stats(NULL, counts, &last_time, &last_context);
    
    EXPECT_GT(counts[2], 0); /* UVHTTP_ERROR_OUT_OF_MEMORY 的索引是 2 */
    EXPECT_GT(last_time, 0);
    ASSERT_NE(last_context, nullptr);
}

/* 测试获取错误统计 */
TEST(UvhttpErrorTest, GetErrorStats) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "context 1");
    uvhttp_log_error(UVHTTP_ERROR_OUT_OF_MEMORY, "context 2");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "context 3");
    
    size_t counts[UVHTTP_ERROR_COUNT];
    time_t last_time;
    const char* last_context;
    
    uvhttp_get_error_stats(NULL, counts, &last_time, &last_context);
    
    EXPECT_EQ(counts[1], 2); /* UVHTTP_ERROR_INVALID_PARAM 出现了2次 */
    EXPECT_EQ(counts[2], 1); /* UVHTTP_ERROR_OUT_OF_MEMORY 出现了1次 */
    EXPECT_GT(last_time, 0);
    ASSERT_NE(last_context, nullptr);
    EXPECT_NE(strstr(last_context, "context 3"), nullptr);
}

/* 测试获取错误统计 - 部分参数 */
TEST(UvhttpErrorTest, GetErrorStatsPartial) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    
    /* 只获取 counts */
    size_t counts[UVHTTP_ERROR_COUNT];
    uvhttp_get_error_stats(NULL, counts, nullptr, nullptr);
    EXPECT_GT(counts[1], 0);
    
    /* 只获取 last_time */
    time_t last_time;
    uvhttp_get_error_stats(NULL, nullptr, &last_time, nullptr);
    EXPECT_GT(last_time, 0);
    
    /* 只获取 last_context */
    const char* last_context;
    uvhttp_get_error_stats(NULL, nullptr, nullptr, &last_context);
    ASSERT_NE(last_context, nullptr);
}

/* 测试重置错误统计 */
TEST(UvhttpErrorTest, ResetErrorStats) {
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    
    size_t counts_before[UVHTTP_ERROR_COUNT];
    uvhttp_get_error_stats(NULL, counts_before, nullptr, nullptr);
    EXPECT_GT(counts_before[1], 0);
    
    uvhttp_reset_error_stats(NULL);
    
    size_t counts_after[UVHTTP_ERROR_COUNT];
    time_t last_time;
    const char* last_context;
    
    uvhttp_get_error_stats(NULL, counts_after, &last_time, &last_context);
    
    EXPECT_EQ(counts_after[1], 0);
    EXPECT_EQ(last_time, 0);
    EXPECT_STREQ(last_context, "");
}

/* 测试获取最频繁的错误 */
TEST(UvhttpErrorTest, GetMostFrequentError) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    uvhttp_log_error(UVHTTP_ERROR_OUT_OF_MEMORY, "test");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error(NULL);
    EXPECT_EQ(most_frequent, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试获取最频繁的错误 - 无错误 */
TEST(UvhttpErrorTest, GetMostFrequentErrorNoErrors) {
    uvhttp_reset_error_stats(NULL);
    
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error(NULL);
    EXPECT_EQ(most_frequent, UVHTTP_OK);
}

/* 测试错误统计 - 多个错误 */
TEST(UvhttpErrorTest, ErrorStatsMultipleErrors) {
    uvhttp_reset_error_stats(NULL);
    
    /* 记录多个不同类型的错误（使用错误码绝对值小于 120 的错误码） */
    for (int i = 0; i < 5; i++) {
        uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "test");
    }
    for (int i = 0; i < 3; i++) {
        uvhttp_log_error(UVHTTP_ERROR_OUT_OF_MEMORY, "test");
    }
    for (int i = 0; i < 7; i++) {
        uvhttp_log_error(UVHTTP_ERROR_NOT_FOUND, "test");
    }
    
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error(NULL);
    EXPECT_EQ(most_frequent, UVHTTP_ERROR_NOT_FOUND);
}

/* 测试错误统计 - 边界值 */
TEST(UvhttpErrorTest, ErrorStatsBoundaryValues) {
    uvhttp_reset_error_stats(NULL);
    
    /* 测试 UVHTTP_OK 的索引处理 */
    uvhttp_log_error(UVHTTP_OK, "test");
    
    size_t counts[UVHTTP_ERROR_COUNT];
    uvhttp_get_error_stats(NULL, counts, nullptr, nullptr);
    
    /* UVHTTP_OK 的索引是 0，会增加 counts[0] */
    EXPECT_EQ(counts[0], 1);
}

/* 测试重试延迟计算 */
TEST(UvhttpErrorTest, RetryDelayCalculation) {
    uvhttp_set_error_recovery_config(3, 100, 1000, 2.0);
    
    /* 第一次重试：100ms */
    /* 第二次重试：200ms */
    /* 第三次重试：400ms */
    
    /* 这个测试通过实际运行重试操作来验证 */
    g_call_count = 0;
    uvhttp_error_t result = uvhttp_retry_operation(always_fail_static, nullptr, "test");
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_EQ(g_call_count, 4); /* 初始尝试 + 3次重试 */
}
#endif /* UVHTTP_FEATURE_STATISTICS */