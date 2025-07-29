#include <array>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/network_address_with_time.h>
#include <string>

using namespace neo::network::p2p::payloads;
using namespace neo::io;

/**
 * @brief Test fixture for NetworkAddressWithTime
 */
class NetworkAddressWithTimeTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // No specific setup needed
    }
};

TEST_F(NetworkAddressWithTimeTest, DefaultConstructor)
{
    NetworkAddressWithTime address;

    EXPECT_EQ(0u, address.GetTimestamp());
    EXPECT_EQ(0u, address.GetServices());
    EXPECT_EQ(0u, address.GetPort());

    // Default address should be all zeros
    auto bytes = address.GetAddressBytes();
    for (auto byte : bytes)
    {
        EXPECT_EQ(0, byte);
    }
}

TEST_F(NetworkAddressWithTimeTest, ParameterizedConstructor)
{
    uint32_t timestamp = 1234567890;
    uint64_t services = 0x01;
    std::string ipAddress = "192.168.1.1";
    uint16_t port = 10333;

    NetworkAddressWithTime address(timestamp, services, ipAddress, port);

    EXPECT_EQ(timestamp, address.GetTimestamp());
    EXPECT_EQ(services, address.GetServices());
    EXPECT_EQ(ipAddress, address.GetAddress());
    EXPECT_EQ(port, address.GetPort());
}

TEST_F(NetworkAddressWithTimeTest, IPv4Address)
{
    NetworkAddressWithTime address;
    std::string ipv4 = "192.168.1.100";

    address.SetAddress(ipv4);
    EXPECT_EQ(ipv4, address.GetAddress());
    EXPECT_TRUE(address.IsIPv4());
    EXPECT_FALSE(address.IsIPv6());

    // Check IPv4-mapped IPv6 format
    auto bytes = address.GetAddressBytes();
    // First 10 bytes should be 0
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(0, bytes[i]);
    }
    // Next 2 bytes should be 0xFF
    EXPECT_EQ(0xFF, bytes[10]);
    EXPECT_EQ(0xFF, bytes[11]);
    // Last 4 bytes should be the IPv4 address
    EXPECT_EQ(192, bytes[12]);
    EXPECT_EQ(168, bytes[13]);
    EXPECT_EQ(1, bytes[14]);
    EXPECT_EQ(100, bytes[15]);
}

TEST_F(NetworkAddressWithTimeTest, IPv6Address)
{
    NetworkAddressWithTime address;
    std::string ipv6 = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";

    address.SetAddress(ipv6);
    EXPECT_EQ(ipv6, address.GetAddress());
    EXPECT_FALSE(address.IsIPv4());
    EXPECT_TRUE(address.IsIPv6());
}

TEST_F(NetworkAddressWithTimeTest, GetEndpoint)
{
    NetworkAddressWithTime address(0, 0, "192.168.1.1", 10333);
    EXPECT_EQ("192.168.1.1:10333", address.GetEndpoint());

    NetworkAddressWithTime addressv6(0, 0, "::1", 20333);
    EXPECT_EQ("[::1]:20333", address.GetEndpoint());
}

TEST_F(NetworkAddressWithTimeTest, GettersAndSetters)
{
    NetworkAddressWithTime address;

    // Test timestamp
    uint32_t timestamp = 9876543210;
    address.SetTimestamp(timestamp);
    EXPECT_EQ(timestamp, address.GetTimestamp());

    // Test services
    uint64_t services = 0xFF;
    address.SetServices(services);
    EXPECT_EQ(services, address.GetServices());

    // Test port
    uint16_t port = 30333;
    address.SetPort(port);
    EXPECT_EQ(port, address.GetPort());

    // Test address bytes
    std::array<uint8_t, 16> addressBytes;
    for (int i = 0; i < 16; ++i)
    {
        addressBytes[i] = static_cast<uint8_t>(i);
    }
    address.SetAddressBytes(addressBytes);
    EXPECT_EQ(addressBytes, address.GetAddressBytes());
}

TEST_F(NetworkAddressWithTimeTest, GetSize)
{
    NetworkAddressWithTime address;
    EXPECT_EQ(NetworkAddressWithTime::Size, address.GetSize());
    EXPECT_EQ(30, address.GetSize());  // 4 + 8 + 16 + 2
}

