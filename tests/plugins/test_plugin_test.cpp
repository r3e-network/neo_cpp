#include <gtest/gtest.h>
#include <memory>
#include <neo/node/neo_system.h>
#include <neo/persistence/memory_store_provider.h>
#include <neo/plugins/plugin_manager.h>
#include <neo/plugins/test_plugin.h>

using namespace neo;
using namespace neo::plugins;

class TestPluginTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create store provider
        auto storeProvider = std::make_shared<persistence::MemoryStoreProvider>();

        // Create Neo system
        neoSystem_ = std::make_shared<node::NeoSystem>(storeProvider);

        // Create test plugin
        plugin_ = std::make_shared<TestPlugin>();
    }

    void TearDown() override
    {
        plugin_.reset();
        neoSystem_.reset();
    }

    std::shared_ptr<node::NeoSystem> neoSystem_;
    std::shared_ptr<TestPlugin> plugin_;
};

TEST_F(TestPluginTest, TestInitialize)
{
    // Initialize plugin
    std::unordered_map<std::string, std::string> settings;
    EXPECT_TRUE(plugin_->Initialize(neoSystem_, settings));

    // Check plugin properties
    EXPECT_EQ("Test", plugin_->GetName());
    EXPECT_EQ("A test plugin", plugin_->GetDescription());
    EXPECT_EQ("1.0", plugin_->GetVersion());
    EXPECT_EQ("Neo C++ Team", plugin_->GetAuthor());
}

TEST_F(TestPluginTest, TestStartStop)
{
    // Initialize plugin
    std::unordered_map<std::string, std::string> settings;
    EXPECT_TRUE(plugin_->Initialize(neoSystem_, settings));

    // Start plugin
    EXPECT_TRUE(plugin_->Start());
    EXPECT_TRUE(plugin_->IsRunning());

    // Stop plugin
    EXPECT_TRUE(plugin_->Stop());
    EXPECT_FALSE(plugin_->IsRunning());
}

TEST_F(TestPluginTest, TestMessage)
{
    // Initialize plugin
    std::unordered_map<std::string, std::string> settings;
    EXPECT_TRUE(plugin_->Initialize(neoSystem_, settings));

    // Test message
    EXPECT_TRUE(plugin_->TestOnMessage("Test message"));
}

TEST_F(TestPluginTest, TestPluginManager)
{
    // Get plugin manager
    auto& manager = PluginManager::GetInstance();

    // Register plugin factory
    manager.RegisterPluginFactory(std::make_shared<TestPluginFactory>());

    // Load plugins
    std::unordered_map<std::string, std::string> settings;
    EXPECT_TRUE(manager.LoadPlugins(neoSystem_, settings));

    // Get plugins
    auto plugins = manager.GetPlugins();
    EXPECT_FALSE(plugins.empty());

    // Get plugin
    auto plugin = manager.GetPlugin("Test");
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ("Test", plugin->GetName());

    // Start plugins
    EXPECT_TRUE(manager.StartPlugins());

    // Stop plugins
    EXPECT_TRUE(manager.StopPlugins());
}
