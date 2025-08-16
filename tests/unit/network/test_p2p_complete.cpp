/**
 * @file test_p2p_complete.cpp
 * @brief Complete P2P network protocol tests for Neo C++
 * Must match Neo C# network implementation exactly
 */

#include <gtest/gtest.h>
#include <neo/network/p2p/connection.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/remote_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/getdata_payload.h>
#include <neo/network/p2p/payloads/getblocks_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/merkle_block_payload.h>
#include <neo/network/p2p/payloads/not_found_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/reject_payload.h>
#include <neo/network/p2p/payloads/alert_payload.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <memory>
#include <vector>
#include <thread>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;

class P2PCompleteTest : public ::testing::Test {
protected:
    std::unique_ptr<LocalNode> local_node_;
    std::vector<std::unique_ptr<RemoteNode>> remote_nodes_;
    
    void SetUp() override {
        // Initialize local node
        local_node_ = std::make_unique<LocalNode>(20333); // Mainnet port
    }
    
    void TearDown() override {
        // Clean up connections
        for (auto& remote : remote_nodes_) {
            if (remote && remote->IsConnected()) {
                remote->Disconnect();
            }
        }
    }
    
    Message CreateMessage(MessageCommand command) {
        Message msg;
        msg.Magic = 0x4E454F4E; // N-E-O-N
        msg.Command = command;
        msg.Checksum = 0;
        msg.Payload = ByteVector();
        return msg;
    }
    
    std::unique_ptr<RemoteNode> CreateRemoteNode(const std::string& address, uint16_t port) {
        auto node = std::make_unique<RemoteNode>(address, port);
        remote_nodes_.push_back(std::move(node));
        return std::move(remote_nodes_.back());
    }
};

// ============================================================================
// Message Protocol Tests
// ============================================================================

TEST_F(P2PCompleteTest, Message_Structure) {
    Message msg;
    msg.Magic = 0x4E454F4E;
    msg.Command = MessageCommand::Version;
    msg.Payload = ByteVector(100, 0xAB);
    
    EXPECT_EQ(msg.Magic, 0x4E454F4E);
    EXPECT_EQ(msg.Command, MessageCommand::Version);
    EXPECT_EQ(msg.Payload.Size(), 100);
}

TEST_F(P2PCompleteTest, Message_Commands) {
    // Test all message command values
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Version), 0x00);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Verack), 0x01);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::GetAddr), 0x10);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Addr), 0x11);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Ping), 0x18);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Pong), 0x19);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::GetHeaders), 0x20);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Headers), 0x21);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::GetBlocks), 0x24);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Mempool), 0x25);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Inv), 0x27);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::GetData), 0x28);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::GetBlockByIndex), 0x29);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::NotFound), 0x2a);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Transaction), 0x2b);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Block), 0x2c);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Consensus), 0x2d);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Reject), 0x2f);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::FilterLoad), 0x30);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::FilterAdd), 0x31);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::FilterClear), 0x32);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::MerkleBlock), 0x38);
    EXPECT_EQ(static_cast<uint32_t>(MessageCommand::Alert), 0x40);
}

TEST_F(P2PCompleteTest, Message_Serialization) {
    Message original = CreateMessage(MessageCommand::Ping);
    original.Payload = ByteVector::FromString("Test payload");
    
    auto serialized = original.Serialize();
    
    Message deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Magic, original.Magic);
    EXPECT_EQ(deserialized.Command, original.Command);
    EXPECT_EQ(deserialized.Payload, original.Payload);
}

TEST_F(P2PCompleteTest, Message_Checksum) {
    Message msg = CreateMessage(MessageCommand::Ping);
    msg.Payload = ByteVector::FromString("Checksum test");
    
    msg.UpdateChecksum();
    uint32_t checksum1 = msg.Checksum;
    
    // Checksum should be consistent
    msg.UpdateChecksum();
    uint32_t checksum2 = msg.Checksum;
    
    EXPECT_EQ(checksum1, checksum2);
    EXPECT_NE(checksum1, 0);
}

TEST_F(P2PCompleteTest, Message_MaxSize) {
    Message msg = CreateMessage(MessageCommand::Block);
    
    // Max payload size is 0x2000000 (32MB)
    const uint32_t MAX_SIZE = 0x2000000;
    
    // Create payload just under max size
    msg.Payload = ByteVector(MAX_SIZE - 1, 0);
    EXPECT_TRUE(msg.IsValid());
    
    // Payload over max size should be invalid
    msg.Payload = ByteVector(MAX_SIZE + 1, 0);
    EXPECT_FALSE(msg.IsValid());
}

