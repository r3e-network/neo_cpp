#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/inventory_vector.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <string>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::network::p2p;
using namespace neo::io;

/**
 * @brief Test fixture for InvPayload
 */
class InvPayloadTest : public testing::Test
{
  protected:
    std::vector<UInt256> testHashes;
    InventoryType testType;

    void SetUp() override
    {
        // Initialize test data
        testType = InventoryType::Transaction;

        // Create test hashes
        testHashes.push_back(
            UInt256::FromHexString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
        testHashes.push_back(
            UInt256::FromHexString("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321"));
        testHashes.push_back(
            UInt256::FromHexString("0xaaaabbbbccccddddaaaabbbbccccddddaaaabbbbccccddddaaaabbbbccccdddd"));
    }
};

TEST_F(InvPayloadTest, DefaultConstructor)
{
    InvPayload payload;

    EXPECT_EQ(InventoryType::Transaction, payload.GetType());
    EXPECT_TRUE(payload.GetHashes().empty());
    EXPECT_TRUE(payload.GetInventories().empty());
}

TEST_F(InvPayloadTest, ParameterizedConstructor_TypeAndHashes)
{
    InvPayload payload(testType, testHashes);

    EXPECT_EQ(testType, payload.GetType());
    EXPECT_EQ(testHashes, payload.GetHashes());
    EXPECT_EQ(testHashes.size(), payload.GetInventories().size());
}

TEST_F(InvPayloadTest, ParameterizedConstructor_Inventories)
{
    std::vector<InventoryVector> inventories;
    for (const auto& hash : testHashes)
    {
        inventories.emplace_back(testType, hash);
    }

    InvPayload payload(inventories);

    EXPECT_EQ(testType, payload.GetType());
    EXPECT_EQ(testHashes, payload.GetHashes());
    EXPECT_EQ(inventories.size(), payload.GetInventories().size());
}

TEST_F(InvPayloadTest, GettersAndSetters)
{
    InvPayload payload;

    // Test Type
    payload.SetType(InventoryType::Block);
    EXPECT_EQ(InventoryType::Block, payload.GetType());

    // Test Hashes
    payload.SetHashes(testHashes);
    EXPECT_EQ(testHashes, payload.GetHashes());

    // Test updating type
    payload.SetType(InventoryType::Consensus);
    EXPECT_EQ(InventoryType::Consensus, payload.GetType());
}

TEST_F(InvPayloadTest, MaxHashesCount)
{
    // Verify the constant is set correctly
    EXPECT_EQ(500, InvPayload::MaxHashesCount);
}

TEST_F(InvPayloadTest, GetSize)
{
    InvPayload payload(testType, testHashes);

    // Size should be: 1 byte (type) + 1 byte (count) + (32 bytes * hash count)
    uint32_t expectedSize = 1 + 1 + (32 * testHashes.size());
    EXPECT_EQ(expectedSize, payload.GetSize());
}

TEST_F(InvPayloadTest, Serialization)
{
    InvPayload original(testType, testHashes);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    InvPayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetType(), deserialized.GetType());
    EXPECT_EQ(original.GetHashes(), deserialized.GetHashes());
}

TEST_F(InvPayloadTest, JsonSerialization)
{
    InvPayload original(testType, testHashes);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    InvPayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetType(), deserialized.GetType());
    EXPECT_EQ(original.GetHashes(), deserialized.GetHashes());
}

TEST_F(InvPayloadTest, CreateStaticMethod)
{
    InvPayload payload = InvPayload::Create(testType, testHashes);

    EXPECT_EQ(testType, payload.GetType());
    EXPECT_EQ(testHashes, payload.GetHashes());
}

TEST_F(InvPayloadTest, CreateGroup_SmallList)
{
    // Test with list smaller than MaxHashesCount
    std::vector<InvPayload> payloads = InvPayload::CreateGroup(testType, testHashes);

    EXPECT_EQ(1u, payloads.size());
    EXPECT_EQ(testType, payloads[0].GetType());
    EXPECT_EQ(testHashes, payloads[0].GetHashes());
}

