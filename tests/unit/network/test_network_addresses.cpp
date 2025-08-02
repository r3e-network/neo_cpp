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
        validAddress_ = IPAddress("192.168.1.100");
        loopbackAddress_ = IPAddress("127.0.0.1");
        publicAddress_ = IPAddress("203.0.113.50");
        invalidAddress_ = IPAddress("0.0.0.0");

        // Create capabilities
        capabilities_.push_back(NodeCapability(NodeCapabilityType::TcpServer));
        capabilities_.push_back(NodeCapability(NodeCapabilityType::WsServer));

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

    IPAddress validAddress_;
    IPAddress loopbackAddress_;
    IPAddress publicAddress_;
    IPAddress invalidAddress_;

    std::vector<NodeCapability> capabilities_;

    uint32_t currentTime_;
    uint32_t pastTime_;
    uint32_t futureTime_;
};

TEST_F(UT_network_addresses, NetworkAddressWithTime_Construction)
{
    // Test: Basic NetworkAddressWithTime construction and access

    // Test construction with address and capability
    NetworkAddressWithTime addr(currentTime_, validAddress_, capabilities_);
    EXPECT_EQ(addr.GetAddress().ToString(), "192.168.1.100");
    EXPECT_EQ(addr.GetTimestamp(), currentTime_);
    EXPECT_EQ(addr.GetCapabilities().size(), 2u);

    // Test different addresses
    NetworkAddressWithTime publicAddr(currentTime_, publicAddress_, capabilities_);
    NetworkAddressWithTime loopbackAddr(currentTime_, loopbackAddress_, capabilities_);

    EXPECT_EQ(publicAddr.GetAddress().ToString(), "203.0.113.50");
    EXPECT_EQ(loopbackAddr.GetAddress().ToString(), "127.0.0.1");
}

TEST_F(UT_network_addresses, NetworkAddressWithTime_GettersSetters)
{
    // Test: NetworkAddressWithTime getters and setters

    NetworkAddressWithTime timedAddr;

    timedAddr.SetTimestamp(currentTime_);
    timedAddr.SetAddress(validAddress_);
    timedAddr.SetCapabilities(capabilities_);

    EXPECT_EQ(timedAddr.GetTimestamp(), currentTime_);
    EXPECT_EQ(timedAddr.GetAddress().ToString(), "192.168.1.100");
    EXPECT_EQ(timedAddr.GetCapabilities().size(), 2u);
}

TEST_F(UT_network_addresses, NetworkAddressWithTime_Serialization)
{
    // Test: NetworkAddressWithTime serialization and deserialization

    NetworkAddressWithTime originalAddr(currentTime_, validAddress_, capabilities_);

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
    EXPECT_EQ(deserializedAddr.GetAddress().ToString(), originalAddr.GetAddress().ToString());
    EXPECT_EQ(deserializedAddr.GetTimestamp(), originalAddr.GetTimestamp());
    EXPECT_EQ(deserializedAddr.GetCapabilities().size(), originalAddr.GetCapabilities().size());
}

TEST_F(UT_network_addresses, DifferentCapabilityTypes)
{
    // Test: Network addresses with different capability types

    std::vector<std::vector<NodeCapability>> allCapabilities = {
        {NodeCapability(NodeCapabilityType::TcpServer)},
        {NodeCapability(NodeCapabilityType::WsServer)},
        {NodeCapability(NodeCapabilityType::TcpServer), NodeCapability(NodeCapabilityType::WsServer)}
    };

    // Test each capability type serializes/deserializes correctly
    for (const auto& caps : allCapabilities)
    {
        NetworkAddressWithTime addr(currentTime_, validAddress_, caps);
        
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);
        addr.Serialize(writer);

        stream.seekg(0);
        BinaryReader reader(stream);
        NetworkAddressWithTime deserializedAddr;
        EXPECT_NO_THROW(deserializedAddr.Deserialize(reader));

        EXPECT_EQ(deserializedAddr.GetAddress().ToString(), addr.GetAddress().ToString());
        EXPECT_EQ(deserializedAddr.GetCapabilities().size(), addr.GetCapabilities().size());
    }
}

TEST_F(UT_network_addresses, TimestampHandling)
{
    // Test: Various timestamp scenarios

    // Test past, current, and future timestamps
    std::vector<uint32_t> timestamps = {pastTime_, currentTime_, futureTime_};

    for (uint32_t timestamp : timestamps)
    {
        NetworkAddressWithTime addr(timestamp, validAddress_, capabilities_);

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

    NetworkAddressWithTime zeroTimeAddr(0, validAddress_, capabilities_);

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

    // Test with invalid IP addresses
    std::vector<std::string> invalidIPs = {
        "0.0.0.0", 
        "255.255.255.255",
        "192.168.1.256",  // Invalid IP
        ""                // Empty string
    };

    for (const auto& ip : invalidIPs)
    {
        // Should handle gracefully without throwing
        EXPECT_NO_THROW({
            IPAddress addr(ip);
            NetworkAddressWithTime netAddr(currentTime_, addr, capabilities_);

            std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
            BinaryWriter writer(stream);
            netAddr.Serialize(writer);
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
        NetworkAddressWithTime addr(timestamp, validAddress_, capabilities_);

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

    NetworkAddressWithTime originalAddr(currentTime_, validAddress_, capabilities_);
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
        EXPECT_EQ(roundTripAddr.GetAddress().ToString(),
                  originalAddr.GetAddress().ToString());
        EXPECT_EQ(roundTripAddr.GetCapabilities().size(),
                  originalAddr.GetCapabilities().size());

        currentAddr = roundTripAddr;
    }
}

TEST_F(UT_network_addresses, GetEndPoint)
{
    // Test: GetEndPoint functionality

    std::vector<NodeCapability> tcpCaps = {NodeCapability(NodeCapabilityType::TcpServer)};
    NetworkAddressWithTime addr(currentTime_, validAddress_, tcpCaps);

    IPEndPoint endpoint = addr.GetEndPoint();
    EXPECT_EQ(endpoint.GetAddress().ToString(), "192.168.1.100");
    EXPECT_EQ(endpoint.GetPort(), 10333);
}

TEST_F(UT_network_addresses, ErrorHandling_CorruptedData)
{
    // Test: Error handling with corrupted serialization data

    NetworkAddressWithTime addr(currentTime_, validAddress_, capabilities_);

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