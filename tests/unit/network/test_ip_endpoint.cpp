#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/ip_endpoint.h>
#include <sstream>

using namespace neo::network;
using namespace neo::io;

TEST(IPAddressTest, Constructor)
{
    // Default constructor
    IPAddress address1;
    EXPECT_EQ(address1.GetAddressLength(), 0);

    // String constructor
    IPAddress address2("127.0.0.1");
    EXPECT_EQ(address2.GetAddressLength(), 4);
    EXPECT_EQ(address2.GetAddressBytes()[0], 127);
    EXPECT_EQ(address2.GetAddressBytes()[1], 0);
    EXPECT_EQ(address2.GetAddressBytes()[2], 0);
    EXPECT_EQ(address2.GetAddressBytes()[3], 1);

    // Integer constructor
    IPAddress address3(0x7F000001);
    EXPECT_EQ(address3.GetAddressLength(), 4);
    EXPECT_EQ(address3.GetAddressBytes()[0], 127);
    EXPECT_EQ(address3.GetAddressBytes()[1], 0);
    EXPECT_EQ(address3.GetAddressBytes()[2], 0);
    EXPECT_EQ(address3.GetAddressBytes()[3], 1);

    // Byte array constructor
    uint8_t bytes[] = {127, 0, 0, 1};
    IPAddress address4(bytes, 4);
    EXPECT_EQ(address4.GetAddressLength(), 4);
    EXPECT_EQ(address4.GetAddressBytes()[0], 127);
    EXPECT_EQ(address4.GetAddressBytes()[1], 0);
    EXPECT_EQ(address4.GetAddressBytes()[2], 0);
    EXPECT_EQ(address4.GetAddressBytes()[3], 1);
}

TEST(IPAddressTest, ToString)
{
    // IPv4
    IPAddress address1("127.0.0.1");
    EXPECT_EQ(address1.ToString(), "127.0.0.1");

    // IPv6
    IPAddress address2("::1");
    EXPECT_EQ(address2.GetAddressLength(), 16);
    EXPECT_EQ(address2.ToString(), "::1");
}

TEST(IPAddressTest, Equality)
{
    IPAddress address1("127.0.0.1");
    IPAddress address2("127.0.0.1");
    IPAddress address3("192.168.0.1");

    EXPECT_TRUE(address1 == address2);
    EXPECT_FALSE(address1 == address3);

    EXPECT_FALSE(address1 != address2);
    EXPECT_TRUE(address1 != address3);
}

TEST(IPAddressTest, Loopback)
{
    IPAddress address = IPAddress::Loopback();
    EXPECT_EQ(address.GetAddressLength(), 4);
    EXPECT_EQ(address.GetAddressBytes()[0], 127);
    EXPECT_EQ(address.GetAddressBytes()[1], 0);
    EXPECT_EQ(address.GetAddressBytes()[2], 0);
    EXPECT_EQ(address.GetAddressBytes()[3], 1);
    EXPECT_EQ(address.ToString(), "127.0.0.1");
}

TEST(IPAddressTest, Any)
{
    IPAddress address = IPAddress::Any();
    EXPECT_EQ(address.GetAddressLength(), 4);
    EXPECT_EQ(address.GetAddressBytes()[0], 0);
    EXPECT_EQ(address.GetAddressBytes()[1], 0);
    EXPECT_EQ(address.GetAddressBytes()[2], 0);
    EXPECT_EQ(address.GetAddressBytes()[3], 0);
    EXPECT_EQ(address.ToString(), "0.0.0.0");
}

TEST(IPAddressTest, Parse)
{
    // IPv4
    IPAddress address1 = IPAddress::Parse("127.0.0.1");
    EXPECT_EQ(address1.GetAddressLength(), 4);
    EXPECT_EQ(address1.GetAddressBytes()[0], 127);
    EXPECT_EQ(address1.GetAddressBytes()[1], 0);
    EXPECT_EQ(address1.GetAddressBytes()[2], 0);
    EXPECT_EQ(address1.GetAddressBytes()[3], 1);

    // IPv6
    IPAddress address2 = IPAddress::Parse("::1");
    EXPECT_EQ(address2.GetAddressLength(), 16);
    EXPECT_EQ(address2.GetAddressBytes()[15], 1);

    // Invalid
    EXPECT_THROW(IPAddress::Parse("invalid"), std::invalid_argument);
}

TEST(IPAddressTest, TryParse)
{
    IPAddress address;

    // IPv4
    EXPECT_TRUE(IPAddress::TryParse("127.0.0.1", address));
    EXPECT_EQ(address.GetAddressLength(), 4);
    EXPECT_EQ(address.GetAddressBytes()[0], 127);
    EXPECT_EQ(address.GetAddressBytes()[1], 0);
    EXPECT_EQ(address.GetAddressBytes()[2], 0);
    EXPECT_EQ(address.GetAddressBytes()[3], 1);

    // IPv6
    EXPECT_TRUE(IPAddress::TryParse("::1", address));
    EXPECT_EQ(address.GetAddressLength(), 16);
    EXPECT_EQ(address.GetAddressBytes()[15], 1);

    // Invalid
    EXPECT_FALSE(IPAddress::TryParse("invalid", address));
}

TEST(IPEndPointTest, Constructor)
{
    // Default constructor
    IPEndPoint endpoint1;
    EXPECT_EQ(endpoint1.GetAddress().GetAddressLength(), 0);
    EXPECT_EQ(endpoint1.GetPort(), 0);

    // Address and port constructor
    IPAddress address("127.0.0.1");
    IPEndPoint endpoint2(address, 8080);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressLength(), 4);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[0], 127);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[1], 0);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[2], 0);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[3], 1);
    EXPECT_EQ(endpoint2.GetPort(), 8080);
}

