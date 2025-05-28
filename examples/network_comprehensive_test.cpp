#include <gtest/gtest.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/message_flags.h>
#include <neo/network/p2p/inventory_vector.h>
#include <neo/network/p2p/network_address.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;

// Test suite for Network module
class NetworkComprehensiveTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up common test resources
    }

    void TearDown() override
    {
        // Clean up common test resources
    }
};

// Test IPEndpoint
TEST_F(NetworkComprehensiveTest, IPEndpoint)
{
    // Test IPv4 endpoint
    IPEndpoint endpoint1("127.0.0.1", 10333);
    EXPECT_EQ(endpoint1.GetAddress(), "127.0.0.1");
    EXPECT_EQ(endpoint1.GetPort(), 10333);
    EXPECT_EQ(endpoint1.ToString(), "127.0.0.1:10333");

    // Test IPv6 endpoint
    IPEndpoint endpoint2("::1", 10333);
    EXPECT_EQ(endpoint2.GetAddress(), "::1");
    EXPECT_EQ(endpoint2.GetPort(), 10333);
    EXPECT_EQ(endpoint2.ToString(), "[::1]:10333");

    // Test equality
    IPEndpoint endpoint3("127.0.0.1", 10333);
    EXPECT_EQ(endpoint1, endpoint3);
    EXPECT_NE(endpoint1, endpoint2);
}

// Test NetworkAddress
TEST_F(NetworkComprehensiveTest, NetworkAddress)
{
    // Test network address
    NetworkAddress address1(IPEndpoint("127.0.0.1", 10333));
    EXPECT_EQ(address1.GetEndpoint().GetAddress(), "127.0.0.1");
    EXPECT_EQ(address1.GetEndpoint().GetPort(), 10333);

    // Test serialization
    ByteVector data;
    BinaryWriter writer(data);
    address1.Serialize(writer);

    BinaryReader reader(data);
    NetworkAddress address2;
    address2.Deserialize(reader);

    EXPECT_EQ(address1.GetEndpoint().GetAddress(), address2.GetEndpoint().GetAddress());
    EXPECT_EQ(address1.GetEndpoint().GetPort(), address2.GetEndpoint().GetPort());
}

// Test NodeCapability
TEST_F(NetworkComprehensiveTest, NodeCapability)
{
    // Test node capability
    NodeCapability capability1(NodeCapabilityType::TcpServer, 10333);
    EXPECT_EQ(capability1.GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(capability1.GetPort(), 10333);

    // Test serialization
    ByteVector data;
    BinaryWriter writer(data);
    capability1.Serialize(writer);

    BinaryReader reader(data);
    NodeCapability capability2;
    capability2.Deserialize(reader);

    EXPECT_EQ(capability1.GetType(), capability2.GetType());
    EXPECT_EQ(capability1.GetPort(), capability2.GetPort());
}

// Test InventoryVector
TEST_F(NetworkComprehensiveTest, InventoryVector)
{
    // Test inventory vector
    UInt256 hash = UInt256::Parse("0x0000000000000000000000000000000000000000000000000000000000000001");
    InventoryVector inv1(InventoryType::Block, hash);
    EXPECT_EQ(inv1.GetType(), InventoryType::Block);
    EXPECT_EQ(inv1.GetHash(), hash);

    // Test serialization
    ByteVector data;
    BinaryWriter writer(data);
    inv1.Serialize(writer);

    BinaryReader reader(data);
    InventoryVector inv2;
    inv2.Deserialize(reader);

    EXPECT_EQ(inv1.GetType(), inv2.GetType());
    EXPECT_EQ(inv1.GetHash(), inv2.GetHash());
}

// Test Message
TEST_F(NetworkComprehensiveTest, Message)
{
    // Test message
    ByteVector payload = {0x01, 0x02, 0x03};
    Message message1(p2p::MessageCommand::Version, payload, p2p::MessageFlags::None);
    EXPECT_EQ(message1.GetCommand(), p2p::MessageCommand::Version);
    EXPECT_EQ(message1.GetPayload(), payload);
    EXPECT_EQ(message1.GetFlags(), p2p::MessageFlags::None);

    // Test serialization
    ByteVector data;
    BinaryWriter writer(data);
    message1.Serialize(writer);

    BinaryReader reader(data);
    Message message2;
    message2.Deserialize(reader);

    EXPECT_EQ(message1.GetCommand(), message2.GetCommand());
    EXPECT_EQ(message1.GetPayload(), message2.GetPayload());
    EXPECT_EQ(message1.GetFlags(), message2.GetFlags());
}

// Test VersionPayload
TEST_F(NetworkComprehensiveTest, VersionPayload)
{
    // Test version payload
    uint32_t version = 0;
    uint64_t services = 1;
    uint64_t timestamp = 1234567890;
    uint16_t port = 10333;
    uint32_t nonce = 123456;
    std::string userAgent = "Neo-CPP";
    uint32_t startHeight = 0;
    bool relay = true;

    VersionPayload payload1(version, services, timestamp, port, nonce, userAgent, startHeight, relay);
    EXPECT_EQ(payload1.GetVersion(), version);
    EXPECT_EQ(payload1.GetServices(), services);
    EXPECT_EQ(payload1.GetTimestamp(), timestamp);
    EXPECT_EQ(payload1.GetPort(), port);
    EXPECT_EQ(payload1.GetNonce(), nonce);
    EXPECT_EQ(payload1.GetUserAgent(), userAgent);
    EXPECT_EQ(payload1.GetStartHeight(), startHeight);
    EXPECT_EQ(payload1.GetRelay(), relay);

    // Test serialization
    ByteVector data;
    BinaryWriter writer(data);
    payload1.Serialize(writer);

    BinaryReader reader(data);
    VersionPayload payload2;
    payload2.Deserialize(reader);

    EXPECT_EQ(payload1.GetVersion(), payload2.GetVersion());
    EXPECT_EQ(payload1.GetServices(), payload2.GetServices());
    EXPECT_EQ(payload1.GetTimestamp(), payload2.GetTimestamp());
    EXPECT_EQ(payload1.GetPort(), payload2.GetPort());
    EXPECT_EQ(payload1.GetNonce(), payload2.GetNonce());
    EXPECT_EQ(payload1.GetUserAgent(), payload2.GetUserAgent());
    EXPECT_EQ(payload1.GetStartHeight(), payload2.GetStartHeight());
    EXPECT_EQ(payload1.GetRelay(), payload2.GetRelay());
}

int main(int argc, char** argv)
{
    std::cout << "Running Network comprehensive test..." << std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
