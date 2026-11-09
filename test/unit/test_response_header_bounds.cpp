/*
 * Response header storage bounds.
 *
 * uvhttp_response_set_header used to copy with UVHTTP_MAX_HEADER_VALUE_SIZE
 * (4096) as the bound while uvhttp_header_t::value is only
 * UVHTTP_HEADER_VALUE_BUFFER_SIZE (2048) bytes. A 2048..4095-byte value
 * passed validation and was written past the last inline header, corrupting
 * the heap. These tests pin the boundary: the largest fitting value is stored
 * verbatim, anything larger is rejected instead of truncated or overflowed.
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <string>

extern "C" {
#include "uvhttp_common.h"
#include "uvhttp_constants.h"
#include "uvhttp_response.h"
}

class ResponseHeaderBounds : public ::testing::Test {
   protected:
    uvhttp_response_t res_{};

    void SetUp() override {
        memset(&res_, 0, sizeof(res_));
        ASSERT_EQ(uvhttp_response_init(&res_, (void*)0x1234), UVHTTP_OK);
        ASSERT_EQ(uvhttp_response_set_status(&res_, 200), UVHTTP_OK);
    }

    void TearDown() override {
        uvhttp_response_cleanup(&res_);
    }
};

TEST_F(ResponseHeaderBounds, ValueAtFieldLimitAccepted) {
    std::string value(UVHTTP_HEADER_VALUE_BUFFER_SIZE - 1, 'v');

    EXPECT_EQ(uvhttp_response_set_header(&res_, "X-Long", value.c_str()),
              UVHTTP_OK);
    ASSERT_EQ(res_.header_count, 1u);
    EXPECT_EQ(strlen(res_.headers[0].value), value.size());
    EXPECT_EQ(std::string(res_.headers[0].value), value);
}

TEST_F(ResponseHeaderBounds, ValueOverFieldLimitRejected) {
    std::string value(UVHTTP_HEADER_VALUE_BUFFER_SIZE, 'v');

    EXPECT_EQ(uvhttp_response_set_header(&res_, "X-Long", value.c_str()),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(res_.header_count, 0u);
}

TEST_F(ResponseHeaderBounds, NameOverFieldLimitRejected) {
    std::string name(UVHTTP_HEADER_NAME_BUFFER_SIZE, 'n');

    EXPECT_EQ(uvhttp_response_set_header(&res_, name.c_str(), "value"),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(res_.header_count, 0u);
}

/* The last inline header is the one the old overflow ran past; fill the
 * inline array and confirm the oversized value is still rejected. */
TEST_F(ResponseHeaderBounds, FullInlineHeaderArrayRejectsOversize) {
    std::string small(8, 'x');
    for (size_t i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY; i++) {
        char name[32];
        snprintf(name, sizeof(name), "X-%zu", i);
        ASSERT_EQ(uvhttp_response_set_header(&res_, name, small.c_str()),
                  UVHTTP_OK);
    }
    ASSERT_EQ(res_.header_count, UVHTTP_INLINE_HEADERS_CAPACITY);

    std::string value(UVHTTP_HEADER_VALUE_BUFFER_SIZE, 'v');
    EXPECT_EQ(uvhttp_response_set_header(&res_, "X-Overflow", value.c_str()),
              UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(res_.header_count, UVHTTP_INLINE_HEADERS_CAPACITY);
}
