#include <gtest/gtest.h>
#include <neo/plugins/rpc_server_plugin.h>
#include <neo/node/node.h>
#include <neo/rpc/rpc_server.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <memory>
#include <string>
#include <unordered_map>

using namespace neo::plugins;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::persistence;

class RpcServerPluginTest : public ::testing::Test
{
protected:
    std::shared_ptr<Node> node_;
    std::shared_ptr<RPCServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;

    void SetUp() override
    {
        // Create node
        auto store = std::make_shared<MemoryStore>();
        auto storeProvider = std::make_shared<StoreProvider>(store);
        node_ = std::make_shared<Node>(storeProvider, settings_);
        rpcServer_ = std::make_shared<RPCServer>(node_, 10332);
    }

    void TearDown() override
    {
        rpcServer_.reset();
        node_.reset();
    }
};

TEST_F(RpcServerPluginTest, Constructor)
{
    RpcServerPlugin plugin;
    EXPECT_EQ(plugin.GetName(), "RpcServer");
    EXPECT_EQ(plugin.GetDescription(), "Provides RPC server functionality");
    EXPECT_EQ(plugin.GetVersion(), "1.0");
    EXPECT_EQ(plugin.GetAuthor(), "Neo C++ Team");
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, Initialize)
{
    RpcServerPlugin plugin;
    
    // Initialize plugin
    bool result = plugin.Initialize(node_, rpcServer_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, InitializeWithSettings)
{
    RpcServerPlugin plugin;
    
    // Set settings
    std::unordered_map<std::string, std::string> settings = {
        {"Port", "10333"},
        {"EnableCors", "true"},
        {"EnableAuth", "true"},
        {"Username", "neo"},
        {"Password", "password"}
    };
    
    // Initialize plugin
    bool result = plugin.Initialize(node_, rpcServer_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, StartStop)
{
    RpcServerPlugin plugin;
    
    // Initialize plugin
    plugin.Initialize(node_, rpcServer_, settings_);
    
    // Start plugin
    bool result1 = plugin.Start();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(plugin.IsRunning());
    
    // Stop plugin
    bool result2 = plugin.Stop();
    EXPECT_TRUE(result2);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, RegisterMethod)
{
    RpcServerPlugin plugin;
    
    // Initialize plugin
    plugin.Initialize(node_, rpcServer_, settings_);
    
    // Register method
    plugin.RegisterMethod("test", [](const nlohmann::json& params) {
        return "test";
    });
    
    // Start plugin
    plugin.Start();
    
    // Stop plugin
    plugin.Stop();
}

TEST_F(RpcServerPluginTest, Factory)
{
    RpcServerPluginFactory factory;
    auto plugin = factory.CreatePlugin();
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->GetName(), "RpcServer");
}
