#include <gtest/gtest.h>
#include <neo/sdk.h>

// Test SDK initialization
TEST(SDKTest, Initialize) {
    EXPECT_TRUE(neo::sdk::Initialize());
    neo::sdk::Shutdown();
}

// Test SDK version
TEST(SDKTest, Version) {
    std::string version = neo::sdk::GetVersion();
    EXPECT_EQ(version, "1.0.0");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}