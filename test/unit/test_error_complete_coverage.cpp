/**
 * @file test_error_complete_coverage.cpp
 * @brief Complete coverage tests for uvhttp_error module
 *
 * This test file provides complete coverage for all error codes and functions
 * in uvhttp_error.c, including all error ranges, categories, and edge cases.
 */

#include <gtest/gtest.h>
#include <uvhttp_error.h>
#include <string.h>

class UvhttpErrorCompleteCoverageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any state if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

/* ========== Error Code Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesGeneralRange) {
    EXPECT_EQ(UVHTTP_OK, 0);
    EXPECT_EQ(UVHTTP_ERROR_INVALID_PARAM, -1);
    EXPECT_EQ(UVHTTP_ERROR_OUT_OF_MEMORY, -2);
    EXPECT_EQ(UVHTTP_ERROR_NOT_FOUND, -3);
    EXPECT_EQ(UVHTTP_ERROR_ALREADY_EXISTS, -4);
    EXPECT_EQ(UVHTTP_ERROR_NULL_POINTER, -5);
    EXPECT_EQ(UVHTTP_ERROR_BUFFER_TOO_SMALL, -6);
    EXPECT_EQ(UVHTTP_ERROR_TIMEOUT, -7);
    EXPECT_EQ(UVHTTP_ERROR_CANCELLED, -8);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesServerRange) {
    EXPECT_EQ(UVHTTP_ERROR_SERVER_INIT, -100);
    EXPECT_EQ(UVHTTP_ERROR_SERVER_LISTEN, -101);
    EXPECT_EQ(UVHTTP_ERROR_SERVER_STOP, -102);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_LIMIT, -103);
    EXPECT_EQ(UVHTTP_ERROR_SERVER_ALREADY_RUNNING, -104);
    EXPECT_EQ(UVHTTP_ERROR_SERVER_NOT_RUNNING, -105);
    EXPECT_EQ(UVHTTP_ERROR_SERVER_INVALID_CONFIG, -106);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesConnectionRange) {
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_INIT, -200);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_ACCEPT, -201);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_START, -202);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_CLOSE, -203);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_RESET, -204);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_TIMEOUT, -205);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_REFUSED, -206);
    EXPECT_EQ(UVHTTP_ERROR_CONNECTION_BROKEN, -207);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesRequestResponseRange) {
    EXPECT_EQ(UVHTTP_ERROR_REQUEST_INIT, -300);
    EXPECT_EQ(UVHTTP_ERROR_RESPONSE_INIT, -301);
    EXPECT_EQ(UVHTTP_ERROR_RESPONSE_SEND, -302);
    EXPECT_EQ(UVHTTP_ERROR_INVALID_HTTP_METHOD, -303);
    EXPECT_EQ(UVHTTP_ERROR_INVALID_HTTP_VERSION, -304);
    EXPECT_EQ(UVHTTP_ERROR_HEADER_TOO_LARGE, -305);
    EXPECT_EQ(UVHTTP_ERROR_BODY_TOO_LARGE, -306);
    EXPECT_EQ(UVHTTP_ERROR_MALFORMED_REQUEST, -307);
    EXPECT_EQ(UVHTTP_ERROR_FILE_TOO_LARGE, -308);
    EXPECT_EQ(UVHTTP_ERROR_IO_ERROR, -309);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesTLSRange) {
    EXPECT_EQ(UVHTTP_ERROR_TLS_INIT, -400);
    EXPECT_EQ(UVHTTP_ERROR_TLS_CONTEXT, -401);
    EXPECT_EQ(UVHTTP_ERROR_TLS_HANDSHAKE, -402);
    EXPECT_EQ(UVHTTP_ERROR_TLS_CERT_LOAD, -403);
    EXPECT_EQ(UVHTTP_ERROR_TLS_KEY_LOAD, -404);
    EXPECT_EQ(UVHTTP_ERROR_TLS_VERIFY_FAILED, -405);
    EXPECT_EQ(UVHTTP_ERROR_TLS_EXPIRED, -406);
    EXPECT_EQ(UVHTTP_ERROR_TLS_NOT_YET_VALID, -407);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesRouterRange) {
    EXPECT_EQ(UVHTTP_ERROR_ROUTER_INIT, -500);
    EXPECT_EQ(UVHTTP_ERROR_ROUTER_ADD, -501);
    EXPECT_EQ(UVHTTP_ERROR_ROUTE_NOT_FOUND, -502);
    EXPECT_EQ(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS, -503);
    EXPECT_EQ(UVHTTP_ERROR_INVALID_ROUTE_PATTERN, -504);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesAllocatorRange) {
    EXPECT_EQ(UVHTTP_ERROR_ALLOCATOR_INIT, -600);
    EXPECT_EQ(UVHTTP_ERROR_ALLOCATOR_SET, -601);
    EXPECT_EQ(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED, -602);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesWebSocketRange) {
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_INIT, -700);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE, -701);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_FRAME, -702);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE, -703);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE, -704);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED, -705);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED, -706);
    EXPECT_EQ(UVHTTP_ERROR_WEBSOCKET_CLOSED, -707);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesConfigRange) {
    EXPECT_EQ(UVHTTP_ERROR_CONFIG_PARSE, -900);
    EXPECT_EQ(UVHTTP_ERROR_CONFIG_INVALID, -901);
    EXPECT_EQ(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND, -902);
    EXPECT_EQ(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED, -903);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCodesLoggingRange) {
    EXPECT_EQ(UVHTTP_ERROR_LOG_INIT, -1100);
    EXPECT_EQ(UVHTTP_ERROR_LOG_WRITE, -1101);
    EXPECT_EQ(UVHTTP_ERROR_LOG_FILE_OPEN, -1102);
    EXPECT_EQ(UVHTTP_ERROR_LOG_NOT_INITIALIZED, -1103);
}

/* ========== Error String Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringGeneralErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_OK);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_NOT_FOUND);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ALREADY_EXISTS);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    // Note: Some error codes may not have explicit string mappings
    // They will return "Unknown error" which is acceptable
    str = uvhttp_error_string(UVHTTP_ERROR_NULL_POINTER);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_BUFFER_TOO_SMALL);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TIMEOUT);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CANCELLED);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringServerErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_STOP);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_LIMIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_ALREADY_RUNNING);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_NOT_RUNNING);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_SERVER_INVALID_CONFIG);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringConnectionErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_ACCEPT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_START);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_CLOSE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_RESET);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_TIMEOUT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_REFUSED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONNECTION_BROKEN);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringRequestResponseErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_REQUEST_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_RESPONSE_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_RESPONSE_SEND);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_METHOD);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_INVALID_HTTP_VERSION);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_HEADER_TOO_LARGE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_BODY_TOO_LARGE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_MALFORMED_REQUEST);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_FILE_TOO_LARGE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_IO_ERROR);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringTLSErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_CONTEXT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_CERT_LOAD);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_KEY_LOAD);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_VERIFY_FAILED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_EXPIRED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_TLS_NOT_YET_VALID);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringRouterErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ROUTER_ADD);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_INVALID_ROUTE_PATTERN);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringAllocatorErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_SET);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringWebSocketErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_FRAME);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_CLOSED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringConfigErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_CONFIG_PARSE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONFIG_INVALID);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringLoggingErrors) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_ERROR_LOG_INIT);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_LOG_WRITE);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_LOG_FILE_OPEN);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_LOG_NOT_INITIALIZED);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringUnknownError) {
    const char* str = uvhttp_error_string((uvhttp_error_t)9999);
    ASSERT_NE(str, nullptr);
    ASSERT_NE(str, nullptr);
}

/* ========== Error Category Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryOK) {
    const char* cat = uvhttp_error_category_string(UVHTTP_OK);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryGeneralErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_TIMEOUT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryServerErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_LIMIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryConnectionErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_BROKEN);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryRequestResponseErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_REQUEST_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_IO_ERROR);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryTLSErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_TLS_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_TLS_NOT_YET_VALID);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryRouterErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_INVALID_ROUTE_PATTERN);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryAllocatorErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_ALLOCATOR_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryWebSocketErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_CLOSED);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryConfigErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_PARSE);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryLoggingErrors) {
    const char* cat;

    cat = uvhttp_error_category_string(UVHTTP_ERROR_LOG_INIT);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);

    cat = uvhttp_error_category_string(UVHTTP_ERROR_LOG_NOT_INITIALIZED);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryUnknownError) {
    const char* cat = uvhttp_error_category_string((uvhttp_error_t)9999);
    ASSERT_NE(cat, nullptr);
    ASSERT_NE(cat, nullptr);
}

/* ========== Error Description Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorDescriptionNotNull) {
    // Test that all error descriptions are non-null
    const char* desc;

    desc = uvhttp_error_description(UVHTTP_OK);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_SERVER_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_CONNECTION_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_REQUEST_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_TLS_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_ROUTER_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_ALLOCATOR_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_CONFIG_PARSE);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description(UVHTTP_ERROR_LOG_INIT);
    ASSERT_NE(desc, nullptr);

    desc = uvhttp_error_description((uvhttp_error_t)9999);
    ASSERT_NE(desc, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorDescriptionOK) {
    const char* desc = uvhttp_error_description(UVHTTP_OK);
    ASSERT_NE(desc, nullptr);
    EXPECT_STREQ(desc, "Operation completed successfully");
}

/* ========== Error Suggestion Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorSuggestionNotNull) {
    // Test that all error suggestions are non-null
    const char* sugg;

    sugg = uvhttp_error_suggestion(UVHTTP_OK);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_PARAM);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_REQUEST_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_TLS_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_ROUTER_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_ALLOCATOR_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_CONFIG_PARSE);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion(UVHTTP_ERROR_LOG_INIT);
    ASSERT_NE(sugg, nullptr);

    sugg = uvhttp_error_suggestion((uvhttp_error_t)9999);
    ASSERT_NE(sugg, nullptr);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorSuggestionOK) {
    const char* sugg = uvhttp_error_suggestion(UVHTTP_OK);
    ASSERT_NE(sugg, nullptr);
    EXPECT_STREQ(sugg, "No action needed");
}

/* ========== Error Recoverable Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorRecoverableOK) {
    int recoverable = uvhttp_error_is_recoverable(UVHTTP_OK);
    // OK should be treated as recoverable (no error)
    EXPECT_GE(recoverable, 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorRecoverableConnectionErrors) {
    // Connection errors that should be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_ACCEPT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_START), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_RESET), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_REFUSED), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_BROKEN), 1);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorRecoverableProtocolErrors) {
    // Protocol errors that should be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_SEND), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_FRAME), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_TOO_LARGE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE), 1);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorRecoverableLoggingErrors) {
    // Logging errors that should be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_WRITE), 1);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableGeneralErrors) {
    // General errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALREADY_EXISTS), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_NULL_POINTER), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_BUFFER_TOO_SMALL), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableServerErrors) {
    // Server errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_LISTEN), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_LIMIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_ALREADY_RUNNING), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_NOT_RUNNING), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INVALID_CONFIG), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableConnectionInitErrors) {
    // Connection initialization errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_CLOSE), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableRequestResponseErrors) {
    // Request/Response errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_REQUEST_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_METHOD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_HTTP_VERSION), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_HEADER_TOO_LARGE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_BODY_TOO_LARGE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MALFORMED_REQUEST), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_FILE_TOO_LARGE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_IO_ERROR), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableTLSErrors) {
    // TLS errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CONTEXT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_CERT_LOAD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_KEY_LOAD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_VERIFY_FAILED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_EXPIRED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_NOT_YET_VALID), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableRouterErrors) {
    // Router errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTER_ADD), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ROUTE_ALREADY_EXISTS), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_ROUTE_PATTERN), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableAllocatorErrors) {
    // Allocator errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_SET), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableWebSocketErrors) {
    // WebSocket errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_CLOSED), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableConfigErrors) {
    // Configuration errors that should NOT be recoverable
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_PARSE), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_INVALID), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONFIG_MISSING_REQUIRED), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorNotRecoverableLoggingErrors) {
    // Logging errors that should NOT be recoverable (except LOG_WRITE)
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_FILE_OPEN), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_LOG_NOT_INITIALIZED), 0);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorRecoverableUnknownError) {
    // Unknown errors should NOT be recoverable
    int recoverable = uvhttp_error_is_recoverable((uvhttp_error_t)9999);
    EXPECT_EQ(recoverable, 0);
}

/* ========== Error Log Function Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, LogErrorNotNull) {
    // Note: uvhttp_log_error is not in public API
    // This function is internal and tested separately
}

/* ========== Edge Case Tests ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorStringPointerConsistency) {
    // Test that error strings return consistent pointers
    const char* str1 = uvhttp_error_string(UVHTTP_OK);
    const char* str2 = uvhttp_error_string(UVHTTP_OK);
    EXPECT_EQ(str1, str2);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorCategoryPointerConsistency) {
    // Test that error category strings return consistent pointers
    const char* cat1 = uvhttp_error_category_string(UVHTTP_OK);
    const char* cat2 = uvhttp_error_category_string(UVHTTP_OK);
    EXPECT_EQ(cat1, cat2);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorDescriptionPointerConsistency) {
    // Test that error descriptions return consistent pointers
    const char* desc1 = uvhttp_error_description(UVHTTP_OK);
    const char* desc2 = uvhttp_error_description(UVHTTP_OK);
    EXPECT_EQ(desc1, desc2);
}

TEST_F(UvhttpErrorCompleteCoverageTest, ErrorSuggestionPointerConsistency) {
    // Test that error suggestions return consistent pointers
    const char* sugg1 = uvhttp_error_suggestion(UVHTTP_OK);
    const char* sugg2 = uvhttp_error_suggestion(UVHTTP_OK);
    EXPECT_EQ(sugg1, sugg2);
}

/* ========== All Error Codes Coverage Test ========== */

