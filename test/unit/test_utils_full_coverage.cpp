/**
 * @file test_utils_full_coverage.cpp
 * @brief Comprehensive coverage tests for uvhttp_utils module
 * 
 * This test file aims to achieve 100% coverage for uvhttp_utils.c by testing:
 * - All utility functions
 * - NULL parameter handling
 * - String operations
 * - Response handling
 * - Validation functions
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "uvhttp_utils.h"
#include "uvhttp_response.h"
#include "uvhttp_error.h"

#include <cstring>
#include <climits>

class UvhttpUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Safe string copy tests
TEST_F(UvhttpUtilsTest, SafeStrncpyValidStrings) {
    const char* src = "test string";
    char dest[100];
    
    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    EXPECT_GT(result, 0);
    EXPECT_STREQ(dest, src);
}

TEST_F(UvhttpUtilsTest, SafeStrncpyEmptyString) {
    const char* src = "";
    char dest[100];
    
    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_EQ(dest[0], '\0');
}

TEST_F(UvhttpUtilsTest, SafeStrncpyNullDest) {
    const char* src = "test string";
    
    int result = uvhttp_safe_strncpy(nullptr, src, 100);
    EXPECT_EQ(result, 0);
}

TEST_F(UvhttpUtilsTest, SafeStrncpyNullSrc) {
    char dest[100];
    
    int result = uvhttp_safe_strncpy(dest, nullptr, sizeof(dest));
    EXPECT_EQ(result, 0);
}

TEST_F(UvhttpUtilsTest, SafeStrncpyZeroSize) {
    const char* src = "test string";
    char dest[100];
    
    int result = uvhttp_safe_strncpy(dest, src, 0);
    EXPECT_EQ(result, 0);
}

TEST_F(UvhttpUtilsTest, SafeStrncpyExactSize) {
    const char* src = "test";
    char dest[5];  // Exactly the size of source string
    
    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    EXPECT_GT(result, 0);
    EXPECT_STREQ(dest, src);
}

TEST_F(UvhttpUtilsTest, SafeStrncpyTruncated) {
    const char* src = "test string";
    char dest[5];  // Smaller than source string
    
    int result = uvhttp_safe_strncpy(dest, src, sizeof(dest));
    EXPECT_GT(result, 0);
    EXPECT_EQ(dest[4], '\0');  // Should be null-terminated
}

// Status code validation tests
TEST_F(UvhttpUtilsTest, IsValidStatusCodeValidRange) {
    EXPECT_TRUE(uvhttp_is_valid_status_code(100));
    EXPECT_TRUE(uvhttp_is_valid_status_code(200));
    EXPECT_TRUE(uvhttp_is_valid_status_code(404));
    EXPECT_TRUE(uvhttp_is_valid_status_code(500));
    EXPECT_TRUE(uvhttp_is_valid_status_code(599));
}

TEST_F(UvhttpUtilsTest, IsValidStatusCodeBelowMin) {
    EXPECT_FALSE(uvhttp_is_valid_status_code(99));
    EXPECT_FALSE(uvhttp_is_valid_status_code(0));
    EXPECT_FALSE(uvhttp_is_valid_status_code(-1));
}

TEST_F(UvhttpUtilsTest, IsValidStatusCodeAboveMax) {
    EXPECT_FALSE(uvhttp_is_valid_status_code(600));
    EXPECT_FALSE(uvhttp_is_valid_status_code(1000));
}

TEST_F(UvhttpUtilsTest, IsValidStatusCodeBoundary) {
    EXPECT_TRUE(uvhttp_is_valid_status_code(100));
    EXPECT_TRUE(uvhttp_is_valid_status_code(599));
    EXPECT_FALSE(uvhttp_is_valid_status_code(99));
    EXPECT_FALSE(uvhttp_is_valid_status_code(600));
}

// IP address validation tests
TEST_F(UvhttpUtilsTest, IsValidIPAddressValidIPv4) {
    EXPECT_TRUE(uvhttp_is_valid_ip_address("192.168.1.1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("127.0.0.1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("0.0.0.0"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("255.255.255.255"));
}

TEST_F(UvhttpUtilsTest, IsValidIPAddressValidIPv6) {
    EXPECT_TRUE(uvhttp_is_valid_ip_address("::1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("2001:0db8:85a3:0000:0000:8a2e:0370:7334"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("fe80::1"));
}

TEST_F(UvhttpUtilsTest, IsValidIPAddressInvalidIPv4) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1.256"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1.1.1"));
}

TEST_F(UvhttpUtilsTest, IsValidIPAddressInvalidString) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address("not an ip"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address(""));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1.abc"));
}

TEST_F(UvhttpUtilsTest, IsValidIPAddressNull) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address(nullptr));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}