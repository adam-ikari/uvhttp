/* uvhttp_error.c API 覆盖率测试 - 测试所有错误码和错误处理函数 */

#include <gtest/gtest.h>
#include "uvhttp_error.h"
#include <string.h>
#include <limits.h>

/* ========== 测试所有通用错误码 ========== */

TEST(UvhttpErrorApiTest, GeneralErrorCodes) {
    EXPECT_EQ((int)UVHTTP_OK, 0);
    EXPECT_EQ((int)UVHTTP_ERROR_INVALID_PARAM, -1);
    EXPECT_EQ((int)UVHTTP_ERROR_OUT_OF_MEMORY, -2);
    EXPECT_EQ((int)UVHTTP_ERROR_NOT_FOUND, -3);
    EXPECT_EQ((int)UVHTTP_ERROR_ALREADY_EXISTS, -4);
    EXPECT_EQ((int)UVHTTP_ERROR_NULL_POINTER, -5);
    EXPECT_EQ((int)UVHTTP_ERROR_BUFFER_TOO_SMALL, -6);
    EXPECT_EQ((int)UVHTTP_ERROR_TIMEOUT, -7);
    EXPECT_EQ((int)UVHTTP_ERROR_CANCELLED, -8);
    EXPECT_EQ((int)UVHTTP_ERROR_NOT_SUPPORTED, -9);
}

/* ========== 测试所有服务器错误码 ========== */

TEST(UvhttpErrorApiTest, ServerErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_INIT, -100);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_LISTEN, -101);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_STOP, -102);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_LIMIT, -103);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_ALREADY_RUNNING, -104);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_NOT_RUNNING, -105);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_INVALID_CONFIG, -106);
}

/* ========== 测试所有连接错误码 ========== */

TEST(UvhttpErrorApiTest, ConnectionErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_INIT, -200);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_ACCEPT, -201);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_START, -202);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_CLOSE, -203);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_RESET, -204);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_TIMEOUT, -205);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_REFUSED, -206);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_BROKEN, -207);
}

/* ========== 测试所有请求/响应错误码 ========== */

TEST(UvhttpErrorApiTest, RequestResponseErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_REQUEST_INIT, -300);
    EXPECT_EQ((int)UVHTTP_ERROR_RESPONSE_INIT, -301);
    EXPECT_EQ((int)UVHTTP_ERROR_RESPONSE_SEND, -302);
    EXPECT_EQ((int)UVHTTP_ERROR_INVALID_HTTP_METHOD, -303);
    EXPECT_EQ((int)UVHTTP_ERROR_INVALID_HTTP_VERSION, -304);
    EXPECT_EQ((int)UVHTTP_ERROR_HEADER_TOO_LARGE, -305);
    EXPECT_EQ((int)UVHTTP_ERROR_BODY_TOO_LARGE, -306);
    EXPECT_EQ((int)UVHTTP_ERROR_MALFORMED_REQUEST, -307);
    EXPECT_EQ((int)UVHTTP_ERROR_FILE_TOO_LARGE, -308);
    EXPECT_EQ((int)UVHTTP_ERROR_IO_ERROR, -309);
}

/* ========== 测试所有 TLS 错误码 ========== */

TEST(UvhttpErrorApiTest, TlsErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_INIT, -400);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_CONTEXT, -401);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_HANDSHAKE, -402);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_CERT_LOAD, -403);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_KEY_LOAD, -404);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_VERIFY_FAILED, -405);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_EXPIRED, -406);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_NOT_YET_VALID, -407);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_CERT, -408);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_KEY, -409);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_CA, -410);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_VERIFY, -411);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_READ, -412);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_WRITE, -413);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_INVALID_PARAM, -414);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_MEMORY, -415);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_NOT_IMPLEMENTED, -416);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_PARSE, -417);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_NO_CERT, -418);
    /* TLS 非阻塞状态（正数） */
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_WANT_READ, 1);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_WANT_WRITE, 2);
}

/* ========== 测试所有路由器错误码 ========== */

TEST(UvhttpErrorApiTest, RouterErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_ROUTER_INIT, -500);
    EXPECT_EQ((int)UVHTTP_ERROR_ROUTER_ADD, -501);
    EXPECT_EQ((int)UVHTTP_ERROR_ROUTE_NOT_FOUND, -502);
    EXPECT_EQ((int)UVHTTP_ERROR_ROUTE_ALREADY_EXISTS, -503);
    EXPECT_EQ((int)UVHTTP_ERROR_INVALID_ROUTE_PATTERN, -504);
}

/* ========== 测试限流错误码 ========== */

TEST(UvhttpErrorApiTest, RateLimitErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_RATE_LIMIT_EXCEEDED, -550);
}

/* ========== 测试分配器错误码 ========== */

TEST(UvhttpErrorApiTest, AllocatorErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_ALLOCATOR_INIT, -600);
    EXPECT_EQ((int)UVHTTP_ERROR_ALLOCATOR_SET, -601);
    EXPECT_EQ((int)UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED, -602);
}

/* ========== 测试 WebSocket 错误码 ========== */