TEST_F(InvPayloadTest, CreateGroup_LargeList)
{
    // Create a list larger than MaxHashesCount
    std::vector<UInt256> manyHashes;
    for (int i = 0; i < 1200; ++i)
    {  // 1200 > 500 * 2
        std::string hashStr = "0x";
        for (int j = 0; j < 64; ++j)
        {
            hashStr += std::to_string(i % 10);
        }
        manyHashes.push_back(UInt256::FromHexString(hashStr));
    }

    std::vector<InvPayload> payloads = InvPayload::CreateGroup(testType, manyHashes);

    // Should be split into 3 groups (500 + 500 + 200)
    EXPECT_EQ(3u, payloads.size());
    EXPECT_EQ(500u, payloads[0].GetHashes().size());
    EXPECT_EQ(500u, payloads[1].GetHashes().size());
    EXPECT_EQ(200u, payloads[2].GetHashes().size());

    // All should have the same type
    for (const auto& payload : payloads)
    {
        EXPECT_EQ(testType, payload.GetType());
    }
}

TEST_F(InvPayloadTest, EmptyHashes)
{
    std::vector<UInt256> emptyHashes;
    InvPayload payload(testType, emptyHashes);

    EXPECT_EQ(testType, payload.GetType());
    EXPECT_TRUE(payload.GetHashes().empty());
    EXPECT_TRUE(payload.GetInventories().empty());
}

TEST_F(InvPayloadTest, DifferentInventoryTypes)
{
    // Test all inventory types
    std::vector<InventoryType> types = {InventoryType::Transaction, InventoryType::Block, InventoryType::Consensus,
                                        InventoryType::Extensible};

    for (const auto& type : types)
    {
        InvPayload payload(type, testHashes);
        EXPECT_EQ(type, payload.GetType());
        EXPECT_EQ(testHashes, payload.GetHashes());
    }
}

TEST_F(InvPayloadTest, InventoryTypeValues)
{
    // Verify enum values
    EXPECT_EQ(0x01, static_cast<uint8_t>(InventoryType::Transaction));
    EXPECT_EQ(0x02, static_cast<uint8_t>(InventoryType::Block));
    EXPECT_EQ(0xe0, static_cast<uint8_t>(InventoryType::Consensus));
    EXPECT_EQ(0xf0, static_cast<uint8_t>(InventoryType::Extensible));
}

TEST_F(InvPayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips
    InvPayload original(testType, testHashes);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        InvPayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testType, deserialized.GetType());
        EXPECT_EQ(testHashes, deserialized.GetHashes());
    }
}

TEST_F(InvPayloadTest, MaximumHashes)
{
    // Test with exactly MaxHashesCount hashes
    std::vector<UInt256> maxHashes;
    for (int i = 0; i < InvPayload::MaxHashesCount; ++i)
    {
        std::string hashStr = "0x";
        for (int j = 0; j < 64; ++j)
        {
            hashStr += std::to_string((i + j) % 10);
        }
        maxHashes.push_back(UInt256::FromHexString(hashStr));
    }

    InvPayload payload(testType, maxHashes);
    EXPECT_EQ(InvPayload::MaxHashesCount, static_cast<int>(payload.GetHashes().size()));

    // CreateGroup should still create only one payload
    std::vector<InvPayload> payloads = InvPayload::CreateGroup(testType, maxHashes);
    EXPECT_EQ(1u, payloads.size());
    EXPECT_EQ(InvPayload::MaxHashesCount, static_cast<int>(payloads[0].GetHashes().size()));
}

TEST_F(InvPayloadTest, UpdateAfterConstruction)
{
    InvPayload payload(InventoryType::Transaction, testHashes);

    // Update type
    payload.SetType(InventoryType::Block);
    EXPECT_EQ(InventoryType::Block, payload.GetType());

    // Update hashes
    std::vector<UInt256> newHashes;
    newHashes.push_back(UInt256::Random());
    newHashes.push_back(UInt256::Random());

    payload.SetHashes(newHashes);
    EXPECT_EQ(newHashes, payload.GetHashes());
    EXPECT_EQ(2u, payload.GetHashes().size());
}
