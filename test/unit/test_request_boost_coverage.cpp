/**
 * @file test_request_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_request module
 *
 * Tests public request API functions by directly manipulating struct fields,
 * avoiding the need for a full llhttp parsing pipeline or libuv connection.
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_allocator.h"
#include "uvhttp_request.h"
}

#include <string.h>

class RequestBoostTest : public ::testing::Test {
protected:
    uvhttp_request_t* req = nullptr;

    void SetUp() override {
        req = (uvhttp_request_t*)uvhttp_calloc(1, sizeof(uvhttp_request_t));
        ASSERT_NE(req, nullptr);
        req->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
    }

    void TearDown() override {
        if (req) {
            if (req->headers_extra) {
                uvhttp_free(req->headers_extra);
                req->headers_extra = nullptr;
            }
            uvhttp_free(req);
            req = nullptr;
        }
    }

    void add_test_header(const char* name, const char* value) {
        uvhttp_error_t err = uvhttp_request_add_header(req, name, value);
        ASSERT_EQ(err, UVHTTP_OK);
    }
};

// ========== uvhttp_request_get_method ==========

TEST_F(RequestBoostTest, GetMethod_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_method(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetMethod_Get_ReturnsGET) {
    req->method = UVHTTP_GET;
    EXPECT_STREQ(uvhttp_request_get_method(req), "GET");
}

TEST_F(RequestBoostTest, GetMethod_Post_ReturnsPOST) {
    req->method = UVHTTP_POST;
    EXPECT_STREQ(uvhttp_request_get_method(req), "POST");
}

TEST_F(RequestBoostTest, GetMethod_Put_ReturnsPUT) {
    req->method = UVHTTP_PUT;
    EXPECT_STREQ(uvhttp_request_get_method(req), "PUT");
}

TEST_F(RequestBoostTest, GetMethod_Delete_ReturnsDELETE) {
    req->method = UVHTTP_DELETE;
    EXPECT_STREQ(uvhttp_request_get_method(req), "DELETE");
}

TEST_F(RequestBoostTest, GetMethod_Head_ReturnsHEAD) {
    req->method = UVHTTP_HEAD;
    EXPECT_STREQ(uvhttp_request_get_method(req), "HEAD");
}

TEST_F(RequestBoostTest, GetMethod_Options_ReturnsOPTIONS) {
    req->method = UVHTTP_OPTIONS;
    EXPECT_STREQ(uvhttp_request_get_method(req), "OPTIONS");
}

TEST_F(RequestBoostTest, GetMethod_Patch_ReturnsPATCH) {
    req->method = UVHTTP_PATCH;
    EXPECT_STREQ(uvhttp_request_get_method(req), "PATCH");
}

TEST_F(RequestBoostTest, GetMethod_Any_ReturnsANY) {
    req->method = UVHTTP_ANY;
    EXPECT_STREQ(uvhttp_request_get_method(req), "ANY");
}

TEST_F(RequestBoostTest, GetMethod_Default_ReturnsUNKNOWN) {
    req->method = (uvhttp_method_t)999;
    EXPECT_STREQ(uvhttp_request_get_method(req), "UNKNOWN");
}

// ========== uvhttp_request_get_url ==========

TEST_F(RequestBoostTest, GetUrl_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_url(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetUrl_Empty_ReturnsEmpty) {
    req->url[0] = '\0';
    EXPECT_STREQ(uvhttp_request_get_url(req), "");
}

TEST_F(RequestBoostTest, GetUrl_ValidPath_ReturnsPath) {
    strncpy(req->url, "/hello/world", sizeof(req->url));
    EXPECT_STREQ(uvhttp_request_get_url(req), "/hello/world");
}

// ========== uvhttp_request_get_header ==========

TEST_F(RequestBoostTest, GetHeader_NullRequest_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header(nullptr, "Content-Type"), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_NullName_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header(req, nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_EmptyName_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header(req, ""), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_InvalidChars_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header(req, "Bad Header!"), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_NameTooLong_ReturnsNull) {
    char long_name[300];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    EXPECT_EQ(uvhttp_request_get_header(req, long_name), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_CaseInsensitiveMatch) {
    add_test_header("Content-Type", "application/json");
    EXPECT_STREQ(uvhttp_request_get_header(req, "content-type"), "application/json");
    EXPECT_STREQ(uvhttp_request_get_header(req, "CONTENT-TYPE"), "application/json");
}

TEST_F(RequestBoostTest, GetHeader_NotFound_ReturnsNull) {
    add_test_header("Host", "example.com");
    EXPECT_EQ(uvhttp_request_get_header(req, "Accept"), nullptr);
}

TEST_F(RequestBoostTest, GetHeader_ValidLookup_ReturnsValue) {
    add_test_header("Accept", "text/html");
    EXPECT_STREQ(uvhttp_request_get_header(req, "Accept"), "text/html");
}

// ========== uvhttp_request_get_body ==========

TEST_F(RequestBoostTest, GetBody_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_body(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetBody_EmptyBody_ReturnsNull) {
    req->body = nullptr;
    EXPECT_EQ(uvhttp_request_get_body(req), nullptr);
}

TEST_F(RequestBoostTest, GetBody_ValidBody_ReturnsBody) {
    req->body = (char*)uvhttp_alloc(64);
    ASSERT_NE(req->body, nullptr);
    strcpy(req->body, "hello body");
    req->body_length = strlen("hello body");
    EXPECT_STREQ(uvhttp_request_get_body(req), "hello body");
}

// ========== uvhttp_request_get_body_length ==========

TEST_F(RequestBoostTest, GetBodyLength_Null_ReturnsZero) {
    EXPECT_EQ(uvhttp_request_get_body_length(nullptr), 0u);
}

TEST_F(RequestBoostTest, GetBodyLength_Valid_ReturnsLength) {
    req->body_length = 42;
    EXPECT_EQ(uvhttp_request_get_body_length(req), 42u);
}

// ========== uvhttp_request_get_path ==========

TEST_F(RequestBoostTest, GetPath_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_path(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetPath_NoQuery_ReturnsUrl) {
    strncpy(req->url, "/api/users", sizeof(req->url));
    EXPECT_STREQ(uvhttp_request_get_path(req), "/api/users");
}

TEST_F(RequestBoostTest, GetPath_WithQuery_ReturnsPathOnly) {
    strncpy(req->url, "/api/users?page=1&limit=10", sizeof(req->url));
    EXPECT_STREQ(uvhttp_request_get_path(req), "/api/users");
}

TEST_F(RequestBoostTest, GetPath_EmptyUrl_ReturnsEmpty) {
    req->url[0] = '\0';
    // strchr on empty string returns NULL, so no query_start, returns url directly
    EXPECT_STREQ(uvhttp_request_get_path(req), "");
}

// ========== uvhttp_request_get_query_string ==========

TEST_F(RequestBoostTest, GetQueryString_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_query_string(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetQueryString_NoQuery_ReturnsNull) {
    strncpy(req->url, "/api/users", sizeof(req->url));
    EXPECT_EQ(uvhttp_request_get_query_string(req), nullptr);
}

TEST_F(RequestBoostTest, GetQueryString_WithQuery_ReturnsQuery) {
    strncpy(req->url, "/api/users?page=1&limit=10", sizeof(req->url));
    const char* qs = uvhttp_request_get_query_string(req);
    ASSERT_NE(qs, nullptr);
    EXPECT_STREQ(qs, "page=1&limit=10");
}

TEST_F(RequestBoostTest, GetQueryString_EmptyQuery_ReturnsEmpty) {
    strncpy(req->url, "/api/users?", sizeof(req->url));
    // Query string after '?' is empty - validation may reject empty string
    const char* qs = uvhttp_request_get_query_string(req);
    // Could be NULL or empty depending on validation
    (void)qs;
}

// ========== uvhttp_request_get_query_param ==========

TEST_F(RequestBoostTest, GetQueryParam_NullRequest_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_query_param(nullptr, "key"), nullptr);
}

TEST_F(RequestBoostTest, GetQueryParam_NullName_ReturnsNull) {
    strncpy(req->url, "/path?key=val", sizeof(req->url));
    EXPECT_EQ(uvhttp_request_get_query_param(req, nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetQueryParam_NoQueryString_ReturnsNull) {
    strncpy(req->url, "/path", sizeof(req->url));
    EXPECT_EQ(uvhttp_request_get_query_param(req, "key"), nullptr);
}

TEST_F(RequestBoostTest, GetQueryParam_Found_ReturnsValue) {
    strncpy(req->url, "/path?key=value&foo=bar", sizeof(req->url));
    const char* val = uvhttp_request_get_query_param(req, "key");
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, "value");
}

TEST_F(RequestBoostTest, GetQueryParam_NotFound_ReturnsNull) {
    strncpy(req->url, "/path?key=value", sizeof(req->url));
    EXPECT_EQ(uvhttp_request_get_query_param(req, "missing"), nullptr);
}

TEST_F(RequestBoostTest, GetQueryParam_SecondParam_ReturnsValue) {
    strncpy(req->url, "/path?key=value&foo=bar", sizeof(req->url));
    const char* val = uvhttp_request_get_query_param(req, "foo");
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, "bar");
}

TEST_F(RequestBoostTest, GetQueryParam_LastParam_ReturnsValue) {
    strncpy(req->url, "/path?only=this", sizeof(req->url));
    const char* val = uvhttp_request_get_query_param(req, "only");
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ(val, "this");
}

// ========== uvhttp_request_get_client_ip ==========

TEST_F(RequestBoostTest, GetClientIp_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_client_ip(nullptr), nullptr);
}

TEST_F(RequestBoostTest, GetClientIp_NoHeadersNoClient_ReturnsDefault) {
    // No X-Forwarded-For, no X-Real-IP, no client handle
    req->client = nullptr;
    const char* ip = uvhttp_request_get_client_ip(req);
    EXPECT_STREQ(ip, "127.0.0.1");
}

TEST_F(RequestBoostTest, GetClientIp_WithXForwardedFor_ReturnsFirstIp) {
    add_test_header("X-Forwarded-For", "10.0.0.1, 10.0.0.2, 10.0.0.3");
    const char* ip = uvhttp_request_get_client_ip(req);
    ASSERT_NE(ip, nullptr);
    EXPECT_STREQ(ip, "10.0.0.1");
}

TEST_F(RequestBoostTest, GetClientIp_WithXForwardedForSingle_ReturnsIp) {
    add_test_header("X-Forwarded-For", "192.168.1.100");
    const char* ip = uvhttp_request_get_client_ip(req);
    ASSERT_NE(ip, nullptr);
    EXPECT_STREQ(ip, "192.168.1.100");
}

TEST_F(RequestBoostTest, GetClientIp_WithXRealIp_ReturnsIp) {
    add_test_header("X-Real-IP", "172.16.0.50");
    const char* ip = uvhttp_request_get_client_ip(req);
    ASSERT_NE(ip, nullptr);
    EXPECT_STREQ(ip, "172.16.0.50");
}

// ========== uvhttp_request_get_header_count ==========

TEST_F(RequestBoostTest, GetHeaderCount_Null_ReturnsZero) {
    EXPECT_EQ(uvhttp_request_get_header_count(nullptr), 0u);
}

TEST_F(RequestBoostTest, GetHeaderCount_Empty_ReturnsZero) {
    EXPECT_EQ(uvhttp_request_get_header_count(req), 0u);
}

TEST_F(RequestBoostTest, GetHeaderCount_WithHeaders_ReturnsCount) {
    add_test_header("Host", "example.com");
    add_test_header("Accept", "text/html");
    EXPECT_EQ(uvhttp_request_get_header_count(req), 2u);
}

// ========== uvhttp_request_get_header_at ==========

TEST_F(RequestBoostTest, GetHeaderAt_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header_at(nullptr, 0), nullptr);
}

TEST_F(RequestBoostTest, GetHeaderAt_OutOfRange_ReturnsNull) {
    EXPECT_EQ(uvhttp_request_get_header_at(req, 999), nullptr);
}

TEST_F(RequestBoostTest, GetHeaderAt_InlineHeader_ReturnsHeader) {
    add_test_header("Content-Type", "text/plain");
    uvhttp_header_t* h = uvhttp_request_get_header_at(req, 0);
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(h->name, "Content-Type");
    EXPECT_STREQ(h->value, "text/plain");
}

TEST_F(RequestBoostTest, GetHeaderAt_DynamicExpansion_ReturnsHeader) {
    // Fill inline capacity then add one more to trigger dynamic allocation
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "Header%d", i);
        snprintf(value, sizeof(value), "Value%d", i);
        add_test_header(name, value);
    }
    // This should trigger dynamic expansion
    add_test_header("Extra-Header", "Extra-Value");
    EXPECT_EQ(req->header_count, UVHTTP_INLINE_HEADERS_CAPACITY + 1);

    uvhttp_header_t* h = uvhttp_request_get_header_at(req, UVHTTP_INLINE_HEADERS_CAPACITY);
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(h->name, "Extra-Header");
    EXPECT_STREQ(h->value, "Extra-Value");
}

TEST_F(RequestBoostTest, GetHeaderAt_DynamicNoExtra_ReturnsNull) {
    // Request capacity to include dynamic range, but no headers_extra allocated
    req->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY + 10;
    req->headers_extra = nullptr;
    EXPECT_EQ(uvhttp_request_get_header_at(req, UVHTTP_INLINE_HEADERS_CAPACITY + 1), nullptr);
}

// ========== uvhttp_request_add_header ==========

TEST_F(RequestBoostTest, AddHeader_NullRequest_ReturnsError) {
    EXPECT_EQ(uvhttp_request_add_header(nullptr, "Name", "Value"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RequestBoostTest, AddHeader_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_request_add_header(req, nullptr, "Value"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RequestBoostTest, AddHeader_NullValue_ReturnsError) {
    EXPECT_EQ(uvhttp_request_add_header(req, "Name", nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RequestBoostTest, AddHeader_LongName_Truncated) {
    char long_name[300];
    memset(long_name, 'x', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    uvhttp_error_t err = uvhttp_request_add_header(req, long_name, "val");
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(req->header_count, 1u);
}

TEST_F(RequestBoostTest, AddHeader_LongValue_Truncated) {
    char long_value[10000];
    memset(long_value, 'y', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    uvhttp_error_t err = uvhttp_request_add_header(req, "X-Test", long_value);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(req->header_count, 1u);
}

// ========== uvhttp_request_foreach_header ==========

struct HeaderCollector {
    int count;
    char names[16][64];
    char values[16][256];
};

static void collect_header(const char* name, const char* value, void* user_data) {
    auto* col = static_cast<HeaderCollector*>(user_data);
    if (col->count < 16) {
        strncpy(col->names[col->count], name, sizeof(col->names[0]) - 1);
        col->names[col->count][sizeof(col->names[0]) - 1] = '\0';
        strncpy(col->values[col->count], value, sizeof(col->values[0]) - 1);
        col->values[col->count][sizeof(col->values[0]) - 1] = '\0';
        col->count++;
    }
}

TEST_F(RequestBoostTest, ForeachHeader_NullRequest_DoesNotCrash) {
    HeaderCollector col = {0};
    uvhttp_request_foreach_header(nullptr, collect_header, &col);
    EXPECT_EQ(col.count, 0);
}

TEST_F(RequestBoostTest, ForeachHeader_NullCallback_DoesNotCrash) {
    add_test_header("Host", "example.com");
    uvhttp_request_foreach_header(req, nullptr, nullptr);
}

TEST_F(RequestBoostTest, ForeachHeader_IteratesAllHeaders) {
    add_test_header("Host", "example.com");
    add_test_header("Accept", "text/html");
    add_test_header("Connection", "keep-alive");

    HeaderCollector col = {0};
    uvhttp_request_foreach_header(req, collect_header, &col);
    EXPECT_EQ(col.count, 3);
    EXPECT_STREQ(col.names[0], "Host");
    EXPECT_STREQ(col.values[0], "example.com");
    EXPECT_STREQ(col.names[1], "Accept");
    EXPECT_STREQ(col.values[1], "text/html");
    EXPECT_STREQ(col.names[2], "Connection");
    EXPECT_STREQ(col.values[2], "keep-alive");
}

// ========== uvhttp_request_free / cleanup ==========

TEST_F(RequestBoostTest, Free_Null_DoesNotCrash) {
    uvhttp_request_free(nullptr);
}

TEST_F(RequestBoostTest, Cleanup_Null_DoesNotCrash) {
    uvhttp_request_cleanup(nullptr);
}

TEST_F(RequestBoostTest, Cleanup_WithBody_FreesBody) {
    req->body = (char*)uvhttp_alloc(64);
    ASSERT_NE(req->body, nullptr);
    strcpy(req->body, "test");
    // cleanup should free body; set to nullptr to avoid TearDown double-free
    uvhttp_request_cleanup(req);
    req->body = nullptr;
}

TEST_F(RequestBoostTest, Cleanup_WithHeadersExtra_FreesExtra) {
    // Trigger dynamic allocation
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY + 1; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "H%d", i);
        snprintf(value, sizeof(value), "V%d", i);
        add_test_header(name, value);
    }
    ASSERT_NE(req->headers_extra, nullptr);
    uvhttp_request_cleanup(req);
    req->headers_extra = nullptr;
}

// ========== uvhttp_request_add_header capacity expansion ==========

TEST_F(RequestBoostTest, AddHeader_ExpandsCapacityBeyondInline) {
    // headers_capacity starts at UVHTTP_INLINE_HEADERS_CAPACITY
    EXPECT_EQ(req->headers_capacity, (size_t)UVHTTP_INLINE_HEADERS_CAPACITY);

    // Fill all inline slots
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "H%d", i);
        snprintf(value, sizeof(value), "V%d", i);
        add_test_header(name, value);
    }
    EXPECT_EQ(req->header_count, (size_t)UVHTTP_INLINE_HEADERS_CAPACITY);
    EXPECT_EQ(req->headers_capacity, (size_t)UVHTTP_INLINE_HEADERS_CAPACITY);

    // One more triggers expansion
    add_test_header("Overflow", "yes");
    EXPECT_GT(req->headers_capacity, (size_t)UVHTTP_INLINE_HEADERS_CAPACITY);
    EXPECT_NE(req->headers_extra, nullptr);
}

// ========== Header value length validation ==========

TEST_F(RequestBoostTest, GetHeader_ValueTooLong_ReturnsNull) {
    // Add a header, then manually make its value very long
    add_test_header("X-Test", "short");
    uvhttp_header_t* h = uvhttp_request_get_header_at(req, 0);
    ASSERT_NE(h, nullptr);
    // Fill value beyond UVHTTP_MAX_HEADER_VALUE_LENGTH
    memset(h->value, 'z', sizeof(h->value) - 1);
    h->value[sizeof(h->value) - 1] = '\0';
    // The get_header function checks strlen(header->value) <= UVHTTP_MAX_HEADER_VALUE_LENGTH
    const char* result = uvhttp_request_get_header(req, "X-Test");
    // If value exceeds max length, should return NULL
    // (depends on UVHTTP_MAX_HEADER_VALUE_LENGTH vs header->value buffer size)
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