TEST(UvhttpErrorApiTest, WebSocketErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_INIT, -700);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_HANDSHAKE, -701);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_FRAME, -702);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_TOO_LARGE, -703);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE, -704);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED, -705);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED, -706);
    EXPECT_EQ((int)UVHTTP_ERROR_WEBSOCKET_CLOSED, -707);
}

/* ========== 测试配置错误码 ========== */

TEST(UvhttpErrorApiTest, ConfigErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_CONFIG_PARSE, -900);
    EXPECT_EQ((int)UVHTTP_ERROR_CONFIG_INVALID, -901);
    EXPECT_EQ((int)UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND, -902);
    EXPECT_EQ((int)UVHTTP_ERROR_CONFIG_MISSING_REQUIRED, -903);
}

/* ========== 测试日志错误码 ========== */

TEST(UvhttpErrorApiTest, LogErrorCodes) {
    EXPECT_EQ((int)UVHTTP_ERROR_LOG_INIT, -1100);
    EXPECT_EQ((int)UVHTTP_ERROR_LOG_WRITE, -1101);
    EXPECT_EQ((int)UVHTTP_ERROR_LOG_FILE_OPEN, -1102);
    EXPECT_EQ((int)UVHTTP_ERROR_LOG_NOT_INITIALIZED, -1103);
}

/* ========== 测试 uvhttp_error_string 函数 ========== */

TEST(UvhttpErrorApiTest, ErrorStringOk) {
    const char* str = uvhttp_error_string(UVHTTP_OK);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringGeneralErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_TIMEOUT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringServerErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringConnectionErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringTlsErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_TLS_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringRouterErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringWebSocketErrors) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringUnknownError) {
    const char* str = uvhttp_error_string((uvhttp_error_t)9999);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorStringExtremeValues) {
    const char* str = uvhttp_error_string((uvhttp_error_t)INT_MAX);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_string((uvhttp_error_t)INT_MIN);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

/* ========== 测试 uvhttp_error_category_string 函数 ========== */

TEST(UvhttpErrorApiTest, ErrorCategoryStringOk) {
    const char* str = uvhttp_error_category_string(UVHTTP_OK);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringGeneralErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringServerErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_SERVER_LISTEN);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringConnectionErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringTlsErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_TLS_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_TLS_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringRouterErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringWebSocketErrors) {
    const char* str = uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorCategoryStringUnknownError) {
    const char* str = uvhttp_error_category_string((uvhttp_error_t)9999);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

/* ========== 测试 uvhttp_error_description 函数 ========== */

TEST(UvhttpErrorApiTest, ErrorDescriptionOk) {
    const char* str = uvhttp_error_description(UVHTTP_OK);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionGeneralErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionServerErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_SERVER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_SERVER_LISTEN);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionConnectionErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionTlsErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_TLS_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_TLS_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionRouterErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_ROUTER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionWebSocketErrors) {
    const char* str = uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorDescriptionUnknownError) {
    const char* str = uvhttp_error_description((uvhttp_error_t)9999);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

/* ========== 测试 uvhttp_error_suggestion 函数 ========== */

TEST(UvhttpErrorApiTest, ErrorSuggestionOk) {
    const char* str = uvhttp_error_suggestion(UVHTTP_OK);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionGeneralErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionServerErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_LISTEN);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionConnectionErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionTlsErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionRouterErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_ROUTER_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionWebSocketErrors) {
    const char* str = uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INIT);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

TEST(UvhttpErrorApiTest, ErrorSuggestionUnknownError) {
    const char* str = uvhttp_error_suggestion((uvhttp_error_t)9999);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
}

/* ========== 测试 uvhttp_error_is_recoverable 函数 ========== */

TEST(UvhttpErrorApiTest, ErrorIsRecoverableOk) {
    int result = uvhttp_error_is_recoverable(UVHTTP_OK);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableGeneralErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_TIMEOUT);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableServerErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_LISTEN);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableConnectionErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableTlsErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_INIT);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableRouterErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_INIT);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableWebSocketErrors) {
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INIT);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableUnknownError) {
    int result = uvhttp_error_is_recoverable((uvhttp_error_t)9999);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, ErrorIsRecoverableExtremeValues) {
    int result = uvhttp_error_is_recoverable((uvhttp_error_t)INT_MAX);
    EXPECT_GE(result, 0);
    
    result = uvhttp_error_is_recoverable((uvhttp_error_t)INT_MIN);
    EXPECT_GE(result, 0);
}

/* ========== 测试 TLS 非阻塞状态（正数错误码） ========== */

TEST(UvhttpErrorApiTest, TlsWantRead) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_WANT_READ);
    EXPECT_GE(result, 0);
}

TEST(UvhttpErrorApiTest, TlsWantWrite) {
    const char* str = uvhttp_error_string(UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_category_string(UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_description(UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    str = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_NE(str, nullptr);
    EXPECT_NE(strlen(str), (size_t)0);
    
    int result = uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_WANT_WRITE);
    EXPECT_GE(result, 0);
}