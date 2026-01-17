/* uvhttp_error.c 覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_error.h"

TEST(UvhttpErrorCoverageTest, ErrorString) {
    const char* str;

    str = uvhttp_error_string(UVHTTP_OK);
    EXPECT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(str, nullptr);

    str = uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE(str, nullptr);

    str = uvhttp_error_string((uvhttp_error_t)9999);
    EXPECT_NE(str, nullptr);
}

TEST(UvhttpErrorCoverageTest, SetErrorRecoveryConfig) {
    uvhttp_set_error_recovery_config(3, 100, 5000, 2.0);
}

TEST(UvhttpErrorCoverageTest, ResetErrorStats) {
    uvhttp_reset_error_stats();
}