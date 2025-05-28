#include <gtest/gtest.h>
#include <neo/plugins/application_logs_plugin.h>
#include <neo/node/node.h>
#include <neo/rpc/rpc_server.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <neo/ledger/transaction.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/io/uint256.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

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
    std::shared_ptr<Node> node_;
    std::shared_ptr<RPCServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;
    std::string tempDir_;

    void SetUp() override
    {
        // Create temporary directory
        tempDir_ = std::filesystem::temp_directory_path().string() + "/neo_test_logs";
        std::filesystem::create_directories(tempDir_);
        
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
    bool result = plugin.Initialize(node_, rpcServer_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(ApplicationLogsPluginTest, InitializeWithSettings)
{
    ApplicationLogsPlugin plugin;
    
    // Set settings
    std::unordered_map<std::string, std::string> settings = {
        {"LogPath", tempDir_}
    };
    
    // Initialize plugin
    bool result = plugin.Initialize(node_, rpcServer_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(ApplicationLogsPluginTest, StartStop)
{
    ApplicationLogsPlugin plugin;
    
    // Set settings
    std::unordered_map<std::string, std::string> settings = {
        {"LogPath", tempDir_}
    };
    
    // Initialize plugin
    plugin.Initialize(node_, rpcServer_, settings);
    
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
    std::unordered_map<std::string, std::string> settings = {
        {"LogPath", tempDir_}
    };
    
    // Initialize plugin
    plugin.Initialize(node_, rpcServer_, settings);
    
    // Start plugin
    plugin.Start();
    
    // Get application log for non-existent transaction
    UInt256 txHash;
    auto log = plugin.GetApplicationLog(txHash);
    EXPECT_EQ(log, nullptr);
    
    // Stop plugin
    plugin.Stop();
}

TEST_F(ApplicationLogsPluginTest, Factory)
{
    ApplicationLogsPluginFactory factory;
    auto plugin = factory.CreatePlugin();
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->GetName(), "ApplicationLogs");
}
