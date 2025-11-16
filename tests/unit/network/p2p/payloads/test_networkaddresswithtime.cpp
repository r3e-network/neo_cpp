#include <gtest/gtest.h>
#include <sstream>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/ip_address.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/payloads/network_address_with_time.h>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;
using neo::network::IPAddress;

namespace
{
NodeCapability CreateTcpCapability(uint16_t port)
{
    NodeCapability capability(NodeCapabilityType::TcpServer);
    capability.SetPort(port);
    return capability;
}
}  // namespace

TEST(NetworkAddressWithTimeTest, DefaultConstructor)
{
    NetworkAddressWithTime address;
    EXPECT_EQ(0u, address.GetTimestamp());
    EXPECT_EQ("0.0.0.0", address.GetAddress());
    EXPECT_TRUE(address.GetCapabilities().empty());
    EXPECT_EQ(0u, address.GetPort());
}

TEST(NetworkAddressWithTimeTest, ParameterizedConstructor)
{
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(CreateTcpCapability(10333));
    NetworkAddressWithTime address(123, IPAddress("192.168.1.50"), capabilities);

    EXPECT_EQ(123u, address.GetTimestamp());
    EXPECT_EQ("192.168.1.50", address.GetAddress());
    ASSERT_EQ(1u, address.GetCapabilities().size());
    EXPECT_EQ(NodeCapabilityType::TcpServer, address.GetCapabilities()[0].GetType());
    EXPECT_EQ(10333u, address.GetPort());
}

TEST(NetworkAddressWithTimeTest, SetPortAddsCapability)
{
    NetworkAddressWithTime address;
    address.SetPort(20333);
    EXPECT_EQ(20333u, address.GetPort());
    ASSERT_EQ(1u, address.GetCapabilities().size());
    EXPECT_EQ(NodeCapabilityType::TcpServer, address.GetCapabilities()[0].GetType());
    EXPECT_EQ(20333u, address.GetCapabilities()[0].GetPort());
}

TEST(NetworkAddressWithTimeTest, SerializationRoundTrip)
{
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(CreateTcpCapability(10333));
    NetworkAddressWithTime original(987654u, IPAddress("10.0.0.5"), capabilities);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    original.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    NetworkAddressWithTime deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
    EXPECT_EQ(original.GetAddress(), deserialized.GetAddress());
    EXPECT_EQ(original.GetPort(), deserialized.GetPort());
    EXPECT_EQ(original.GetCapabilities().size(), deserialized.GetCapabilities().size());
}

TEST(NetworkAddressWithTimeTest, JsonRoundTrip)
{
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(CreateTcpCapability(20333));
    NetworkAddressWithTime original(42, IPAddress("2001:db8::1"), capabilities);

    JsonWriter writer;
    original.SerializeJson(writer);
    auto jsonValue = writer.GetJson();
    JsonReader reader(jsonValue);

    NetworkAddressWithTime deserialized;
    deserialized.DeserializeJson(reader);

    EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
    EXPECT_EQ(original.GetAddress(), deserialized.GetAddress());
    EXPECT_EQ(original.GetPort(), deserialized.GetPort());
}

TEST(NetworkAddressWithTimeTest, EqualityOperators)
{
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(CreateTcpCapability(10333));
    NetworkAddressWithTime address1(1, IPAddress("1.2.3.4"), capabilities);
    NetworkAddressWithTime address2(1, IPAddress("1.2.3.4"), capabilities);
    NetworkAddressWithTime address3(2, IPAddress("1.2.3.4"), capabilities);

    EXPECT_TRUE(address1 == address2);
    EXPECT_FALSE(address1 == address3);
    EXPECT_TRUE(address1 != address3);
}
