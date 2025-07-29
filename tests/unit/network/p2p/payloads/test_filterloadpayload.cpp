#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <string>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::io;

/**
 * @brief Test fixture for FilterLoadPayload
 */
class FilterLoadPayloadTest : public testing::Test
{
  protected:
    ByteVector testFilter;
    uint8_t testK;
    uint32_t testTweak;
    uint8_t testFlags;

    void SetUp() override
    {
        // Initialize test data
        testFilter = ByteVector({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
        testK = 5;
        testTweak = 0x12345678;
        testFlags = 0x01;
    }
};

TEST_F(FilterLoadPayloadTest, DefaultConstructor)
{
    FilterLoadPayload payload;

    EXPECT_TRUE(payload.GetFilter().empty());
    EXPECT_EQ(0, payload.GetK());
    EXPECT_EQ(0u, payload.GetTweak());
    EXPECT_EQ(0, payload.GetFlags());
}

TEST_F(FilterLoadPayloadTest, ParameterizedConstructor)
{
    FilterLoadPayload payload(testFilter, testK, testTweak, testFlags);

    EXPECT_EQ(testFilter, payload.GetFilter());
    EXPECT_EQ(testK, payload.GetK());
    EXPECT_EQ(testTweak, payload.GetTweak());
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(FilterLoadPayloadTest, GettersAndSetters)
{
    FilterLoadPayload payload;

    // Test Filter
    payload.SetFilter(testFilter);
    EXPECT_EQ(testFilter, payload.GetFilter());

    // Test K
    payload.SetK(testK);
    EXPECT_EQ(testK, payload.GetK());

    // Test Tweak
    payload.SetTweak(testTweak);
    EXPECT_EQ(testTweak, payload.GetTweak());

    // Test Flags
    payload.SetFlags(testFlags);
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(FilterLoadPayloadTest, MaxFilterSize)
{
    // Verify the constant is set correctly
    EXPECT_EQ(36000u, FilterLoadPayload::MaxFilterSize);
}

TEST_F(FilterLoadPayloadTest, Serialization)
{
    FilterLoadPayload original(testFilter, testK, testTweak, testFlags);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    FilterLoadPayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetFilter(), deserialized.GetFilter());
    EXPECT_EQ(original.GetK(), deserialized.GetK());
    EXPECT_EQ(original.GetTweak(), deserialized.GetTweak());
    EXPECT_EQ(original.GetFlags(), deserialized.GetFlags());
}

TEST_F(FilterLoadPayloadTest, JsonSerialization)
{
    FilterLoadPayload original(testFilter, testK, testTweak, testFlags);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    FilterLoadPayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetFilter(), deserialized.GetFilter());
    EXPECT_EQ(original.GetK(), deserialized.GetK());
    EXPECT_EQ(original.GetTweak(), deserialized.GetTweak());
    EXPECT_EQ(original.GetFlags(), deserialized.GetFlags());
}

TEST_F(FilterLoadPayloadTest, EmptyFilter)
{
    ByteVector emptyFilter;
    FilterLoadPayload payload(emptyFilter, testK, testTweak, testFlags);

    EXPECT_TRUE(payload.GetFilter().empty());
    EXPECT_EQ(testK, payload.GetK());
    EXPECT_EQ(testTweak, payload.GetTweak());
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(FilterLoadPayloadTest, LargeFilter)
{
    // Test with a large filter (but under max size)
    ByteVector largeFilter(1000, 0xFF);
    FilterLoadPayload payload(largeFilter, testK, testTweak, testFlags);

    EXPECT_EQ(1000u, payload.GetFilter().Size());
    EXPECT_EQ(largeFilter, payload.GetFilter());
}

TEST_F(FilterLoadPayloadTest, MaxSizeFilter)
{
    // Test with maximum allowed filter size
    ByteVector maxFilter(FilterLoadPayload::MaxFilterSize, 0xAA);
    FilterLoadPayload payload(maxFilter, testK, testTweak, testFlags);

    EXPECT_EQ(FilterLoadPayload::MaxFilterSize, payload.GetFilter().Size());
    EXPECT_EQ(maxFilter, payload.GetFilter());
}

TEST_F(FilterLoadPayloadTest, DifferentKValues)
{
    // Test different values for K (number of hash functions)
    for (uint8_t k = 0; k <= 50; k += 10)
    {
        FilterLoadPayload payload(testFilter, k, testTweak, testFlags);
        EXPECT_EQ(k, payload.GetK());
    }
}

TEST_F(FilterLoadPayloadTest, DifferentTweakValues)
{
    // Test different tweak values
    std::vector<uint32_t> tweaks = {0, 1, 0xFFFFFFFF, 0x12345678, 0x87654321};

    for (uint32_t tweak : tweaks)
    {
        FilterLoadPayload payload(testFilter, testK, tweak, testFlags);
        EXPECT_EQ(tweak, payload.GetTweak());
    }
}

TEST_F(FilterLoadPayloadTest, DifferentFlags)
{
    // Test different flag values
    for (uint8_t flags = 0; flags < 8; ++flags)
    {
        FilterLoadPayload payload(testFilter, testK, testTweak, flags);
        EXPECT_EQ(flags, payload.GetFlags());
    }
}

TEST_F(FilterLoadPayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips
    FilterLoadPayload original(testFilter, testK, testTweak, testFlags);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        FilterLoadPayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testFilter, deserialized.GetFilter());
        EXPECT_EQ(testK, deserialized.GetK());
        EXPECT_EQ(testTweak, deserialized.GetTweak());
        EXPECT_EQ(testFlags, deserialized.GetFlags());
    }
}

TEST_F(FilterLoadPayloadTest, BloomFilterScenario)
{
    // Test a typical bloom filter scenario

    // Create a bloom filter with specific parameters
    // 512 bytes filter size, 10 hash functions, random tweak
    ByteVector bloomFilter(512, 0x00);

    // Set some bits in the filter (simulate adding elements)
    bloomFilter[0] = 0x01;
    bloomFilter[10] = 0xFF;
    bloomFilter[100] = 0x55;
    bloomFilter[511] = 0x80;

    uint8_t numHashFunctions = 10;
    uint32_t randomTweak = 0xDEADBEEF;
    uint8_t filterFlags = 0x00;  // BLOOM_UPDATE_NONE

    FilterLoadPayload payload(bloomFilter, numHashFunctions, randomTweak, filterFlags);

    EXPECT_EQ(512u, payload.GetFilter().Size());
    EXPECT_EQ(numHashFunctions, payload.GetK());
    EXPECT_EQ(randomTweak, payload.GetTweak());
    EXPECT_EQ(filterFlags, payload.GetFlags());

    // Verify specific bits are set
    EXPECT_EQ(0x01, payload.GetFilter()[0]);
    EXPECT_EQ(0xFF, payload.GetFilter()[10]);
    EXPECT_EQ(0x55, payload.GetFilter()[100]);
    EXPECT_EQ(0x80, payload.GetFilter()[511]);
}

TEST_F(FilterLoadPayloadTest, UpdateAfterConstruction)
{
    FilterLoadPayload payload(testFilter, testK, testTweak, testFlags);

    // Update all fields
    ByteVector newFilter({0xFF, 0xEE, 0xDD, 0xCC});
    uint8_t newK = 15;
    uint32_t newTweak = 0xABCDEF00;
    uint8_t newFlags = 0x02;

    payload.SetFilter(newFilter);
    payload.SetK(newK);
    payload.SetTweak(newTweak);
    payload.SetFlags(newFlags);

    EXPECT_EQ(newFilter, payload.GetFilter());
    EXPECT_EQ(newK, payload.GetK());
    EXPECT_EQ(newTweak, payload.GetTweak());
    EXPECT_EQ(newFlags, payload.GetFlags());
}
