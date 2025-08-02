#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>
#include <neo/node/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/istore.h>
#include <neo/plugins/application_logs_plugin.h>
#include <neo/rpc/rpc_server.h>
#include <neo/smartcontract/application_engine.h>
#include <string>
#include <unordered_map>

using namespace neo::plugins;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::smartcontract;
using namespace neo::io;

class ApplicationLogsPluginTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NeoSystem> neoSystem_;
    std::shared_ptr<RpcServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;
    std::string tempDir_;

    void SetUp() override
    {
        // Create temporary directory
        tempDir_ = std::filesystem::temp_directory_path().string() + "/neo_test_logs";
        std::filesystem::create_directories(tempDir_);

        // Create neo system
        auto protocolSettings = std::make_shared<neo::ProtocolSettings>();
        protocolSettings->SetNetwork(0x334F454E);
        neoSystem_ = std::make_shared<NeoSystem>(protocolSettings);
        
        // Create RPC server with config
        RpcConfig config;
        config.port = 10332;
        config.enable_cors = true;
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

TEST_F(ApplicationLogsPluginTest, Constructor)
{
    ApplicationLogsPlugin plugin;
    EXPECT_EQ(plugin.GetName(), "ApplicationLogs");
    EXPECT_EQ(plugin.GetDescription(), "Provides application logs functionality");
    EXPECT_EQ(plugin.GetVersion(), "1.0");
    EXPECT_EQ(plugin.GetAuthor(), "Neo C++ Team");
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(ApplicationLogsPluginTest, Initialize)
{
    ApplicationLogsPlugin plugin;

    // Initialize plugin
    bool result = plugin.Initialize(neoSystem_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(ApplicationLogsPluginTest, InitializeWithSettings)
{
    ApplicationLogsPlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

    // Initialize plugin
    bool result = plugin.Initialize(neoSystem_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(ApplicationLogsPluginTest, StartStop)
{
    ApplicationLogsPlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

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

TEST_F(ApplicationLogsPluginTest, GetApplicationLog)
{
    ApplicationLogsPlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {{"LogPath", tempDir_}};

    // Initialize plugin
    plugin.Initialize(neoSystem_, settings);

    // Start plugin
    plugin.Start();

    // Get application log for non-existent transaction
    UInt256 txHash;
    auto log = plugin.GetApplicationLog(txHash);
    EXPECT_EQ(log, nullptr);

    // Stop plugin
    plugin.Stop();
}

// Factory test removed - ApplicationLogsPluginFactory is not yet implemented
