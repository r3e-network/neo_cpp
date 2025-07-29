#include <chrono>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/network_address.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::network::p2p;
using namespace neo::io;

/**
 * @brief Test fixture for AddrPayload
 */
class AddrPayloadTest : public testing::Test
{
  protected:
    std::vector<NetworkAddressWithTime> testAddresses;

    void SetUp() override
    {
        // Create test addresses
        uint32_t currentTime = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count());

        // Add some test addresses
        testAddresses.push_back(NetworkAddressWithTime(currentTime,
                                                       0x01,  // services
                                                       "192.168.1.1", 10333));

        testAddresses.push_back(NetworkAddressWithTime(currentTime - 3600,  // 1 hour ago
                                                       0x01, "10.0.0.1", 10333));

        testAddresses.push_back(NetworkAddressWithTime(currentTime - 7200,  // 2 hours ago
                                                       0x01, "172.16.0.1", 10333));
    }
};

TEST_F(AddrPayloadTest, DefaultConstructor)
{
    AddrPayload payload;

    EXPECT_TRUE(payload.GetAddressList().empty());
    EXPECT_EQ(0, payload.GetSize());
}

TEST_F(AddrPayloadTest, ParameterizedConstructor)
{
    AddrPayload payload(testAddresses);

    EXPECT_EQ(testAddresses.size(), payload.GetAddressList().size());
    EXPECT_EQ(testAddresses, payload.GetAddressList());
}

TEST_F(AddrPayloadTest, GettersAndSetters)
{
    AddrPayload payload;

    // Initially empty
    EXPECT_TRUE(payload.GetAddressList().empty());

    // Set addresses
    payload.SetAddressList(testAddresses);
    EXPECT_EQ(testAddresses.size(), payload.GetAddressList().size());
    EXPECT_EQ(testAddresses, payload.GetAddressList());

    // Update with new addresses
    std::vector<NetworkAddressWithTime> newAddresses;
    newAddresses.push_back(NetworkAddressWithTime(0, 0x01, "1.2.3.4", 10333));
    payload.SetAddressList(newAddresses);
    EXPECT_EQ(1u, payload.GetAddressList().size());
    EXPECT_EQ(newAddresses, payload.GetAddressList());
}

TEST_F(AddrPayloadTest, MaxCountToSend)
{
    // Verify the constant is set correctly
    EXPECT_EQ(200, AddrPayload::MaxCountToSend);
}

TEST_F(AddrPayloadTest, Serialization)
{
    AddrPayload original(testAddresses);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    AddrPayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetAddressList().size(), deserialized.GetAddressList().size());
    for (size_t i = 0; i < original.GetAddressList().size(); ++i)
    {
        const auto& orig = original.GetAddressList()[i];
        const auto& deser = deserialized.GetAddressList()[i];

        EXPECT_EQ(orig.GetTimestamp(), deser.GetTimestamp());
        EXPECT_EQ(orig.GetServices(), deser.GetServices());
        EXPECT_EQ(orig.GetAddress(), deser.GetAddress());
        EXPECT_EQ(orig.GetPort(), deser.GetPort());
    }
}

TEST_F(AddrPayloadTest, JsonSerialization)
{
    AddrPayload original(testAddresses);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    AddrPayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare sizes
    EXPECT_EQ(original.GetAddressList().size(), deserialized.GetAddressList().size());
}

TEST_F(AddrPayloadTest, EmptyAddressList)
{
    AddrPayload payload;

    // Serialize empty payload
    MemoryStream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    AddrPayload deserialized;
    deserialized.Deserialize(reader);

    EXPECT_TRUE(deserialized.GetAddressList().empty());
}

TEST_F(AddrPayloadTest, SingleAddress)
{
    std::vector<NetworkAddressWithTime> singleAddress;
    singleAddress.push_back(NetworkAddressWithTime(1234567890, 0x01, "127.0.0.1", 10333));

    AddrPayload payload(singleAddress);
    EXPECT_EQ(1u, payload.GetAddressList().size());
    EXPECT_EQ("127.0.0.1", payload.GetAddressList()[0].GetAddress());
}

