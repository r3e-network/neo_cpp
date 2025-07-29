#include <chrono>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/network_address.h>
#include <neo/network/p2p/node_capability.h>
#include <sstream>
#include <vector>

using namespace neo::network;
using namespace neo::network::p2p;
using namespace neo::io;

class UT_network_addresses : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment with various network addresses
        validEndpoint_ = IPEndPoint("192.168.1.100", 10333);
        loopbackEndpoint_ = IPEndPoint("127.0.0.1", 10333);
        publicEndpoint_ = IPEndPoint("203.0.113.50", 10333);
        invalidEndpoint_ = IPEndPoint("0.0.0.0", 0);

        // Create network addresses with different capabilities
        fullNodeAddress_ = NetworkAddress(validEndpoint_, NodeCapabilityType::FullNode);
        tcpServerAddress_ = NetworkAddress(publicEndpoint_, NodeCapabilityType::TcpServer);
        wsServerAddress_ = NetworkAddress(loopbackEndpoint_, NodeCapabilityType::WsServer);

        // Create timestamped addresses
        auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                       .count();
        currentTime_ = static_cast<uint32_t>(now);
        pastTime_ = static_cast<uint32_t>(now - 3600);    // 1 hour ago
        futureTime_ = static_cast<uint32_t>(now + 3600);  // 1 hour in future
    }

    void TearDown() override
    {
        // Cleanup
    }

    IPEndPoint validEndpoint_;
    IPEndPoint loopbackEndpoint_;
    IPEndPoint publicEndpoint_;
    IPEndPoint invalidEndpoint_;

    NetworkAddress fullNodeAddress_;
    NetworkAddress tcpServerAddress_;
    NetworkAddress wsServerAddress_;

    uint32_t currentTime_;
    uint32_t pastTime_;
    uint32_t futureTime_;
};

TEST_F(UT_network_addresses, NetworkAddress_Construction)
{
    // Test: Basic NetworkAddress construction and access

    // Test construction with endpoint and capability
    NetworkAddress addr(validEndpoint_, NodeCapabilityType::FullNode);
    EXPECT_EQ(addr.GetEndpoint().GetAddress(), "192.168.1.100");
    EXPECT_EQ(addr.GetEndpoint().GetPort(), 10333);

    // Test different capability types
    NetworkAddress tcpAddr(publicEndpoint_, NodeCapabilityType::TcpServer);
    NetworkAddress wsAddr(loopbackEndpoint_, NodeCapabilityType::WsServer);

    EXPECT_EQ(tcpAddr.GetEndpoint().GetAddress(), "203.0.113.50");
    EXPECT_EQ(wsAddr.GetEndpoint().GetAddress(), "127.0.0.1");
}

TEST_F(UT_network_addresses, NetworkAddressWithTime_Construction)
{
    // Test: NetworkAddressWithTime construction and access

    NetworkAddressWithTime timedAddr(currentTime_, fullNodeAddress_);

    EXPECT_EQ(timedAddr.GetTimestamp(), currentTime_);
    EXPECT_EQ(timedAddr.GetAddress().GetEndpoint().GetAddress(), "192.168.1.100");
    EXPECT_EQ(timedAddr.GetAddress().GetEndpoint().GetPort(), 10333);
}

TEST_F(UT_network_addresses, NetworkAddress_Serialization)
{
    // Test: NetworkAddress serialization and deserialization

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    fullNodeAddress_.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    NetworkAddress deserializedAddr;
    deserializedAddr.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserializedAddr.GetEndpoint().GetAddress(), fullNodeAddress_.GetEndpoint().GetAddress());
    EXPECT_EQ(deserializedAddr.GetEndpoint().GetPort(), fullNodeAddress_.GetEndpoint().GetPort());
}

TEST_F(UT_network_addresses, NetworkAddressWithTime_Serialization)
{
    // Test: NetworkAddressWithTime serialization and deserialization

    NetworkAddressWithTime originalAddr(pastTime_, tcpServerAddress_);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    originalAddr.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    NetworkAddressWithTime deserializedAddr;
    deserializedAddr.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserializedAddr.GetTimestamp(), pastTime_);
    EXPECT_EQ(deserializedAddr.GetAddress().GetEndpoint().GetAddress(), "203.0.113.50");
    EXPECT_EQ(deserializedAddr.GetAddress().GetEndpoint().GetPort(), 10333);
}

TEST_F(UT_network_addresses, DifferentCapabilityTypes)
{
    // Test: Network addresses with different capability types

    std::vector<NetworkAddress> addresses = {NetworkAddress(validEndpoint_, NodeCapabilityType::FullNode),
                                             NetworkAddress(publicEndpoint_, NodeCapabilityType::TcpServer),
                                             NetworkAddress(loopbackEndpoint_, NodeCapabilityType::WsServer)};

    // Test each capability type serializes/deserializes correctly
    for (const auto& addr : addresses)
    {
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        addr.Serialize(writer);

        stream.seekg(0);
        BinaryReader reader(stream);
        NetworkAddress deserializedAddr;
        EXPECT_NO_THROW(deserializedAddr.Deserialize(reader));

        EXPECT_EQ(deserializedAddr.GetEndpoint().GetAddress(), addr.GetEndpoint().GetAddress());
        EXPECT_EQ(deserializedAddr.GetEndpoint().GetPort(), addr.GetEndpoint().GetPort());
    }
}

