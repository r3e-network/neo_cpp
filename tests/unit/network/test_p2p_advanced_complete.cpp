#include <gtest/gtest.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/capabilities/server_capability.h>
#include <neo/network/p2p/capabilities/full_node_capability.h>
#include <neo/network/tcp_connection.h>
#include <neo/core/neo_system.h>
#include <chrono>
#include <thread>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::network::p2p::capabilities;
using namespace neo::network;
using namespace neo::core;

namespace neo::network::p2p::tests
{

class P2PAdvancedCompleteTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test neo system
        mock_system = std::make_shared<NeoSystem>();
        
        // Create local node
        local_node = std::make_unique<LocalNode>(mock_system);
        
        // Initialize test endpoints
        local_endpoint = IPEndpoint("127.0.0.1", 10333);
        remote_endpoint = IPEndpoint("127.0.0.1", 10334);
        
        // Setup test capabilities
        server_capability = std::make_shared<ServerCapability>(ServerCapability::Type::TcpServer, 10333);
        full_node_capability = std::make_shared<FullNodeCapability>(12345); // Start height
    }

    void TearDown() override
    {
        if (local_node && local_node->IsRunning()) {
            local_node->Stop();
        }
    }

    std::shared_ptr<NeoSystem> mock_system;
    std::unique_ptr<LocalNode> local_node;
    IPEndpoint local_endpoint;
    IPEndpoint remote_endpoint;
    std::shared_ptr<ServerCapability> server_capability;
    std::shared_ptr<FullNodeCapability> full_node_capability;
};

// LocalNode Tests - Matching C# UT_LocalNode.cs
TEST_F(P2PAdvancedCompleteTest, LocalNodeInitialization)
{
    // Test: LocalNode initialization and configuration
    EXPECT_FALSE(local_node->IsRunning());
    EXPECT_EQ(local_node->GetConnectedCount(), 0);
    EXPECT_EQ(local_node->GetUnconnectedCount(), 0);
    
    // Configure local node
    local_node->Configure(local_endpoint, 20); // Max 20 connections
    
    // Start the node
    EXPECT_TRUE(local_node->Start());
    EXPECT_TRUE(local_node->IsRunning());
    EXPECT_EQ(local_node->GetListenPort(), 10333);
}

TEST_F(P2PAdvancedCompleteTest, LocalNodePeerManagement)
{
    // Test: Peer management functionality
    local_node->Configure(local_endpoint, 20);
    local_node->Start();
    
    // Add peer addresses
    std::vector<IPEndpoint> peer_addresses = {
        IPEndpoint("127.0.0.1", 10334),
        IPEndpoint("127.0.0.1", 10335),
        IPEndpoint("127.0.0.1", 10336)
    };
    
    for (const auto& address : peer_addresses) {
        local_node->AddPeerAddress(address);
    }
    
    EXPECT_GE(local_node->GetUnconnectedCount(), peer_addresses.size());
    
    // Test connection attempts
    local_node->ConnectToPeers();
    
    // Wait for connection attempts
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Note: In real test, peers would need to be actually running
    // Here we're testing the connection attempt mechanism
}

TEST_F(P2PAdvancedCompleteTest, LocalNodeCapabilityAdvertisement)
{
    // Test: Node capability advertisement
    local_node->Configure(local_endpoint, 20);
    
    // Add capabilities
    local_node->AddCapability(server_capability);
    local_node->AddCapability(full_node_capability);
    
    auto capabilities = local_node->GetCapabilities();
    EXPECT_EQ(capabilities.size(), 2);
    
    // Verify capabilities are properly set
    bool has_server_cap = false;
    bool has_fullnode_cap = false;
    
    for (const auto& cap : capabilities) {
        if (cap->GetType() == NodeCapabilityType::TcpServer) {
            has_server_cap = true;
            auto server_cap = std::static_pointer_cast<ServerCapability>(cap);
            EXPECT_EQ(server_cap->GetPort(), 10333);
        } else if (cap->GetType() == NodeCapabilityType::FullNode) {
            has_fullnode_cap = true;
            auto fullnode_cap = std::static_pointer_cast<FullNodeCapability>(cap);
            EXPECT_EQ(fullnode_cap->GetStartHeight(), 12345);
        }
    }
    
    EXPECT_TRUE(has_server_cap);
    EXPECT_TRUE(has_fullnode_cap);
}

