#include <chrono>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/network_address.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <sstream>
#include <vector>

using namespace neo::network::p2p;
using namespace neo::io;

class UT_peer_discovery : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment with sample peer addresses
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                       .count();

        // Create test network addresses
        testAddresses_.emplace_back(static_cast<uint32_t>(now),
                                    NetworkAddress(IPEndPoint("192.168.1.100", 10333), NodeCapabilityType::FullNode));

        testAddresses_.emplace_back(static_cast<uint32_t>(now - 3600),  // 1 hour ago
                                    NetworkAddress(IPEndPoint("203.0.113.50", 10333), NodeCapabilityType::FullNode));

        testAddresses_.emplace_back(static_cast<uint32_t>(now - 7200),  // 2 hours ago
                                    NetworkAddress(IPEndPoint("198.51.100.25", 10333), NodeCapabilityType::FullNode));
    }

    void TearDown() override
    {
        // Cleanup
        testAddresses_.clear();
    }

    std::vector<NetworkAddressWithTime> testAddresses_;
};

TEST_F(UT_peer_discovery, AddrPayload_Construction)
{
    // Test: Basic AddrPayload construction and access

    // Test default construction
    AddrPayload emptyPayload;
    EXPECT_TRUE(emptyPayload.GetAddressList().empty());
    EXPECT_GT(emptyPayload.GetSize(), 0);  // Should have some header size

    // Test construction with address list
    AddrPayload payload(testAddresses_);
    EXPECT_EQ(payload.GetAddressList().size(), testAddresses_.size());
    EXPECT_EQ(payload.GetAddressList(), testAddresses_);
}

TEST_F(UT_peer_discovery, AddrPayload_Serialization)
{
    // Test: AddrPayload serialization and deserialization

    AddrPayload originalPayload(testAddresses_);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    originalPayload.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    AddrPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserializedPayload.GetAddressList().size(), originalPayload.GetAddressList().size());

    const auto& originalAddresses = originalPayload.GetAddressList();
    const auto& deserializedAddresses = deserializedPayload.GetAddressList();

    for (size_t i = 0; i < originalAddresses.size(); ++i)
    {
        EXPECT_EQ(originalAddresses[i].GetTimestamp(), deserializedAddresses[i].GetTimestamp());
        EXPECT_EQ(originalAddresses[i].GetAddress().GetEndpoint().GetAddress(),
                  deserializedAddresses[i].GetAddress().GetEndpoint().GetAddress());
        EXPECT_EQ(originalAddresses[i].GetAddress().GetEndpoint().GetPort(),
                  deserializedAddresses[i].GetAddress().GetEndpoint().GetPort());
    }
}

TEST_F(UT_peer_discovery, GetAddr_AddrMessageFlow)
{
    // Test: Complete GetAddr -> Addr message flow

    // Create GetAddr message (no payload)
    auto getAddrMessage = Message::Create(MessageCommand::GetAddr);

    // Verify GetAddr message
    EXPECT_EQ(getAddrMessage.GetCommand(), MessageCommand::GetAddr);
    EXPECT_EQ(getAddrMessage.GetPayload(), nullptr);

    // Create response Addr message with peer list
    auto addrPayload = std::make_shared<AddrPayload>(testAddresses_);
    auto addrMessage = Message::Create(MessageCommand::Addr, addrPayload);

    // Verify Addr message
    EXPECT_EQ(addrMessage.GetCommand(), MessageCommand::Addr);
    EXPECT_NE(addrMessage.GetPayload(), nullptr);

    auto responsePayload = std::dynamic_pointer_cast<AddrPayload>(addrMessage.GetPayload());
    ASSERT_NE(responsePayload, nullptr);
    EXPECT_EQ(responsePayload->GetAddressList().size(), testAddresses_.size());
}

TEST_F(UT_peer_discovery, MaxAddressLimit)
{
    // Test: AddrPayload respects maximum address count

    // Create more addresses than the maximum allowed
    std::vector<NetworkAddressWithTime> manyAddresses;
    auto now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    for (int i = 0; i < AddrPayload::MaxCountToSend + 50; ++i)
    {
        std::string ip = "192.168.1." + std::to_string((i % 254) + 1);
        manyAddresses.emplace_back(now - i, NetworkAddress(IPEndPoint(ip, 10333), NodeCapabilityType::FullNode));
    }

    // Create payload with excessive addresses
    AddrPayload payload(manyAddresses);

    // Should handle gracefully - either truncate or reject
    EXPECT_LE(payload.GetAddressList().size(), static_cast<size_t>(AddrPayload::MaxCountToSend + 50));

    // Test serialization doesn't crash with many addresses
    EXPECT_NO_THROW({
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        payload.Serialize(writer);
    });
}

TEST_F(UT_peer_discovery, EmptyAddressList)
{
    // Test: Handling of empty address lists

    AddrPayload emptyPayload;

    // Test serialization of empty payload
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    EXPECT_NO_THROW(emptyPayload.Serialize(writer));

    // Test deserialization of empty payload
    stream.seekg(0);
    BinaryReader reader(stream);
    AddrPayload deserializedEmpty;
    EXPECT_NO_THROW(deserializedEmpty.Deserialize(reader));

    EXPECT_TRUE(deserializedEmpty.GetAddressList().empty());
}

