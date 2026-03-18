/**
 * @file test_error_basic_coverage.cpp
 * @brief Basic coverage tests for uvhttp_error module
 * 
 * This test file aims to improve coverage for uvhttp_error.c by testing:
 * - Error code to string conversion
 * - Error categorization
 * - Error recoverability checking
 * - All error code branches
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uvhttp_error.h"

class UvhttpErrorBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Error string tests
TEST_F(UvhttpErrorBasicTest, ErrorStringSuccess) {
    const char* str = uvhttp_error_string(UVHTTP_OK);
    EXPECT_STREQ(str, "Success");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringGeneralErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM), "Invalid parameter");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY), "Out of memory");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_NOT_FOUND), "Not found");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ALREADY_EXISTS), "Already exists");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_NULL_POINTER), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_BUFFER_TOO_SMALL), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TIMEOUT), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CANCELLED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_NOT_SUPPORTED), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringServerErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT), "Server initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN), "Server listen failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_STOP), "Server stop failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_LIMIT), "Connection limit reached");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_NOT_RUNNING), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_SERVER_INVALID_CONFIG), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringConnectionErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT), "Connection initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_ACCEPT), "Connection accept failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_START), "Connection start failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_CLOSE), "Connection close failed");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_RESET), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_TIMEOUT), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_REFUSED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_BROKEN), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringRequestResponseErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_REQUEST_INIT), "Request initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_INIT), "Response initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_SEND), "Response send failed");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_METHOD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_VERSION), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_HEADER_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_BODY_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_MALFORMED_REQUEST), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringFileErrors) {
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_FILE_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_IO_ERROR), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringTLSErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_INIT), "TLS initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_CONTEXT), "TLS context creation failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE), "TLS handshake failed");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_CERT_LOAD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_KEY_LOAD), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_VERIFY_FAILED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_EXPIRED), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_TLS_NOT_YET_VALID), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringRouterErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT), "Router initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTER_ADD), "Router add failed");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringWebSocketErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT), "WebSocket initialization failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), "WebSocket handshake failed");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_FRAME), "WebSocket frame processing failed");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringConfigErrors) {
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_PARSE), "Configuration parse error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_INVALID), "Invalid configuration");
    // Note: These error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringLogErrors) {
    // Note: All log error codes are not implemented in uvhttp_error_string() and will return "Unknown error"
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_LOG_INIT), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_LOG_WRITE), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_LOG_FILE_OPEN), "Unknown error");
    EXPECT_STREQ(uvhttp_error_string(UVHTTP_ERROR_LOG_NOT_INITIALIZED), "Unknown error");
}

TEST_F(UvhttpErrorBasicTest, ErrorStringUnknown) {
    const char* str = uvhttp_error_string((uvhttp_error_t)99999);
    EXPECT_STREQ(str, "Unknown error");
}

// Error recoverability tests
TEST_F(UvhttpErrorBasicTest, ErrorRecoverableConnectionErrors) {
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_ACCEPT));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_START));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_RESET));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_REFUSED));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_BROKEN));
}

TEST_F(UvhttpErrorBasicTest, ErrorRecoverableProtocolErrors) {
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_SEND));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_FRAME));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE));
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE));
}

TEST_F(UvhttpErrorBasicTest, ErrorRecoverableLogError) {
    EXPECT_TRUE(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_WRITE));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableGeneralErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_NOT_FOUND));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALREADY_EXISTS));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_NULL_POINTER));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_BUFFER_TOO_SMALL));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableServerErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_LISTEN));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_ALREADY_RUNNING));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_NOT_RUNNING));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INVALID_CONFIG));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableConnectionInit) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_CLOSE));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableRequestResponse) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_REQUEST_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_INIT));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableProtocolErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_METHOD));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_VERSION));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_HEADER_TOO_LARGE));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_BODY_TOO_LARGE));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_MALFORMED_REQUEST));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableFileErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_FILE_TOO_LARGE));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_IO_ERROR));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableTLSErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CONTEXT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CERT_LOAD));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_KEY_LOAD));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_VERIFY_FAILED));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_EXPIRED));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_NOT_YET_VALID));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableRouterErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_ADD));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_NOT_FOUND));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableConfigErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_PARSE));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_INVALID));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableLogErrors) {
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_INIT));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_FILE_OPEN));
    EXPECT_FALSE(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_NOT_INITIALIZED));
}

TEST_F(UvhttpErrorBasicTest, ErrorNotRecoverableUnknown) {
    EXPECT_FALSE(uvhttp_error_is_recoverable((uvhttp_error_t)99999));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}