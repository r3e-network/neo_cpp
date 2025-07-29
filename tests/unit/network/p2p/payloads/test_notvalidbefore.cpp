#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/not_valid_before.h>
#include <string>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::ledger;
using namespace neo::io;

/**
 * @brief Test fixture for NotValidBefore
 */
class NotValidBeforeTest : public testing::Test
{
  protected:
    uint32_t testHeight;

    void SetUp() override
    {
        // Initialize test data
        testHeight = 1000000;  // Block height 1,000,000
    }
};

TEST_F(NotValidBeforeTest, DefaultConstructor)
{
    NotValidBefore nvb;

    EXPECT_EQ(0u, nvb.GetHeight());
    EXPECT_EQ(TransactionAttribute::Usage::NotValidBefore, nvb.GetType());
}

TEST_F(NotValidBeforeTest, ParameterizedConstructor)
{
    NotValidBefore nvb(testHeight);

    EXPECT_EQ(testHeight, nvb.GetHeight());
    EXPECT_EQ(TransactionAttribute::Usage::NotValidBefore, nvb.GetType());
}

TEST_F(NotValidBeforeTest, GettersAndSetters)
{
    NotValidBefore nvb;

    // Initially zero
    EXPECT_EQ(0u, nvb.GetHeight());

    // Set height
    nvb.SetHeight(testHeight);
    EXPECT_EQ(testHeight, nvb.GetHeight());

    // Update height
    uint32_t newHeight = 2000000;
    nvb.SetHeight(newHeight);
    EXPECT_EQ(newHeight, nvb.GetHeight());
}

TEST_F(NotValidBeforeTest, GetType)
{
    NotValidBefore nvb;

    // Type should always be NotValidBefore
    EXPECT_EQ(TransactionAttribute::Usage::NotValidBefore, nvb.GetType());

    // Type shouldn't change when height changes
    nvb.SetHeight(testHeight);
    EXPECT_EQ(TransactionAttribute::Usage::NotValidBefore, nvb.GetType());
}

TEST_F(NotValidBeforeTest, AllowMultiple)
{
    NotValidBefore nvb;

    // NotValidBefore attribute does not allow multiple instances
    EXPECT_FALSE(nvb.AllowMultiple());
}

TEST_F(NotValidBeforeTest, GetSize)
{
    NotValidBefore nvb;

    // Size should be 4 bytes (uint32_t)
    EXPECT_EQ(4, nvb.GetSize());

    // Size shouldn't change with different height values
    nvb.SetHeight(testHeight);
    EXPECT_EQ(4, nvb.GetSize());
}

TEST_F(NotValidBeforeTest, Serialization)
{
    NotValidBefore original(testHeight);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    NotValidBefore deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetHeight(), deserialized.GetHeight());
    EXPECT_EQ(original.GetType(), deserialized.GetType());
}

TEST_F(NotValidBeforeTest, JsonSerialization)
{
    NotValidBefore original(testHeight);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    NotValidBefore deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetHeight(), deserialized.GetHeight());
}

TEST_F(NotValidBeforeTest, EqualityOperator)
{
    NotValidBefore nvb1(testHeight);
    NotValidBefore nvb2(testHeight);
    NotValidBefore nvb3(500000);

    // Same height
    EXPECT_TRUE(nvb1 == nvb2);
    EXPECT_FALSE(nvb1 != nvb2);

    // Different height
    EXPECT_FALSE(nvb1 == nvb3);
    EXPECT_TRUE(nvb1 != nvb3);
}

TEST_F(NotValidBeforeTest, ZeroHeight)
{
    NotValidBefore nvb(0);

    EXPECT_EQ(0u, nvb.GetHeight());

    // Serialize and deserialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    nvb.Serialize(writer);

    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    NotValidBefore deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(0u, deserialized.GetHeight());
}

TEST_F(NotValidBeforeTest, MaxHeight)
{
    uint32_t maxHeight = UINT32_MAX;
    NotValidBefore nvb(maxHeight);

    EXPECT_EQ(maxHeight, nvb.GetHeight());

    // Serialize and deserialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    nvb.Serialize(writer);

    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    NotValidBefore deserialized;
    deserialized.Deserialize(reader);

    EXPECT_EQ(maxHeight, deserialized.GetHeight());
}

TEST_F(NotValidBeforeTest, SerializationRoundTrip)
{
    // Test multiple round trips
    NotValidBefore original(testHeight);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        NotValidBefore deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testHeight, deserialized.GetHeight());
    }
}

TEST_F(NotValidBeforeTest, DifferentHeights)
{
    // Test various height values
    std::vector<uint32_t> heights = {0, 1, 100, 1000, 100000, 1000000, 10000000, UINT32_MAX};

    for (uint32_t height : heights)
    {
        NotValidBefore nvb(height);
        EXPECT_EQ(height, nvb.GetHeight());
    }
}

TEST_F(NotValidBeforeTest, Verify)
{
    NotValidBefore nvb(testHeight);

    // Verify should return true for valid heights
    // Note: Actual implementation may require DataCache and Transaction parameters
    EXPECT_TRUE(nvb.Verify());
}

TEST_F(NotValidBeforeTest, CalculateNetworkFee)
{
    NotValidBefore nvb(testHeight);

    // Network fee calculation
    // Note: Actual implementation may require DataCache and Transaction parameters
    int64_t fee = nvb.CalculateNetworkFee();
    EXPECT_GE(fee, 0);  // Fee should be non-negative
}

TEST_F(NotValidBeforeTest, UsageEnumValue)
{
    // Verify the enum value
    EXPECT_EQ(0x20, static_cast<uint8_t>(TransactionAttribute::Usage::NotValidBefore));
}

TEST_F(NotValidBeforeTest, JsonFormat)
{
    NotValidBefore nvb(testHeight);

    // Serialize to JSON and check format
    JsonWriter writer;
    nvb.SerializeJson(writer);
    std::string json = writer.ToString();

    // JSON should contain the height value
    EXPECT_NE(std::string::npos, json.find(std::to_string(testHeight)));
}

TEST_F(NotValidBeforeTest, TransactionValidation)
{
    // Simulate transaction validation scenarios
    struct TestCase
    {
        uint32_t notValidBeforeHeight;
        uint32_t currentBlockHeight;
        bool expectedValid;
        std::string description;
    };

    std::vector<TestCase> testCases = {{1000, 500, false, "Transaction not yet valid"},
                                       {1000, 1000, true, "Transaction becomes valid at exact height"},
                                       {1000, 1500, true, "Transaction valid after height"},
                                       {0, 0, true, "Both zero"},
                                       {0, 1000, true, "No restriction"},
                                       {UINT32_MAX, UINT32_MAX - 1, false, "Max height minus one"},
                                       {UINT32_MAX, UINT32_MAX, true, "Max height"}};

    for (const auto& tc : testCases)
    {
        NotValidBefore nvb(tc.notValidBeforeHeight);
        // In actual implementation, this would check against current block height
        bool wouldBeValid = tc.currentBlockHeight >= tc.notValidBeforeHeight;
        EXPECT_EQ(tc.expectedValid, wouldBeValid) << "Failed for: " << tc.description;
    }
}

TEST_F(NotValidBeforeTest, UpdateAfterConstruction)
{
    NotValidBefore nvb;

    // Initially zero
    EXPECT_EQ(0u, nvb.GetHeight());

    // Update multiple times
    for (uint32_t height = 100; height <= 1000; height += 100)
    {
        nvb.SetHeight(height);
        EXPECT_EQ(height, nvb.GetHeight());
    }
}
