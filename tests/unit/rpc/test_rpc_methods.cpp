#include <gtest/gtest.h>
#include <memory>
#include <neo/io/json.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/rpc/rpc_methods.h>

using namespace neo::rpc;
using namespace neo::node;
using namespace neo::persistence;
using namespace neo::io;

class RPCMethodsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create protocol settings
        auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
        
        // Create a neo system with memory store
        neoSystem = std::make_shared<NeoSystem>(protocolSettings);
    }

    std::shared_ptr<NeoSystem> neoSystem;
};

TEST_F(RPCMethodsTest, GetVersion)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetVersion(neoSystem, params);

    EXPECT_TRUE(result.contains("port"));
    EXPECT_TRUE(result.contains("nonce"));
    EXPECT_TRUE(result.contains("useragent"));
}

TEST_F(RPCMethodsTest, GetBlockCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetBlockCount(neoSystem, params);

    EXPECT_GE(result.get<uint32_t>(), 0);
}

TEST_F(RPCMethodsTest, GetConnectionCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetConnectionCount(neoSystem, params);

    EXPECT_GE(result.get<uint32_t>(), 0);
}

TEST_F(RPCMethodsTest, GetPeers)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetPeers(neoSystem, params);

    EXPECT_TRUE(result.contains("connected"));
    EXPECT_TRUE(result["connected"].is_array());
}

TEST_F(RPCMethodsTest, GetCommittee)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetCommittee(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetValidators(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetNextBlockValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetNextBlockValidators(neoSystem, params);

    EXPECT_TRUE(result.is_array());
}
