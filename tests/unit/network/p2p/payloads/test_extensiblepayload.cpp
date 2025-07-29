#include <gtest/gtest.h>
#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/witness.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <string>
#include <vector>

using namespace neo::network::p2p::payloads;
using namespace neo::ledger;
using namespace neo::io;

/**
 * @brief Test fixture for ExtensiblePayload
 */
class ExtensiblePayloadTest : public testing::Test
{
  protected:
    std::string testCategory;
    uint32_t testValidBlockStart;
    uint32_t testValidBlockEnd;
    UInt160 testSender;
    ByteVector testData;
    Witness testWitness;

    void SetUp() override
    {
        // Initialize test data
        testCategory = "TestCategory";
        testValidBlockStart = 100;
        testValidBlockEnd = 200;
        testSender = UInt160::FromHexString("0x1234567890abcdef1234567890abcdef12345678");
        testData = ByteVector({0x01, 0x02, 0x03, 0x04, 0x05});

        // Create test witness
        ByteVector invocationScript({0x40});    // Push 64 bytes (signature placeholder)
        ByteVector verificationScript({0x21});  // Push 33 bytes (public key placeholder)
        testWitness = Witness(invocationScript, verificationScript);
    }
};

TEST_F(ExtensiblePayloadTest, DefaultConstructor)
{
    ExtensiblePayload payload;

    EXPECT_TRUE(payload.GetCategory().empty());
    EXPECT_EQ(0u, payload.GetValidBlockStart());
    EXPECT_EQ(0u, payload.GetValidBlockEnd());
    EXPECT_EQ(UInt160::Zero(), payload.GetSender());
    EXPECT_TRUE(payload.GetData().empty());
}

