/**
 * @file test_version_full_coverage.cpp
 * @brief Comprehensive coverage tests for uvhttp_version module
 *
 * This test file provides comprehensive coverage for all functions in uvhttp_version.c
 * including version queries, build information, feature checking, and helper functions.
 */

#include <gtest/gtest.h>
#include <uvhttp_version.h>
#include <uvhttp_error.h>
#include <string.h>

class UvhttpVersionFullCoverageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset any state if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

/* ========== Version Information Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, GetVersionString) {
    const char* version = uvhttp_get_version_string();
    
    ASSERT_NE(version, nullptr);
    ASSERT_STRNE(version, "");
    
    // Verify version format (should be like "X.Y.Z")
    int major, minor, patch;
    int result = sscanf(version, "%d.%d.%d", &major, &minor, &patch);
    ASSERT_EQ(result, 3);
    ASSERT_GE(major, 0);
    ASSERT_GE(minor, 0);
    ASSERT_GE(patch, 0);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionAllParams) {
    int major = -1, minor = -1, patch = -1;
    
    uvhttp_get_version(&major, &minor, &patch);
    
    ASSERT_GE(major, 0);
    ASSERT_GE(minor, 0);
    ASSERT_GE(patch, 0);
    
    // Verify consistency with version string
    const char* version_str = uvhttp_get_version_string();
    int expected_major, expected_minor, expected_patch;
    sscanf(version_str, "%d.%d.%d", &expected_major, &expected_minor, &expected_patch);
    
    EXPECT_EQ(major, expected_major);
    EXPECT_EQ(minor, expected_minor);
    EXPECT_EQ(patch, expected_patch);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionNullMajor) {
    int minor = -1, patch = -1;
    
    uvhttp_get_version(nullptr, &minor, &patch);
    
    ASSERT_GE(minor, 0);
    ASSERT_GE(patch, 0);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionNullMinor) {
    int major = -1, patch = -1;
    
    uvhttp_get_version(&major, nullptr, &patch);
    
    ASSERT_GE(major, 0);
    ASSERT_GE(patch, 0);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionNullPatch) {
    int major = -1, minor = -1;
    
    uvhttp_get_version(&major, &minor, nullptr);
    
    ASSERT_GE(major, 0);
    ASSERT_GE(minor, 0);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionAllNull) {
    // Should not crash
    uvhttp_get_version(nullptr, nullptr, nullptr);
}

TEST_F(UvhttpVersionFullCoverageTest, GetVersionInt) {
    int version_int = uvhttp_get_version_int();
    
    ASSERT_GE(version_int, 0);
    
    // Verify format: major * 10000 + minor * 100 + patch
    int major, minor, patch;
    uvhttp_get_version(&major, &minor, &patch);
    
    int expected_int = major * 10000 + minor * 100 + patch;
    EXPECT_EQ(version_int, expected_int);
}

/* ========== Build Information Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, GetBuildInfoValid) {
    uvhttp_build_info_t info;
    uvhttp_error_t result = uvhttp_get_build_info(&info);
    
    ASSERT_EQ(result, UVHTTP_OK);
    
    // Verify version information
    ASSERT_NE(info.version_string, nullptr);
    ASSERT_STRNE(info.version_string, "");
    ASSERT_GE(info.version_major, 0);
    ASSERT_GE(info.version_minor, 0);
    ASSERT_GE(info.version_patch, 0);
    ASSERT_GE(info.version_int, 0);
    
    // Verify build information
    ASSERT_NE(info.build_type, nullptr);
    ASSERT_STRNE(info.build_type, "");
    ASSERT_NE(info.build_date, nullptr);
    ASSERT_STRNE(info.build_date, "");
    ASSERT_NE(info.build_time, nullptr);
    ASSERT_STRNE(info.build_time, "");
    ASSERT_NE(info.compiler, nullptr);
    ASSERT_STRNE(info.compiler, "");
    ASSERT_NE(info.platform, nullptr);
    ASSERT_STRNE(info.platform, "");
    
    // Verify build type is either "Debug" or "Release"
    bool valid_build_type = (strcmp(info.build_type, "Debug") == 0) ||
                            (strcmp(info.build_type, "Release") == 0);
    ASSERT_TRUE(valid_build_type);
    
    // Verify allocator type
    ASSERT_NE(info.allocator_type, nullptr);
    ASSERT_STRNE(info.allocator_type, "");
    
    // Verify memory configuration
    ASSERT_GT(info.max_connections, 0);
    ASSERT_GT(info.max_headers, 0);
    ASSERT_GT(info.max_body_size, 0);
    ASSERT_GT(info.buffer_size, 0);
    
    // Protocol upgrade should always be enabled
    EXPECT_EQ(info.feature_protocol_upgrade, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, GetBuildInfoNull) {
    uvhttp_error_t result = uvhttp_get_build_info(nullptr);
    
    ASSERT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoConsistency) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // Verify version consistency
    const char* version_str = uvhttp_get_version_string();
    EXPECT_STREQ(info.version_string, version_str);
    
    int major, minor, patch;
    uvhttp_get_version(&major, &minor, &patch);
    EXPECT_EQ(info.version_major, major);
    EXPECT_EQ(info.version_minor, minor);
    EXPECT_EQ(info.version_patch, patch);
    
    int version_int = uvhttp_get_version_int();
    EXPECT_EQ(info.version_int, version_int);
}

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoHelperConsistency) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // Verify helper functions consistency
    EXPECT_STREQ(info.build_type, uvhttp_get_build_type());
    EXPECT_STREQ(info.compiler, uvhttp_get_compiler_info());
    EXPECT_STREQ(info.platform, uvhttp_get_platform_info());
    EXPECT_STREQ(info.allocator_type, uvhttp_get_allocator_type());
}

/* ========== Feature Checking Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledNull) {
    int result = uvhttp_is_feature_enabled(nullptr);
    
    ASSERT_EQ(result, -1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledWebSocket) {
    int result = uvhttp_is_feature_enabled("websocket");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledStaticFiles) {
    int result = uvhttp_is_feature_enabled("static_files");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledTLS) {
    int result = uvhttp_is_feature_enabled("tls");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledMiddleware) {
    int result = uvhttp_is_feature_enabled("middleware");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledLogging) {
    int result = uvhttp_is_feature_enabled("logging");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledRouterCache) {
    int result = uvhttp_is_feature_enabled("router_cache");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledLRUCache) {
    int result = uvhttp_is_feature_enabled("lru_cache");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledRateLimit) {
    int result = uvhttp_is_feature_enabled("rate_limit");
    
    ASSERT_GE(result, 0);
    ASSERT_LE(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledProtocolUpgrade) {
    int result = uvhttp_is_feature_enabled("protocol_upgrade");
    
    // Protocol upgrade should always be enabled
    EXPECT_EQ(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledAllocator) {
    int result = uvhttp_is_feature_enabled("allocator");
    
    // Allocator should always be enabled
    EXPECT_EQ(result, 1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledInvalid) {
    int result = uvhttp_is_feature_enabled("invalid_feature");
    
    ASSERT_EQ(result, -1);
}

TEST_F(UvhttpVersionFullCoverageTest, IsFeatureEnabledEmptyString) {
    int result = uvhttp_is_feature_enabled("");
    
    ASSERT_EQ(result, -1);
}

TEST_F(UvhttpVersionFullCoverageTest, FeatureConsistencyWithBuildInfo) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // Verify feature flags consistency
    int ws_enabled = uvhttp_is_feature_enabled("websocket");
    EXPECT_EQ(ws_enabled, info.feature_websocket);
    
    int sf_enabled = uvhttp_is_feature_enabled("static_files");
    EXPECT_EQ(sf_enabled, info.feature_static_files);
    
    int tls_enabled = uvhttp_is_feature_enabled("tls");
    EXPECT_EQ(tls_enabled, info.feature_tls);
    
    int mw_enabled = uvhttp_is_feature_enabled("middleware");
    EXPECT_EQ(mw_enabled, info.feature_middleware);
    
    int log_enabled = uvhttp_is_feature_enabled("logging");
    EXPECT_EQ(log_enabled, info.feature_logging);
    
    int rc_enabled = uvhttp_is_feature_enabled("router_cache");
    EXPECT_EQ(rc_enabled, info.feature_router_cache);
    
    int lru_enabled = uvhttp_is_feature_enabled("lru_cache");
    EXPECT_EQ(lru_enabled, info.feature_lru_cache);
    
    int rl_enabled = uvhttp_is_feature_enabled("rate_limit");
    EXPECT_EQ(rl_enabled, info.feature_rate_limit);
    
    int pu_enabled = uvhttp_is_feature_enabled("protocol_upgrade");
    EXPECT_EQ(pu_enabled, info.feature_protocol_upgrade);
}

/* ========== Helper Function Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, GetAllocatorType) {
    const char* allocator = uvhttp_get_allocator_type();
    
    ASSERT_NE(allocator, nullptr);
    ASSERT_STRNE(allocator, "");
    
    // Verify valid allocator types
    bool valid_allocator = (strcmp(allocator, "system") == 0) ||
                           (strcmp(allocator, "mimalloc") == 0) ||
                           (strcmp(allocator, "custom") == 0);
    ASSERT_TRUE(valid_allocator);
}

TEST_F(UvhttpVersionFullCoverageTest, GetBuildType) {
    const char* build_type = uvhttp_get_build_type();
    
    ASSERT_NE(build_type, nullptr);
    ASSERT_STRNE(build_type, "");
    
    // Verify valid build types
    bool valid_type = (strcmp(build_type, "Debug") == 0) ||
                      (strcmp(build_type, "Release") == 0);
    ASSERT_TRUE(valid_type);
}

TEST_F(UvhttpVersionFullCoverageTest, GetCompilerInfo) {
    const char* compiler = uvhttp_get_compiler_info();
    
    ASSERT_NE(compiler, nullptr);
    ASSERT_STRNE(compiler, "");
    
    // Verify compiler info format (should start with compiler name)
    bool valid_compiler = (strncmp(compiler, "Clang ", 6) == 0) ||
                          (strncmp(compiler, "GCC ", 4) == 0) ||
                          (strncmp(compiler, "MSVC", 4) == 0) ||
                          (strcmp(compiler, "Unknown") == 0);
    ASSERT_TRUE(valid_compiler);
}

TEST_F(UvhttpVersionFullCoverageTest, GetPlatformInfo) {
    const char* platform = uvhttp_get_platform_info();
    
    ASSERT_NE(platform, nullptr);
    ASSERT_STRNE(platform, "");
    
    // Verify valid platforms
    bool valid_platform = (strcmp(platform, "Linux") == 0) ||
                          (strcmp(platform, "macOS") == 0) ||
                          (strcmp(platform, "Windows") == 0) ||
                          (strcmp(platform, "Unknown") == 0);
    ASSERT_TRUE(valid_platform);
}

/* ========== Print Function Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, PrintBuildInfo) {
    // Should not crash or assert
    testing::internal::CaptureStdout();
    uvhttp_print_build_info();
    std::string output = testing::internal::GetCapturedStdout();
    
    // Verify output contains expected sections
    ASSERT_NE(output.find("UVHTTP Build Information"), std::string::npos);
    ASSERT_NE(output.find("Version:"), std::string::npos);
    ASSERT_NE(output.find("Build:"), std::string::npos);
    ASSERT_NE(output.find("Features:"), std::string::npos);
    ASSERT_NE(output.find("Allocator:"), std::string::npos);
}

TEST_F(UvhttpVersionFullCoverageTest, PrintBuildInfoContainsVersion) {
    testing::internal::CaptureStdout();
    uvhttp_print_build_info();
    std::string output = testing::internal::GetCapturedStdout();
    
    const char* version = uvhttp_get_version_string();
    ASSERT_NE(output.find(version), std::string::npos);
}

TEST_F(UvhttpVersionFullCoverageTest, PrintBuildInfoContainsBuildType) {
    testing::internal::CaptureStdout();
    uvhttp_print_build_info();
    std::string output = testing::internal::GetCapturedStdout();
    
    const char* build_type = uvhttp_get_build_type();
    ASSERT_NE(output.find(build_type), std::string::npos);
}

/* ========== Integration Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, VersionAPIIntegration) {
    // Test that all version APIs work together consistently
    const char* version_str = uvhttp_get_version_string();
    int major, minor, patch;
    uvhttp_get_version(&major, &minor, &patch);
    int version_int = uvhttp_get_version_int();
    
    // Build expected version string
    char expected_str[32];
    snprintf(expected_str, sizeof(expected_str), "%d.%d.%d", major, minor, patch);
    EXPECT_STREQ(version_str, expected_str);
    
    // Verify integer version calculation
    int expected_int = major * 10000 + minor * 100 + patch;
    EXPECT_EQ(version_int, expected_int);
}

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoCompleteFields) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // Verify all string fields are non-null and non-empty
    EXPECT_NE(info.version_string, nullptr);
    EXPECT_STRNE(info.version_string, "");
    
    EXPECT_NE(info.build_type, nullptr);
    EXPECT_STRNE(info.build_type, "");
    
    EXPECT_NE(info.build_date, nullptr);
    EXPECT_STRNE(info.build_date, "");
    
    EXPECT_NE(info.build_time, nullptr);
    EXPECT_STRNE(info.build_time, "");
    
    EXPECT_NE(info.compiler, nullptr);
    EXPECT_STRNE(info.compiler, "");
    
    EXPECT_NE(info.platform, nullptr);
    EXPECT_STRNE(info.platform, "");
    
    EXPECT_NE(info.allocator_type, nullptr);
    EXPECT_STRNE(info.allocator_type, "");
    
    // TLS version can be NULL if TLS is disabled
    if (info.tls_enabled) {
        EXPECT_NE(info.tls_version, nullptr);
        EXPECT_STRNE(info.tls_version, "");
    }
}

TEST_F(UvhttpVersionFullCoverageTest, AllFeaturesCheckable) {
    // Test that all documented features can be checked
    const char* features[] = {
        "websocket",
        "static_files",
        "tls",
        "middleware",
        "logging",
        "router_cache",
        "lru_cache",
        "rate_limit",
        "protocol_upgrade",
        "allocator"
    };
    
    for (size_t i = 0; i < sizeof(features) / sizeof(features[0]); i++) {
        int result = uvhttp_is_feature_enabled(features[i]);
        EXPECT_GE(result, 0);
        EXPECT_LE(result, 1);
    }
}

/* ========== Edge Case Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, GetVersionStringMultipleCalls) {
    const char* v1 = uvhttp_get_version_string();
    const char* v2 = uvhttp_get_version_string();
    
    // Should return the same pointer (static allocation)
    EXPECT_EQ(v1, v2);
}

TEST_F(UvhttpVersionFullCoverageTest, GetBuildInfoMultipleCalls) {
    uvhttp_build_info_t info1, info2;
    uvhttp_get_build_info(&info1);
    uvhttp_get_build_info(&info2);
    
    // Version strings should be the same pointer
    EXPECT_EQ(info1.version_string, info2.version_string);
    EXPECT_EQ(info1.build_type, info2.build_type);
    EXPECT_EQ(info1.compiler, info2.compiler);
    EXPECT_EQ(info1.platform, info2.platform);
}

TEST_F(UvhttpVersionFullCoverageTest, HelperFunctionsMultipleCalls) {
    const char* a1 = uvhttp_get_allocator_type();
    const char* a2 = uvhttp_get_allocator_type();
    EXPECT_EQ(a1, a2);
    
    const char* b1 = uvhttp_get_build_type();
    const char* b2 = uvhttp_get_build_type();
    EXPECT_EQ(b1, b2);
    
    const char* c1 = uvhttp_get_compiler_info();
    const char* c2 = uvhttp_get_compiler_info();
    EXPECT_EQ(c1, c2);
    
    const char* p1 = uvhttp_get_platform_info();
    const char* p2 = uvhttp_get_platform_info();
    EXPECT_EQ(p1, p2);
}

/* ========== Feature Flag Verification Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoFeatureFlagsRange) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // All feature flags should be 0 or 1
    EXPECT_GE(info.feature_websocket, 0);
    EXPECT_LE(info.feature_websocket, 1);
    
    EXPECT_GE(info.feature_static_files, 0);
    EXPECT_LE(info.feature_static_files, 1);
    
    EXPECT_GE(info.feature_tls, 0);
    EXPECT_LE(info.feature_tls, 1);
    
    EXPECT_GE(info.feature_middleware, 0);
    EXPECT_LE(info.feature_middleware, 1);
    
    EXPECT_GE(info.feature_logging, 0);
    EXPECT_LE(info.feature_logging, 1);
    
    EXPECT_GE(info.feature_router_cache, 0);
    EXPECT_LE(info.feature_router_cache, 1);
    
    EXPECT_GE(info.feature_lru_cache, 0);
    EXPECT_LE(info.feature_lru_cache, 1);
    
    EXPECT_GE(info.feature_rate_limit, 0);
    EXPECT_LE(info.feature_rate_limit, 1);
}

/* ========== Memory Configuration Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoMemoryConfiguration) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    // Verify memory configuration values are reasonable
    EXPECT_GT(info.max_connections, 0);
    EXPECT_GT(info.max_headers, 0);
    EXPECT_GT(info.max_body_size, 0);
    EXPECT_GT(info.buffer_size, 0);
    
    // Verify reasonable ranges
    EXPECT_LE(info.max_connections, 100000);
    EXPECT_LE(info.max_headers, 1000);
    EXPECT_LE(info.buffer_size, 1048576); // 1MB max buffer size
}

/* ========== Router Configuration Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoRouterConfiguration) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    if (info.feature_router_cache) {
        EXPECT_GT(info.router_hash_size, 0);
        EXPECT_GT(info.router_hot_cache_size, 0);
        EXPECT_GT(info.router_hybrid_threshold, 0);
    }
}

/* ========== Cache Configuration Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoCacheConfiguration) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    if (info.feature_lru_cache) {
        EXPECT_GT(info.lru_cache_size, 0);
        EXPECT_GT(info.lru_cache_max_memory, 0);
    }
}

/* ========== TLS Configuration Tests ========== */

TEST_F(UvhttpVersionFullCoverageTest, BuildInfoTLSConfiguration) {
    uvhttp_build_info_t info;
    uvhttp_get_build_info(&info);
    
    if (info.tls_enabled) {
        EXPECT_NE(info.tls_version, nullptr);
        EXPECT_STRNE(info.tls_version, "");
    } else {
        // TLS version can be NULL when TLS is disabled
        // This is acceptable
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}