TEST_F(NetworkAddressWithTimeTest, Equality)
{
    NetworkAddressWithTime address1(100, 1, "192.168.1.1", 10333);
    NetworkAddressWithTime address2(100, 1, "192.168.1.1", 10333);
    NetworkAddressWithTime address3(200, 1, "192.168.1.1", 10333);
    NetworkAddressWithTime address4(100, 2, "192.168.1.1", 10333);
    NetworkAddressWithTime address5(100, 1, "192.168.1.2", 10333);
    NetworkAddressWithTime address6(100, 1, "192.168.1.1", 20333);

    EXPECT_EQ(address1, address2);
    EXPECT_NE(address1, address3);
    EXPECT_NE(address1, address4);
    EXPECT_NE(address1, address5);
    EXPECT_NE(address1, address6);
}

TEST_F(NetworkAddressWithTimeTest, Serialization)
{
    NetworkAddressWithTime original(1234567890, 0x01, "192.168.1.100", 10333);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    NetworkAddressWithTime deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
    EXPECT_EQ(original.GetServices(), deserialized.GetServices());
    EXPECT_EQ(original.GetAddress(), deserialized.GetAddress());
    EXPECT_EQ(original.GetPort(), deserialized.GetPort());
    EXPECT_EQ(original, deserialized);
}

TEST_F(NetworkAddressWithTimeTest, JsonSerialization)
{
    NetworkAddressWithTime original(1234567890, 0x01, "192.168.1.100", 10333);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    NetworkAddressWithTime deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetTimestamp(), deserialized.GetTimestamp());
    EXPECT_EQ(original.GetServices(), deserialized.GetServices());
    EXPECT_EQ(original.GetAddress(), deserialized.GetAddress());
    EXPECT_EQ(original.GetPort(), deserialized.GetPort());
}

TEST_F(NetworkAddressWithTimeTest, FromIPv4)
{
    uint32_t timestamp = 1234567890;
    uint64_t services = 0x01;
    std::string ipv4 = "10.0.0.1";
    uint16_t port = 10333;

    auto address = NetworkAddressWithTime::FromIPv4(timestamp, services, ipv4, port);

    EXPECT_EQ(timestamp, address.GetTimestamp());
    EXPECT_EQ(services, address.GetServices());
    EXPECT_EQ(ipv4, address.GetAddress());
    EXPECT_EQ(port, address.GetPort());
    EXPECT_TRUE(address.IsIPv4());
    EXPECT_FALSE(address.IsIPv6());
}

TEST_F(NetworkAddressWithTimeTest, FromIPv6)
{
    uint32_t timestamp = 1234567890;
    uint64_t services = 0x01;
    std::string ipv6 = "2001:db8::1";
    uint16_t port = 10333;

    auto address = NetworkAddressWithTime::FromIPv6(timestamp, services, ipv6, port);

    EXPECT_EQ(timestamp, address.GetTimestamp());
    EXPECT_EQ(services, address.GetServices());
    EXPECT_EQ(ipv6, address.GetAddress());
    EXPECT_EQ(port, address.GetPort());
    EXPECT_FALSE(address.IsIPv4());
    EXPECT_TRUE(address.IsIPv6());
}

TEST_F(NetworkAddressWithTimeTest, SpecialAddresses)
{
    // Test localhost IPv4
    NetworkAddressWithTime localhost4(0, 0, "127.0.0.1", 10333);
    EXPECT_EQ("127.0.0.1", localhost4.GetAddress());
    EXPECT_TRUE(localhost4.IsIPv4());

    // Test localhost IPv6
    NetworkAddressWithTime localhost6(0, 0, "::1", 10333);
    EXPECT_EQ("::1", localhost6.GetAddress());
    EXPECT_TRUE(localhost6.IsIPv6());

    // Test any address IPv4
    NetworkAddressWithTime any4(0, 0, "0.0.0.0", 10333);
    EXPECT_EQ("0.0.0.0", any4.GetAddress());
    EXPECT_TRUE(any4.IsIPv4());

    // Test any address IPv6
    NetworkAddressWithTime any6(0, 0, "::", 10333);
    EXPECT_EQ("::", any6.GetAddress());
    EXPECT_TRUE(any6.IsIPv6());
}

TEST_F(NetworkAddressWithTimeTest, SerializedSize)
{
    NetworkAddressWithTime address(1234567890, 0x01, "192.168.1.1", 10333);

    MemoryStream stream;
    BinaryWriter writer(stream);
    address.Serialize(writer);

    EXPECT_EQ(static_cast<size_t>(NetworkAddressWithTime::Size), stream.Length());
}