TEST_F(ExtensiblePayloadTest, ParameterizedConstructor)
{
    ExtensiblePayload payload(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    EXPECT_EQ(testCategory, payload.GetCategory());
    EXPECT_EQ(testValidBlockStart, payload.GetValidBlockStart());
    EXPECT_EQ(testValidBlockEnd, payload.GetValidBlockEnd());
    EXPECT_EQ(testSender, payload.GetSender());
    EXPECT_EQ(testData, payload.GetData());
    EXPECT_EQ(testWitness.GetInvocationScript(), payload.GetWitness().GetInvocationScript());
    EXPECT_EQ(testWitness.GetVerificationScript(), payload.GetWitness().GetVerificationScript());
}

TEST_F(ExtensiblePayloadTest, GettersAndSetters)
{
    ExtensiblePayload payload;

    // Test Category
    payload.SetCategory(testCategory);
    EXPECT_EQ(testCategory, payload.GetCategory());

    // Test ValidBlockStart
    payload.SetValidBlockStart(testValidBlockStart);
    EXPECT_EQ(testValidBlockStart, payload.GetValidBlockStart());

    // Test ValidBlockEnd
    payload.SetValidBlockEnd(testValidBlockEnd);
    EXPECT_EQ(testValidBlockEnd, payload.GetValidBlockEnd());

    // Test Sender
    payload.SetSender(testSender);
    EXPECT_EQ(testSender, payload.GetSender());

    // Test Data
    payload.SetData(testData);
    EXPECT_EQ(testData, payload.GetData());

    // Test Witness
    payload.SetWitness(testWitness);
    EXPECT_EQ(testWitness.GetInvocationScript(), payload.GetWitness().GetInvocationScript());
    EXPECT_EQ(testWitness.GetVerificationScript(), payload.GetWitness().GetVerificationScript());
}

TEST_F(ExtensiblePayloadTest, Serialization)
{
    ExtensiblePayload original(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    ExtensiblePayload deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetCategory(), deserialized.GetCategory());
    EXPECT_EQ(original.GetValidBlockStart(), deserialized.GetValidBlockStart());
    EXPECT_EQ(original.GetValidBlockEnd(), deserialized.GetValidBlockEnd());
    EXPECT_EQ(original.GetSender(), deserialized.GetSender());
    EXPECT_EQ(original.GetData(), deserialized.GetData());
    EXPECT_EQ(original.GetWitness().GetInvocationScript(), deserialized.GetWitness().GetInvocationScript());
    EXPECT_EQ(original.GetWitness().GetVerificationScript(), deserialized.GetWitness().GetVerificationScript());
}

TEST_F(ExtensiblePayloadTest, JsonSerialization)
{
    ExtensiblePayload original(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    ExtensiblePayload deserialized;
    deserialized.DeserializeJson(reader);

    // Compare basic properties
    EXPECT_EQ(original.GetCategory(), deserialized.GetCategory());
    EXPECT_EQ(original.GetValidBlockStart(), deserialized.GetValidBlockStart());
    EXPECT_EQ(original.GetValidBlockEnd(), deserialized.GetValidBlockEnd());
    EXPECT_EQ(original.GetSender(), deserialized.GetSender());
}

TEST_F(ExtensiblePayloadTest, IsValidFor)
{
    ExtensiblePayload payload(testCategory, 100, 200, testSender, testData, testWitness);

    // Before valid range
    EXPECT_FALSE(payload.IsValidFor(50));
    EXPECT_FALSE(payload.IsValidFor(99));

    // Within valid range
    EXPECT_TRUE(payload.IsValidFor(100));
    EXPECT_TRUE(payload.IsValidFor(150));
    EXPECT_TRUE(payload.IsValidFor(200));

    // After valid range
    EXPECT_FALSE(payload.IsValidFor(201));
    EXPECT_FALSE(payload.IsValidFor(300));
}

TEST_F(ExtensiblePayloadTest, GetHash)
{
    ExtensiblePayload payload1(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);
    ExtensiblePayload payload2(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    // Same payloads should have same hash
    UInt256 hash1 = payload1.GetHash();
    UInt256 hash2 = payload2.GetHash();
    EXPECT_EQ(hash1, hash2);

    // Hash should be consistent across multiple calls
    EXPECT_EQ(hash1, payload1.GetHash());

    // Different payload should have different hash
    payload2.SetCategory("DifferentCategory");
    UInt256 hash3 = payload2.GetHash();
    EXPECT_NE(hash1, hash3);
}

TEST_F(ExtensiblePayloadTest, GetSize)
{
    ExtensiblePayload payload(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    size_t size = payload.GetSize();
    EXPECT_GT(size, 0u);

    // Size should include all components
    // Category length + category + 4 (start) + 4 (end) + 20 (sender) + data length + data + witness
    size_t minExpectedSize = testCategory.length() + 4 + 4 + 20 + testData.Size();
    EXPECT_GE(size, minExpectedSize);
}

TEST_F(ExtensiblePayloadTest, EmptyCategory)
{
    ExtensiblePayload payload("", testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    EXPECT_TRUE(payload.GetCategory().empty());

    // Should still serialize/deserialize correctly
    MemoryStream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);

    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    ExtensiblePayload deserialized;
    deserialized.Deserialize(reader);

    EXPECT_TRUE(deserialized.GetCategory().empty());
}

TEST_F(ExtensiblePayloadTest, LargeData)
{
    // Test with large data payload
    ByteVector largeData(1024 * 10, 0xFF);  // 10KB
    ExtensiblePayload payload(testCategory, testValidBlockStart, testValidBlockEnd, testSender, largeData, testWitness);

    EXPECT_EQ(largeData.Size(), payload.GetData().Size());
    EXPECT_EQ(largeData, payload.GetData());
}

TEST_F(ExtensiblePayloadTest, GetUnsignedData)
{
    ExtensiblePayload payload(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    ByteVector unsignedData = payload.GetUnsignedData();
    EXPECT_GT(unsignedData.Size(), 0u);

    // Unsigned data should not include witness
    EXPECT_LT(unsignedData.Size(), payload.GetSize());
}

TEST_F(ExtensiblePayloadTest, CreateStaticMethod)
{
    auto payload =
        ExtensiblePayload::Create(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData);

    ASSERT_NE(nullptr, payload);
    EXPECT_EQ(testCategory, payload->GetCategory());
    EXPECT_EQ(testValidBlockStart, payload->GetValidBlockStart());
    EXPECT_EQ(testValidBlockEnd, payload->GetValidBlockEnd());
    EXPECT_EQ(testSender, payload->GetSender());
    EXPECT_EQ(testData, payload->GetData());
}

TEST_F(ExtensiblePayloadTest, DifferentCategories)
{
    // Test various category strings
    std::vector<std::string> categories = {"Oracle",         "DBFTCommit", "DBFTPrepareRequest", "DBFTPrepareResponse",
                                           "DBFTChangeView", "StateRoot",  "CustomCategory123"};

    for (const auto& category : categories)
    {
        ExtensiblePayload payload(category, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);
        EXPECT_EQ(category, payload.GetCategory());
    }
}

TEST_F(ExtensiblePayloadTest, SerializationRoundTrip)
{
    // Test multiple round trips
    ExtensiblePayload original(testCategory, testValidBlockStart, testValidBlockEnd, testSender, testData, testWitness);

    for (int i = 0; i < 3; ++i)
    {
        MemoryStream stream;
        BinaryWriter writer(stream);
        original.Serialize(writer);

        stream.Seek(0, SeekOrigin::Begin);
        BinaryReader reader(stream);
        ExtensiblePayload deserialized;
        deserialized.Deserialize(reader);

        // Update original for next iteration
        original = deserialized;

        // Verify consistency
        EXPECT_EQ(testCategory, deserialized.GetCategory());
        EXPECT_EQ(testValidBlockStart, deserialized.GetValidBlockStart());
        EXPECT_EQ(testValidBlockEnd, deserialized.GetValidBlockEnd());
        EXPECT_EQ(testSender, deserialized.GetSender());
        EXPECT_EQ(testData, deserialized.GetData());
    }
}

TEST_F(ExtensiblePayloadTest, EdgeCaseBlockRanges)
{
    // Test edge cases for block ranges
    struct TestCase
    {
        uint32_t start;
        uint32_t end;
        uint32_t testBlock;
        bool expectedValid;
    };

    std::vector<TestCase> testCases = {{0, 0, 0, true},           {0, UINT32_MAX, 1000000, true},
                                       {1000, 1000, 1000, true},  {1000, 1000, 999, false},
                                       {1000, 1000, 1001, false}, {UINT32_MAX - 1, UINT32_MAX, UINT32_MAX, true}};

    for (const auto& tc : testCases)
    {
        ExtensiblePayload payload(testCategory, tc.start, tc.end, testSender, testData, testWitness);
        EXPECT_EQ(tc.expectedValid, payload.IsValidFor(tc.testBlock))
            << "Failed for start=" << tc.start << ", end=" << tc.end << ", block=" << tc.testBlock;
    }
}
