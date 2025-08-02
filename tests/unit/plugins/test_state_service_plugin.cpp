#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/istore.h>
#include <neo/plugins/state_service_plugin.h>
#include <neo/rpc/rpc_server.h>
#include <string>
#include <unordered_map>

using namespace neo::plugins;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::io;

class StateServicePluginTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NeoSystem> neoSystem_;
    std::shared_ptr<RpcServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;
    std::string tempDir_;

    void SetUp() override
    {
        // Create temporary directory
        tempDir_ = std::filesystem::temp_directory_path().string() + "/neo_test_state";
        std::filesystem::create_directories(tempDir_);

        // Create node
        auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
        protocolSettings->SetNetwork(0x334F454E);
        neoSystem_ = std::make_shared<NeoSystem>(protocolSettings);
        RpcConfig config;
        config.port = 10332;
        rpcServer_ = std::make_shared<RpcServer>(config);
    }

    void TearDown() override
    {
        rpcServer_.reset();
        neoSystem_.reset();

        // Remove temporary directory
        std::filesystem::remove_all(tempDir_);
    }
};

TEST_F(StateServicePluginTest, Constructor)
{
    StateServicePlugin plugin;
    EXPECT_EQ(plugin.GetName(), "StateService");
    EXPECT_EQ(plugin.GetDescription(), "Provides state service functionality");
    EXPECT_EQ(plugin.GetVersion(), "1.0");
    EXPECT_EQ(plugin.GetAuthor(), "Neo C++ Team");
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(StateServicePluginTest, Initialize)
{
    StateServicePlugin plugin;

    // Initialize plugin
    bool result = plugin.Initialize(neoSystem_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(StateServicePluginTest, InitializeWithSettings)
{
    StateServicePlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"StatePath", tempDir_}};

    // Initialize plugin
    bool result = plugin.Initialize(neoSystem_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(StateServicePluginTest, StartStop)
{
    StateServicePlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"StatePath", tempDir_}};

    // Initialize plugin
    plugin.Initialize(neoSystem_, settings);

    // Start plugin
    bool result1 = plugin.Start();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(plugin.IsRunning());

    // Stop plugin
    bool result2 = plugin.Stop();
    EXPECT_TRUE(result2);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(StateServicePluginTest, GetStateRoot)
{
    StateServicePlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"StatePath", tempDir_}};

    // Initialize plugin
    plugin.Initialize(neoSystem_, settings);

    // Start plugin
    plugin.Start();

    // Get state root for non-existent block
    uint32_t index = 0;
    auto stateRoot1 = plugin.GetStateRoot(index);
    EXPECT_EQ(stateRoot1, nullptr);

    UInt256 hash;
    auto stateRoot2 = plugin.GetStateRoot(hash);
    EXPECT_EQ(stateRoot2, nullptr);

    // Stop plugin
    plugin.Stop();
}

TEST_F(StateServicePluginTest, Factory)
{
    StateServicePluginFactory factory;
    auto plugin = factory.CreatePlugin();
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->GetName(), "StateService");
}
