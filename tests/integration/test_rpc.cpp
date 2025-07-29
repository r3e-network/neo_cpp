#include <gtest/gtest.h>
#include <neo/rpc/rpc_server.h>

using namespace neo::rpc;

TEST(RPCIntegrationTest, TestRPCServerInitialization)
{
    // Create RPC server
    auto server = std::make_shared<RPCServer>("127.0.0.1", 10332);

    // Check initial state
    EXPECT_FALSE(server->IsRunning());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