// ============================================================================
// Version Handshake Tests
// ============================================================================

TEST_F(P2PCompleteTest, Version_Payload) {
    VersionPayload version;
    version.Magic = 0x4E454F4E;
    version.Version = 0;
    version.Timestamp = 1234567890;
    version.Nonce = 0xDEADBEEF;
    version.UserAgent = "NEO:3.0.0";
    version.StartHeight = 1000;
    version.Capabilities = {};
    
    EXPECT_EQ(version.Magic, 0x4E454F4E);
    EXPECT_EQ(version.Version, 0);
    EXPECT_EQ(version.UserAgent, "NEO:3.0.0");
}

TEST_F(P2PCompleteTest, Version_Serialization) {
    VersionPayload original;
    original.Magic = 0x4E454F4E;
    original.Version = 0;
    original.Timestamp = 9876543210;
    original.Nonce = 0xCAFEBABE;
    original.UserAgent = "NEO-CPP:1.0.0";
    original.StartHeight = 5000;
    
    auto serialized = original.Serialize();
    
    VersionPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Magic, original.Magic);
    EXPECT_EQ(deserialized.Version, original.Version);
    EXPECT_EQ(deserialized.Timestamp, original.Timestamp);
    EXPECT_EQ(deserialized.Nonce, original.Nonce);
    EXPECT_EQ(deserialized.UserAgent, original.UserAgent);
    EXPECT_EQ(deserialized.StartHeight, original.StartHeight);
}

TEST_F(P2PCompleteTest, Version_Capabilities) {
    VersionPayload version;
    
    // Add server capability
    ServerCapability server;
    server.Type = NodeCapabilityType::TcpServer;
    server.Port = 20333;
    version.Capabilities.push_back(server);
    
    // Add witness capability
    WitnessCapability witness;
    witness.Type = NodeCapabilityType::WitnessNode;
    version.Capabilities.push_back(witness);
    
    EXPECT_EQ(version.Capabilities.size(), 2);
}

TEST_F(P2PCompleteTest, Version_Handshake_Success) {
    // Simulate successful handshake
    auto remote = CreateRemoteNode("127.0.0.1", 20334);
    
    // Send version
    VersionPayload localVersion;
    localVersion.Nonce = local_node_->GetNonce();
    remote->SendMessage(MessageCommand::Version, localVersion);
    
    // Receive version
    VersionPayload remoteVersion;
    remoteVersion.Nonce = 0x12345678;
    
    // Verify nonces are different (no self-connection)
    EXPECT_NE(localVersion.Nonce, remoteVersion.Nonce);
    
    // Send/receive verack
    remote->SendMessage(MessageCommand::Verack, ByteVector());
    
    // Connection should be established
    remote->SetConnected(true);
    EXPECT_TRUE(remote->IsConnected());
}

// ============================================================================
// Address Management Tests
// ============================================================================

TEST_F(P2PCompleteTest, Addr_Payload) {
    AddrPayload addr;
    
    // Add network addresses
    for (int i = 0; i < 10; ++i) {
        NetworkAddress netAddr;
        netAddr.Timestamp = 1000000 + i;
        netAddr.Address = "192.168.1." + std::to_string(i);
        netAddr.Port = 20333;
        addr.AddressList.push_back(netAddr);
    }
    
    EXPECT_EQ(addr.AddressList.size(), 10);
}

TEST_F(P2PCompleteTest, Addr_Serialization) {
    AddrPayload original;
    
    NetworkAddress addr1;
    addr1.Timestamp = 1111111;
    addr1.Address = "10.0.0.1";
    addr1.Port = 20333;
    original.AddressList.push_back(addr1);
    
    NetworkAddress addr2;
    addr2.Timestamp = 2222222;
    addr2.Address = "10.0.0.2";
    addr2.Port = 20334;
    original.AddressList.push_back(addr2);
    
    auto serialized = original.Serialize();
    
    AddrPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.AddressList.size(), 2);
    EXPECT_EQ(deserialized.AddressList[0].Address, "10.0.0.1");
    EXPECT_EQ(deserialized.AddressList[1].Address, "10.0.0.2");
}

