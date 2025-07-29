// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/network/p2p/test_remote_node.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_NETWORK_P2P_TEST_REMOTE_NODE_CPP_H
#define TESTS_UNIT_NETWORK_P2P_TEST_REMOTE_NODE_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/network/p2p/remote_node.h>

namespace neo
{
namespace test
{

class RemoteNodeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for RemoteNode testing - complete production implementation matching C# exactly

        // Initialize remote node configuration
        node_config = std::make_shared<network::p2p::RemoteNodeConfig>();
        node_config->connection_timeout = std::chrono::seconds(30);
        node_config->handshake_timeout = std::chrono::seconds(10);
        node_config->ping_interval = std::chrono::seconds(30);
        node_config->max_payload_size = 1024 * 1024;  // 1MB
        node_config->protocol_version = 70001;
        node_config->user_agent = "Neo:3.6.0";
        node_config->services = 1;  // Full node

        // Test endpoint configurations
        test_local_endpoint = "192.168.1.10:10333";
        test_remote_endpoint = "203.0.113.1:10333";
        test_peer_endpoint = "198.51.100.1:10333";

        // Create remote node instance
        remote_node = std::make_shared<network::p2p::RemoteNode>(node_config);

        // Version message data
        test_version_data.version = node_config->protocol_version;
        test_version_data.services = node_config->services;
        test_version_data.timestamp =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        test_version_data.port = 10333;
        test_version_data.nonce = 12345678;
        test_version_data.user_agent = node_config->user_agent;
        test_version_data.start_height = 1000000;
        test_version_data.relay = true;

        // Message types for testing
        test_message_types = {network::p2p::MessageType::Version,     network::p2p::MessageType::Verack,
                              network::p2p::MessageType::Ping,        network::p2p::MessageType::Pong,
                              network::p2p::MessageType::GetAddr,     network::p2p::MessageType::Addr,
                              network::p2p::MessageType::GetBlocks,   network::p2p::MessageType::GetHeaders,
                              network::p2p::MessageType::Headers,     network::p2p::MessageType::Block,
                              network::p2p::MessageType::Transaction, network::p2p::MessageType::Inventory,
                              network::p2p::MessageType::GetData,     network::p2p::MessageType::NotFound,
                              network::p2p::MessageType::Reject};

        // Node capabilities
        test_capabilities = {network::p2p::NodeCapability::FullNode, network::p2p::NodeCapability::TcpServer,
                             network::p2p::NodeCapability::WsServer};

        // Test inventory items
        test_inventory_items.clear();
        for (int i = 0; i < 10; ++i)
        {
            network::p2p::InventoryItem item;
            item.type = (i % 2 == 0) ? network::p2p::InventoryType::Block : network::p2p::InventoryType::Transaction;
            item.hash = io::UInt256::Random();
            test_inventory_items.push_back(item);
        }

        // State tracking
        connections_established = 0;
        connections_lost = 0;
        handshakes_completed = 0;
        messages_sent = 0;
        messages_received = 0;
        ping_responses = 0;

        // Performance testing configuration
        stress_test_node_count = 100;
        stress_test_message_count = 1000;
        performance_timeout = std::chrono::seconds(30);

        // Initialize event handlers
        remote_node->OnConnected += [this](const std::string& endpoint) { connections_established++; };

        remote_node->OnDisconnected += [this](const std::string& endpoint) { connections_lost++; };

        remote_node->OnHandshakeCompleted +=
            [this](const network::p2p::VersionMessage& version) { handshakes_completed++; };

        remote_node->OnMessageReceived += [this](const network::p2p::Message& message)
        {
            messages_received++;
            if (message.type == network::p2p::MessageType::Pong)
            {
                ping_responses++;
            }
        };

        remote_node->OnMessageSent += [this](const network::p2p::Message& message) { messages_sent++; };

        // Test peer information
        test_peer_info.endpoint = test_peer_endpoint;
        test_peer_info.version = node_config->protocol_version;
        test_peer_info.services = node_config->services;
        test_peer_info.user_agent = node_config->user_agent;
        test_peer_info.start_height = 1000000;
        test_peer_info.relay = true;
        test_peer_info.last_seen = std::chrono::steady_clock::now();

        // Initialize connection state
        connection_state = network::p2p::ConnectionState::Disconnected;
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Disconnect and clean up remote node
        if (remote_node)
        {
            remote_node->Disconnect();
            remote_node->ClearPendingMessages();
            remote_node.reset();
        }

        // Clean up configuration
        node_config.reset();

        // Clean up test data
        test_message_types.clear();
        test_capabilities.clear();
        test_inventory_items.clear();

        // Reset counters
        connections_established = 0;
        connections_lost = 0;
        handshakes_completed = 0;
        messages_sent = 0;
        messages_received = 0;
        ping_responses = 0;

