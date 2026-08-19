/**
 * @file test_response_boost_coverage_extra.cpp
 * @brief Additional coverage boost tests for uvhttp_response module
 *
 * Targets uncovered paths in uvhttp_response.c:
 * - get_status_text default case (line 111-112)
 * - build_response_headers control char skip (lines 160, 167, 170)
 * - build_data buffer realloc path (lines 609-632)
 * - send_response_data null checks
 * - response_send_raw null checks
 * - Various header combination edge cases
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_allocator.h"
#include "uvhttp_response.h"
}

#include <string.h>
#include <uv.h>

class ResponseBoostExtraTest : public ::testing::Test {
protected:
    uvhttp_response_t* resp = nullptr;

    void SetUp() override {
        resp = (uvhttp_response_t*)uvhttp_calloc(1, sizeof(uvhttp_response_t));
        ASSERT_NE(resp, nullptr);
        resp->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
        resp->status_code = 200;
        resp->keepalive = 1;
    }

    void TearDown() override {
        if (resp) {
            if (resp->body) {
                uvhttp_free(resp->body);
                resp->body = nullptr;
            }
            if (resp->headers_extra) {
                uvhttp_free(resp->headers_extra);
                resp->headers_extra = nullptr;
            }
            uvhttp_free(resp);
            resp = nullptr;
        }
    }

    void set_body(const char* text) {
        if (resp->body) {
            uvhttp_free(resp->body);
        }
        size_t len = strlen(text);
        resp->body = (char*)uvhttp_alloc(len);
        ASSERT_NE(resp->body, nullptr);
        memcpy(resp->body, text, len);
        resp->body_length = len;
    }

    /* Helper: build response data and return as string */
    std::string build_and_get() {
        char* data = nullptr;
        size_t len = 0;
        uvhttp_error_t err = uvhttp_response_build_data(resp, &data, &len);
        if (err != UVHTTP_OK || !data) {
            return "";
        }
        std::string result(data, len);
        uvhttp_free(data);
        return result;
    }
};

// ========== get_status_text default case ==========

TEST_F(ResponseBoostExtraTest, BuildData_UnknownStatusCode_ReturnsUnknown) {
    // Status code 399 is in valid range (100-599) but not in the switch cases,
    // triggering the default "Unknown" branch of get_status_text.
    resp->status_code = 399;
    std::string output = build_and_get();
    EXPECT_NE(output.find("HTTP/1.1 399 Unknown\r\n"), std::string::npos)
        << "Expected 'Unknown' status text for code 399, got: " << output.substr(0, 80);
}

TEST_F(ResponseBoostExtraTest, BuildData_StatusCode101_SwitchingProtocols) {
    // 101 (Switching Protocols) is in valid range and in the switch
    resp->status_code = 101;
    std::string output = build_and_get();
    EXPECT_NE(output.find("HTTP/1.1 101 Switching Protocols\r\n"), std::string::npos)
        << "Expected 'Switching Protocols' status text for code 101, got: " << output.substr(0, 80);
}

TEST_F(ResponseBoostExtraTest, BuildData_StatusCode202_ReturnsUnknown) {
    // 202 (Accepted) is in valid range but not in the switch
    resp->status_code = 202;
    std::string output = build_and_get();
    EXPECT_NE(output.find("HTTP/1.1 202 Unknown\r\n"), std::string::npos)
        << "Expected 'Unknown' status text for code 202, got: " << output.substr(0, 80);
}

TEST_F(ResponseBoostExtraTest, BuildData_StatusCode599_ReturnsUnknown) {
    // 599 is the max valid status code, also not in the switch
    resp->status_code = 599;
    std::string output = build_and_get();
    EXPECT_NE(output.find("HTTP/1.1 599 Unknown\r\n"), std::string::npos)
        << "Expected 'Unknown' status text for code 599, got: " << output.substr(0, 80);
}

// ========== build_response_headers control character skip ==========