// RemoteNode Tests - Matching C# UT_RemoteNode.cs
TEST_F(P2PAdvancedCompleteTest, RemoteNodeHandshake)
{
    // Test: Remote node handshake process
    auto connection = std::make_shared<TcpConnection>(remote_endpoint);
    auto remote_node = std::make_unique<RemoteNode>(mock_system, connection);
    
    // Create version message for handshake
    VersionPayload version;
    version.Version = PROTOCOL_VERSION;
    version.Services = NodeService::Network | NodeService::StateRoot;
    version.Timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    version.Port = 10333;
    version.Nonce = 123456789;
    version.UserAgent = "/NEO:3.6.0/";
    version.StartHeight = 12345;
    version.Relay = true;
    
    // Add capabilities
    version.Capabilities.push_back(server_capability);
    version.Capabilities.push_back(full_node_capability);
    
    // Process version message (simulate incoming handshake)
    EXPECT_TRUE(remote_node->ProcessVersionMessage(version));
    EXPECT_EQ(remote_node->GetVersion(), PROTOCOL_VERSION);
    EXPECT_EQ(remote_node->GetNonce(), 123456789);
    EXPECT_EQ(remote_node->GetStartHeight(), 12345);
    EXPECT_TRUE(remote_node->GetRelay());
}

TEST_F(P2PAdvancedCompleteTest, RemoteNodeMessageHandling)
{
    // Test: Remote node message processing
    auto connection = std::make_shared<TcpConnection>(remote_endpoint);
    auto remote_node = std::make_unique<RemoteNode>(mock_system, connection);
    
    // Complete handshake first
    VersionPayload version;
    version.Version = PROTOCOL_VERSION;
    version.Nonce = 123456789;
    version.UserAgent = "/NEO:3.6.0/";
    version.StartHeight = 12345;
    remote_node->ProcessVersionMessage(version);
    remote_node->CompleteHandshake();
    
    // Test ping message
    PingPayload ping;
    ping.LastBlockIndex = 12500;
    ping.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ping.Nonce = 987654321;
    
    EXPECT_TRUE(remote_node->ProcessPingMessage(ping));
    EXPECT_EQ(remote_node->GetLastBlockIndex(), 12500);
    
    // Test inventory message
    InvPayload inv;
    inv.Type = InventoryType::Block;
    inv.Hashes = {
        UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"),
        UInt256::Parse("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321")
    };
    
    EXPECT_TRUE(remote_node->ProcessInvMessage(inv));
    EXPECT_EQ(remote_node->GetKnownHashes().size(), 2);
}

// Message Processing Tests - Matching C# UT_Message.cs
TEST_F(P2PAdvancedCompleteTest, MessageSerialization)
{
    // Test: Message serialization/deserialization
    PingPayload ping;
    ping.LastBlockIndex = 12345;
    ping.Timestamp = 1640995200000; // Fixed timestamp for test
    ping.Nonce = 987654321;
    
    // Serialize message
    Message message(MessageCommand::Ping, ping);
    auto serialized = message.Serialize();
    
    EXPECT_GT(serialized.Size(), 0);
    EXPECT_EQ(message.GetCommand(), MessageCommand::Ping);
    EXPECT_EQ(message.GetMagic(), NETWORK_MAGIC);
    
    // Deserialize message
    Message deserialized_message;
    EXPECT_TRUE(deserialized_message.Deserialize(serialized.AsSpan()));
    EXPECT_EQ(deserialized_message.GetCommand(), MessageCommand::Ping);
    
    // Verify payload
    auto deserialized_ping = deserialized_message.GetPayload<PingPayload>();
    EXPECT_EQ(deserialized_ping.LastBlockIndex, 12345);
    EXPECT_EQ(deserialized_ping.Timestamp, 1640995200000);
    EXPECT_EQ(deserialized_ping.Nonce, 987654321);
}

TEST_F(P2PAdvancedCompleteTest, MessageValidation)
{
    // Test: Message validation
    PingPayload ping;
    ping.LastBlockIndex = 12345;
    ping.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ping.Nonce = 987654321;
    
    Message message(MessageCommand::Ping, ping);
    
    // Valid message
    EXPECT_TRUE(message.IsValid());
    EXPECT_TRUE(message.VerifyChecksum());
    
    // Test with invalid magic
    message.SetMagic(0x12345678); // Invalid magic
    EXPECT_FALSE(message.IsValid());
    
    // Reset to valid magic
    message.SetMagic(NETWORK_MAGIC);
    EXPECT_TRUE(message.IsValid());
}