TEST(IPEndPointTest, ToString)
{
    // IPv4
    IPAddress address1("127.0.0.1");
    IPEndPoint endpoint1(address1, 8080);
    EXPECT_EQ(endpoint1.ToString(), "127.0.0.1:8080");

    // IPv6
    IPAddress address2("::1");
    IPEndPoint endpoint2(address2, 8080);
    EXPECT_EQ(endpoint2.ToString(), "[::1]:8080");
}

TEST(IPEndPointTest, Serialization)
{
    // Create an endpoint
    IPAddress address("127.0.0.1");
    IPEndPoint endpoint(address, 8080);

    // Serialize
    std::stringstream stream;
    BinaryWriter writer(stream);
    endpoint.Serialize(writer);

    // Deserialize
    stream.seekg(0);
    BinaryReader reader(stream);
    IPEndPoint endpoint2;
    endpoint2.Deserialize(reader);

    // Check
    EXPECT_EQ(endpoint2.GetAddress().GetAddressLength(), 4);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[0], 127);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[1], 0);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[2], 0);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[3], 1);
    EXPECT_EQ(endpoint2.GetPort(), 8080);
}

TEST(IPEndPointTest, Equality)
{
    IPAddress address1("127.0.0.1");
    IPEndPoint endpoint1(address1, 8080);

    IPAddress address2("127.0.0.1");
    IPEndPoint endpoint2(address2, 8080);

    IPAddress address3("192.168.0.1");
    IPEndPoint endpoint3(address3, 8080);

    IPAddress address4("127.0.0.1");
    IPEndPoint endpoint4(address4, 9090);

    EXPECT_TRUE(endpoint1 == endpoint2);
    EXPECT_FALSE(endpoint1 == endpoint3);
    EXPECT_FALSE(endpoint1 == endpoint4);

    EXPECT_FALSE(endpoint1 != endpoint2);
    EXPECT_TRUE(endpoint1 != endpoint3);
    EXPECT_TRUE(endpoint1 != endpoint4);
}

TEST(IPEndPointTest, Parse)
{
    // IPv4
    IPEndPoint endpoint1 = IPEndPoint::Parse("127.0.0.1:8080");
    EXPECT_EQ(endpoint1.GetAddress().GetAddressLength(), 4);
    EXPECT_EQ(endpoint1.GetAddress().GetAddressBytes()[0], 127);
    EXPECT_EQ(endpoint1.GetAddress().GetAddressBytes()[1], 0);
    EXPECT_EQ(endpoint1.GetAddress().GetAddressBytes()[2], 0);
    EXPECT_EQ(endpoint1.GetAddress().GetAddressBytes()[3], 1);
    EXPECT_EQ(endpoint1.GetPort(), 8080);

    // IPv6
    IPEndPoint endpoint2 = IPEndPoint::Parse("[::1]:8080");
    EXPECT_EQ(endpoint2.GetAddress().GetAddressLength(), 16);
    EXPECT_EQ(endpoint2.GetAddress().GetAddressBytes()[15], 1);
    EXPECT_EQ(endpoint2.GetPort(), 8080);

    // Invalid
    EXPECT_THROW(IPEndPoint::Parse("invalid"), std::invalid_argument);
    EXPECT_THROW(IPEndPoint::Parse("127.0.0.1"), std::invalid_argument);
    EXPECT_THROW(IPEndPoint::Parse("127.0.0.1:invalid"), std::invalid_argument);
    EXPECT_THROW(IPEndPoint::Parse("[::1]"), std::invalid_argument);
    EXPECT_THROW(IPEndPoint::Parse("[::1]:invalid"), std::invalid_argument);
}

TEST(IPEndPointTest, TryParse)
{
    IPEndPoint endpoint;

    // IPv4
    EXPECT_TRUE(IPEndPoint::TryParse("127.0.0.1:8080", endpoint));
    EXPECT_EQ(endpoint.GetAddress().GetAddressLength(), 4);
    EXPECT_EQ(endpoint.GetAddress().GetAddressBytes()[0], 127);
    EXPECT_EQ(endpoint.GetAddress().GetAddressBytes()[1], 0);
    EXPECT_EQ(endpoint.GetAddress().GetAddressBytes()[2], 0);
    EXPECT_EQ(endpoint.GetAddress().GetAddressBytes()[3], 1);
    EXPECT_EQ(endpoint.GetPort(), 8080);

    // IPv6
    EXPECT_TRUE(IPEndPoint::TryParse("[::1]:8080", endpoint));
    EXPECT_EQ(endpoint.GetAddress().GetAddressLength(), 16);
    EXPECT_EQ(endpoint.GetAddress().GetAddressBytes()[15], 1);
    EXPECT_EQ(endpoint.GetPort(), 8080);

    // Invalid
    EXPECT_FALSE(IPEndPoint::TryParse("invalid", endpoint));
    EXPECT_FALSE(IPEndPoint::TryParse("127.0.0.1", endpoint));
    EXPECT_FALSE(IPEndPoint::TryParse("127.0.0.1:invalid", endpoint));
    EXPECT_FALSE(IPEndPoint::TryParse("[::1]", endpoint));
    EXPECT_FALSE(IPEndPoint::TryParse("[::1]:invalid", endpoint));
}