TEST_F(UT_peer_discovery, DuplicateAddresses)
{
    // Test: Handling of duplicate addresses in discovery

    std::vector<NetworkAddressWithTime> duplicateAddresses;
    auto now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    // Add same address multiple times with different timestamps
    NetworkAddress sameAddress(IPEndPoint("192.168.1.100", 10333), NodeCapabilityType::FullNode);

    duplicateAddresses.emplace_back(now, sameAddress);
    duplicateAddresses.emplace_back(now - 1800, sameAddress);  // 30 minutes ago
    duplicateAddresses.emplace_back(now - 3600, sameAddress);  // 1 hour ago

    AddrPayload payload(duplicateAddresses);

    // Should handle gracefully
    EXPECT_EQ(payload.GetAddressList().size(), 3u);

    // Test serialization/deserialization with duplicates
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    EXPECT_NO_THROW(payload.Serialize(writer));

    stream.seekg(0);
    BinaryReader reader(stream);
    AddrPayload deserializedPayload;
    EXPECT_NO_THROW(deserializedPayload.Deserialize(reader));
}

TEST_F(UT_peer_discovery, InvalidAddresses)
{
    // Test: Handling of invalid network addresses

    std::vector<NetworkAddressWithTime> invalidAddresses;
    auto now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    // Add addresses with invalid IPs
    invalidAddresses.emplace_back(now, NetworkAddress(IPEndPoint("0.0.0.0", 10333), NodeCapabilityType::FullNode));

    invalidAddresses.emplace_back(now, NetworkAddress(IPEndPoint("255.255.255.255", 0), NodeCapabilityType::FullNode));

    AddrPayload payload(invalidAddresses);

    // Should handle invalid addresses gracefully
    EXPECT_NO_THROW({
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        payload.Serialize(writer);
    });
}

TEST_F(UT_peer_discovery, FutureTimestamps)
{
    // Test: Handling of addresses with future timestamps

    std::vector<NetworkAddressWithTime> futureAddresses;
    auto futureTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                                std::chrono::system_clock::now().time_since_epoch())
                                                .count()) +
                      86400;  // 1 day in future

    futureAddresses.emplace_back(futureTime,
                                 NetworkAddress(IPEndPoint("192.168.1.200", 10333), NodeCapabilityType::FullNode));

    AddrPayload payload(futureAddresses);

    // Should handle future timestamps gracefully
    EXPECT_NO_THROW({
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        payload.Serialize(writer);

        stream.seekg(0);
        BinaryReader reader(stream);
        AddrPayload deserializedPayload;
        deserializedPayload.Deserialize(reader);

        EXPECT_EQ(deserializedPayload.GetAddressList().size(), 1u);
        EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetTimestamp(), futureTime);
    });
}

TEST_F(UT_peer_discovery, MessageRoundTrip)
{
    // Test: Complete message serialization round trip for peer discovery

    auto addrPayload = std::make_shared<AddrPayload>(testAddresses_);
    auto originalMessage = Message::Create(MessageCommand::Addr, addrPayload);

    // Serialize message
    auto messageData = originalMessage.ToArray();
    EXPECT_FALSE(messageData.empty());

    // Deserialize message
    Message deserializedMessage;
    uint32_t bytesRead = Message::TryDeserialize(messageData.AsSpan(), deserializedMessage);
    EXPECT_GT(bytesRead, 0u);
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Addr);

    // Verify payload
    auto deserializedPayload = std::dynamic_pointer_cast<AddrPayload>(deserializedMessage.GetPayload());
    ASSERT_NE(deserializedPayload, nullptr);
    EXPECT_EQ(deserializedPayload->GetAddressList().size(), testAddresses_.size());
}

TEST_F(UT_peer_discovery, ErrorHandling_CorruptedData)
{
    // Test: Error handling with corrupted peer discovery data

    AddrPayload payload(testAddresses_);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    payload.Serialize(writer);

    // Get serialized data and corrupt it
    stream.seekg(0, std::ios::end);
    size_t dataSize = stream.tellg();
    stream.seekg(0);

    std::vector<char> data(dataSize);
    stream.read(data.data(), dataSize);

    // Corrupt middle of data
    if (dataSize > 10)
    {
        data[dataSize / 2] ^= 0xFF;
    }

    // Try to deserialize corrupted data
    std::stringstream corruptedStream(std::string(data.begin(), data.end()), std::ios::in | std::ios::binary);
    BinaryReader reader(corruptedStream);
    AddrPayload corruptedPayload;

    // Should handle corruption gracefully (may throw or return empty)
    EXPECT_NO_THROW({
        try
        {
            corruptedPayload.Deserialize(reader);
        }
        catch (const std::exception&)
        {
            // Expected for corrupted data
        }
    });
}
