/* uvhttp_utils.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <uv.h>
#include "uvhttp_utils.h"
#include "uvhttp_response.h"
#include "uvhttp_request.h"
#include "uvhttp_connection.h"

static uvhttp_response_t* create_test_response(void) {
    uv_loop_t* loop = uv_default_loop();
    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    if (!client) return nullptr;

    uv_tcp_init(loop, client);

    uvhttp_response_t* resp = (uvhttp_response_t*)malloc(sizeof(uvhttp_response_t));
    if (!resp) {
        uv_close((uv_handle_t*)client, nullptr);
        free(client);
        return nullptr;
    }

    uvhttp_response_init(resp, client);
    return resp;
}

static void destroy_test_response(uvhttp_response_t* resp) {
    if (!resp) return;

    uv_tcp_t* client = resp->client;
    uvhttp_response_free(resp);

    if (client) {
        uv_close((uv_handle_t*)client, nullptr);
        free(client);
    }
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyNormal) {
    char dest[100];
    const char* src = "Hello, World!";

    EXPECT_EQ(uvhttp_safe_strncpy(dest, src, sizeof(dest)), 0);
    EXPECT_STREQ(dest, src);
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyExactSize) {
    char dest[14];
    const char* src = "Hello, World!";

    EXPECT_EQ(uvhttp_safe_strncpy(dest, src, sizeof(dest)), 0);
    EXPECT_STREQ(dest, src);
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyTruncate) {
    char dest[10];
    const char* src = "Hello, World!";

    EXPECT_EQ(uvhttp_safe_strncpy(dest, src, sizeof(dest)), 0);
    EXPECT_STREQ(dest, "Hello, Wo");
    EXPECT_EQ(dest[9], '\0');
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyNullDest) {
    char dest[100];
    const char* src = "Hello, World!";

    EXPECT_EQ(uvhttp_safe_strncpy(nullptr, src, sizeof(dest)), -1);
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyNullSrc) {
    char dest[100];

    EXPECT_EQ(uvhttp_safe_strncpy(dest, nullptr, sizeof(dest)), -1);
}

TEST(UvhttpUtilsFullCoverageTest, SafeStrncpyZeroSize) {
    char dest[100];
    const char* src = "Hello, World!";

    EXPECT_EQ(uvhttp_safe_strncpy(dest, src, 0), -1);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseNormal) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* content = "Hello, World!";
    (void)uvhttp_send_unified_response(resp, content, strlen(content), 200);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseNullResponse) {
    const char* content = "Hello, World!";
    EXPECT_EQ(uvhttp_send_unified_response(nullptr, content, strlen(content), 200), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseNullContent) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(uvhttp_send_unified_response(resp, nullptr, 0, 200), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseInvalidStatus) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* content = "Hello, World!";
    EXPECT_EQ(uvhttp_send_unified_response(resp, content, strlen(content), 999), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseAutoLength) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* content = "Hello, World!";
    (void)uvhttp_send_unified_response(resp, content, 0, 200);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendUnifiedResponseZeroLength) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* content = "";
    EXPECT_EQ(uvhttp_send_unified_response(resp, content, 0, 200), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseNormal) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* error_message = "Not Found";
    const char* details = "Resource does not exist";
    (void)uvhttp_send_error_response(resp, 404, error_message, details);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseNullResponse) {
    const char* error_message = "Not Found";
    EXPECT_EQ(uvhttp_send_error_response(nullptr, 404, error_message, nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseNullMessage) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(uvhttp_send_error_response(resp, 404, nullptr, nullptr), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseInvalidCode) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* error_message = "Not Found";
    EXPECT_EQ(uvhttp_send_error_response(resp, 999, error_message, nullptr), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseLongMessage) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    char long_message[300];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';

    EXPECT_EQ(uvhttp_send_error_response(resp, 404, long_message, nullptr), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseLongDetails) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* error_message = "Not Found";
    char long_details[500];
    memset(long_details, 'B', sizeof(long_details) - 1);
    long_details[sizeof(long_details) - 1] = '\0';

    EXPECT_EQ(uvhttp_send_error_response(resp, 404, error_message, long_details), UVHTTP_ERROR_INVALID_PARAM);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, SendErrorResponseNoDetails) {
    uvhttp_response_t* resp = create_test_response();
    ASSERT_NE(resp, nullptr);

    const char* error_message = "Not Found";
    (void)uvhttp_send_error_response(resp, 404, error_message, nullptr);

    destroy_test_response(resp);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidStatusCodeValid) {
    EXPECT_EQ(uvhttp_is_valid_status_code(100), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(200), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(301), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(404), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(500), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(599), TRUE);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidStatusCodeInvalid) {
    EXPECT_EQ(uvhttp_is_valid_status_code(99), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(600), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(0), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(-1), FALSE);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidContentTypeValid) {
    EXPECT_EQ(uvhttp_is_valid_content_type("text/html"), TRUE);
    EXPECT_EQ(uvhttp_is_valid_content_type("application/json"), TRUE);
    EXPECT_EQ(uvhttp_is_valid_content_type("image/png"), TRUE);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidContentTypeInvalid) {
    EXPECT_EQ(uvhttp_is_valid_content_type(nullptr), FALSE);
    EXPECT_EQ(uvhttp_is_valid_content_type(""), FALSE);
    EXPECT_EQ(uvhttp_is_valid_content_type("text"), FALSE);
    EXPECT_EQ(uvhttp_is_valid_content_type("text/html\""), FALSE);
    EXPECT_EQ(uvhttp_is_valid_content_type("text/html,"), FALSE);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidStringLengthValid) {
    EXPECT_EQ(uvhttp_is_valid_string_length("Hello", 10), TRUE);
    EXPECT_EQ(uvhttp_is_valid_string_length("Hello", 5), TRUE);
    EXPECT_EQ(uvhttp_is_valid_string_length("", 10), TRUE);
}

TEST(UvhttpUtilsFullCoverageTest, IsValidStringLengthInvalid) {
    EXPECT_EQ(uvhttp_is_valid_string_length(nullptr, 10), FALSE);
    EXPECT_EQ(uvhttp_is_valid_string_length("Hello", 4), FALSE);
}

TEST(UvhttpUtilsFullCoverageTest, EdgeCases) {
    EXPECT_EQ(uvhttp_is_valid_status_code(100), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(599), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(101), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(598), TRUE);
    
    EXPECT_EQ(uvhttp_is_valid_string_length("A", 1), TRUE);
    EXPECT_EQ(uvhttp_is_valid_string_length("AB", 1), FALSE);
    
    EXPECT_EQ(uvhttp_is_valid_content_type("a/b"), TRUE);
    EXPECT_EQ(uvhttp_is_valid_content_type("application/vnd.api+json"), TRUE);
}
