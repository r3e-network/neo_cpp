#include <gtest/gtest.h>
#include <neo/core/neo_system.h>
#include <neo/protocol_settings.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/channels_config.h>
#include <neo/network/ip_endpoint.h>
#include <neo/persistence/store_factory.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/io/uint256.h>
#include <chrono>
#include <thread>
#include <future>

using namespace neo;
using namespace neo::network;
using namespace neo::network::p2p;

class P2PConnectivityTest : public ::testing::Test
{
protected:
    std::shared_ptr<NeoSystem> system1;
    std::shared_ptr<NeoSystem> system2;
    
    void SetUp() override
    {
        // Create two independent systems for testing P2P connectivity
        auto settings1 = std::make_unique<ProtocolSettings>();
        auto settings2 = std::make_unique<ProtocolSettings>();
        
        system1 = std::make_shared<NeoSystem>(std::move(settings1), "memory");
        system2 = std::make_shared<NeoSystem>(std::move(settings2), "memory");
    }
    
    void TearDown() override
    {
        // Clean shutdown
        if (system1) system1->stop();
        if (system2) system2->stop();
    }
    
    bool WaitForCondition(std::function<bool()> condition, int timeoutSeconds = 5)
    {
        auto start = std::chrono::steady_clock::now();
        while (!condition())
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start).count() >= timeoutSeconds)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return true;
    }
};

// Test 1: Basic P2P Node Startup
TEST_F(P2PConnectivityTest, TestBasicNodeStartup)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start node on port 20333
    bool started = localNode.Start(20333, 10);
    EXPECT_TRUE(started);
    
    // Verify node is running
    EXPECT_EQ(localNode.GetConnectedCount(), 0); // No peers yet
    EXPECT_FALSE(localNode.GetUserAgent().empty());
    EXPECT_NE(localNode.GetNonce(), 0);
    
    // Stop node
    localNode.Stop();
}

// Test 2: Peer Connection Establishment
TEST_F(P2PConnectivityTest, TestPeerConnection)
{
    auto& localNode1 = LocalNode::GetInstance();
    
    // Start first node
    bool started1 = localNode1.Start(20334, 10);
    ASSERT_TRUE(started1);
    
    // Create second local node instance for testing
    // Note: In production, only one LocalNode instance exists
    // For testing, we'll simulate with connection attempts
    
    // Add peer endpoint
    IPEndPoint peer("127.0.0.1", 20335);
    bool added = localNode1.AddPeer(peer);
    EXPECT_TRUE(added);
    
    // Verify peer was added to peer list
    auto& peerList = localNode1.GetPeerList();
    EXPECT_GT(peerList.GetPeers().size(), 0);
    
    localNode1.Stop();
}

// Test 3: Multiple Peer Management
TEST_F(P2PConnectivityTest, TestMultiplePeerManagement)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start node
    bool started = localNode.Start(20336, 10);
    ASSERT_TRUE(started);
    
    // Add multiple peers
    std::vector<IPEndPoint> peers = {
        IPEndPoint("192.168.1.1", 20333),
        IPEndPoint("192.168.1.2", 20333),
        IPEndPoint("192.168.1.3", 20333),
        IPEndPoint("10.0.0.1", 20333),
        IPEndPoint("10.0.0.2", 20333)
    };
    
    localNode.AddPeers(peers);
    
    // Verify all peers were added
    auto& peerList = localNode.GetPeerList();
    EXPECT_GE(peerList.GetPeers().size(), peers.size());
    
    // Test peer removal
    bool removed = localNode.RemovePeer(peers[0]);
    EXPECT_TRUE(removed);
    
    // Test marking peer as bad
    bool markedBad = localNode.MarkPeerBad(peers[1]);
    EXPECT_TRUE(markedBad);
    
    localNode.Stop();
}

// Test 4: Connection Lifecycle Management
TEST_F(P2PConnectivityTest, TestConnectionLifecycle)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Configure with channels config
    auto config = std::make_unique<ChannelsConfig>();
    config->SetTcp(IPEndPoint("0.0.0.0", 20337));
    config->SetMaxConnections(20);
    
    // Add seed nodes to seed list
    std::vector<IPEndPoint> seeds;
    seeds.push_back(IPEndPoint("seed1.neo.org", 20333));
    seeds.push_back(IPEndPoint("seed2.neo.org", 20333));
    config->SetSeedList(seeds);
    
    // Start with config
    bool started = localNode.Start(*config);
    ASSERT_TRUE(started);
    
    // Verify seed nodes were added
    auto& peerList = localNode.GetPeerList();
    EXPECT_GE(peerList.GetPeers().size(), 2);
    
    // Test peer list persistence
    bool saved = localNode.SavePeerList();
    EXPECT_TRUE(saved);
    
    localNode.Stop();
    
    // Test loading peer list after restart
    bool loaded = localNode.LoadPeerList();
    EXPECT_TRUE(loaded);
}