// TaskManagerMailbox Tests - Matching C# UT_TaskManagerMailbox.cs
TEST_F(P2PAdvancedCompleteTest, TaskManagerMailbox)
{
    // Test: Task manager mailbox functionality
    auto mailbox = std::make_unique<TaskManagerMailbox>();
    
    // Test task scheduling
    bool task_executed = false;
    auto task = [&task_executed]() {
        task_executed = true;
    };
    
    mailbox->ScheduleTask(task);
    
    // Process tasks
    mailbox->ProcessTasks();
    
    EXPECT_TRUE(task_executed);
}

TEST_F(P2PAdvancedCompleteTest, TaskManagerPriorityHandling)
{
    // Test: Priority task handling
    auto mailbox = std::make_unique<TaskManagerMailbox>();
    
    std::vector<int> execution_order;
    
    // Schedule tasks with different priorities
    mailbox->ScheduleTask([&execution_order]() { execution_order.push_back(1); }, TaskPriority::Low);
    mailbox->ScheduleTask([&execution_order]() { execution_order.push_back(2); }, TaskPriority::High);
    mailbox->ScheduleTask([&execution_order]() { execution_order.push_back(3); }, TaskPriority::Medium);
    
    // Process all tasks
    mailbox->ProcessTasks();
    
    // High priority should execute first
    EXPECT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 2); // High priority
    EXPECT_EQ(execution_order[1], 3); // Medium priority  
    EXPECT_EQ(execution_order[2], 1); // Low priority
}

// TaskSession Tests - Matching C# UT_TaskSession.cs
TEST_F(P2PAdvancedCompleteTest, TaskSession)
{
    // Test: Task session management
    auto session = std::make_unique<TaskSession>();
    
    EXPECT_FALSE(session->IsActive());
    EXPECT_EQ(session->GetTaskCount(), 0);
    
    // Start session
    session->Start();
    EXPECT_TRUE(session->IsActive());
    
    // Add tasks
    session->AddTask([]() { /* Task 1 */ });
    session->AddTask([]() { /* Task 2 */ });
    session->AddTask([]() { /* Task 3 */ });
    
    EXPECT_EQ(session->GetTaskCount(), 3);
    
    // Execute tasks
    session->ExecuteAll();
    EXPECT_EQ(session->GetTaskCount(), 0); // Tasks should be cleared after execution
    
    // Stop session
    session->Stop();
    EXPECT_FALSE(session->IsActive());
}

// RemoteNodeMailbox Tests - Matching C# UT_RemoteNodeMailbox.cs
TEST_F(P2PAdvancedCompleteTest, RemoteNodeMailbox)
{
    // Test: Remote node mailbox functionality
    auto connection = std::make_shared<TcpConnection>(remote_endpoint);
    auto remote_node = std::make_unique<RemoteNode>(mock_system, connection);
    auto mailbox = remote_node->GetMailbox();
    
    // Test message queuing
    PingPayload ping;
    ping.LastBlockIndex = 12345;
    ping.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ping.Nonce = 987654321;
    
    Message message(MessageCommand::Ping, ping);
    
    // Queue message
    mailbox->QueueMessage(message);
    EXPECT_EQ(mailbox->GetQueueSize(), 1);
    
    // Process messages
    mailbox->ProcessMessages();
    EXPECT_EQ(mailbox->GetQueueSize(), 0); // Should be processed
}

TEST_F(P2PAdvancedCompleteTest, RemoteNodeMailboxOverflow)
{
    // Test: Mailbox overflow handling
    auto connection = std::make_shared<TcpConnection>(remote_endpoint);
    auto remote_node = std::make_unique<RemoteNode>(mock_system, connection);
    auto mailbox = remote_node->GetMailbox();
    
    // Configure small queue size for testing
    mailbox->SetMaxQueueSize(5);
    
    PingPayload ping;
    ping.LastBlockIndex = 12345;
    Message message(MessageCommand::Ping, ping);
    
    // Fill queue to capacity
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(mailbox->QueueMessage(message));
    }
    EXPECT_EQ(mailbox->GetQueueSize(), 5);
    
    // Next message should fail (queue full)
    EXPECT_FALSE(mailbox->QueueMessage(message));
    EXPECT_EQ(mailbox->GetQueueSize(), 5);
    
    // Process one message to make space
    mailbox->ProcessSingleMessage();
    EXPECT_EQ(mailbox->GetQueueSize(), 4);
    
    // Now should be able to queue again
    EXPECT_TRUE(mailbox->QueueMessage(message));
    EXPECT_EQ(mailbox->GetQueueSize(), 5);
}

