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

/* 错误恢复和统计功能已删除，符合极简工程原则 */