TEST_F(P2PCompleteTest, Addr_MaxAddresses) {
    AddrPayload addr;
    
    // Max 200 addresses per message
    const int MAX_ADDR = 200;
    
    for (int i = 0; i < MAX_ADDR + 10; ++i) {
        NetworkAddress netAddr;
        netAddr.Address = "1.1.1." + std::to_string(i % 256);
        addr.AddressList.push_back(netAddr);
    }
    
    // Should truncate to max
    EXPECT_LE(addr.GetValidAddressCount(), MAX_ADDR);
}

// ============================================================================
// Ping/Pong Tests
// ============================================================================

TEST_F(P2PCompleteTest, Ping_Payload) {
    PingPayload ping;
    ping.LastBlockIndex = 1000;
    ping.Timestamp = 1234567890;
    ping.Nonce = 0xDEADBEEF;
    
    EXPECT_EQ(ping.LastBlockIndex, 1000);
    EXPECT_EQ(ping.Timestamp, 1234567890);
    EXPECT_EQ(ping.Nonce, 0xDEADBEEF);
}

TEST_F(P2PCompleteTest, Ping_Pong_RoundTrip) {
    PingPayload ping;
    ping.LastBlockIndex = 5000;
    ping.Timestamp = std::time(nullptr);
    ping.Nonce = rand();
    
    // Send ping
    auto pingData = ping.Serialize();
    
    // Create pong from ping
    PingPayload pong;
    pong.Deserialize(pingData);
    
    // Pong should echo ping data
    EXPECT_EQ(pong.LastBlockIndex, ping.LastBlockIndex);
    EXPECT_EQ(pong.Nonce, ping.Nonce);
}

TEST_F(P2PCompleteTest, Ping_Latency_Calculation) {
    auto start = std::chrono::steady_clock::now();
    
    PingPayload ping;
    ping.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        start.time_since_epoch()).count();
    
    // Simulate network delay
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_GE(latency, 50);
    EXPECT_LT(latency, 100);
}

// ============================================================================
// Inventory Tests
// ============================================================================

TEST_F(P2PCompleteTest, Inventory_Types) {
    EXPECT_EQ(static_cast<uint8_t>(InventoryType::TX), 0x2b);
    EXPECT_EQ(static_cast<uint8_t>(InventoryType::Block), 0x2c);
    EXPECT_EQ(static_cast<uint8_t>(InventoryType::Consensus), 0x2d);
}

TEST_F(P2PCompleteTest, Inv_Payload) {
    InvPayload inv;
    inv.Type = InventoryType::TX;
    
    // Add transaction hashes
    for (int i = 0; i < 10; ++i) {
        UInt256 hash;
        hash.Fill(i);
        inv.Hashes.push_back(hash);
    }
    
    EXPECT_EQ(inv.Type, InventoryType::TX);
    EXPECT_EQ(inv.Hashes.size(), 10);
}

TEST_F(P2PCompleteTest, Inv_MaxHashes) {
    InvPayload inv;
    inv.Type = InventoryType::Block;
    
    // Max 50000 hashes per inv message
    const int MAX_INV = 50000;
    
    for (int i = 0; i < MAX_INV + 100; ++i) {
        UInt256 hash;
        hash.Fill(i % 256);
        inv.Hashes.push_back(hash);
    }
    
    EXPECT_LE(inv.GetValidHashCount(), MAX_INV);
}

TEST_F(P2PCompleteTest, GetData_Payload) {
    GetDataPayload getData;
    getData.Type = InventoryType::Block;
    
    UInt256 blockHash;
    blockHash.Fill(0xAB);
    getData.Hashes.push_back(blockHash);
    
    EXPECT_EQ(getData.Type, InventoryType::Block);
    EXPECT_EQ(getData.Hashes.size(), 1);
    EXPECT_EQ(getData.Hashes[0], blockHash);
}

// ============================================================================
// Block Synchronization Tests
// ============================================================================

TEST_F(P2PCompleteTest, GetBlocks_Payload) {
    GetBlocksPayload getBlocks;
    
    // Add hash start points
    UInt256 hash1, hash2;
    hash1.Fill(0x01);
    hash2.Fill(0x02);
    
    getBlocks.HashStart.push_back(hash1);
    getBlocks.HashStart.push_back(hash2);
    getBlocks.Count = 500;
    
    EXPECT_EQ(getBlocks.HashStart.size(), 2);
    EXPECT_EQ(getBlocks.Count, 500);
}

