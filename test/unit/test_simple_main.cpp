#include <gtest/gtest.h>

TEST(SimpleTest, One) {
    EXPECT_EQ(1, 1);
}

TEST(SimpleTest, Two) {
    EXPECT_EQ(2, 2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}