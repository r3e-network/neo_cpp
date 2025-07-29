#include <gtest/gtest.h>
#include <memory>
#include <neo/io/json.h>
#include <neo/node/node.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
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
        // Create a store provider with a memory store
        auto store = std::make_shared<MemoryStore>();
        auto storeProvider = std::make_shared<StoreProvider>(store);

        // Create a node
        std::unordered_map<std::string, std::string> settings;
        node = std::make_shared<Node>(storeProvider, settings);
    }

    std::shared_ptr<Node> node;
};

TEST_F(RPCMethodsTest, GetVersion)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetVersion(node, params);

    EXPECT_TRUE(result.contains("port"));
    EXPECT_TRUE(result.contains("nonce"));
    EXPECT_TRUE(result.contains("useragent"));
}

TEST_F(RPCMethodsTest, GetBlockCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetBlockCount(node, params);

    EXPECT_GE(result.get<uint32_t>(), 0);
}

TEST_F(RPCMethodsTest, GetConnectionCount)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetConnectionCount(node, params);

    EXPECT_GE(result.get<uint32_t>(), 0);
}

TEST_F(RPCMethodsTest, GetPeers)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetPeers(node, params);

    EXPECT_TRUE(result.contains("connected"));
    EXPECT_TRUE(result["connected"].is_array());
}

TEST_F(RPCMethodsTest, GetCommittee)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetCommittee(node, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetValidators(node, params);

    EXPECT_TRUE(result.is_array());
}

TEST_F(RPCMethodsTest, GetNextBlockValidators)
{
    nlohmann::json params = nlohmann::json::array();
    auto result = RPCMethods::GetNextBlockValidators(node, params);

    EXPECT_TRUE(result.is_array());
}
