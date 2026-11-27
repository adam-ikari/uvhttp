/*
 * uvhttp_respond — atomic response helper.
 *
 * The helper must reject bad input before touching the response, so a failed
 * call never leaves a half-configured response behind. Wire-level success is
 * covered in test_http_response_semantics.cpp (the /atomic route).
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_response.h"

#include <stdlib.h>
#include <string.h>

namespace {

typedef struct {
    int dummy;
} mock_client_t;

class ResponseAtomicTest : public ::testing::Test {
   protected:
    void SetUp() override {
        ASSERT_EQ(uvhttp_response_init(&response_, &client_), UVHTTP_OK);
    }
    void TearDown() override { uvhttp_response_cleanup(&response_); }
    uvhttp_response_t response_;
    mock_client_t client_;
};

}  // namespace

TEST_F(ResponseAtomicTest, RejectsNullResponse) {
    EXPECT_EQ(uvhttp_respond(NULL, 200, "text/plain", "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseAtomicTest, RejectsBadStatusWithoutMutation) {
    EXPECT_EQ(uvhttp_respond(&response_, 0, "text/plain", "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_respond(&response_, 1000, "text/plain", "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
    EXPECT_EQ(response_.body, nullptr);
}

TEST_F(ResponseAtomicTest, RejectsMismatchedBodyAndLengthWithoutMutation) {
    EXPECT_EQ(uvhttp_respond(&response_, 200, NULL, NULL, 4),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_respond(&response_, 200, NULL, "x", 0),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
    EXPECT_EQ(response_.body, nullptr);
}

TEST_F(ResponseAtomicTest, RejectsOversizedBodyWithoutMutation) {
    EXPECT_EQ(uvhttp_respond(&response_, 200, NULL, "x", UVHTTP_MAX_BODY_SIZE + 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.body, nullptr);
}

TEST_F(ResponseAtomicTest, RejectsControlCharsInContentTypeWithoutMutation) {
    EXPECT_EQ(uvhttp_respond(&response_, 200, "text/plain\r\nX: y", "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
    EXPECT_EQ(response_.body, nullptr);
}

TEST_F(ResponseAtomicTest, RejectsBadHeaderWithoutMutation) {
    uvhttp_header_view_t headers[] = {{"X-Ok", "v"}, {"Bad\r\nName", "x"}};
    EXPECT_EQ(uvhttp_respond_with_headers(&response_, 200, headers, 2, "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
}

TEST_F(ResponseAtomicTest, RejectsNullHeaderArrayWithCount) {
    EXPECT_EQ(uvhttp_respond_with_headers(&response_, 200, nullptr, 1, "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
}

TEST_F(ResponseAtomicTest, RejectsAlreadySentResponse) {
    response_.sent = 1;
    EXPECT_EQ(uvhttp_respond(&response_, 200, NULL, "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseAtomicTest, PrepareBuildsWithoutSending) {
    uvhttp_header_view_t headers[] = {{"X-Test", "v"}};
    ASSERT_EQ(uvhttp_response_prepare(&response_, 201, headers, 1, "body", 4),
              UVHTTP_OK);
    EXPECT_EQ(response_.status_code, 201);
    EXPECT_EQ(response_.header_count, 1u);
    EXPECT_EQ(response_.sent, 0);
    EXPECT_EQ(response_.body_length, 4u);
}

TEST_F(ResponseAtomicTest, PrepareRejectsBadHeaderWithoutMutation) {
    uvhttp_header_view_t headers[] = {{"Bad\r\nName", "v"}};
    EXPECT_EQ(uvhttp_response_prepare(&response_, 201, headers, 1, "body", 4),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(response_.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response_.header_count, 0u);
    EXPECT_EQ(response_.sent, 0);
}
