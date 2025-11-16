#include <chrono>
#include <sstream>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/ip_address.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/payloads/addr_payload.h>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;
using neo::network::IPAddress;

namespace
{
NetworkAddressWithTime MakeAddress(const std::string& ip, uint16_t port, uint32_t timestamp)
{
    NodeCapability tcp(NodeCapabilityType::TcpServer);
    tcp.SetPort(port);
    std::vector<NodeCapability> caps{tcp};
    return NetworkAddressWithTime(timestamp, IPAddress(ip), caps);
}
}  // namespace

class AddrPayloadTest : public testing::Test
{
  protected:
    std::vector<NetworkAddressWithTime> addresses_;

    void SetUp() override
    {
        auto now = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());
        addresses_.push_back(MakeAddress("192.168.1.1", 10333, now));
        addresses_.push_back(MakeAddress("10.0.0.1", 10333, now - 60));
    }
};

TEST_F(AddrPayloadTest, DefaultConstructor)
{
    AddrPayload payload;
    EXPECT_TRUE(payload.GetAddressList().empty());
    EXPECT_EQ(1u, payload.GetSize());
}

TEST_F(AddrPayloadTest, ParameterizedConstructor)
{
    AddrPayload payload(addresses_);
    EXPECT_EQ(addresses_, payload.GetAddressList());
}

TEST_F(AddrPayloadTest, SerializationRoundTrip)
{
    AddrPayload original(addresses_);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);
    original.Serialize(writer);

    stream.seekg(0);
    BinaryReader reader(stream);
    AddrPayload deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(original.GetAddressList().size(), deserialized.GetAddressList().size());
    EXPECT_EQ(original.GetAddressList()[0].GetAddress(), deserialized.GetAddressList()[0].GetAddress());
    EXPECT_EQ(original.GetAddressList()[0].GetPort(), deserialized.GetAddressList()[0].GetPort());
}

TEST_F(AddrPayloadTest, JsonRoundTrip)
{
    AddrPayload payload(addresses_);
    JsonWriter writer;
    payload.SerializeJson(writer);

    auto jsonValue = writer.GetJson();
    JsonReader reader(jsonValue);
    AddrPayload restored;
    restored.DeserializeJson(reader);
    EXPECT_EQ(payload.GetAddressList().size(), restored.GetAddressList().size());
}

TEST_F(AddrPayloadTest, GetSizeTracksEntries)
{
    AddrPayload payload(addresses_);
    size_t expected = (addresses_.size() < 0xFD) ? 1 : 3;
    for (const auto& addr : addresses_)
    {
        expected += addr.GetSize();
    }
    EXPECT_EQ(expected, payload.GetSize());
}

TEST_F(AddrPayloadTest, SetAddressList)
{
    AddrPayload payload;
    payload.SetAddressList(addresses_);
    EXPECT_EQ(addresses_, payload.GetAddressList());
}

TEST_F(AddrPayloadTest, MaximumAddresses)
{
    std::vector<NetworkAddressWithTime> many;
    for (int i = 0; i < AddrPayload::MaxCountToSend; ++i)
    {
        std::string ip = "10.0." + std::to_string(i / 256) + "." + std::to_string(i % 256);
        many.push_back(MakeAddress(ip, 10333, static_cast<uint32_t>(i)));
    }
    AddrPayload payload(many);
    EXPECT_EQ(AddrPayload::MaxCountToSend, static_cast<int>(payload.GetAddressList().size()));
}