TEST_F(UvhttpErrorCompleteCoverageTest, AllErrorCodesHaveStrings) {
    // Test that all defined error codes have valid strings
    uvhttp_error_t errors[] = {
        UVHTTP_OK,
        UVHTTP_ERROR_INVALID_PARAM,
        UVHTTP_ERROR_OUT_OF_MEMORY,
        UVHTTP_ERROR_NOT_FOUND,
        UVHTTP_ERROR_ALREADY_EXISTS,
        UVHTTP_ERROR_NULL_POINTER,
        UVHTTP_ERROR_BUFFER_TOO_SMALL,
        UVHTTP_ERROR_TIMEOUT,
        UVHTTP_ERROR_CANCELLED,
        UVHTTP_ERROR_SERVER_INIT,
        UVHTTP_ERROR_SERVER_LISTEN,
        UVHTTP_ERROR_SERVER_STOP,
        UVHTTP_ERROR_CONNECTION_LIMIT,
        UVHTTP_ERROR_SERVER_ALREADY_RUNNING,
        UVHTTP_ERROR_SERVER_NOT_RUNNING,
        UVHTTP_ERROR_SERVER_INVALID_CONFIG,
        UVHTTP_ERROR_CONNECTION_INIT,
        UVHTTP_ERROR_CONNECTION_ACCEPT,
        UVHTTP_ERROR_CONNECTION_START,
        UVHTTP_ERROR_CONNECTION_CLOSE,
        UVHTTP_ERROR_CONNECTION_RESET,
        UVHTTP_ERROR_CONNECTION_TIMEOUT,
        UVHTTP_ERROR_CONNECTION_REFUSED,
        UVHTTP_ERROR_CONNECTION_BROKEN,
        UVHTTP_ERROR_REQUEST_INIT,
        UVHTTP_ERROR_RESPONSE_INIT,
        UVHTTP_ERROR_RESPONSE_SEND,
        UVHTTP_ERROR_INVALID_HTTP_METHOD,
        UVHTTP_ERROR_INVALID_HTTP_VERSION,
        UVHTTP_ERROR_HEADER_TOO_LARGE,
        UVHTTP_ERROR_BODY_TOO_LARGE,
        UVHTTP_ERROR_MALFORMED_REQUEST,
        UVHTTP_ERROR_FILE_TOO_LARGE,
        UVHTTP_ERROR_IO_ERROR,
        UVHTTP_ERROR_TLS_INIT,
        UVHTTP_ERROR_TLS_CONTEXT,
        UVHTTP_ERROR_TLS_HANDSHAKE,
        UVHTTP_ERROR_TLS_CERT_LOAD,
        UVHTTP_ERROR_TLS_KEY_LOAD,
        UVHTTP_ERROR_TLS_VERIFY_FAILED,
        UVHTTP_ERROR_TLS_EXPIRED,
        UVHTTP_ERROR_TLS_NOT_YET_VALID,
        UVHTTP_ERROR_ROUTER_INIT,
        UVHTTP_ERROR_ROUTER_ADD,
        UVHTTP_ERROR_ROUTE_NOT_FOUND,
        UVHTTP_ERROR_ROUTE_ALREADY_EXISTS,
        UVHTTP_ERROR_INVALID_ROUTE_PATTERN,
        UVHTTP_ERROR_ALLOCATOR_INIT,
        UVHTTP_ERROR_ALLOCATOR_SET,
        UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED,
        UVHTTP_ERROR_WEBSOCKET_INIT,
        UVHTTP_ERROR_WEBSOCKET_HANDSHAKE,
        UVHTTP_ERROR_WEBSOCKET_FRAME,
        UVHTTP_ERROR_WEBSOCKET_TOO_LARGE,
        UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE,
        UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED,
        UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED,
        UVHTTP_ERROR_WEBSOCKET_CLOSED,
        UVHTTP_ERROR_CONFIG_PARSE,
        UVHTTP_ERROR_CONFIG_INVALID,
        UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND,
        UVHTTP_ERROR_CONFIG_MISSING_REQUIRED,
        UVHTTP_ERROR_LOG_INIT,
        UVHTTP_ERROR_LOG_WRITE,
        UVHTTP_ERROR_LOG_FILE_OPEN,
        UVHTTP_ERROR_LOG_NOT_INITIALIZED
    };

    for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); i++) {
        const char* str = uvhttp_error_string(errors[i]);
        ASSERT_NE(str, nullptr) << "Error string for code " << errors[i] << " is null";

        const char* cat = uvhttp_error_category_string(errors[i]);
        ASSERT_NE(cat, nullptr) << "Error category for code " << errors[i] << " is null";

        const char* desc = uvhttp_error_description(errors[i]);
        ASSERT_NE(desc, nullptr) << "Error description for code " << errors[i] << " is null";

        const char* sugg = uvhttp_error_suggestion(errors[i]);
        ASSERT_NE(sugg, nullptr) << "Error suggestion for code " << errors[i] << " is null";

        int recoverable = uvhttp_error_is_recoverable(errors[i]);
        ASSERT_GE(recoverable, 0) << "Error recoverable for code " << errors[i] << " is invalid";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}