TEST_F(UT_network_addresses, TimestampHandling)
{
    // Test: Various timestamp scenarios

    // Test past, current, and future timestamps
    std::vector<uint32_t> timestamps = {pastTime_, currentTime_, futureTime_};

    for (uint32_t timestamp : timestamps)
    {
        NetworkAddressWithTime addr(timestamp, fullNodeAddress_);

        // Serialize
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        addr.Serialize(writer);

        // Deserialize
        stream.seekg(0);
        BinaryReader reader(stream);
        NetworkAddressWithTime deserializedAddr;
        deserializedAddr.Deserialize(reader);

        EXPECT_EQ(deserializedAddr.GetTimestamp(), timestamp);
    }
}

TEST_F(UT_network_addresses, EdgeCase_ZeroTimestamp)
{
    // Test: Zero timestamp handling

    NetworkAddressWithTime zeroTimeAddr(0, fullNodeAddress_);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    EXPECT_NO_THROW(zeroTimeAddr.Serialize(writer));

    stream.seekg(0);
    BinaryReader reader(stream);
    NetworkAddressWithTime deserializedAddr;
    EXPECT_NO_THROW(deserializedAddr.Deserialize(reader));

    EXPECT_EQ(deserializedAddr.GetTimestamp(), 0u);
}

TEST_F(UT_network_addresses, InvalidIPAddresses)
{
    // Test: Handling of invalid IP addresses

    // Test with invalid IP endpoints
    IPEndPoint invalidIPs[] = {
        IPEndPoint("0.0.0.0", 10333), IPEndPoint("255.255.255.255", 10333),
        IPEndPoint("192.168.1.256", 10333),  // Invalid IP
        IPEndPoint("192.168.1.100", 0)       // Invalid port
    };

    for (const auto& endpoint : invalidIPs)
    {
        // Should handle gracefully without throwing
        EXPECT_NO_THROW({
            NetworkAddress addr(endpoint, NodeCapabilityType::FullNode);

            std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
            BinaryWriter writer(stream);
            addr.Serialize(writer);
        });
    }
}

TEST_F(UT_network_addresses, ExtremeTimestamps)
{
    // Test: Extreme timestamp values

    uint32_t extremeTimestamps[] = {
        0,              // Minimum
        1,              // Near minimum
        UINT32_MAX,     // Maximum
        UINT32_MAX - 1  // Near maximum
    };

    for (uint32_t timestamp : extremeTimestamps)
    {
        NetworkAddressWithTime addr(timestamp, fullNodeAddress_);

        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        EXPECT_NO_THROW(addr.Serialize(writer));

        stream.seekg(0);
        BinaryReader reader(stream);
        NetworkAddressWithTime deserializedAddr;
        EXPECT_NO_THROW(deserializedAddr.Deserialize(reader));

        EXPECT_EQ(deserializedAddr.GetTimestamp(), timestamp);
    }
}

TEST_F(UT_network_addresses, SerializationRoundTrip)
{
    // Test: Multiple serialization/deserialization cycles

    NetworkAddressWithTime originalAddr(currentTime_, fullNodeAddress_);
    NetworkAddressWithTime currentAddr = originalAddr;

    // Perform multiple round trips
    for (int i = 0; i < 5; ++i)
    {
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        currentAddr.Serialize(writer);

        stream.seekg(0);
        BinaryReader reader(stream);
        NetworkAddressWithTime roundTripAddr;
        roundTripAddr.Deserialize(reader);

        // Verify integrity maintained
        EXPECT_EQ(roundTripAddr.GetTimestamp(), originalAddr.GetTimestamp());
        EXPECT_EQ(roundTripAddr.GetAddress().GetEndpoint().GetAddress(),
                  originalAddr.GetAddress().GetEndpoint().GetAddress());
        EXPECT_EQ(roundTripAddr.GetAddress().GetEndpoint().GetPort(),
                  originalAddr.GetAddress().GetEndpoint().GetPort());

        currentAddr = roundTripAddr;
    }
}

TEST_F(UT_network_addresses, ErrorHandling_CorruptedData)
{
    // Test: Error handling with corrupted serialization data

    NetworkAddressWithTime addr(currentTime_, fullNodeAddress_);

    // Serialize
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    addr.Serialize(writer);

    // Get data and corrupt it
    stream.seekg(0, std::ios::end);
    size_t dataSize = stream.tellg();
    stream.seekg(0);

    std::vector<char> data(dataSize);
    stream.read(data.data(), dataSize);

    // Corrupt data at various positions
    for (size_t corruptPos = 0; corruptPos < std::min(dataSize, size_t(8)); ++corruptPos)
    {
        auto corruptedData = data;
        corruptedData[corruptPos] ^= 0xFF;

        std::stringstream corruptedStream(std::string(corruptedData.begin(), corruptedData.end()),
                                          std::ios::in | std::ios::binary);
        BinaryReader reader(corruptedStream);
        NetworkAddressWithTime corruptedAddr;

        // Should handle corruption gracefully
        EXPECT_NO_THROW({
            try
            {
                corruptedAddr.Deserialize(reader);
            }
            catch (const std::exception&)
            {
                // Expected for corrupted data
            }
        });
    }
}
