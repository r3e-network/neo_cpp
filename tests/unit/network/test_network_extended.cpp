#include <gtest/gtest.h>
#include <neo/network/remote_node.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/getblocks_payload.h>
#include <neo/network/p2p/payloads/merkleblock_payload.h>
#include <neo/network/message.h>

using namespace neo::network;
using namespace neo::network::p2p::payloads;

class NetworkExtendedTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(NetworkExtendedTest, TestRemoteNode)
{
    // Create remote node
    RemoteNode node("127.0.0.1", 20333);
    
    EXPECT_EQ(node.GetAddress(), "127.0.0.1");
    EXPECT_EQ(node.GetPort(), 20333);
    EXPECT_FALSE(node.IsConnected());
    
    // Test connection state
    node.SetConnected(true);
    EXPECT_TRUE(node.IsConnected());
    
    // Test version
    node.SetVersion(70016);
    EXPECT_EQ(node.GetVersion(), 70016);
}

TEST_F(NetworkExtendedTest, TestP2PMessage)
{
    // Create a P2P message
    Message msg(MessageType::Version);
    
    EXPECT_EQ(msg.GetType(), MessageType::Version);
    EXPECT_EQ(msg.GetCommand(), "version");
    
    // Test payload
    VersionPayload payload;
    payload.Version = 70016;
    payload.Services = 1;
    payload.Timestamp = std::time(nullptr);
    payload.Port = 20333;
    payload.Nonce = 0x12345678;
    payload.UserAgent = "/Neo:3.5.0/";
    payload.StartHeight = 1000;
    payload.Relay = true;
    
    msg.SetPayload(payload);
    EXPECT_FALSE(msg.GetPayload().empty());
}

TEST_F(NetworkExtendedTest, TestVersionPayload)
{
    VersionPayload payload;
    payload.Version = 70016;
    payload.Services = 1;
    payload.Timestamp = std::time(nullptr);
    payload.Port = 20333;
    payload.Nonce = 0x12345678;
    payload.UserAgent = "/Neo:3.5.0/";
    payload.StartHeight = 1000;
    payload.Relay = true;
    
    // Serialize
    ByteVector serialized = payload.Serialize();
    EXPECT_GT(serialized.Size(), 0);
    
    // Deserialize
    VersionPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Version, payload.Version);
    EXPECT_EQ(deserialized.Services, payload.Services);
    EXPECT_EQ(deserialized.Port, payload.Port);
    EXPECT_EQ(deserialized.Nonce, payload.Nonce);
    EXPECT_EQ(deserialized.UserAgent, payload.UserAgent);
    EXPECT_EQ(deserialized.StartHeight, payload.StartHeight);
    EXPECT_EQ(deserialized.Relay, payload.Relay);
}

TEST_F(NetworkExtendedTest, TestAddrPayload)
{
    AddrPayload payload;
    
    // Add network addresses
    NetworkAddress addr1;
    addr1.Timestamp = std::time(nullptr);
    addr1.Services = 1;
    addr1.IP = "192.168.1.1";
    addr1.Port = 20333;
    
    NetworkAddress addr2;
    addr2.Timestamp = std::time(nullptr);
    addr2.Services = 1;
    addr2.IP = "192.168.1.2";
    addr2.Port = 20334;
    
    payload.Addresses.push_back(addr1);
    payload.Addresses.push_back(addr2);
    
    EXPECT_EQ(payload.Addresses.size(), 2);
    
    // Serialize and deserialize
    ByteVector serialized = payload.Serialize();
    AddrPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Addresses.size(), 2);
    EXPECT_EQ(deserialized.Addresses[0].IP, addr1.IP);
    EXPECT_EQ(deserialized.Addresses[1].Port, addr2.Port);
}

TEST_F(NetworkExtendedTest, TestGetBlocksPayload)
{
    GetBlocksPayload payload;
    
    // Set hash start
    payload.HashStart = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    payload.Count = 500;
    
    EXPECT_EQ(payload.Count, 500);
    EXPECT_FALSE(payload.HashStart.IsZero());
    
    // Serialize and deserialize
    ByteVector serialized = payload.Serialize();
    GetBlocksPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.HashStart, payload.HashStart);
    EXPECT_EQ(deserialized.Count, payload.Count);
}

TEST_F(NetworkExtendedTest, TestInvPayload)
{
    InvPayload payload;
    payload.Type = InventoryType::Block;
    
    // Add hashes
    payload.Hashes.push_back(UInt256::Parse("0x1111111111111111111111111111111111111111111111111111111111111111"));
    payload.Hashes.push_back(UInt256::Parse("0x2222222222222222222222222222222222222222222222222222222222222222"));
    payload.Hashes.push_back(UInt256::Parse("0x3333333333333333333333333333333333333333333333333333333333333333"));
    
    EXPECT_EQ(payload.Type, InventoryType::Block);
    EXPECT_EQ(payload.Hashes.size(), 3);
    
    // Serialize and deserialize
    ByteVector serialized = payload.Serialize();
    InvPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Type, payload.Type);
    EXPECT_EQ(deserialized.Hashes.size(), payload.Hashes.size());
    EXPECT_EQ(deserialized.Hashes[0], payload.Hashes[0]);
}

TEST_F(NetworkExtendedTest, TestMerkleBlockPayload)
{
    MerkleBlockPayload payload;
    
    // Set block header
    payload.Version = 0;
    payload.PrevHash = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000000");
    payload.MerkleRoot = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    payload.Timestamp = std::time(nullptr);
    payload.Index = 12345;
    payload.NextConsensus = UInt160::Parse("0x1234567890abcdef1234567890abcdef12345678");
    
    // Set transaction count and flags
    payload.TxCount = 10;
    payload.Flags = ByteVector::Parse("ff00ff00");
    
    // Add hashes
    payload.Hashes.push_back(UInt256::Parse("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    payload.Hashes.push_back(UInt256::Parse("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    
    EXPECT_EQ(payload.TxCount, 10);
    EXPECT_EQ(payload.Index, 12345);
    EXPECT_EQ(payload.Hashes.size(), 2);
    
    // Serialize and deserialize
    ByteVector serialized = payload.Serialize();
    MerkleBlockPayload deserialized;
    deserialized.Deserialize(serialized);
    
    EXPECT_EQ(deserialized.Index, payload.Index);
    EXPECT_EQ(deserialized.TxCount, payload.TxCount);
    EXPECT_EQ(deserialized.MerkleRoot, payload.MerkleRoot);
}