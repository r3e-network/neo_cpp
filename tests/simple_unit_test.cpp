#include <gtest/gtest.h>

// Simple test to verify the test infrastructure is working
TEST(SimpleTest, BasicAssertions) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
    EXPECT_NE(1, 2);
}

TEST(SimpleTest, StringComparison) {
    std::string hello = "Hello";
    EXPECT_EQ(hello, "Hello");
    EXPECT_NE(hello, "World");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}