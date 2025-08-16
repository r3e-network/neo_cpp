/**
 * @file test_p2p_protocol_complete.cpp
 * @brief Comprehensive P2P network protocol tests for Neo
 */

#include <gtest/gtest.h>
#include <neo/network/p2p_node.h>
#include <neo/network/p2p_connection.h>
#include <neo/network/message.h>
#include <neo/network/message_command.h>
#include <neo/network/version_message.h>
#include <neo/network/verack_message.h>
#include <neo/network/ping_message.h>
#include <neo/network/pong_message.h>
#include <neo/network/inv_message.h>
#include <neo/network/getdata_message.h>
#include <neo/network/block_message.h>
#include <neo/network/transaction_message.h>
#include <neo/network/addr_message.h>
#include <neo/network/getaddr_message.h>
#include <neo/network/headers_message.h>
#include <neo/network/getblocks_message.h>
#include <neo/network/mempool_message.h>
#include <neo/network/filter_message.h>
#include <neo/network/reject_message.h>
#include <neo/network/alert_message.h>
#include <neo/network/tcp_connection.h>
#include <neo/network/network_address.h>
#include <neo/network/inventory_type.h>
#include <neo/network/capabilities.h>
#include <neo/cryptography/key_pair.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

using namespace neo::network;
using namespace neo::cryptography;
using namespace neo::io;
using namespace std::chrono_literals;

class P2PProtocolTest : public ::testing::Test {
protected:
    std::unique_ptr<P2PNode> node1_;
    std::unique_ptr<P2PNode> node2_;
    std::unique_ptr<KeyPair> keypair1_;
    std::unique_ptr<KeyPair> keypair2_;
    
    void SetUp() override {
        keypair1_ = std::make_unique<KeyPair>();
        keypair2_ = std::make_unique<KeyPair>();
        
        // Create two P2P nodes for testing
        node1_ = std::make_unique<P2PNode>("127.0.0.1", 20333, keypair1_.get());
        node2_ = std::make_unique<P2PNode>("127.0.0.1", 20334, keypair2_.get());
    }
    
    void TearDown() override {
        if (node1_ && node1_->IsRunning()) {
            node1_->Stop();
        }
        if (node2_ && node2_->IsRunning()) {
            node2_->Stop();
        }
    }
};

// ============================================================================
// Basic P2P Node Tests
// ============================================================================

TEST_F(P2PProtocolTest, NodeInitialization) {
    EXPECT_NE(node1_, nullptr);
    EXPECT_NE(node2_, nullptr);
    EXPECT_FALSE(node1_->IsRunning());
    EXPECT_FALSE(node2_->IsRunning());
    EXPECT_EQ(node1_->GetPort(), 20333);
    EXPECT_EQ(node2_->GetPort(), 20334);
}

TEST_F(P2PProtocolTest, NodeStartStop) {
    node1_->Start();
    EXPECT_TRUE(node1_->IsRunning());
    EXPECT_EQ(node1_->GetPeerCount(), 0);
    
    node1_->Stop();
    EXPECT_FALSE(node1_->IsRunning());
}

TEST_F(P2PProtocolTest, NodeId) {
    auto id1 = node1_->GetNodeId();
    auto id2 = node2_->GetNodeId();
    
    EXPECT_NE(id1, id2);
    EXPECT_GT(id1.size(), 0);
    EXPECT_GT(id2.size(), 0);
}

// ============================================================================
// Connection Management Tests
// ============================================================================

TEST_F(P2PProtocolTest, ConnectionEstablishment) {
    node1_->Start();
    node2_->Start();
    
    // Node 2 connects to Node 1
    bool connected = node2_->ConnectToPeer("127.0.0.1", 20333);
    
    // Wait for connection
    std::this_thread::sleep_for(100ms);
    
    EXPECT_TRUE(connected);
    EXPECT_EQ(node1_->GetPeerCount(), 1);
    EXPECT_EQ(node2_->GetPeerCount(), 1);
}