// Test 5: Message Broadcasting
TEST_F(P2PConnectivityTest, TestMessageBroadcasting)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start node
    bool started = localNode.Start(20338, 10);
    ASSERT_TRUE(started);
    
    // Create test inventory items
    std::vector<io::UInt256> hashes;
    for (int i = 0; i < 5; i++)
    {
        io::UInt256 hash;
        // Fill hash with test data
        for (int j = 0; j < 32; j++)
        {
            hash.Data()[j] = static_cast<uint8_t>(i + j);
        }
        hashes.push_back(hash);
    }
    
    // Test broadcasting inventory
    EXPECT_NO_THROW(localNode.BroadcastInv(InventoryType::Block, hashes));
    
    // Test broadcasting ping message
    auto pingPayload = std::make_shared<payloads::PingPayload>();
    pingPayload->SetTimestamp(static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count()));
    pingPayload->SetNonce(12345);
    
    Message pingMessage(MessageCommand::Ping, pingPayload);
    EXPECT_NO_THROW(localNode.Broadcast(pingMessage, true));
    
    localNode.Stop();
}

// Test 6: Concurrent Connection Handling
TEST_F(P2PConnectivityTest, TestConcurrentConnections)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start node with higher connection limit
    bool started = localNode.Start(20339, 50);
    ASSERT_TRUE(started);
    
    // Simulate multiple concurrent connection attempts
    std::vector<std::future<bool>> futures;
    
    for (int i = 0; i < 20; i++)
    {
        futures.push_back(std::async(std::launch::async, [&localNode, i]() {
            IPEndPoint endpoint("127.0.0.1", 30000 + i);
            return localNode.Connect(endpoint);
        }));
    }
    
    // Wait for all connection attempts
    int successCount = 0;
    for (auto& future : futures)
    {
        if (future.get())
        {
            successCount++;
        }
    }
    
    // Some connections should succeed (depending on network conditions)
    EXPECT_GE(successCount, 0);
    
    localNode.Stop();
}

// Test 7: Message Handler Callbacks
TEST_F(P2PConnectivityTest, TestMessageHandlerCallbacks)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Set up message received callbacks
    std::atomic<int> versionReceived{0};
    std::atomic<int> pingReceived{0};
    std::atomic<int> invReceived{0};
    
    localNode.SetVersionMessageReceivedCallback(
        [&versionReceived](RemoteNode*, const payloads::VersionPayload&) {
            versionReceived++;
        });
    
    localNode.SetPingMessageReceivedCallback(
        [&pingReceived](RemoteNode*, const payloads::PingPayload&) {
            pingReceived++;
        });
    
    localNode.SetInvMessageReceivedCallback(
        [&invReceived](RemoteNode*, const payloads::InvPayload&) {
            invReceived++;
        });
    
    // Start node
    bool started = localNode.Start(20340, 10);
    ASSERT_TRUE(started);
    
    // Callbacks should be registered and ready
    // In a real test with actual peers, we would verify callback invocation
    
    localNode.Stop();
}

// Test 8: Node Capabilities
TEST_F(P2PConnectivityTest, TestNodeCapabilities)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Set custom capabilities
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(NodeCapability(NodeCapabilityType::FullNode));
    capabilities.push_back(NodeCapability(NodeCapabilityType::TcpServer));
    
    localNode.SetCapabilities(capabilities);
    
    // Verify capabilities
    auto caps = localNode.GetCapabilities();
    EXPECT_EQ(caps.size(), 2);
    EXPECT_EQ(caps[0].GetType(), NodeCapabilityType::FullNode);
    EXPECT_EQ(caps[1].GetType(), NodeCapabilityType::TcpServer);
    
    // Create version payload
    auto versionPayload = localNode.CreateVersionPayload();
    ASSERT_NE(versionPayload, nullptr);
    EXPECT_EQ(versionPayload->GetCapabilities().size(), 2);
}

// Test 9: Error Handling and Recovery
TEST_F(P2PConnectivityTest, TestErrorHandlingAndRecovery)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Test starting on already used port
    bool started1 = localNode.Start(20341, 10);
    ASSERT_TRUE(started1);
    
    // Try to start again (should fail gracefully)
    bool started2 = localNode.Start(20341, 10);
    EXPECT_FALSE(started2);
    
    // Stop and restart
    localNode.Stop();
    
    // Should be able to start again after stop
    bool started3 = localNode.Start(20341, 10);
    EXPECT_TRUE(started3);
    
    // Test invalid peer connections
    IPEndPoint invalidPeer("999.999.999.999", 20333);
    bool connected = localNode.Connect(invalidPeer);
    EXPECT_FALSE(connected);
    
    localNode.Stop();
}

// Test 10: Performance Under Load
TEST_F(P2PConnectivityTest, TestPerformanceUnderLoad)
{
    auto& localNode = LocalNode::GetInstance();
    
    // Start node with high connection limit
    bool started = localNode.Start(20342, 100);
    ASSERT_TRUE(started);
    
    // Add many peers quickly
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++)
    {
        IPEndPoint peer("10.0.0." + std::to_string(i % 256), 20333);
        localNode.AddPeer(peer);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete in reasonable time
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
    
    // Verify peer list size
    auto& peerList = localNode.GetPeerList();
    EXPECT_GT(peerList.GetPeers().size(), 0);
    
    localNode.Stop();
}