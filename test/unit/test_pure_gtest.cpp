/*
 * Pure Google Test - 测试 Google Test 框架本身是否有内存泄漏
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

/* 纯 Google Test 测试 - 不使用任何 UVHTTP 代码 */
TEST(PureGTest, StringOperations) {
    std::string str = "Hello, World!";
    str += " This is a test.";
    ASSERT_EQ(str.length(), 29);
}

TEST(PureGTest, VectorOperations) {
    std::vector<int> vec;
    for (int i = 0; i < 100; i++) {
        vec.push_back(i);
    }
    ASSERT_EQ(vec.size(), 100);
}

TEST(PureGTest, NestedVectors) {
    std::vector<std::vector<std::string>> nested;
    for (int i = 0; i < 10; i++) {
        std::vector<std::string> inner;
        for (int j = 0; j < 10; j++) {
            inner.push_back("item_" + std::to_string(j));
        }
        nested.push_back(inner);
    }
    ASSERT_EQ(nested.size(), 10);
    ASSERT_EQ(nested[0].size(), 10);
}