TEST_F(ResponseBoostExtraTest, BuildData_HeaderValueWithControlChar_Skipped) {
    // Directly set a header with control character bypassing set_header validation
    // to test the build_response_headers control char skip path
    resp->headers[0].name[0] = 'X';
    resp->headers[0].name[1] = '-';
    resp->headers[0].name[2] = 'E';
    resp->headers[0].name[3] = 'v';
    resp->headers[0].name[4] = 'i';
    resp->headers[0].name[5] = 'l';
    resp->headers[0].name[6] = '\0';

    // Value with \x01 (SOH control character, < 0x20, not tab)
    resp->headers[0].value[0] = 'b';
    resp->headers[0].value[1] = 'a';
    resp->headers[0].value[2] = 'd';
    resp->headers[0].value[3] = '\x01';  // control char
    resp->headers[0].value[4] = 'v';
    resp->headers[0].value[5] = '\0';
    resp->header_count = 1;

    std::string output = build_and_get();
    // The control char header should be skipped - not present in output
    EXPECT_EQ(output.find("X-Evil:"), std::string::npos)
        << "Header with control chars should be skipped, but found in output";
    // Verify the response still has status line and defaults
    EXPECT_NE(output.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_HeaderValueWithCarriageReturn_Skipped) {
    // Header value with CR should be skipped (HTTP response splitting prevention)
    resp->headers[0].name[0] = 'X';
    resp->headers[0].name[1] = '-';
    resp->headers[0].name[2] = 'C';
    resp->headers[0].name[3] = 'R';
    resp->headers[0].name[4] = '\0';

    resp->headers[0].value[0] = 'i';
    resp->headers[0].value[1] = 'n';
    resp->headers[0].value[2] = 'j';
    resp->headers[0].value[3] = 'e';
    resp->headers[0].value[4] = 'c';
    resp->headers[0].value[5] = 't';
    resp->headers[0].value[6] = '\r';
    resp->headers[0].value[7] = '\n';
    resp->headers[0].value[8] = 'X';
    resp->headers[0].value[9] = '-';
    resp->headers[0].value[10] = 'H';
    resp->headers[0].value[11] = 'i';
    resp->headers[0].value[12] = 'j';
    resp->headers[0].value[13] = 'a';
    resp->headers[0].value[14] = 'c';
    resp->headers[0].value[15] = 'k';
    resp->headers[0].value[16] = ':';
    resp->headers[0].value[17] = ' ';
    resp->headers[0].value[18] = '1';
    resp->headers[0].value[19] = '\0';
    resp->header_count = 1;

    std::string output = build_and_get();
    EXPECT_EQ(output.find("X-CR:"), std::string::npos)
        << "Header with CR should be skipped";
}

TEST_F(ResponseBoostExtraTest, BuildData_HeaderValueWithDeleteChar_Skipped) {
    // Header value with DEL (0x7F) should be skipped
    resp->headers[0].name[0] = 'X';
    resp->headers[0].name[1] = '-';
    resp->headers[0].name[2] = 'D';
    resp->headers[0].name[3] = 'E';
    resp->headers[0].name[4] = 'L';
    resp->headers[0].name[5] = '\0';

    resp->headers[0].value[0] = 'b';
    resp->headers[0].value[1] = 'a';
    resp->headers[0].value[2] = 'd';
    resp->headers[0].value[3] = '\x7F';  // DEL character
    resp->headers[0].value[4] = '\0';
    resp->header_count = 1;

    std::string output = build_and_get();
    EXPECT_EQ(output.find("X-DEL:"), std::string::npos)
        << "Header with DEL char should be skipped";
}

TEST_F(ResponseBoostExtraTest, BuildData_MixedValidAndControlCharHeaders) {
    // First header is valid, second has control chars, third is valid
    // The valid headers should appear, the control char one should be skipped
    uvhttp_response_set_header(resp, "X-Good", "valid-value");

    // Directly set invalid header
    resp->headers[1].name[0] = 'X';
    resp->headers[1].name[1] = '-';
    resp->headers[1].name[2] = 'B';
    resp->headers[1].name[3] = 'a';
    resp->headers[1].name[4] = 'd';
    resp->headers[1].name[5] = '\0';
    resp->headers[1].value[0] = '\x02';
    resp->headers[1].value[1] = '\0';
    resp->header_count = 2;

    uvhttp_response_set_header(resp, "X-Also-Good", "also-valid");

    std::string output = build_and_get();
    EXPECT_NE(output.find("X-Good: valid-value\r\n"), std::string::npos)
        << "Valid header should be present";
    EXPECT_EQ(output.find("X-Bad:"), std::string::npos)
        << "Control char header should be skipped";
    EXPECT_NE(output.find("X-Also-Good: also-valid\r\n"), std::string::npos)
        << "Valid header after skipped one should be present";
}

// ========== build_data buffer realloc path (many large headers) ==========

TEST_F(ResponseBoostExtraTest, BuildData_ManyLargeHeaders_TriggersRealloc) {
    // The initial buffer is UVHTTP_INITIAL_BUFFER_SIZE * 2 = 16384 bytes.
    // Add enough headers with large values to exceed the buffer, triggering
    // the realloc path in build_data (lines 609-632).
    // Each header line is roughly: "X-HeaderNN: <value>\r\n"
    // With values of ~1500 bytes, ~12 headers = ~18000+ bytes > 16384.

    char large_value[1501];
    memset(large_value, 'A', 1500);
    large_value[1500] = '\0';

    for (int i = 0; i < 12; i++) {
        char name[32];
        snprintf(name, sizeof(name), "X-Large-%02d", i);
        uvhttp_error_t err = uvhttp_response_set_header(resp, name, large_value);
        ASSERT_EQ(err, UVHTTP_OK) << "Failed to set header " << i;
    }

    std::string output = build_and_get();
    ASSERT_FALSE(output.empty()) << "build_data should succeed after realloc";

    // Verify the headers are present in the output
    EXPECT_NE(output.find("X-Large-00:"), std::string::npos);
    EXPECT_NE(output.find("X-Large-11:"), std::string::npos);

    // Verify the response is well-formed
    EXPECT_NE(output.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(output.find("\r\n\r\n"), std::string::npos);  // header/body separator
}

TEST_F(ResponseBoostExtraTest, BuildData_FillInlineThenDynamicExpansion) {
    // Fill all 32 inline headers, then add more to trigger dynamic allocation,
    // then the total should still build correctly.
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY + 5; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "X-Idx-%02d", i);
        snprintf(value, sizeof(value), "val-%02d", i);
        uvhttp_error_t err = uvhttp_response_set_header(resp, name, value);
        ASSERT_EQ(err, UVHTTP_OK) << "Failed to set header " << i;
    }

    EXPECT_EQ(resp->header_count, (size_t)(UVHTTP_INLINE_HEADERS_CAPACITY + 5));
    EXPECT_NE(resp->headers_extra, nullptr) << "Dynamic expansion should have occurred";

    std::string output = build_and_get();
    ASSERT_FALSE(output.empty());

    // Check a few headers from the inline and dynamic ranges
    EXPECT_NE(output.find("X-Idx-00: val-00\r\n"), std::string::npos);
    EXPECT_NE(output.find("X-Idx-31: val-31\r\n"), std::string::npos);  // last inline
    EXPECT_NE(output.find("X-Idx-36: val-36\r\n"), std::string::npos);  // in dynamic
}

