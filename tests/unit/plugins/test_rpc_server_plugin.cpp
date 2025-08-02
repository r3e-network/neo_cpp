#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <neo/node/neo_system.h>
#include <neo/persistence/memory_store.h>
#include <neo/plugins/rpc_server_plugin.h>
#include <neo/rpc/rpc_server.h>
#include <string>
#include <unordered_map>
#include <neo/protocol_settings.h>

using namespace neo::plugins;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::persistence;
using namespace neo;

class RpcServerPluginTest : public ::testing::Test
{
  protected:
    std::shared_ptr<neo::node::NeoSystem> neoSystem_;
    std::shared_ptr<RpcServerPlugin> plugin_;
    std::unordered_map<std::string, std::string> settings_;

    void SetUp() override
    {
        // Create protocol settings
        auto protocolSettings = std::make_shared<ProtocolSettings>();
        protocolSettings->SetNetwork(0x334F454E);
        protocolSettings->SetMillisecondsPerBlock(15000);
        
        // Create neo system
        neoSystem_ = std::make_shared<neo::node::NeoSystem>(protocolSettings);
        
        // Create plugin
        plugin_ = std::make_shared<RpcServerPlugin>();
        
        // Default settings
        settings_["port"] = "10332";
        settings_["enableCors"] = "true";
        settings_["enableAuth"] = "false";
    }

    void TearDown() override
    {
        if (plugin_ && plugin_->IsRunning())
        {
            plugin_->Stop();
        }
        plugin_.reset();
        neoSystem_.reset();
    }
};

TEST_F(RpcServerPluginTest, Constructor)
{
    RpcServerPlugin plugin;
    EXPECT_EQ(plugin.GetName(), "RpcServer");
    EXPECT_FALSE(plugin.GetDescription().empty());
    EXPECT_FALSE(plugin.GetVersion().empty());
    EXPECT_FALSE(plugin.GetAuthor().empty());
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, Initialize)
{
    // Initialize plugin with system and settings
    bool result = plugin_->Initialize(neoSystem_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin_->IsRunning());
}

TEST_F(RpcServerPluginTest, InitializeWithSettings)
{
    RpcServerPlugin plugin;
    
    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"port", "10333"},
                                                             {"enableCors", "true"},
                                                             {"enableAuth", "true"},
                                                             {"username", "neo"},
                                                             {"password", "password"}};

    // Initialize plugin with system and settings
    bool result = plugin.Initialize(neoSystem_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(RpcServerPluginTest, StartStop)
{
    // Initialize plugin
    plugin_->Initialize(neoSystem_, settings_);

    // Start plugin
    bool result1 = plugin_->Start();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(plugin_->IsRunning());

    // Stop plugin
    bool result2 = plugin_->Stop();
    EXPECT_TRUE(result2);
    EXPECT_FALSE(plugin_->IsRunning());
}

TEST_F(RpcServerPluginTest, RegisterMethod)
{
    // Initialize plugin
    plugin_->Initialize(neoSystem_, settings_);

    // Register method
    plugin_->RegisterMethod("test", [](const nlohmann::json& params) { 
        nlohmann::json result;
        result["response"] = "test";
        return result;
    });

    // Start plugin
    plugin_->Start();

    // Stop plugin
    plugin_->Stop();
}

TEST_F(RpcServerPluginTest, Factory)
{
    RpcServerPluginFactory factory;
    auto plugin = factory.CreatePlugin();
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->GetName(), "RpcServer");
}
