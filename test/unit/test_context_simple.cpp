#include <gtest/gtest.h>
#include <stddef.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "test_loop_helper.h"

/* 测试上下文创建和销毁 */
TEST(UvhttpContextSimpleTest, CreateDestroy) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());

    uvhttp_context_t* context;
    uvhttp_error_t result = uvhttp_context_create(loop.get(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);
    EXPECT_EQ(context->loop, loop.get());
    EXPECT_EQ(context->initialized, 0);

    uvhttp_context_destroy(context);
}