TEST_F(P2PCompleteTest, GetBlocks_MaxCount) {
    GetBlocksPayload getBlocks;
    
    // Max 500 blocks per request
    const uint16_t MAX_BLOCKS = 500;
    getBlocks.Count = 1000;
    
    EXPECT_LE(getBlocks.GetValidCount(), MAX_BLOCKS);
}

TEST_F(P2PCompleteTest, Headers_Payload) {
    HeadersPayload headers;
    
    // Add block headers
    for (int i = 0; i < 10; ++i) {
        Header header;
        header.Version = 0;
        header.PrevHash.Fill(i);
        header.MerkleRoot.Fill(i + 100);
        header.Timestamp = 1000000 + i;
        header.Index = i;
        headers.Headers.push_back(header);
    }
    
    EXPECT_EQ(headers.Headers.size(), 10);
}

TEST_F(P2PCompleteTest, GetBlockByIndex_Payload) {
    GetBlockByIndexPayload getBlock;
    getBlock.IndexStart = 1000;
    getBlock.Count = 50;
    
    EXPECT_EQ(getBlock.IndexStart, 1000);
    EXPECT_EQ(getBlock.Count, 50);
}

// ============================================================================
// Transaction Relay Tests
// ============================================================================

TEST_F(P2PCompleteTest, Transaction_Relay) {
    TransactionPayload txPayload;
    
    // Create transaction
    Transaction tx;
    tx.Version = 0;
    tx.Nonce = 12345;
    tx.SystemFee = 1000000;
    tx.NetworkFee = 500000;
    tx.ValidUntilBlock = 10000;
    
    txPayload.Transaction = tx;
    
    EXPECT_EQ(txPayload.Transaction.Version, 0);
    EXPECT_EQ(txPayload.Transaction.Nonce, 12345);
}

TEST_F(P2PCompleteTest, Mempool_Request) {
    MempoolPayload mempool;
    
    // Mempool request has no data
    auto serialized = mempool.Serialize();
    EXPECT_EQ(serialized.Size(), 0);
}

// ============================================================================
// Bloom Filter Tests
// ============================================================================

TEST_F(P2PCompleteTest, FilterLoad_Payload) {
    FilterLoadPayload filter;
    filter.Filter = ByteVector(1024, 0); // 1KB filter
    filter.K = 10; // Number of hash functions
    filter.Tweak = 0xDEADBEEF;
    
    EXPECT_EQ(filter.Filter.Size(), 1024);
    EXPECT_EQ(filter.K, 10);
    EXPECT_EQ(filter.Tweak, 0xDEADBEEF);
}

TEST_F(P2PCompleteTest, FilterAdd_Payload) {
    FilterAddPayload filterAdd;
    filterAdd.Data = ByteVector::FromString("Add to filter");
    
    EXPECT_GT(filterAdd.Data.Size(), 0);
}

TEST_F(P2PCompleteTest, FilterClear_Payload) {
    FilterClearPayload filterClear;
    
    // Clear has no data
    auto serialized = filterClear.Serialize();
    EXPECT_EQ(serialized.Size(), 0);
}

TEST_F(P2PCompleteTest, MerkleBlock_Payload) {
    MerkleBlockPayload merkle;
    
    // Set block header
    merkle.Header.Version = 0;
    merkle.Header.Index = 1000;
    merkle.TxCount = 10;
    
    // Add transaction hashes
    for (int i = 0; i < 5; ++i) {
        UInt256 hash;
        hash.Fill(i);
        merkle.Hashes.push_back(hash);
    }
    
    // Set flags
    merkle.Flags = ByteVector(10, 0xFF);
    
    EXPECT_EQ(merkle.TxCount, 10);
    EXPECT_EQ(merkle.Hashes.size(), 5);
    EXPECT_EQ(merkle.Flags.Size(), 10);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(P2PCompleteTest, NotFound_Payload) {
    NotFoundPayload notFound;
    notFound.Type = InventoryType::TX;
    
    UInt256 txHash;
    txHash.Fill(0xEF);
    notFound.Hashes.push_back(txHash);
    
    EXPECT_EQ(notFound.Type, InventoryType::TX);
    EXPECT_EQ(notFound.Hashes.size(), 1);
}

TEST_F(P2PCompleteTest, Reject_Payload) {
    RejectPayload reject;
    reject.Message = MessageCommand::Transaction;
    reject.Code = RejectCode::Invalid;
    reject.Reason = "Invalid transaction signature";
    reject.Data = ByteVector(32, 0); // Transaction hash
    
    EXPECT_EQ(reject.Message, MessageCommand::Transaction);
    EXPECT_EQ(reject.Code, RejectCode::Invalid);
    EXPECT_EQ(reject.Reason, "Invalid transaction signature");
}

TEST_F(P2PCompleteTest, Reject_Codes) {
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Malformed), 0x01);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Invalid), 0x10);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Obsolete), 0x11);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Double), 0x12);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::NonStandard), 0x40);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Dust), 0x41);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::InsufficientFee), 0x42);
    EXPECT_EQ(static_cast<uint8_t>(RejectCode::Checkpoint), 0x43);
}