// ========== build_data with custom Content-Type, Content-Length, Connection ==========

TEST_F(ResponseBoostExtraTest, BuildData_CustomContentLength_NotOverwritten) {
    // When Content-Length is already set as a header, it should not be
    // auto-generated
    uvhttp_response_set_header(resp, "Content-Length", "42");
    set_body("This body is definitely not 42 bytes long at all!");

    std::string output = build_and_get();
    // Should have our custom Content-Length, not the auto-generated one
    EXPECT_NE(output.find("Content-Length: 42\r\n"), std::string::npos);
    // Should NOT have Content-Length matching the actual body length
    char auto_len[64];
    snprintf(auto_len, sizeof(auto_len), "Content-Length: %zu", resp->body_length);
    EXPECT_EQ(output.find(auto_len), std::string::npos)
        << "Auto-generated Content-Length should not appear when custom is set";
}

TEST_F(ResponseBoostExtraTest, BuildData_CustomConnection_NotOverwritten) {
    // When Connection header is already set, keepalive logic should be skipped
    resp->keepalive = 1;
    uvhttp_response_set_header(resp, "Connection", "upgrade");

    std::string output = build_and_get();
    EXPECT_NE(output.find("Connection: upgrade\r\n"), std::string::npos);
    // Should NOT have auto-generated Connection: keep-alive
    EXPECT_EQ(output.find("Connection: keep-alive"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_CustomContentType_NotOverwritten) {
    // When Content-Type is already set, default text/plain should not be added
    uvhttp_response_set_header(resp, "Content-Type", "application/xml");

    std::string output = build_and_get();
    EXPECT_NE(output.find("Content-Type: application/xml\r\n"), std::string::npos);
    // Should NOT have default Content-Type: text/plain
    EXPECT_EQ(output.find("Content-Type: text/plain"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_AllThreeCustomHeaders) {
    // Set all three "auto-generated" headers manually
    uvhttp_response_set_header(resp, "Content-Type", "application/octet-stream");
    uvhttp_response_set_header(resp, "Content-Length", "0");
    uvhttp_response_set_header(resp, "Connection", "teardown");

    std::string output = build_and_get();
    EXPECT_NE(output.find("Content-Type: application/octet-stream\r\n"), std::string::npos);
    EXPECT_NE(output.find("Content-Length: 0\r\n"), std::string::npos);
    EXPECT_NE(output.find("Connection: teardown\r\n"), std::string::npos);
    // No auto-generated headers
    EXPECT_EQ(output.find("Content-Type: text/plain"), std::string::npos);
    EXPECT_EQ(output.find("Connection: keep-alive"), std::string::npos);
    EXPECT_EQ(output.find("Connection: close"), std::string::npos);
}

// ========== Keepalive edge cases ==========

TEST_F(ResponseBoostExtraTest, BuildData_KeepAlive0_CustomConnectionNotOverwritten) {
    // keepalive=0 but Connection is already set
    resp->keepalive = 0;
    uvhttp_response_set_header(resp, "Connection", "keep-alive");

    std::string output = build_and_get();
    // Should use the custom value, not "close"
    EXPECT_NE(output.find("Connection: keep-alive\r\n"), std::string::npos);
    EXPECT_EQ(output.find("Connection: close"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_KeepAlive1_IncludesKeepAliveTimeout) {
    // When keepalive=1 and no custom Connection, should include Keep-Alive header
    resp->keepalive = 1;
    std::string output = build_and_get();
    EXPECT_NE(output.find("Connection: keep-alive\r\n"), std::string::npos);
    EXPECT_NE(output.find("Keep-Alive: timeout="), std::string::npos);
}

// ========== send_response_data null parameter tests (lines 428-432) ==========

// Forward declaration of internal function not in any public header
extern "C" {
uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t* response,
                                         const char* data, size_t length);
}

TEST_F(ResponseBoostExtraTest, SendResponseData_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_send_response_data(nullptr, "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, SendResponseData_NullData_ReturnsError) {
    EXPECT_EQ(uvhttp_send_response_data(resp, nullptr, 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, SendResponseData_ZeroLength_ReturnsError) {
    EXPECT_EQ(uvhttp_send_response_data(resp, "x", 0),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== response_send_raw null checks ==========

TEST_F(ResponseBoostExtraTest, SendRaw_NullData_ReturnsError) {
    EXPECT_EQ(uvhttp_response_send_raw(nullptr, 10, (void*)0x1, resp),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, SendRaw_ZeroLength_ReturnsError) {
    char buf[] = "test";
    EXPECT_EQ(uvhttp_response_send_raw(buf, 0, (void*)0x1, resp),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, SendRaw_NullClient_ReturnsError) {
    char buf[] = "test";
    EXPECT_EQ(uvhttp_response_send_raw(buf, 4, nullptr, resp),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== build_data with empty body ==========

TEST_F(ResponseBoostExtraTest, BuildData_EmptyBody_ContentLengthZero) {
    // No body set, should still include Content-Length: 0
    std::string output = build_and_get();
    EXPECT_NE(output.find("Content-Length: 0\r\n"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_WithBody_ContentLengthMatches) {
    set_body("Hello");
    std::string output = build_and_get();
    EXPECT_NE(output.find("Content-Length: 5\r\n"), std::string::npos);
    // Body should be at the end
    EXPECT_NE(output.find("Hello"), std::string::npos);
}

// ========== build_data response already sent ==========

TEST_F(ResponseBoostExtraTest, BuildData_AlreadySent_ReturnsNull) {
    resp->sent = 1;
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(len, 0u);
}

// ========== build_data with no headers set (default headers only) ==========

TEST_F(ResponseBoostExtraTest, BuildData_NoCustomHeaders_HasDefaults) {
    // Should have default Content-Type: text/plain, Content-Length: 0,
    // Connection: keep-alive, Keep-Alive header
    std::string output = build_and_get();
    EXPECT_NE(output.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(output.find("Content-Type: text/plain\r\n"), std::string::npos);
    EXPECT_NE(output.find("Content-Length: 0\r\n"), std::string::npos);
    EXPECT_NE(output.find("Connection: keep-alive\r\n"), std::string::npos);
    EXPECT_NE(output.find("Keep-Alive: timeout="), std::string::npos);
}

// ========== build_data with body and multiple headers ==========

TEST_F(ResponseBoostExtraTest, BuildData_MultipleHeadersAndBody_FullResponse) {
    resp->status_code = 200;
    uvhttp_response_set_header(resp, "X-Request-Id", "abc-123");
    uvhttp_response_set_header(resp, "X-Custom", "some-value");
    uvhttp_response_set_header(resp, "Cache-Control", "no-cache");
    set_body("{\"status\":\"ok\"}");

    std::string output = build_and_get();

    // Status line
    EXPECT_NE(output.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    // Custom headers
    EXPECT_NE(output.find("X-Request-Id: abc-123\r\n"), std::string::npos);
    EXPECT_NE(output.find("X-Custom: some-value\r\n"), std::string::npos);
    EXPECT_NE(output.find("Cache-Control: no-cache\r\n"), std::string::npos);
    // Default headers
    EXPECT_NE(output.find("Content-Type: text/plain\r\n"), std::string::npos);
    EXPECT_NE(output.find("Content-Length: 15\r\n"), std::string::npos);
    EXPECT_NE(output.find("Connection: keep-alive\r\n"), std::string::npos);
    // Body at the end
    EXPECT_NE(output.find("{\"status\":\"ok\"}"), std::string::npos);
}

// ========== Control chars: NULL string in contains_control_chars ==========

TEST_F(ResponseBoostExtraTest, BuildData_HeaderWithNullName_Behavior) {
    // Setting header_count > actual headers with NULL name
    // This tests the null check in get_header_at
    resp->header_count = 1;
    // headers[0] is zeroed from calloc, so name is empty string
    // Empty name should still work (no crash)
    std::string output = build_and_get();
    EXPECT_FALSE(output.empty());
}

// ========== Large body test ==========

TEST_F(ResponseBoostExtraTest, BuildData_LargeBody_CorrectContentLength) {
    // Create a body larger than the initial header buffer
    size_t body_size = 32768;
    char* large_body = (char*)uvhttp_alloc(body_size);
    ASSERT_NE(large_body, nullptr);
    memset(large_body, 'X', body_size);

    if (resp->body) {
        uvhttp_free(resp->body);
    }
    resp->body = large_body;
    resp->body_length = body_size;

    std::string output = build_and_get();
    ASSERT_FALSE(output.empty());

    char expected_cl[64];
    snprintf(expected_cl, sizeof(expected_cl), "Content-Length: %zu", body_size);
    EXPECT_NE(output.find(expected_cl), std::string::npos);
}

// ========== Dynamic expansion realloc path (headers_extra reallocation) ==========

TEST_F(ResponseBoostExtraTest, SetHeader_ExpandsMultipleTimes_Works) {
    // Fill inline, then add more to trigger reallocation of headers_extra
    // Initial: 32 inline. After filling, capacity doubles to 64.
    // Adding 33rd header triggers first alloc. Adding more triggers realloc.
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY + 5; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "X-H%02d", i);
        snprintf(value, sizeof(value), "V%02d", i);
        EXPECT_EQ(uvhttp_response_set_header(resp, name, value), UVHTTP_OK);
    }
    EXPECT_EQ(resp->header_count, (size_t)(UVHTTP_INLINE_HEADERS_CAPACITY + 5));

    // Verify all headers are accessible
    for (size_t i = 0; i < resp->header_count; i++) {
        uvhttp_header_t* h = uvhttp_response_get_header_at(resp, i);
        ASSERT_NE(h, nullptr) << "Header at index " << i << " should not be null";
    }
}

TEST_F(ResponseBoostExtraTest, SetHeader_ReallocPath_TriggersMallocAndMaxCapacity) {
    // Target: cover uvhttp_response.c lines 332-334 (first malloc when
    // old_extra_count==0) and lines 314-316/320-321 (max capacity reached).
    //
    // With UVHTTP_MAX_HEADERS=64 and UVHTTP_INLINE_HEADERS_CAPACITY=32:
    //   - Headers 0-31: stored inline (capacity=32)
    //   - Header 32: triggers expansion to capacity=64, malloc (lines 332-334)
    //   - Headers 33-63: stored in dynamic array
    //   - Header 64: expansion requested, but 64*2=128 > UVHTTP_MAX_HEADERS=64,
    //     capped to 64, which equals current capacity -> returns OUT_OF_MEMORY
    //     (lines 314-316, 320-321)
    //
    // Note: The realloc path (lines 337-338) is unreachable when
    // UVHTTP_MAX_HEADERS <= 64 because capacity can't grow beyond 64.

    // Add all 64 headers (the maximum)
    for (int i = 0; i < 64; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "X-R%02d", i);
        snprintf(value, sizeof(value), "realloc-val-%02d", i);
        uvhttp_error_t err = uvhttp_response_set_header(resp, name, value);
        ASSERT_EQ(err, UVHTTP_OK) << "Failed to set header " << i;
    }

    EXPECT_EQ(resp->header_count, (size_t)64);
    EXPECT_EQ(resp->headers_capacity, (size_t)64);
    EXPECT_NE(resp->headers_extra, nullptr);

    // Verify boundary headers are accessible and correct
    uvhttp_header_t* h0 = uvhttp_response_get_header_at(resp, 0);
    ASSERT_NE(h0, nullptr);
    EXPECT_STREQ(h0->name, "X-R00");
    EXPECT_STREQ(h0->value, "realloc-val-00");

    uvhttp_header_t* h31 = uvhttp_response_get_header_at(resp, 31);
    ASSERT_NE(h31, nullptr);
    EXPECT_STREQ(h31->name, "X-R31");

    uvhttp_header_t* h32 = uvhttp_response_get_header_at(resp, 32);
    ASSERT_NE(h32, nullptr);
    EXPECT_STREQ(h32->name, "X-R32");

    uvhttp_header_t* h63 = uvhttp_response_get_header_at(resp, 63);
    ASSERT_NE(h63, nullptr);
    EXPECT_STREQ(h63->name, "X-R63");
    EXPECT_STREQ(h63->value, "realloc-val-63");

    // Verify the 65th header fails with OUT_OF_MEMORY (max capacity reached)
    uvhttp_error_t err = uvhttp_response_set_header(resp, "X-Overflow", "nope");
    EXPECT_EQ(err, UVHTTP_ERROR_OUT_OF_MEMORY);

    // Verify the response still builds correctly with all 64 headers
    std::string output = build_and_get();
    ASSERT_FALSE(output.empty());
    EXPECT_NE(output.find("X-R00: realloc-val-00\r\n"), std::string::npos);
    EXPECT_NE(output.find("X-R32: realloc-val-32\r\n"), std::string::npos);
    EXPECT_NE(output.find("X-R63: realloc-val-63\r\n"), std::string::npos);
}

// ========== build_data header value with tab (allowed) ==========

TEST_F(ResponseBoostExtraTest, BuildData_HeaderValueWithTab_NotSkipped) {
    // Tab (0x09) is explicitly allowed by contains_control_chars
    resp->headers[0].name[0] = 'X';
    resp->headers[0].name[1] = '-';
    resp->headers[0].name[2] = 'T';
    resp->headers[0].name[3] = 'a';
    resp->headers[0].name[4] = 'b';
    resp->headers[0].name[5] = '\0';

    resp->headers[0].value[0] = 'v';
    resp->headers[0].value[1] = 'a';
    resp->headers[0].value[2] = 'l';
    resp->headers[0].value[3] = '\t';  // tab - should be allowed
    resp->headers[0].value[4] = 'u';
    resp->headers[0].value[5] = 'e';
    resp->headers[0].value[6] = '\0';
    resp->header_count = 1;

    std::string output = build_and_get();
    // Tab is allowed, so the header should be present
    EXPECT_NE(output.find("X-Tab:"), std::string::npos)
        << "Header with tab in value should NOT be skipped";
}

// ========== build_data header value with space (allowed) ==========

TEST_F(ResponseBoostExtraTest, BuildData_HeaderValueWithSpace_NotSkipped) {
    uvhttp_response_set_header(resp, "X-Space", "hello world");
    std::string output = build_and_get();
    EXPECT_NE(output.find("X-Space: hello world\r\n"), std::string::npos);
}

// ========== Multiple build_data calls (idempotent) ==========

TEST_F(ResponseBoostExtraTest, BuildData_CalledTwice_SameResult) {
    set_body("test body");
    uvhttp_response_set_header(resp, "X-Test", "value");

    std::string first = build_and_get();
    std::string second = build_and_get();
    EXPECT_EQ(first, second) << "build_data should be idempotent";
}

// ========== build_data with only keepalive=0 ==========

TEST_F(ResponseBoostExtraTest, BuildData_KeepAlive0_CloseConnection) {
    resp->keepalive = 0;
    std::string output = build_and_get();
    EXPECT_NE(output.find("Connection: close\r\n"), std::string::npos);
    // Should NOT have Keep-Alive header
    EXPECT_EQ(output.find("Keep-Alive:"), std::string::npos);
}

// ========== response_send null checks ==========

TEST_F(ResponseBoostExtraTest, ResponseSend_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_send(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, ResponseSend_AlreadySent_ReturnsOK) {
    resp->sent = 1;
    EXPECT_EQ(uvhttp_response_send(resp), UVHTTP_OK);
}

// ========== foreach_header with many headers ==========

struct HeaderCounter {
    int count;
};

static void count_header(const char* name, const char* value, void* user_data) {
    (void)name;
    (void)value;
    auto* counter = static_cast<HeaderCounter*>(user_data);
    counter->count++;
}

TEST_F(ResponseBoostExtraTest, ForeachHeader_ManyHeaders_CountsAll) {
    for (int i = 0; i < 20; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "X-N%02d", i);
        snprintf(value, sizeof(value), "V%02d", i);
        uvhttp_response_set_header(resp, name, value);
    }
    HeaderCounter counter = {0};
    uvhttp_response_foreach_header(resp, count_header, &counter);
    EXPECT_EQ(counter.count, 20);
}

// ========== Header name case sensitivity in Content-Type/Length/Connection detection ==========

TEST_F(ResponseBoostExtraTest, BuildData_UpperCaseContentType_Detected) {
    // The strcasecmp check should match regardless of case
    uvhttp_response_set_header(resp, "CONTENT-TYPE", "application/json");
    set_body("{}");

    std::string output = build_and_get();
    // Should have our custom content type, not the default
    EXPECT_NE(output.find("CONTENT-TYPE: application/json\r\n"), std::string::npos);
    EXPECT_EQ(output.find("Content-Type: text/plain"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_MixedCaseContentLength_Detected) {
    uvhttp_response_set_header(resp, "content-length", "100");
    set_body("Hello");

    std::string output = build_and_get();
    // Custom content-length should be used, not auto-generated
    EXPECT_NE(output.find("content-length: 100\r\n"), std::string::npos);
    // Should not have auto-generated Content-Length: 5
    EXPECT_EQ(output.find("Content-Length: 5\r\n"), std::string::npos);
}

TEST_F(ResponseBoostExtraTest, BuildData_MixedCaseConnection_Detected) {
    resp->keepalive = 1;
    uvhttp_response_set_header(resp, "CONNECTION", "close");

    std::string output = build_and_get();
    EXPECT_NE(output.find("CONNECTION: close\r\n"), std::string::npos);
    // Should not have auto-generated Connection: keep-alive
    EXPECT_EQ(output.find("Connection: keep-alive"), std::string::npos);
}

// ========== send_raw stream type and loop validation ==========

TEST_F(ResponseBoostExtraTest, SendRaw_NonTcpStream_ReturnsError) {
    // Create a fake pipe stream with type != UV_TCP
    uv_pipe_t pipe;
    memset(&pipe, 0, sizeof(pipe));
    pipe.type = (uv_handle_type)UV_NAMED_PIPE;

    const char* data = "hello";
    uvhttp_error_t err = uvhttp_response_send_raw(data, 5, &pipe, resp);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostExtraTest, SendRaw_NullLoop_ReturnsError) {
    // Create a fake TCP stream with type = UV_TCP but loop = NULL
    uv_tcp_t tcp;
    memset(&tcp, 0, sizeof(tcp));
    tcp.type = UV_TCP;
    // loop is NULL due to memset

    const char* data = "hello";
    uvhttp_error_t err = uvhttp_response_send_raw(data, 5, &tcp, resp);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
