#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <neo/cryptography/ecc/key_pair.h>
#include <neo/node/node.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <neo/plugins/dbft_plugin.h>
#include <neo/rpc/rpc_server.h>
#include <neo/wallets/wallet.h>
#include <string>
#include <unordered_map>

using namespace neo::plugins;
using namespace neo::node;
using namespace neo::rpc;
using namespace neo::persistence;
using namespace neo::wallets;
using namespace neo::cryptography::ecc;

class DBFTPluginTest : public ::testing::Test
{
  protected:
    std::shared_ptr<Node> node_;
    std::shared_ptr<RPCServer> rpcServer_;
    std::unordered_map<std::string, std::string> settings_;
    std::string tempDir_;

    void SetUp() override
    {
        // Create temporary directory
        tempDir_ = std::filesystem::temp_directory_path().string() + "/neo_test_dbft";
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

TEST_F(DBFTPluginTest, Constructor)
{
    DBFTPlugin plugin;
    EXPECT_EQ(plugin.GetName(), "DBFT");
    EXPECT_EQ(plugin.GetDescription(), "Provides DBFT consensus functionality");
    EXPECT_EQ(plugin.GetVersion(), "1.0");
    EXPECT_EQ(plugin.GetAuthor(), "Neo C++ Team");
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(DBFTPluginTest, Initialize)
{
    DBFTPlugin plugin;

    // Initialize plugin
    bool result = plugin.Initialize(node_, rpcServer_, settings_);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(DBFTPluginTest, InitializeWithSettings)
{
    DBFTPlugin plugin;

    // Set settings
    std::unordered_map<std::string, std::string> settings = {
        {"WalletPath", tempDir_ + "/wallet.json"}, {"WalletPassword", "password"}, {"AutoStart", "true"}};

    // Initialize plugin
    bool result = plugin.Initialize(node_, rpcServer_, settings);
    EXPECT_TRUE(result);
    EXPECT_FALSE(plugin.IsRunning());
}

TEST_F(DBFTPluginTest, StartStop)
{
    DBFTPlugin plugin;

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

TEST_F(DBFTPluginTest, IsConsensusRunning)
{
    DBFTPlugin plugin;

    // Initialize plugin
    plugin.Initialize(node_, rpcServer_, settings_);

    // Check if consensus is running
    EXPECT_FALSE(plugin.IsConsensusRunning());

    // Start plugin
    plugin.Start();

    // Check if consensus is running
    EXPECT_FALSE(plugin.IsConsensusRunning());

    // Stop plugin
    plugin.Stop();
}

TEST_F(DBFTPluginTest, Factory)
{
    DBFTPluginFactory factory;
    auto plugin = factory.CreatePlugin();
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->GetName(), "DBFT");
}
