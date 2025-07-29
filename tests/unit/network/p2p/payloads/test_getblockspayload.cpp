#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <string>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::io;

/**
 * @brief Test fixture for GetBlocksPayload
 */
class GetBlocksPayloadTest : public testing::Test
{
  protected:
    UInt256 testHashStart;
    int16_t testCount;

    void SetUp() override
    {
        // Initialize test data
        testHashStart = UInt256::FromHexString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
        testCount = 100;
    }
};

TEST_F(GetBlocksPayloadTest, DefaultConstructor)
{
    GetBlocksPayload payload;

    EXPECT_EQ(UInt256::Zero(), payload.GetHashStart());
    EXPECT_EQ(-1, payload.GetCount());  // Default count should be -1
}

TEST_F(GetBlocksPayloadTest, ParameterizedConstructor)
{
    GetBlocksPayload payload(testHashStart);

    EXPECT_EQ(testHashStart, payload.GetHashStart());
    EXPECT_EQ(-1, payload.GetCount());  // Default count should be -1
}

TEST_F(GetBlocksPayloadTest, GettersAndSetters)
{
    GetBlocksPayload payload;

    // Test HashStart
    payload.SetHashStart(testHashStart);
    EXPECT_EQ(testHashStart, payload.GetHashStart());

    // Test Count
    payload.SetCount(testCount);
    EXPECT_EQ(testCount, payload.GetCount());

    // Update values
    UInt256 newHash = UInt256::FromHexString("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");
    int16_t newCount = 500;

    payload.SetHashStart(newHash);
    payload.SetCount(newCount);

    EXPECT_EQ(newHash, payload.GetHashStart());
    EXPECT_EQ(newCount, payload.GetCount());
}

TEST_F(GetBlocksPayloadTest, CreateStaticMethod)
{
    // Test with default count
    GetBlocksPayload payload1 = GetBlocksPayload::Create(testHashStart);
    EXPECT_EQ(testHashStart, payload1.GetHashStart());
    EXPECT_EQ(-1, payload1.GetCount());

    // Test with specific count
    GetBlocksPayload payload2 = GetBlocksPayload::Create(testHashStart, testCount);
    EXPECT_EQ(testHashStart, payload2.GetHashStart());
    EXPECT_EQ(testCount, payload2.GetCount());
}

TEST_F(GetBlocksPayloadTest, GetSize)
{
    GetBlocksPayload payload(testHashStart);
    payload.SetCount(testCount);

    // Size should be: 32 bytes (UInt256) + 2 bytes (int16_t)
    EXPECT_EQ(34, payload.GetSize());
}

TEST_F(GetBlocksPayloadTest, Serialization)
{
    GetBlocksPayload original(testHashStart);
    original.SetCount(testCount);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    GetBlocksPayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetHashStart(), deserialized.GetHashStart());
    EXPECT_EQ(original.GetCount(), deserialized.GetCount());
}

TEST_F(GetBlocksPayloadTest, JsonSerialization)
{
    GetBlocksPayload original(testHashStart);
    original.SetCount(testCount);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    GetBlocksPayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetHashStart(), deserialized.GetHashStart());
    EXPECT_EQ(original.GetCount(), deserialized.GetCount());
}

TEST_F(GetBlocksPayloadTest, ZeroHashStart)
{
    GetBlocksPayload payload(UInt256::Zero());
    payload.SetCount(50);

    EXPECT_EQ(UInt256::Zero(), payload.GetHashStart());
    EXPECT_EQ(50, payload.GetCount());
}

TEST_F(GetBlocksPayloadTest, NegativeCount)
{
    GetBlocksPayload payload(testHashStart);

    // Test negative count (commonly used to request all blocks)
    payload.SetCount(-1);
    EXPECT_EQ(-1, payload.GetCount());

    // Test other negative values
    payload.SetCount(-100);
    EXPECT_EQ(-100, payload.GetCount());
}

TEST_F(GetBlocksPayloadTest, MaxCount)
{
    GetBlocksPayload payload(testHashStart);

    // Test maximum int16_t value
    int16_t maxCount = INT16_MAX;
    payload.SetCount(maxCount);
    EXPECT_EQ(maxCount, payload.GetCount());

    // Test minimum int16_t value
    int16_t minCount = INT16_MIN;
    payload.SetCount(minCount);
    EXPECT_EQ(minCount, payload.GetCount());
}

TEST_F(GetBlocksPayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips
    GetBlocksPayload original(testHashStart);
    original.SetCount(testCount);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        GetBlocksPayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testHashStart, deserialized.GetHashStart());
        EXPECT_EQ(testCount, deserialized.GetCount());
    }
}

TEST_F(GetBlocksPayloadTest, DifferentHashValues)
{
    // Test with various hash values
    std::vector<std::string> hashStrings = {"0x0000000000000000000000000000000000000000000000000000000000000000",
                                            "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
                                            "0x1111111111111111111111111111111111111111111111111111111111111111",
                                            "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef"};

    for (const auto& hashStr : hashStrings)
    {
        UInt256 hash = UInt256::FromHexString(hashStr);
        GetBlocksPayload payload(hash);
        EXPECT_EQ(hash, payload.GetHashStart());
    }
}

TEST_F(GetBlocksPayloadTest, CommonUsageScenarios)
{
    // Scenario 1: Request all blocks from genesis
    GetBlocksPayload genesisRequest = GetBlocksPayload::Create(UInt256::Zero(), -1);
    EXPECT_EQ(UInt256::Zero(), genesisRequest.GetHashStart());
    EXPECT_EQ(-1, genesisRequest.GetCount());

    // Scenario 2: Request specific number of blocks from a known hash
    GetBlocksPayload specificRequest = GetBlocksPayload::Create(testHashStart, 500);
    EXPECT_EQ(testHashStart, specificRequest.GetHashStart());
    EXPECT_EQ(500, specificRequest.GetCount());

    // Scenario 3: Request single block
    GetBlocksPayload singleBlockRequest = GetBlocksPayload::Create(testHashStart, 1);
    EXPECT_EQ(testHashStart, singleBlockRequest.GetHashStart());
    EXPECT_EQ(1, singleBlockRequest.GetCount());
}

TEST_F(GetBlocksPayloadTest, UpdateAfterConstruction)
{
    GetBlocksPayload payload;

    // Initially default values
    EXPECT_EQ(UInt256::Zero(), payload.GetHashStart());
    EXPECT_EQ(-1, payload.GetCount());

    // Update multiple times
    for (int i = 0; i < 5; ++i)
    {
        std::string hashStr = "0x";
        for (int j = 0; j < 64; ++j)
        {
            hashStr += std::to_string(i);
        }
        UInt256 hash = UInt256::FromHexString(hashStr);
        int16_t count = static_cast<int16_t>(i * 100);

        payload.SetHashStart(hash);
        payload.SetCount(count);

        EXPECT_EQ(hash, payload.GetHashStart());
        EXPECT_EQ(count, payload.GetCount());
    }
}
