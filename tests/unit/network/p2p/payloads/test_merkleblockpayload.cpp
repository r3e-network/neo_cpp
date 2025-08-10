#include <gtest/gtest.h>
#include <memory>
#include <neo/network/p2p/payloads/header.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/merkle_block_payload.h>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::io;

/**
 * @brief Test fixture for MerkleBlockPayload
 */
class MerkleBlockPayloadTest : public testing::Test
{
  protected:
    std::shared_ptr<Header> testHeader;
    std::vector<UInt256> testHashes;
    ByteVector testFlags;
    uint32_t testTransactionCount;

    void SetUp() override
    {
        // Create test header
        testHeader = std::make_shared<Header>();
        // In a real implementation, we would set proper header values

        // Create test transaction hashes
        testHashes.push_back(
            UInt256::FromHexString("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
        testHashes.push_back(
            UInt256::FromHexString("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321"));
        testHashes.push_back(
            UInt256::FromHexString("0xaaaabbbbccccddddaaaabbbbccccddddaaaabbbbccccddddaaaabbbbccccdddd"));

        // Create test flags (bit field indicating which transactions are included)
        testFlags = ByteVector({0x0F, 0x00});  // First 4 bits set

        testTransactionCount = 10;  // Total transactions in the block
    }
};

TEST_F(MerkleBlockPayloadTest, DefaultConstructor)
{
    MerkleBlockPayload payload;

    EXPECT_EQ(nullptr, payload.GetHeader());
    EXPECT_EQ(0u, payload.GetTransactionCount());
    EXPECT_TRUE(payload.GetHashes().empty());
    EXPECT_TRUE(payload.GetFlags().empty());
}

TEST_F(MerkleBlockPayloadTest, ParameterizedConstructor)
{
    MerkleBlockPayload payload(testHeader, testTransactionCount, testHashes, testFlags);

    EXPECT_EQ(testHeader, payload.GetHeader());
    EXPECT_EQ(testTransactionCount, payload.GetTransactionCount());
    EXPECT_EQ(testHashes, payload.GetHashes());
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(MerkleBlockPayloadTest, GettersAndSetters)
{
    MerkleBlockPayload payload;

    // Test Header
    payload.SetHeader(testHeader);
    EXPECT_EQ(testHeader, payload.GetHeader());

    // Test TransactionCount
    payload.SetTransactionCount(testTransactionCount);
    EXPECT_EQ(testTransactionCount, payload.GetTransactionCount());

    // Test Hashes
    payload.SetHashes(testHashes);
    EXPECT_EQ(testHashes, payload.GetHashes());
    EXPECT_EQ(3u, payload.GetHashes().size());

    // Test Flags
    payload.SetFlags(testFlags);
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(MerkleBlockPayloadTest, Serialization)
{
    MerkleBlockPayload original(testHeader, testTransactionCount, testHashes, testFlags);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    MerkleBlockPayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_NE(nullptr, deserialized.GetHeader());
    EXPECT_EQ(original.GetTransactionCount(), deserialized.GetTransactionCount());
    EXPECT_EQ(original.GetHashes().size(), deserialized.GetHashes().size());
    for (size_t i = 0; i < original.GetHashes().size(); ++i)
    {
        EXPECT_EQ(original.GetHashes()[i], deserialized.GetHashes()[i]);
    }
    EXPECT_EQ(original.GetFlags(), deserialized.GetFlags());
}

TEST_F(MerkleBlockPayloadTest, JsonSerialization)
{
    MerkleBlockPayload original(testHeader, testTransactionCount, testHashes, testFlags);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    MerkleBlockPayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare basic properties
    EXPECT_EQ(original.GetTransactionCount(), deserialized.GetTransactionCount());
    EXPECT_EQ(original.GetHashes().size(), deserialized.GetHashes().size());
    EXPECT_EQ(original.GetFlags().Size(), deserialized.GetFlags().Size());
}

TEST_F(MerkleBlockPayloadTest, EmptyHashes)
{
    std::vector<UInt256> emptyHashes;
    MerkleBlockPayload payload(testHeader, testTransactionCount, emptyHashes, testFlags);

    EXPECT_TRUE(payload.GetHashes().empty());
    EXPECT_EQ(testTransactionCount, payload.GetTransactionCount());
    EXPECT_EQ(testFlags, payload.GetFlags());
}

TEST_F(MerkleBlockPayloadTest, EmptyFlags)
{
    ByteVector emptyFlags;
    MerkleBlockPayload payload(testHeader, testTransactionCount, testHashes, emptyFlags);

    EXPECT_EQ(testHashes, payload.GetHashes());
    EXPECT_TRUE(payload.GetFlags().empty());
}

TEST_F(MerkleBlockPayloadTest, LargeTransactionCount)
{
    uint32_t largeCount = 1000000;
    MerkleBlockPayload payload(testHeader, largeCount, testHashes, testFlags);

    EXPECT_EQ(largeCount, payload.GetTransactionCount());
}

TEST_F(MerkleBlockPayloadTest, MaximumHashes)
{
    // Test with maximum allowed hashes (typically limited by block size)
    std::vector<UInt256> manyHashes;
    for (int i = 0; i < 100; ++i)
    {
        std::string hashStr = "0x";
        for (int j = 0; j < 64; ++j)
        {
            hashStr += std::to_string(i % 10);
        }
        manyHashes.push_back(UInt256::FromHexString(hashStr));
    }

    MerkleBlockPayload payload(testHeader, 10000, manyHashes, testFlags);
    EXPECT_EQ(100u, payload.GetHashes().size());
}

TEST_F(MerkleBlockPayloadTest, FlagsBitfield)
{
    // Test various flag patterns
    struct TestCase
    {
        ByteVector flags;
        std::string description;
    };

    std::vector<TestCase> testCases = {{ByteVector({0xFF}), "All bits set"},
                                       {ByteVector({0x00}), "No bits set"},
                                       {ByteVector({0xAA}), "Alternating bits"},
                                       {ByteVector({0x01, 0x02, 0x04, 0x08}), "Single bits in each byte"},
                                       {ByteVector({0xFF, 0xFF, 0xFF, 0xFF}), "Multiple bytes all set"}};

    for (const auto& tc : testCases)
    {
        MerkleBlockPayload payload(testHeader, testTransactionCount, testHashes, tc.flags);
        EXPECT_EQ(tc.flags, payload.GetFlags()) << "Failed for: " << tc.description;
    }
}

TEST_F(MerkleBlockPayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips to ensure consistency
    MerkleBlockPayload original(testHeader, testTransactionCount, testHashes, testFlags);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        MerkleBlockPayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testTransactionCount, deserialized.GetTransactionCount());
        EXPECT_EQ(testHashes.size(), deserialized.GetHashes().size());
        EXPECT_EQ(testFlags, deserialized.GetFlags());
    }
}

TEST_F(MerkleBlockPayloadTest, SPVUsageScenario)
{
    // Simulate a typical SPV (Simplified Payment Verification) scenario
    // Where a light client receives a merkle block with only relevant transactions

    // Create a merkle block with 3 out of 10 transactions
    uint32_t totalTransactions = 10;
    std::vector<UInt256> relevantTxHashes;

    // Add 3 relevant transaction hashes
    relevantTxHashes.push_back(UInt256::Random());
    relevantTxHashes.push_back(UInt256::Random());
    relevantTxHashes.push_back(UInt256::Random());

    // Flags indicate which nodes in merkle tree are included
    // For 3 transactions out of 10, we need specific flag pattern
    ByteVector spvFlags({0x1D});  // Example pattern for 3 txs

    MerkleBlockPayload spvPayload(testHeader, totalTransactions, relevantTxHashes, spvFlags);

    EXPECT_EQ(totalTransactions, spvPayload.GetTransactionCount());
    EXPECT_EQ(3u, spvPayload.GetHashes().size());
    EXPECT_FALSE(spvPayload.GetFlags().empty());
}

TEST_F(MerkleBlockPayloadTest, NullHeader)
{
    MerkleBlockPayload payload(nullptr, testTransactionCount, testHashes, testFlags);

    EXPECT_EQ(nullptr, payload.GetHeader());
    EXPECT_EQ(testTransactionCount, payload.GetTransactionCount());
    EXPECT_EQ(testHashes, payload.GetHashes());
}

TEST_F(MerkleBlockPayloadTest, UpdateAfterConstruction)
{
    MerkleBlockPayload payload;

    // Initially empty
    EXPECT_TRUE(payload.GetHashes().empty());

    // Add hashes one by one
    std::vector<UInt256> newHashes;
    newHashes.push_back(UInt256::Random());
    payload.SetHashes(newHashes);
    EXPECT_EQ(1u, payload.GetHashes().size());

    newHashes.push_back(UInt256::Random());
    payload.SetHashes(newHashes);
    EXPECT_EQ(2u, payload.GetHashes().size());

    // Update flags
    ByteVector newFlags({0x03});  // First two bits set
    payload.SetFlags(newFlags);
    EXPECT_EQ(newFlags, payload.GetFlags());
}