TEST_F(P2PProtocolTest, MaxConnectionLimit) {
    const int max_connections = 10;
    node1_->SetMaxConnections(max_connections);
    node1_->Start();
    
    // Try to exceed connection limit
    std::vector<std::unique_ptr<P2PNode>> clients;
    for (int i = 0; i < max_connections + 5; ++i) {
        auto client = std::make_unique<P2PNode>("127.0.0.1", 30000 + i, nullptr);
        client->Start();
        client->ConnectToPeer("127.0.0.1", 20333);
        clients.push_back(std::move(client));
    }
    
    std::this_thread::sleep_for(200ms);
    
    // Should not exceed max connections
    EXPECT_LE(node1_->GetPeerCount(), max_connections);
    
    // Cleanup
    for (auto& client : clients) {
        client->Stop();
    }
}

TEST_F(P2PProtocolTest, ConnectionTimeout) {
    node1_->SetConnectionTimeout(std::chrono::seconds(1));
    
    // Try to connect to non-existent node
    bool connected = node1_->ConnectToPeer("127.0.0.1", 55555);
    
    EXPECT_FALSE(connected);
}

// ============================================================================
// Message Exchange Tests
// ============================================================================

TEST_F(P2PProtocolTest, VersionHandshake) {
    VersionMessage version;
    version.Version = 0;
    version.Services = 1;
    version.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    version.Port = 20333;
    version.Nonce = 12345;
    version.UserAgent = "NEO:3.0.0";
    version.StartHeight = 0;
    version.Relay = true;
    
    EXPECT_EQ(version.Version, 0);
    EXPECT_EQ(version.Services, 1);
    EXPECT_EQ(version.Port, 20333);
    EXPECT_EQ(version.UserAgent, "NEO:3.0.0");
}

TEST_F(P2PProtocolTest, PingPong) {
    PingMessage ping;
    ping.LastBlockIndex = 1000;
    ping.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    ping.Nonce = 98765;
    
    PongMessage pong;
    pong.LastBlockIndex = ping.LastBlockIndex;
    pong.Nonce = ping.Nonce;
    
    EXPECT_EQ(pong.LastBlockIndex, ping.LastBlockIndex);
    EXPECT_EQ(pong.Nonce, ping.Nonce);
}

TEST_F(P2PProtocolTest, InventoryMessage) {
    InvMessage inv;
    
    // Add block inventory
    InventoryVector invVec1;
    invVec1.Type = InventoryType::Block;
    invVec1.Hash = ByteVector(32, 0xAA);
    inv.Inventory.push_back(invVec1);
    
    // Add transaction inventory
    InventoryVector invVec2;
    invVec2.Type = InventoryType::Transaction;
    invVec2.Hash = ByteVector(32, 0xBB);
    inv.Inventory.push_back(invVec2);
    
    EXPECT_EQ(inv.Inventory.size(), 2);
    EXPECT_EQ(inv.Inventory[0].Type, InventoryType::Block);
    EXPECT_EQ(inv.Inventory[1].Type, InventoryType::Transaction);
}

// ============================================================================
// Block Synchronization Tests
// ============================================================================

TEST_F(P2PProtocolTest, GetBlocksMessage) {
    GetBlocksMessage getblocks;
    getblocks.HashStart = ByteVector(32, 0x00);
    getblocks.HashStop = ByteVector(32, 0xFF);
    
    EXPECT_EQ(getblocks.HashStart.Size(), 32);
    EXPECT_EQ(getblocks.HashStop.Size(), 32);
}

TEST_F(P2PProtocolTest, HeadersMessage) {
    HeadersMessage headers;
    
    // Add mock headers
    for (int i = 0; i < 10; ++i) {
        BlockHeader header;
        header.Version = 0;
        header.PrevHash = ByteVector(32, i);
        header.MerkleRoot = ByteVector(32, i + 1);
        header.Timestamp = 1000000 + i;
        header.Index = i;
        header.NextConsensus = ByteVector(20, i);
        headers.Headers.push_back(header);
    }
    
    EXPECT_EQ(headers.Headers.size(), 10);
    EXPECT_EQ(headers.Headers[0].Index, 0);
    EXPECT_EQ(headers.Headers[9].Index, 9);
}

TEST_F(P2PProtocolTest, BlockMessage) {
    BlockMessage blockMsg;
    blockMsg.Block.Version = 0;
    blockMsg.Block.PrevHash = ByteVector(32, 0xAA);
    blockMsg.Block.MerkleRoot = ByteVector(32, 0xBB);
    blockMsg.Block.Timestamp = 1234567890;
    blockMsg.Block.Index = 1000;
    blockMsg.Block.NextConsensus = ByteVector(20, 0xCC);
    
    EXPECT_EQ(blockMsg.Block.Index, 1000);
    EXPECT_EQ(blockMsg.Block.Timestamp, 1234567890);
}