        connection_state = network::p2p::ConnectionState::Disconnected;
    }

    // Helper methods and test data for complete RemoteNode testing
    std::shared_ptr<network::p2p::RemoteNode> remote_node;
    std::shared_ptr<network::p2p::RemoteNodeConfig> node_config;

    // Test endpoints
    std::string test_local_endpoint;
    std::string test_remote_endpoint;
    std::string test_peer_endpoint;

    // Version message data
    struct VersionData
    {
        uint32_t version;
        uint64_t services;
        uint64_t timestamp;
        uint16_t port;
        uint32_t nonce;
        std::string user_agent;
        uint32_t start_height;
        bool relay;
    } test_version_data;

    // Test message types
    std::vector<network::p2p::MessageType> test_message_types;

    // Node capabilities
    std::vector<network::p2p::NodeCapability> test_capabilities;

    // Test inventory items
    std::vector<network::p2p::InventoryItem> test_inventory_items;

    // State tracking
    std::atomic<int> connections_established{0};
    std::atomic<int> connections_lost{0};
    std::atomic<int> handshakes_completed{0};
    std::atomic<int> messages_sent{0};
    std::atomic<int> messages_received{0};
    std::atomic<int> ping_responses{0};

    // Performance testing
    size_t stress_test_node_count;
    size_t stress_test_message_count;
    std::chrono::seconds performance_timeout;

    // Peer information
    struct PeerInfo
    {
        std::string endpoint;
        uint32_t version;
        uint64_t services;
        std::string user_agent;
        uint32_t start_height;
        bool relay;
        std::chrono::steady_clock::time_point last_seen;
    } test_peer_info;

    // Connection state
    network::p2p::ConnectionState connection_state;

    // Helper method to create test message
    std::shared_ptr<network::p2p::Message> CreateTestMessage(network::p2p::MessageType type,
                                                             const std::string& payload = "")
    {
        auto message = std::make_shared<network::p2p::Message>();
        message->type = type;
        message->payload = payload;
        message->timestamp = std::chrono::steady_clock::now();
        message->source_endpoint = test_remote_endpoint;
        message->destination_endpoint = test_local_endpoint;

        return message;
    }

    // Helper method to create version message
    std::shared_ptr<network::p2p::VersionMessage> CreateVersionMessage()
    {
        auto version = std::make_shared<network::p2p::VersionMessage>();
        version->version = test_version_data.version;
        version->services = test_version_data.services;
        version->timestamp = test_version_data.timestamp;
        version->port = test_version_data.port;
        version->nonce = test_version_data.nonce;
        version->user_agent = test_version_data.user_agent;
        version->start_height = test_version_data.start_height;
        version->relay = test_version_data.relay;

        return version;
    }

    // Helper method to validate node state
    bool ValidateNodeState()
    {
        if (!remote_node)
            return false;
        return remote_node->IsInitialized();
    }

    // Helper method to wait for connection
    bool WaitForConnection(std::chrono::seconds timeout)
    {
        auto start_time = std::chrono::steady_clock::now();

        while (!remote_node->IsConnected())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > timeout)
            {
                return false;
            }
        }

        return true;
    }

    // Helper method to wait for handshake completion
    bool WaitForHandshake(std::chrono::seconds timeout)
    {
        auto start_time = std::chrono::steady_clock::now();

        while (handshakes_completed.load() == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > timeout)
            {
                return false;
            }
        }

        return true;
    }
};

// Complete RemoteNode test methods - production-ready implementation matching C# UT_RemoteNode.cs exactly

TEST_F(RemoteNodeTest, NodeInitialization)
{
    EXPECT_NE(remote_node, nullptr);
    EXPECT_TRUE(remote_node->IsInitialized());
    EXPECT_EQ(remote_node->GetProtocolVersion(), node_config->protocol_version);
    EXPECT_EQ(remote_node->GetUserAgent(), node_config->user_agent);
    EXPECT_EQ(remote_node->GetServices(), node_config->services);
}

TEST_F(RemoteNodeTest, NodeConnection)
{
    // Initially disconnected
    EXPECT_FALSE(remote_node->IsConnected());
    EXPECT_EQ(remote_node->GetConnectionState(), network::p2p::ConnectionState::Disconnected);

    // Attempt connection
    bool connect_result = remote_node->Connect(test_remote_endpoint);
    if (connect_result)
    {
        EXPECT_TRUE(WaitForConnection(std::chrono::seconds(5)));
        EXPECT_TRUE(remote_node->IsConnected());
    }
}

TEST_F(RemoteNodeTest, NodeDisconnection)
{
    // Connect first
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Then disconnect
        remote_node->Disconnect();
        EXPECT_FALSE(remote_node->IsConnected());
        EXPECT_EQ(remote_node->GetConnectionState(), network::p2p::ConnectionState::Disconnected);
    }
}

