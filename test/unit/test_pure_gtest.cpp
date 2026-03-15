/*
 * Pure Google Test - Test Google Test framework itself for memory leaks
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

/* Pure Google Test - Does not use any UVHTTP code */
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