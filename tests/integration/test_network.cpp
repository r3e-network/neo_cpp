#include <gtest/gtest.h>
#include <neo/network/p2p_server.h>

using namespace neo::network;

TEST(NetworkIntegrationTest, TestP2PServerInitialization) {
    // Create P2P server
    auto server = std::make_shared<P2PServer>(10333);
    
    // Check initial state
    EXPECT_FALSE(server->IsRunning());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