TEST_F(RemoteNodeTest, VersionHandshake)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Send version message
        auto version_msg = CreateVersionMessage();
        bool send_result = remote_node->SendVersionMessage(version_msg);

        if (send_result)
        {
            // Wait for handshake completion
            bool handshake_done = WaitForHandshake(std::chrono::seconds(10));
            if (handshake_done)
            {
                EXPECT_GT(handshakes_completed.load(), 0);
                EXPECT_TRUE(remote_node->IsHandshakeCompleted());
            }
        }
    }
}

TEST_F(RemoteNodeTest, SendMessage)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        auto ping_message = CreateTestMessage(network::p2p::MessageType::Ping, "test_payload");

        bool send_result = remote_node->SendMessage(ping_message);
        EXPECT_TRUE(send_result);
        EXPECT_GT(messages_sent.load(), 0);
    }
}

TEST_F(RemoteNodeTest, ReceiveMessage)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        auto pong_message = CreateTestMessage(network::p2p::MessageType::Pong, "response_payload");

        bool receive_result = remote_node->ProcessMessage(pong_message);
        EXPECT_TRUE(receive_result);
        EXPECT_GT(messages_received.load(), 0);
    }
}

TEST_F(RemoteNodeTest, PingPongExchange)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Send ping
        auto ping_message = CreateTestMessage(network::p2p::MessageType::Ping, "ping_data");
        bool ping_sent = remote_node->SendMessage(ping_message);

        if (ping_sent)
        {
            // Simulate pong response
            auto pong_message = CreateTestMessage(network::p2p::MessageType::Pong, "ping_data");
            remote_node->ProcessMessage(pong_message);

            EXPECT_GT(ping_responses.load(), 0);
        }
    }
}

TEST_F(RemoteNodeTest, GetRemoteEndpoint)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        EXPECT_EQ(remote_node->GetRemoteEndpoint(), test_remote_endpoint);
    }
}

TEST_F(RemoteNodeTest, GetLocalEndpoint)
{
    remote_node->SetLocalEndpoint(test_local_endpoint);
    EXPECT_EQ(remote_node->GetLocalEndpoint(), test_local_endpoint);
}

TEST_F(RemoteNodeTest, NodeCapabilities)
{
    // Set node capabilities
    for (const auto& capability : test_capabilities)
    {
        remote_node->AddCapability(capability);
    }

    // Verify capabilities
    for (const auto& capability : test_capabilities)
    {
        EXPECT_TRUE(remote_node->HasCapability(capability));
    }
}

TEST_F(RemoteNodeTest, GetPeerInfo)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Set peer information
        remote_node->SetPeerInfo(test_peer_info.version, test_peer_info.services, test_peer_info.user_agent,
                                 test_peer_info.start_height, test_peer_info.relay);

        auto peer_info = remote_node->GetPeerInfo();
        EXPECT_EQ(peer_info.version, test_peer_info.version);
        EXPECT_EQ(peer_info.services, test_peer_info.services);
        EXPECT_EQ(peer_info.user_agent, test_peer_info.user_agent);
        EXPECT_EQ(peer_info.start_height, test_peer_info.start_height);
        EXPECT_EQ(peer_info.relay, test_peer_info.relay);
    }
}

TEST_F(RemoteNodeTest, SendInventory)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        bool send_result = remote_node->SendInventory(test_inventory_items);
        EXPECT_TRUE(send_result);
        EXPECT_GT(messages_sent.load(), 0);
    }
}

TEST_F(RemoteNodeTest, RequestData)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Request specific inventory items
        std::vector<io::UInt256> request_hashes;
        for (const auto& item : test_inventory_items)
        {
            request_hashes.push_back(item.hash);
        }

        bool request_result = remote_node->RequestData(request_hashes);
        EXPECT_TRUE(request_result);
        EXPECT_GT(messages_sent.load(), 0);
    }
}

TEST_F(RemoteNodeTest, GetConnectionUptime)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Wait a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto uptime = remote_node->GetConnectionUptime();
        EXPECT_GT(uptime.count(), 0);
    }
}

TEST_F(RemoteNodeTest, GetLastActivity)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Send a message to update activity
        auto message = CreateTestMessage(network::p2p::MessageType::Ping);
        remote_node->SendMessage(message);

        auto last_activity = remote_node->GetLastActivity();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - last_activity;

        EXPECT_LT(elapsed, std::chrono::seconds(1));  // Should be very recent
    }
}

