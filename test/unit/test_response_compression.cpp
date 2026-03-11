/**
 * @file test_response_compression.cpp
 * @brief HTTP response compression functionality tests
 * 
 * This test suite validates the HTTP response compression feature,
 * including:
 * - Compression API functions
 * - Gzip compression correctness
 * - Compression threshold handling
 * - Zero-overhead when compression disabled
 * 
 * Build configuration:
 * - UVHTTP_FEATURE_COMPRESSION must be enabled for these tests
 * - Requires zlib library
 * 
 * Run with:
 *   ./build/dist/bin/uvhttp_unit_tests --gtest_filter=UvhttpCompressionTest.*
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>

// Include uvhttp headers
#include "uvhttp.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

// Test helpers
static void* dummy_client = (void*)0xDEADBEEF;

class UvhttpCompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize response object
        memset(&response, 0, sizeof(uvhttp_response_t));
        uvhttp_response_init(&response, dummy_client);
    }

    void TearDown() override {
        uvhttp_response_cleanup(&response);
    }

    uvhttp_response_t response;
    
    // Helper function to create test body data
    static std::string create_test_body(size_t size) {
        std::string body;
        body.reserve(size);
        for (size_t i = 0; i < size; i++) {
            body += "Hello, World! ";
        }
        return body;
    }
};

#if UVHTTP_FEATURE_COMPRESSION

// Test 1: Set compress enable/disable
TEST_F(UvhttpCompressionTest, SetCompressEnable) {
    EXPECT_EQ(response.compress, 0);
    
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    
    result = uvhttp_response_set_compress(&response, 0);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 0);
}

// Test 2: Set compress algorithm
TEST_F(UvhttpCompressionTest, SetCompressAlgorithm) {
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Test valid algorithm values
    result = uvhttp_response_set_compress_algorithm(&response, 0);  // auto
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress_algorithm, 0);
    
    result = uvhttp_response_set_compress_algorithm(&response, 1);  // gzip
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress_algorithm, 1);
    
    // Test invalid algorithm value
    result = uvhttp_response_set_compress_algorithm(&response, 99);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// Test 3: Set compress threshold
TEST_F(UvhttpCompressionTest, SetCompressThreshold) {
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Test valid threshold values
    result = uvhttp_response_set_compress_threshold(&response, 512);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress_threshold, 512);
    
    result = uvhttp_response_set_compress_threshold(&response, 1024);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress_threshold, 1024);
    
    // Test invalid threshold value (too large)
    result = uvhttp_response_set_compress_threshold(&response, SIZE_MAX);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// Test 4: Compression API with NULL response
TEST_F(UvhttpCompressionTest, CompressionApiNullResponse) {
    EXPECT_EQ(uvhttp_response_set_compress(NULL, 1), UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(NULL, 1), UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_response_set_compress_threshold(NULL, 1024), UVHTTP_ERROR_INVALID_PARAM);
}

// Test 5: Default threshold when compression enabled
TEST_F(UvhttpCompressionTest, DefaultThresholdWhenEnabled) {
    EXPECT_EQ(response.compress_threshold, 0);
    
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Default threshold should be set to 1024
    EXPECT_EQ(response.compress_threshold, 1024);
}

// Test 6: Set algorithm without enabling compression
TEST_F(UvhttpCompressionTest, SetAlgorithmWithoutEnable) {
    // Try to set algorithm without enabling compression first
    uvhttp_error_t result = uvhttp_response_set_compress_algorithm(&response, 1);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);  // Compression not enabled
}

// Test 7: Small body below threshold should not compress
TEST_F(UvhttpCompressionTest, SmallBodyBelowThreshold) {
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_response_set_compress_threshold(&response, 1024);
    
    // Set small body (100 bytes)
    std::string small_body = "Hello, World! ";
    result = uvhttp_response_set_body(&response, small_body.c_str(), small_body.length());
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Build response data
    char* response_data = NULL;
    size_t response_length = 0;
    result = uvhttp_response_build_data(&response, &response_data, &response_length);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Response should not contain Content-Encoding header
    bool has_gzip = false;
    for (size_t i = 0; i < response.header_count; i++) {
        uvhttp_header_t* header = uvhttp_response_get_header_at(&response, i);
        if (header && strcmp(header->name, "Content-Encoding") == 0) {
            has_gzip = true;
            break;
        }
    }
    EXPECT_FALSE(has_gzip);
    
    uvhttp_free(response_data);
}

// Test 8: Large body above threshold should compress
TEST_F(UvhttpCompressionTest, LargeBodyAboveThreshold) {
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_response_set_compress_threshold(&response, 512);
    
    // Set large body (2000 bytes, repeatable pattern for good compression)
    std::string large_body = create_test_body(2000);
    result = uvhttp_response_set_body(&response, large_body.c_str(), large_body.length());
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Build response data
    char* response_data = NULL;
    size_t response_length = 0;
    result = uvhttp_response_build_data(&response, &response_data, &response_length);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Response should contain Content-Encoding: gzip header
    bool has_gzip = false;
    for (size_t i = 0; i < response.header_count; i++) {
        uvhttp_header_t* header = uvhttp_response_get_header_at(&response, i);
        if (header && strcmp(header->name, "Content-Encoding") == 0) {
            has_gzip = true;
            EXPECT_STREQ(header->value, "gzip");
            break;
        }
    }
    EXPECT_TRUE(has_gzip);
    
    uvhttp_free(response_data);
}

// Test 9: Compression reduces response size
TEST_F(UvhttpCompressionTest, CompressionReducesSize) {
    uvhttp_error_t result = uvhttp_response_set_compress(&response, 1);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_response_set_compress_threshold(&response, 100);
    
    // Set body with highly compressible content (repeating pattern)
    std::string body;
    body.reserve(5000);
    for (int i = 0; i < 500; i++) {
        body += "AAAAABBBBBCCCCCDDDDDEEEEE";
    }
    
    size_t original_size = body.length();
    result = uvhttp_response_set_body(&response, body.c_str(), original_size);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Build response with compression
    char* compressed_data = NULL;
    size_t compressed_length = 0;
    result = uvhttp_response_build_data(&response, &compressed_data, &compressed_length);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Disable compression
    uvhttp_response_cleanup(&response);
    memset(&response, 0, sizeof(uvhttp_response_t));
    uvhttp_response_init(&response, dummy_client);
    uvhttp_response_set_compress(&response, 0);
    
    // Build response without compression
    result = uvhttp_response_set_body(&response, body.c_str(), original_size);
    EXPECT_EQ(result, UVHTTP_OK);
    
    char* uncompressed_data = NULL;
    size_t uncompressed_length = 0;
    result = uvhttp_response_build_data(&response, &uncompressed_data, &uncompressed_length);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Compressed should be significantly smaller
    EXPECT_LT(compressed_length, uncompressed_length);
    
    // Compression ratio should be at least 50% for this highly compressible data
    double compression_ratio = (double)compressed_length / uncompressed_length;
    EXPECT_LT(compression_ratio, 0.5);
    
    uvhttp_free(compressed_data);
    uvhttp_free(uncompressed_data);
}

// Test 10: Compression disabled by default
TEST_F(UvhttpCompressionTest, CompressionDisabledByDefault) {
    // Response should not compress by default
    EXPECT_EQ(response.compress, 0);
    
    std::string large_body = create_test_body(5000);
    uvhttp_response_set_body(&response, large_body.c_str(), large_body.length());
    
    char* response_data = NULL;
    size_t response_length = 0;
    uvhttp_error_t result = uvhttp_response_build_data(&response, &response_data, &response_length);
    EXPECT_EQ(result, UVHTTP_OK);
    
    // Should not have Content-Encoding header
    bool has_gzip = false;
    for (size_t i = 0; i < response.header_count; i++) {
        uvhttp_header_t* header = uvhttp_response_get_header_at(&response, i);
        if (header && strcmp(header->name, "Content-Encoding") == 0) {
            has_gzip = true;
            break;
        }
    }
    EXPECT_FALSE(has_gzip);
    
    uvhttp_free(response_data);
}

#else // UVHTTP_FEATURE_COMPRESSION

// Test that compression API functions compile and return OK when disabled
TEST_F(UvhttpCompressionTest, CompressionDisabledReturnsOk) {
    // These should be no-op when compression is disabled
    EXPECT_EQ(uvhttp_response_set_compress(&response, 1), UVHTTP_OK);
    EXPECT_EQ(response.compress, 0);  // Should not change
    
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(&response, 1), UVHTTP_OK);
    
    EXPECT_EQ(uvhttp_response_set_compress_threshold(&response, 1024), UVHTTP_OK);
}

#endif // UVHTTP_FEATURE_COMPRESSION

// Main function
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}