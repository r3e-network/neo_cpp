#include <iostream>
#include <gtest/gtest.h>

// Simple test to verify that the Google Test framework works
TEST(SimpleTest, Addition) {
    int a = 1;
    int b = 2;
    int result = a + b;
    EXPECT_EQ(result, 3);
}

int main(int argc, char** argv) {
    std::cout << "Running Google Test example..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