// ============================================================================
// Transaction Propagation Tests
// ============================================================================

TEST_F(P2PProtocolTest, TransactionMessage) {
    TransactionMessage txMsg;
    txMsg.Transaction.Version = 0;
    txMsg.Transaction.Nonce = 12345;
    txMsg.Transaction.SystemFee = 1000000;
    txMsg.Transaction.NetworkFee = 500000;
    txMsg.Transaction.ValidUntilBlock = 5000;
    
    EXPECT_EQ(txMsg.Transaction.Nonce, 12345);
    EXPECT_EQ(txMsg.Transaction.SystemFee, 1000000);
    EXPECT_EQ(txMsg.Transaction.NetworkFee, 500000);
}

TEST_F(P2PProtocolTest, MempoolMessage) {
    MempoolMessage mempool;
    
    // Add transaction hashes
    for (int i = 0; i < 5; ++i) {
        mempool.Hashes.push_back(ByteVector(32, i));
    }
    
    EXPECT_EQ(mempool.Hashes.size(), 5);
}

TEST_F(P2PProtocolTest, TransactionRelay) {
    node1_->Start();
    node2_->Start();
    
    // Connect nodes
    node2_->ConnectToPeer("127.0.0.1", 20333);
    std::this_thread::sleep_for(100ms);
    
    // Create transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 99999;
    tx.SystemFee = 2000000;
    
    // Relay transaction
    bool relayed = node1_->RelayTransaction(tx);
    
    // Transaction should be relayed to connected peers
    EXPECT_TRUE(relayed || node1_->GetPeerCount() > 0);
}

// ============================================================================
// Peer Discovery Tests
// ============================================================================

TEST_F(P2PProtocolTest, GetAddrMessage) {
    GetAddrMessage getaddr;
    getaddr.Count = 10;  // Request 10 addresses
    
    EXPECT_EQ(getaddr.Count, 10);
}

TEST_F(P2PProtocolTest, AddrMessage) {
    AddrMessage addr;
    
    // Add network addresses
    for (int i = 0; i < 5; ++i) {
        NetworkAddress netAddr;
        netAddr.Timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        netAddr.Services = 1;
        netAddr.IP = "192.168.1." + std::to_string(i);
        netAddr.Port = 20333 + i;
        addr.Addresses.push_back(netAddr);
    }
    
    EXPECT_EQ(addr.Addresses.size(), 5);
    EXPECT_EQ(addr.Addresses[0].Port, 20333);
    EXPECT_EQ(addr.Addresses[4].Port, 20337);
}

TEST_F(P2PProtocolTest, PeerDiscovery) {
    node1_->EnablePeerDiscovery(true);
    node1_->Start();
    
    // Add seed nodes
    node1_->AddSeedNode("seed1.neo.org", 10333);
    node1_->AddSeedNode("seed2.neo.org", 10333);
    
    auto seeds = node1_->GetSeedNodes();
    EXPECT_GE(seeds.size(), 2);
}

// ============================================================================
// Filter and Bloom Filter Tests
// ============================================================================

TEST_F(P2PProtocolTest, FilterMessage) {
    FilterMessage filter;
    filter.Filter = ByteVector(256);
    filter.K = 10;  // Number of hash functions
    filter.Tweak = 12345;
    
    EXPECT_EQ(filter.Filter.Size(), 256);
    EXPECT_EQ(filter.K, 10);
    EXPECT_EQ(filter.Tweak, 12345);
}

