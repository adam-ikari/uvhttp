#include <gtest/gtest.h>
#include <stddef.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "uvhttp_error.h"

/* 测试上下文完整初始化流程 */
TEST(UvhttpContextInitCoverageTest, FullInitialization) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uvhttp_context_t* context = nullptr;
    uvhttp_error_t result = uvhttp_context_create(loop, &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);

    /* 测试完整初始化 */
    result = uvhttp_context_init(context);
    ASSERT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(context->initialized, 1);

    /* 测试幂等性 - 再次初始化应该成功 */
    result = uvhttp_context_init(context);
    ASSERT_EQ(result, UVHTTP_OK);

    /* 清理 */
    uvhttp_context_destroy(context);
}

/* 测试上下文初始化失败场景 */
TEST(UvhttpContextInitCoverageTest, InitWithNullContext) {
    uvhttp_error_t result = uvhttp_context_init(nullptr);
    ASSERT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* 测试上下文销毁空指针 */
TEST(UvhttpContextInitCoverageTest, DestroyNullContext) {
    uvhttp_context_destroy(nullptr);
    /* 应该不会崩溃 */
}