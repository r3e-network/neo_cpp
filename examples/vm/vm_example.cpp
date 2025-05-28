#include <iostream>
#include <vector>
#include <memory>
#include <gtest/gtest.h>

// Simple test to verify that the build system works
TEST(SimpleTest, Addition) {
    int a = 1;
    int b = 2;
    int result = a + b;
    EXPECT_EQ(result, 3);
}

int main(int argc, char** argv) {
    std::cout << "Running simple test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