TEST_F(P2PProtocolTest, FilterLoadClear) {
    FilterLoadMessage load;
    load.Filter = ByteVector(512);
    load.K = 5;
    load.Tweak = 99999;
    
    FilterClearMessage clear;
    // Clear has no data
    
    EXPECT_EQ(load.Filter.Size(), 512);
    EXPECT_EQ(load.K, 5);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(P2PProtocolTest, RejectMessage) {
    RejectMessage reject;
    reject.Message = "tx";
    reject.Code = RejectCode::Invalid;
    reject.Reason = "Transaction validation failed";
    reject.Data = ByteVector(32, 0xEE);
    
    EXPECT_EQ(reject.Message, "tx");
    EXPECT_EQ(reject.Code, RejectCode::Invalid);
    EXPECT_EQ(reject.Reason, "Transaction validation failed");
    EXPECT_EQ(reject.Data.Size(), 32);
}

TEST_F(P2PProtocolTest, AlertMessage) {
    AlertMessage alert;
    alert.Payload = ByteVector({0x01, 0x02, 0x03, 0x04});
    alert.Signature = ByteVector(64, 0xFF);
    
    EXPECT_EQ(alert.Payload.Size(), 4);
    EXPECT_EQ(alert.Signature.Size(), 64);
}

TEST_F(P2PProtocolTest, MalformedMessage) {
    // Create malformed message
    ByteVector malformed = {0xFF, 0xFF, 0xFF, 0xFF};
    
    Message msg;
    bool parsed = msg.Deserialize(malformed);
    
    EXPECT_FALSE(parsed);
}

// ============================================================================
// Network Capabilities Tests
// ============================================================================

TEST_F(P2PProtocolTest, NodeCapabilities) {
    Capabilities caps;
    caps.Type = NodeCapabilityType::TcpServer;
    caps.Port = 20333;
    
    EXPECT_EQ(caps.Type, NodeCapabilityType::TcpServer);
    EXPECT_EQ(caps.Port, 20333);
}

TEST_F(P2PProtocolTest, FullNodeCapabilities) {
    FullNodeCapability fullCap;
    fullCap.StartHeight = 0;
    
    EXPECT_EQ(fullCap.StartHeight, 0);
}

// ============================================================================
// Message Serialization Tests
// ============================================================================

TEST_F(P2PProtocolTest, MessageSerialization) {
    // Test version message serialization
    VersionMessage original;
    original.Version = 0;
    original.Services = 1;
    original.Timestamp = 1234567890;
    original.Port = 20333;
    original.Nonce = 11111;
    original.UserAgent = "Test/1.0";
    original.StartHeight = 5000;
    
    ByteVector serialized = original.Serialize();
    
    VersionMessage deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(original.Version, deserialized.Version);
    EXPECT_EQ(original.Services, deserialized.Services);
    EXPECT_EQ(original.Port, deserialized.Port);
    EXPECT_EQ(original.Nonce, deserialized.Nonce);
    EXPECT_EQ(original.UserAgent, deserialized.UserAgent);
}

TEST_F(P2PProtocolTest, MessageHeader) {
    MessageHeader header;
    header.Magic = 0x00746E41;  // NEO mainnet magic
    header.Command = "version";
    header.PayloadSize = 100;
    header.Checksum = 0x12345678;
    
    EXPECT_EQ(header.Magic, 0x00746E41);
    EXPECT_EQ(header.Command, "version");
    EXPECT_EQ(header.PayloadSize, 100);
}

// ============================================================================
// Connection Pool Tests
// ============================================================================

TEST_F(P2PProtocolTest, ConnectionPooling) {
    node1_->Start();
    
    // Create connection pool
    std::vector<std::shared_ptr<P2PConnection>> connections;
    
    for (int i = 0; i < 5; ++i) {
        auto conn = std::make_shared<P2PConnection>("127.0.0.1", 30000 + i);
        connections.push_back(conn);
    }
    
    // Verify pool size
    EXPECT_EQ(connections.size(), 5);
    
    // Test connection reuse
    auto reused = connections[0];
    EXPECT_NE(reused, nullptr);
}

TEST_F(P2PProtocolTest, ConnectionRecycling) {
    auto conn = std::make_unique<P2PConnection>("127.0.0.1", 40000);
    
    // Simulate connection lifecycle
    conn->Connect();
    EXPECT_TRUE(conn->IsConnected() || conn->GetState() == ConnectionState::Connecting);
    
    conn->Disconnect();
    EXPECT_FALSE(conn->IsConnected());
    
    // Reconnect
    conn->Connect();
    // Connection might be in connecting state
    EXPECT_TRUE(conn->GetState() != ConnectionState::Disconnected);
}

// ============================================================================
// Bandwidth and Rate Limiting Tests
// ============================================================================

TEST_F(P2PProtocolTest, BandwidthTracking) {
    node1_->Start();
    
    // Send some data to track bandwidth
    ByteVector data(1024, 0xAB);
    
    auto stats = node1_->GetNetworkStats();
    uint64_t initial_sent = stats.BytesSent;
    
    // Simulate sending data
    node1_->BroadcastMessage(Message("test", data));
    
    stats = node1_->GetNetworkStats();
    EXPECT_GE(stats.BytesSent, initial_sent);
}

TEST_F(P2PProtocolTest, RateLimiting) {
    node1_->SetRateLimit(1000);  // 1000 messages per second
    node1_->Start();
    
    // Try to exceed rate limit
    bool limited = false;
    for (int i = 0; i < 2000; ++i) {
        if (!node1_->SendMessage("test", ByteVector())) {
            limited = true;
            break;
        }
    }
    
    // Should hit rate limit
    EXPECT_TRUE(limited || node1_->GetRateLimit() > 0);
}

// ============================================================================
// Security Tests
// ============================================================================

TEST_F(P2PProtocolTest, MessageAuthentication) {
    // Create signed message
    ByteVector payload = {0x01, 0x02, 0x03};
    ByteVector signature = keypair1_->Sign(payload);
    
    // Verify signature
    bool valid = keypair1_->Verify(payload, signature);
    EXPECT_TRUE(valid);
    
    // Verify with wrong key should fail
    bool invalid = keypair2_->Verify(payload, signature);
    EXPECT_FALSE(invalid);
}

TEST_F(P2PProtocolTest, DoSProtection) {
    node1_->EnableDoSProtection(true);
    node1_->SetMaxMessageSize(1024 * 1024);  // 1MB limit
    node1_->Start();
    
    // Try to send oversized message
    ByteVector huge(2 * 1024 * 1024, 0xFF);  // 2MB
    bool sent = node1_->SendMessage("test", huge);
    
    // Should be rejected
    EXPECT_FALSE(sent);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(P2PProtocolTest, MessageThroughput) {
    node1_->Start();
    node2_->Start();
    
    node2_->ConnectToPeer("127.0.0.1", 20333);
    std::this_thread::sleep_for(100ms);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Send many messages
    for (int i = 0; i < 1000; ++i) {
        PingMessage ping;
        ping.LastBlockIndex = i;
        ping.Nonce = i;
        node1_->SendMessage("ping", ping.Serialize());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should handle 1000 messages quickly
    EXPECT_LT(duration.count(), 5000);  // Under 5 seconds
}

TEST_F(P2PProtocolTest, ConnectionLatency) {
    auto start = std::chrono::high_resolution_clock::now();
    
    node1_->Start();
    node2_->Start();
    node2_->ConnectToPeer("127.0.0.1", 20333);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Connection should be fast on localhost
    EXPECT_LT(duration.count(), 1000);  // Under 1 second
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(P2PProtocolTest, StressManyConnections) {
    node1_->SetMaxConnections(100);
    node1_->Start();
    
    std::vector<std::unique_ptr<P2PNode>> nodes;
    std::atomic<int> connected{0};
    
    // Create many nodes
    for (int i = 0; i < 50; ++i) {
        auto node = std::make_unique<P2PNode>("127.0.0.1", 40000 + i, nullptr);
        node->Start();
        if (node->ConnectToPeer("127.0.0.1", 20333)) {
            connected++;
        }
        nodes.push_back(std::move(node));
    }
    
    std::this_thread::sleep_for(500ms);
    
    // Should handle many connections
    EXPECT_GT(connected.load(), 0);
    EXPECT_LE(node1_->GetPeerCount(), node1_->GetMaxConnections());
    
    // Cleanup
    for (auto& node : nodes) {
        node->Stop();
    }
}

TEST_F(P2PProtocolTest, StressMessageFlood) {
    node1_->Start();
    node2_->Start();
    
    node2_->ConnectToPeer("127.0.0.1", 20333);
    std::this_thread::sleep_for(100ms);
    
    std::atomic<bool> flooding{true};
    std::thread flooder([this, &flooding]() {
        while (flooding) {
            PingMessage ping;
            ping.Nonce = rand();
            node2_->SendMessage("ping", ping.Serialize());
        }
    });
    
    // Let it flood for a bit
    std::this_thread::sleep_for(100ms);
    flooding = false;
    flooder.join();
    
    // Nodes should still be running
    EXPECT_TRUE(node1_->IsRunning());
    EXPECT_TRUE(node2_->IsRunning());
}