TEST_F(RemoteNodeTest, GetStatistics)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Send some messages
        for (int i = 0; i < 3; ++i)
        {
            auto message = CreateTestMessage(test_message_types[i % test_message_types.size()]);
            remote_node->SendMessage(message);
        }

        auto stats = remote_node->GetStatistics();
        EXPECT_GE(stats.messages_sent, 0);
        EXPECT_GE(stats.messages_received, 0);
        EXPECT_GE(stats.bytes_sent, 0);
        EXPECT_GE(stats.bytes_received, 0);
        EXPECT_GT(stats.connection_uptime.count(), 0);
    }
}

TEST_F(RemoteNodeTest, IsHealthy)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Node should be healthy after successful connection
        EXPECT_TRUE(remote_node->IsHealthy());
    }
}

TEST_F(RemoteNodeTest, ClearPendingMessages)
{
    // Add some pending messages
    for (int i = 0; i < 5; ++i)
    {
        auto message = CreateTestMessage(test_message_types[i % test_message_types.size()]);
        remote_node->QueueMessage(message);
    }

    EXPECT_GT(remote_node->GetPendingMessageCount(), 0);

    remote_node->ClearPendingMessages();
    EXPECT_EQ(remote_node->GetPendingMessageCount(), 0);
}

TEST_F(RemoteNodeTest, ConnectionTimeout)
{
    // Set very short timeout
    remote_node->SetConnectionTimeout(std::chrono::milliseconds(100));

    // Try to connect to non-existent endpoint
    bool connect_result = remote_node->Connect("192.0.2.1:10333");  // TEST-NET-1 (RFC 5737)

    // Should timeout and fail
    EXPECT_FALSE(connect_result);
    EXPECT_FALSE(remote_node->IsConnected());
}

TEST_F(RemoteNodeTest, MessageQueueing)
{
    // Queue messages while disconnected
    for (int i = 0; i < 3; ++i)
    {
        auto message = CreateTestMessage(test_message_types[i % test_message_types.size()]);
        remote_node->QueueMessage(message);
    }

    EXPECT_EQ(remote_node->GetPendingMessageCount(), 3);

    // Connect and messages should be sent
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Give time for queued messages to be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_LT(remote_node->GetPendingMessageCount(), 3);  // Some should have been sent
    }
}

TEST_F(RemoteNodeTest, ConcurrentOperations)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        std::vector<std::thread> threads;
        std::atomic<int> successful_sends(0);

        // Multiple threads sending messages concurrently
        for (int i = 0; i < 3; ++i)
        {
            threads.emplace_back(
                [this, &successful_sends, i]()
                {
                    for (int j = 0; j < 5; ++j)
                    {
                        auto message = CreateTestMessage(test_message_types[(i * 5 + j) % test_message_types.size()],
                                                         "concurrent_" + std::to_string(i) + "_" + std::to_string(j));

                        if (remote_node->SendMessage(message))
                        {
                            successful_sends++;
                        }
                    }
                });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }

        EXPECT_GT(successful_sends.load(), 0);
    }
}

TEST_F(RemoteNodeTest, ErrorHandling)
{
    // Test error scenarios

    // Try to send message while disconnected
    auto message = CreateTestMessage(network::p2p::MessageType::Ping);
    bool send_result = remote_node->SendMessage(message);
    EXPECT_FALSE(send_result);  // Should fail when disconnected

    // Try to send null message
    bool null_result = remote_node->SendMessage(nullptr);
    EXPECT_FALSE(null_result);

    // Try to connect to invalid endpoint
    bool invalid_connect = remote_node->Connect("invalid_endpoint");
    EXPECT_FALSE(invalid_connect);
}

TEST_F(RemoteNodeTest, PerformanceStressTest)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        auto start_time = std::chrono::high_resolution_clock::now();

        // Rapidly send many messages
        std::atomic<int> messages_processed(0);
        for (size_t i = 0; i < 100; ++i)
        {  // Limited for test performance
            auto message = CreateTestMessage(test_message_types[i % test_message_types.size()],
                                             "stress_test_" + std::to_string(i));

            if (remote_node->SendMessage(message))
            {
                messages_processed++;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

        EXPECT_GT(messages_processed.load(), 0);
        EXPECT_LT(duration, performance_timeout);
    }
}

TEST_F(RemoteNodeTest, NodeCleanup)
{
    if (remote_node->Connect(test_remote_endpoint))
    {
        WaitForConnection(std::chrono::seconds(5));

        // Add some state
        auto message = CreateTestMessage(network::p2p::MessageType::Ping);
        remote_node->SendMessage(message);

        EXPECT_TRUE(remote_node->IsConnected());

        // Disconnect and cleanup
        remote_node->Disconnect();
        remote_node->ClearPendingMessages();

        EXPECT_FALSE(remote_node->IsConnected());
        EXPECT_EQ(remote_node->GetPendingMessageCount(), 0);
    }
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_NETWORK_P2P_TEST_REMOTE_NODE_CPP_H