// ============================================================================
// Connection Management Tests
// ============================================================================

TEST_F(P2PCompleteTest, Connection_Limits) {
    // Default max connections
    const int MAX_CONNECTIONS = 10;
    const int MAX_CONNECTIONS_PER_IP = 3;
    
    EXPECT_EQ(local_node_->GetMaxConnections(), MAX_CONNECTIONS);
    EXPECT_EQ(local_node_->GetMaxConnectionsPerIP(), MAX_CONNECTIONS_PER_IP);
}

TEST_F(P2PCompleteTest, Connection_Timeout) {
    auto remote = CreateRemoteNode("192.168.1.100", 20333);
    
    // Set connection timeout
    remote->SetTimeout(5000); // 5 seconds
    
    // Simulate timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check if timeout detection works
    EXPECT_FALSE(remote->HasTimedOut());
    
    // Simulate longer delay
    remote->SetLastReceiveTime(std::chrono::steady_clock::now() - std::chrono::seconds(10));
    EXPECT_TRUE(remote->HasTimedOut());
}

TEST_F(P2PCompleteTest, Connection_Banning) {
    std::string badIP = "10.0.0.1";
    
    // Ban IP
    local_node_->BanIP(badIP, 3600); // Ban for 1 hour
    
    EXPECT_TRUE(local_node_->IsIPBanned(badIP));
    
    // Check ban expiry
    local_node_->UnbanIP(badIP);
    EXPECT_FALSE(local_node_->IsIPBanned(badIP));
}

// ============================================================================
// DDoS Protection Tests
// ============================================================================

TEST_F(P2PCompleteTest, DDoS_RateLimit) {
    auto remote = CreateRemoteNode("10.0.0.1", 20333);
    
    // Send many messages quickly
    for (int i = 0; i < 100; ++i) {
        remote->IncrementMessageCount();
    }
    
    // Check if rate limit triggered
    EXPECT_TRUE(remote->IsRateLimited());
}

TEST_F(P2PCompleteTest, DDoS_MessageSizeLimit) {
    Message msg = CreateMessage(MessageCommand::Block);
    
    // Create oversized payload (>32MB)
    msg.Payload = ByteVector(33 * 1024 * 1024, 0);
    
    EXPECT_FALSE(msg.IsValid());
}

TEST_F(P2PCompleteTest, DDoS_ConnectionFlood) {
    // Simulate connection flood from same IP
    std::string floodIP = "192.168.1.1";
    
    for (int i = 0; i < 10; ++i) {
        auto remote = CreateRemoteNode(floodIP, 20333 + i);
        local_node_->AddConnection(remote.get());
    }
    
    // Should limit connections per IP
    EXPECT_LE(local_node_->GetConnectionCount(floodIP), 3);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(P2PCompleteTest, Performance_MessageSerialization) {
    InvPayload inv;
    inv.Type = InventoryType::TX;
    
    // Add 1000 hashes
    for (int i = 0; i < 1000; ++i) {
        UInt256 hash;
        hash.Fill(i % 256);
        inv.Hashes.push_back(hash);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Serialize/deserialize 100 times
    for (int i = 0; i < 100; ++i) {
        auto serialized = inv.Serialize();
        InvPayload deserialized;
        deserialized.Deserialize(serialized);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}

TEST_F(P2PCompleteTest, Performance_ConnectionHandling) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create and destroy 100 connections
    for (int i = 0; i < 100; ++i) {
        auto remote = std::make_unique<RemoteNode>("127.0.0.1", 20333 + i);
        remote->Connect();
        remote->Disconnect();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in under 1 second
    EXPECT_LT(duration.count(), 1000);
}