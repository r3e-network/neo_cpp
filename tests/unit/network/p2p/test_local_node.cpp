#include <gtest/gtest.h>
#include <memory>
#include <neo/network/p2p/local_node.h>
#include <neo/core/protocol_settings.h>
#include <neo/persistence/memory_store.h>
#include <string>
#include <vector>

using namespace neo;
using namespace neo::network::p2p;

/**
 * @brief Production test suite for LocalNode
 * Converted from C# UT_LocalNode.cs with full functionality
 */
class LocalNodeTest : public testing::Test
{
protected:
    std::shared_ptr<LocalNode> local_node_;
    std::shared_ptr<core::ProtocolSettings> protocol_settings_;
    std::shared_ptr<persistence::MemoryStore> store_;

    void SetUp() override
    {
        // Initialize production test environment
        protocol_settings_ = std::make_shared<core::ProtocolSettings>();
        store_ = std::make_shared<persistence::MemoryStore>();
        
        // Create LocalNode with proper configuration
        LocalNodeConfig config;
        config.port = 20333;  // Test port
        config.bind_address = "127.0.0.1";
        config.max_connections = 10;
        
        local_node_ = std::make_shared<LocalNode>(config, protocol_settings_, store_);
    }

    void TearDown() override
    {
        // Clean shutdown
        if (local_node_) {
            local_node_->Stop();
            local_node_.reset();
        }
        store_.reset();
        protocol_settings_.reset();
    }
};

// Test LocalNode initialization (converted from C# test)
TEST_F(LocalNodeTest, TestInitialization)
{
    ASSERT_NE(local_node_, nullptr);
    EXPECT_FALSE(local_node_->IsRunning());
    EXPECT_EQ(local_node_->GetConnectedPeersCount(), 0);
}

// Test LocalNode start/stop functionality
TEST_F(LocalNodeTest, TestStartStop)
{
    EXPECT_FALSE(local_node_->IsRunning());
    
    // Start the node
    local_node_->Start();
    EXPECT_TRUE(local_node_->IsRunning());
    
    // Stop the node
    local_node_->Stop();
    EXPECT_FALSE(local_node_->IsRunning());
}

// Test peer connection management
TEST_F(LocalNodeTest, TestPeerManagement)
{
    local_node_->Start();
    
    // Initially no peers
    EXPECT_EQ(local_node_->GetConnectedPeersCount(), 0);
    
    // Test peer list operations
    auto peers = local_node_->GetConnectedPeers();
    EXPECT_EQ(peers.size(), 0);
    
    local_node_->Stop();
}

// Test message broadcasting
TEST_F(LocalNodeTest, TestMessageBroadcast)
{
    local_node_->Start();
    
    // Test message creation and broadcast
    auto message = std::make_shared<Message>(MessageCommand::Ping, io::ByteVector{0x01, 0x02});
    
    // Broadcasting should not throw
    EXPECT_NO_THROW(local_node_->Broadcast(message));
    
    local_node_->Stop();
}

// Test network configuration
TEST_F(LocalNodeTest, TestNetworkConfiguration)
{
    auto config = local_node_->GetConfiguration();
    
    EXPECT_EQ(config.port, 20333);
    EXPECT_EQ(config.bind_address, "127.0.0.1");
    EXPECT_EQ(config.max_connections, 10);
}

// Test protocol compliance
TEST_F(LocalNodeTest, TestProtocolCompliance)
{
    EXPECT_NE(local_node_->GetProtocolSettings(), nullptr);
    
    auto magic = local_node_->GetProtocolSettings()->GetMagicNumber();
    EXPECT_NE(magic, 0);  // Should have valid magic number
}