// ChannelsConfig Tests - Matching C# UT_ChannelsConfig.cs
TEST_F(P2PAdvancedCompleteTest, ChannelsConfig)
{
    // Test: Network channels configuration
    ChannelsConfig config;
    
    // Test default configuration
    EXPECT_GT(config.GetTcpPort(), 0);
    EXPECT_GT(config.GetWsPort(), 0);
    EXPECT_GT(config.GetMaxConnections(), 0);
    EXPECT_GT(config.GetMaxConnectionsPerAddress(), 0);
    
    // Test configuration updates
    config.SetTcpPort(10333);
    config.SetWsPort(10334);
    config.SetMaxConnections(100);
    config.SetMaxConnectionsPerAddress(3);
    
    EXPECT_EQ(config.GetTcpPort(), 10333);
    EXPECT_EQ(config.GetWsPort(), 10334);
    EXPECT_EQ(config.GetMaxConnections(), 100);
    EXPECT_EQ(config.GetMaxConnectionsPerAddress(), 3);
}

TEST_F(P2PAdvancedCompleteTest, ChannelsConfigValidation)
{
    // Test: Configuration validation
    ChannelsConfig config;
    
    // Valid configurations
    EXPECT_TRUE(config.SetTcpPort(10333));
    EXPECT_TRUE(config.SetWsPort(10334));
    EXPECT_TRUE(config.SetMaxConnections(100));
    EXPECT_TRUE(config.SetMaxConnectionsPerAddress(3));
    
    // Invalid configurations
    EXPECT_FALSE(config.SetTcpPort(0));        // Invalid port
    EXPECT_FALSE(config.SetTcpPort(70000));    // Port too high
    EXPECT_FALSE(config.SetMaxConnections(0)); // Invalid connection count
    EXPECT_FALSE(config.SetMaxConnectionsPerAddress(0)); // Invalid per-address limit
}

// Performance and Load Tests
TEST_F(P2PAdvancedCompleteTest, HighVolumeMessageProcessing)
{
    // Test: High volume message processing
    local_node->Configure(local_endpoint, 50);
    local_node->Start();
    
    const int message_count = 1000;
    std::atomic<int> processed_messages{0};
    
    // Create test messages
    PingPayload ping;
    ping.LastBlockIndex = 12345;
    Message message(MessageCommand::Ping, ping);
    
    // Process messages in parallel
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; i++) {
        workers.emplace_back([&]() {
            for (int j = 0; j < message_count / 4; j++) {
                local_node->ProcessMessage(message);
                processed_messages++;
            }
        });
    }
    
    // Wait for all workers to complete
    for (auto& worker : workers) {
        worker.join();
    }
    
    EXPECT_EQ(processed_messages.load(), message_count);
}

TEST_F(P2PAdvancedCompleteTest, ConnectionStressTest)
{
    // Test: Connection stress testing
    local_node->Configure(local_endpoint, 100);
    local_node->Start();
    
    const int connection_count = 50;
    std::vector<std::unique_ptr<RemoteNode>> remote_nodes;
    
    // Create multiple remote node connections
    for (int i = 0; i < connection_count; i++) {
        auto endpoint = IPEndpoint("127.0.0.1", 10400 + i);
        auto connection = std::make_shared<TcpConnection>(endpoint);
        auto remote_node = std::make_unique<RemoteNode>(mock_system, connection);
        remote_nodes.push_back(std::move(remote_node));
    }
    
    // Simulate message exchange
    for (auto& remote_node : remote_nodes) {
        PingPayload ping;
        ping.LastBlockIndex = 12345;
        ping.Nonce = rand();
        
        // This would normally be sent over network
        EXPECT_TRUE(remote_node->ProcessPingMessage(ping));
    }
    
    EXPECT_EQ(remote_nodes.size(), connection_count);
}

}  // namespace neo::network::p2p::tests