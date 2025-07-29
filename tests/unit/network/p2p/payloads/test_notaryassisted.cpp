#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/ledger/transaction_attribute.h>
#include <string>
#include <vector>

using namespace neo::ledger;
using namespace neo::io;

/**
 * @brief Test fixture for NotaryAssisted transaction attribute
 */
class NotaryAssistedTest : public testing::Test
{
  protected:
    uint8_t testNKeys;

    void SetUp() override
    {
        // Initialize test data
        testNKeys = 5;  // Number of keys for notary
    }
};

TEST_F(NotaryAssistedTest, DefaultConstructor)
{
    TransactionAttribute attr;

    // When creating as NotaryAssisted type
    attr.SetUsage(TransactionAttribute::Usage::NotaryAssisted);
    ByteVector data({0x00});  // NKeys = 0
    attr.SetData(data);

    EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
    EXPECT_EQ(1u, attr.GetData().Size());
    EXPECT_EQ(0x00, attr.GetData()[0]);
}

TEST_F(NotaryAssistedTest, ParameterizedConstructor)
{
    ByteVector data({testNKeys});
    TransactionAttribute attr(TransactionAttribute::Usage::NotaryAssisted, data);

    EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
    EXPECT_EQ(1u, attr.GetData().Size());
    EXPECT_EQ(testNKeys, attr.GetData()[0]);
}

TEST_F(NotaryAssistedTest, GettersAndSetters)
{
    TransactionAttribute attr;

    // Set as NotaryAssisted
    attr.SetUsage(TransactionAttribute::Usage::NotaryAssisted);

    // Set NKeys
    ByteVector data({testNKeys});
    attr.SetData(data);

    EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
    EXPECT_EQ(testNKeys, attr.GetData()[0]);

    // Update NKeys
    uint8_t newNKeys = 10;
    ByteVector newData({newNKeys});
    attr.SetData(newData);

    EXPECT_EQ(newNKeys, attr.GetData()[0]);
}

TEST_F(NotaryAssistedTest, UsageEnumValue)
{
    // Verify the enum value for NotaryAssisted
    EXPECT_EQ(0x22, static_cast<uint8_t>(TransactionAttribute::Usage::NotaryAssisted));
}

TEST_F(NotaryAssistedTest, Serialization)
{
    ByteVector data({testNKeys});
    TransactionAttribute original(TransactionAttribute::Usage::NotaryAssisted, data);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    TransactionAttribute deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetUsage(), deserialized.GetUsage());
    EXPECT_EQ(original.GetData(), deserialized.GetData());
    EXPECT_EQ(testNKeys, deserialized.GetData()[0]);
}

TEST_F(NotaryAssistedTest, JsonSerialization)
{
    ByteVector data({testNKeys});
    TransactionAttribute original(TransactionAttribute::Usage::NotaryAssisted, data);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    TransactionAttribute deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetUsage(), deserialized.GetUsage());
    EXPECT_EQ(original.GetData().Size(), deserialized.GetData().Size());
}

TEST_F(NotaryAssistedTest, DifferentNKeysValues)
{
    // Test various NKeys values
    std::vector<uint8_t> nKeysValues = {0, 1, 5, 10, 20, 100, 255};

    for (uint8_t nKeys : nKeysValues)
    {
        ByteVector data({nKeys});
        TransactionAttribute attr(TransactionAttribute::Usage::NotaryAssisted, data);

        EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
        EXPECT_EQ(nKeys, attr.GetData()[0]);
    }
}

TEST_F(NotaryAssistedTest, SerializationRoundTrip)
{
    // Test multiple round trips
    ByteVector data({testNKeys});
    TransactionAttribute original(TransactionAttribute::Usage::NotaryAssisted, data);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        TransactionAttribute deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, deserialized.GetUsage());
        EXPECT_EQ(testNKeys, deserialized.GetData()[0]);
    }
}

TEST_F(NotaryAssistedTest, ZeroNKeys)
{
    // Test with zero keys (edge case)
    ByteVector data({0});
    TransactionAttribute attr(TransactionAttribute::Usage::NotaryAssisted, data);

    EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
    EXPECT_EQ(0, attr.GetData()[0]);
}

TEST_F(NotaryAssistedTest, MaxNKeys)
{
    // Test with maximum value (255)
    ByteVector data({255});
    TransactionAttribute attr(TransactionAttribute::Usage::NotaryAssisted, data);

    EXPECT_EQ(TransactionAttribute::Usage::NotaryAssisted, attr.GetUsage());
    EXPECT_EQ(255, attr.GetData()[0]);
}

TEST_F(NotaryAssistedTest, InvalidDataSize)
{
    // NotaryAssisted should have exactly 1 byte of data
    TransactionAttribute attr;
    attr.SetUsage(TransactionAttribute::Usage::NotaryAssisted);

    // Test with empty data
    ByteVector emptyData;
    attr.SetData(emptyData);
    EXPECT_TRUE(attr.GetData().empty());

    // Test with too much data
    ByteVector tooMuchData({1, 2, 3});
    attr.SetData(tooMuchData);
    EXPECT_EQ(3u, attr.GetData().Size());
}

TEST_F(NotaryAssistedTest, ComparisonWithOtherAttributes)
{
    // Create NotaryAssisted attribute
    ByteVector notaryData({testNKeys});
    TransactionAttribute notaryAttr(TransactionAttribute::Usage::NotaryAssisted, notaryData);

    // Create different attribute type
    ByteVector conflictsData(32, 0xFF);  // 32 bytes for Conflicts
    TransactionAttribute conflictsAttr(TransactionAttribute::Usage::Conflicts, conflictsData);

    // They should be different
    EXPECT_NE(notaryAttr.GetUsage(), conflictsAttr.GetUsage());
    EXPECT_NE(notaryAttr.GetData().Size(), conflictsAttr.GetData().Size());
}

TEST_F(NotaryAssistedTest, UsageInTransaction)
{
    // Simulate usage in a transaction context
    std::vector<TransactionAttribute> attributes;

    // Add NotaryAssisted attribute
    ByteVector data({testNKeys});
    attributes.emplace_back(TransactionAttribute::Usage::NotaryAssisted, data);

    // Add other attributes
    attributes.emplace_back(TransactionAttribute::Usage::HighPriority, ByteVector());

    // Verify NotaryAssisted is present
    bool hasNotaryAssisted = false;
    for (const auto& attr : attributes)
    {
        if (attr.GetUsage() == TransactionAttribute::Usage::NotaryAssisted)
        {
            hasNotaryAssisted = true;
            EXPECT_EQ(testNKeys, attr.GetData()[0]);
        }
    }
    EXPECT_TRUE(hasNotaryAssisted);
}