TEST_F(AddrPayloadTest, MaximumAddresses)
{
    // Create maximum allowed addresses
    std::vector<NetworkAddressWithTime> maxAddresses;
    for (int i = 0; i < AddrPayload::MaxCountToSend; ++i)
    {
        std::string ip = "192.168." + std::to_string(i / 256) + "." + std::to_string(i % 256);
        maxAddresses.push_back(NetworkAddressWithTime(1234567890 + i, 0x01, ip, 10333));
    }

    AddrPayload payload(maxAddresses);
    EXPECT_EQ(AddrPayload::MaxCountToSend, static_cast<int>(payload.GetAddressList().size()));
}

TEST_F(AddrPayloadTest, GetSize)
{
    AddrPayload emptyPayload;
    EXPECT_EQ(0, emptyPayload.GetSize());

    AddrPayload payload(testAddresses);
    // Size should be: count (varint) + (address_size * count)
    // Each NetworkAddressWithTime is 30 bytes
    int expectedSize = 1 + (30 * static_cast<int>(testAddresses.size()));  // 1 byte for count < 253
    EXPECT_EQ(expectedSize, payload.GetSize());
}

TEST_F(AddrPayloadTest, DifferentIPVersions)
{
    std::vector<NetworkAddressWithTime> mixedAddresses;

    // IPv4 address
    mixedAddresses.push_back(NetworkAddressWithTime(1234567890, 0x01, "192.168.1.1", 10333));

    // IPv6 address
    mixedAddresses.push_back(
        NetworkAddressWithTime(1234567890, 0x01, "2001:0db8:85a3:0000:0000:8a2e:0370:7334", 10333));

    // Loopback addresses
    mixedAddresses.push_back(NetworkAddressWithTime(1234567890, 0x01, "127.0.0.1", 10333));

    mixedAddresses.push_back(NetworkAddressWithTime(1234567890, 0x01, "::1", 10333));

    AddrPayload payload(mixedAddresses);
    EXPECT_EQ(4u, payload.GetAddressList().size());
}

TEST_F(AddrPayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips
    AddrPayload original(testAddresses);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        AddrPayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testAddresses.size(), deserialized.GetAddressList().size());
    }
}

TEST_F(AddrPayloadTest, DifferentTimestamps)
{
    std::vector<NetworkAddressWithTime> timedAddresses;
    uint32_t baseTime = 1700000000;

    // Add addresses with different timestamps
    for (int i = 0; i < 10; ++i)
    {
        timedAddresses.push_back(NetworkAddressWithTime(baseTime + (i * 3600),  // Each address 1 hour apart
                                                        0x01, "10.0.0." + std::to_string(i), 10333));
    }

    AddrPayload payload(timedAddresses);
    EXPECT_EQ(10u, payload.GetAddressList().size());

    // Verify timestamps are preserved
    for (size_t i = 0; i < payload.GetAddressList().size(); ++i)
    {
        EXPECT_EQ(baseTime + (i * 3600), payload.GetAddressList()[i].GetTimestamp());
    }
}

TEST_F(AddrPayloadTest, DifferentServices)
{
    std::vector<NetworkAddressWithTime> serviceAddresses;

    // Different service flags
    serviceAddresses.push_back(NetworkAddressWithTime(0, 0x00, "1.1.1.1", 10333));  // No services
    serviceAddresses.push_back(NetworkAddressWithTime(0, 0x01, "2.2.2.2", 10333));  // Network service
    serviceAddresses.push_back(NetworkAddressWithTime(0, 0xFF, "3.3.3.3", 10333));  // All services

    AddrPayload payload(serviceAddresses);
    EXPECT_EQ(3u, payload.GetAddressList().size());
    EXPECT_EQ(0x00u, payload.GetAddressList()[0].GetServices());
    EXPECT_EQ(0x01u, payload.GetAddressList()[1].GetServices());
    EXPECT_EQ(0xFFu, payload.GetAddressList()[2].GetServices());
}

TEST_F(AddrPayloadTest, UpdateAddressList)
{
    AddrPayload payload(testAddresses);

    // Verify initial state
    EXPECT_EQ(testAddresses.size(), payload.GetAddressList().size());

    // Clear addresses
    payload.SetAddressList({});
    EXPECT_TRUE(payload.GetAddressList().empty());

    // Add new addresses
    std::vector<NetworkAddressWithTime> newList;
    for (int i = 0; i < 5; ++i)
    {
        newList.push_back(NetworkAddressWithTime(0, 0x01, "192.168.10." + std::to_string(i), 10333));
    }

    payload.SetAddressList(newList);
    EXPECT_EQ(5u, payload.GetAddressList().size());
}
