#include <gtest/gtest.h>

// Minimal test just to ensure the build system works
TEST(NativeContractsMinimalTest, TestBasicOperation)
{
    ASSERT_TRUE(true);
    ASSERT_EQ(1 + 1, 2);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}