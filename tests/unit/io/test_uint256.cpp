#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint256.h>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace neo::io;

class UT_UInt256 : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize test data
        zeroData_.fill(0x00);
        maxData_.fill(0xFF);

        // Create test data patterns
        testData1_.fill(0xAA);  // 10101010 pattern
        testData2_.fill(0x55);  // 01010101 pattern

        // Mixed pattern
        for (size_t i = 0; i < UInt256::Size; ++i)
        {
            testData3_[i] = static_cast<uint8_t>(i);
        }

        // Create UInt256 instances
        zeroUInt256_ = UInt256(zeroData_);
        maxUInt256_ = UInt256(maxData_);
        testUInt256_1_ = UInt256(testData1_);
        testUInt256_2_ = UInt256(testData2_);
        testUInt256_3_ = UInt256(testData3_);
    }

    void TearDown() override
    {
        // Cleanup
    }

  protected:
    UInt256::value_type zeroData_;
    UInt256::value_type maxData_;
    UInt256::value_type testData1_;
    UInt256::value_type testData2_;
    UInt256::value_type testData3_;

    UInt256 zeroUInt256_;
    UInt256 maxUInt256_;
    UInt256 testUInt256_1_;
    UInt256 testUInt256_2_;
    UInt256 testUInt256_3_;
};

TEST_F(UT_UInt256, ConstructorAndConstants)
{
    // Test: Verify UInt256 constructor and constants

    // Verify size constants
    EXPECT_EQ(UInt256::Size, 32u);
    EXPECT_EQ(UInt256::SIZE, 32u);  // Backward compatibility

    // Test default constructor
    UInt256 defaultConstructed;
    EXPECT_TRUE(defaultConstructed.IsZero());

    // Test data constructor
    UInt256::value_type testData;
    testData.fill(0x42);
    UInt256 dataConstructed(testData);

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(dataConstructed[i], 0x42);
    }

    // Test raw pointer constructor
    uint8_t rawData[32];
    std::fill_n(rawData, 32, 0x33);
    UInt256 rawConstructed(rawData);

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(rawConstructed[i], 0x33);
    }
}

TEST_F(UT_UInt256, ByteSpanConstructor)
{
    // Test: ByteSpan constructor with valid and invalid sizes

    // Test valid size
    std::vector<uint8_t> validData(32, 0x77);
    ByteSpan validSpan(validData.data(), validData.size());

    EXPECT_NO_THROW({
        UInt256 spanConstructed(validSpan);
        for (size_t i = 0; i < UInt256::Size; ++i)
        {
            EXPECT_EQ(spanConstructed[i], 0x77);
        }
    });

    // Test invalid sizes
    std::vector<uint8_t> invalidData1(31, 0x77);  // Too small
    std::vector<uint8_t> invalidData2(33, 0x77);  // Too large

    ByteSpan invalidSpan1(invalidData1.data(), invalidData1.size());
    ByteSpan invalidSpan2(invalidData2.data(), invalidData2.size());

    EXPECT_THROW(UInt256(invalidSpan1), std::invalid_argument);
    EXPECT_THROW(UInt256(invalidSpan2), std::invalid_argument);
}

TEST_F(UT_UInt256, DataAccess)
{
    // Test: Data access methods

    // Test Data() methods
    uint8_t* mutableData = testUInt256_1_.Data();
    const uint8_t* constData = testUInt256_1_.Data();

    EXPECT_NE(mutableData, nullptr);
    EXPECT_NE(constData, nullptr);
    EXPECT_EQ(mutableData, constData);

    // Verify data integrity
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(mutableData[i], 0xAA);
        EXPECT_EQ(constData[i], 0xAA);
    }

    // Test GetData()
    const auto& dataRef = testUInt256_1_.GetData();
    EXPECT_EQ(dataRef.size(), UInt256::Size);

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(dataRef[i], 0xAA);
    }

    // Test size() method
    EXPECT_EQ(testUInt256_1_.size(), UInt256::Size);
}

TEST_F(UT_UInt256, ArraySubscriptOperator)
{
    // Test: Array subscript operator

    UInt256 testValue = testUInt256_3_;

    // Test const access
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(testValue[i], static_cast<uint8_t>(i));
    }

    // Test mutable access
    testValue[0] = 0xFF;
    testValue[31] = 0xEE;

    EXPECT_EQ(testValue[0], 0xFF);
    EXPECT_EQ(testValue[31], 0xEE);

    // Verify other elements unchanged
    for (size_t i = 1; i < 31; ++i)
    {
        EXPECT_EQ(testValue[i], static_cast<uint8_t>(i));
    }
}

TEST_F(UT_UInt256, ConversionMethods)
{
    // Test: Conversion to ByteSpan and ByteVector

    // Test AsSpan()
    ByteSpan span = testUInt256_1_.AsSpan();
    EXPECT_EQ(span.Size(), UInt256::Size);

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(span[i], 0xAA);
    }

    // Test ToArray()
    ByteVector vector = testUInt256_1_.ToArray();
    EXPECT_EQ(vector.Size(), UInt256::Size);

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(vector[i], 0xAA);
    }

    // Verify span and vector contain same data
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(span[i], vector[i]);
    }
}

TEST_F(UT_UInt256, ComparisonOperators)
{
    // Test: Comparison operators

    UInt256 copy1 = testUInt256_1_;
    UInt256 copy2 = testUInt256_1_;

    // Test equality
    EXPECT_TRUE(testUInt256_1_ == copy1);
    EXPECT_TRUE(copy1 == copy2);
    EXPECT_FALSE(testUInt256_1_ == testUInt256_2_);
    EXPECT_FALSE(zeroUInt256_ == maxUInt256_);

    // Test inequality
    EXPECT_FALSE(testUInt256_1_ != copy1);
    EXPECT_TRUE(testUInt256_1_ != testUInt256_2_);
    EXPECT_TRUE(zeroUInt256_ != maxUInt256_);

    // Test less than
    EXPECT_TRUE(zeroUInt256_ < maxUInt256_);
    EXPECT_FALSE(maxUInt256_ < zeroUInt256_);
    EXPECT_FALSE(testUInt256_1_ < copy1);  // Equal values

    // Test greater than
    EXPECT_TRUE(maxUInt256_ > zeroUInt256_);
    EXPECT_FALSE(zeroUInt256_ > maxUInt256_);
    EXPECT_FALSE(testUInt256_1_ > copy1);  // Equal values
}

TEST_F(UT_UInt256, ZeroOperations)
{
    // Test: Zero-related operations

    // Test Zero() static method
    UInt256 zero = UInt256::Zero();
    EXPECT_TRUE(zero.IsZero());
    EXPECT_EQ(zero, zeroUInt256_);

    // Test IsZero() with various values
    EXPECT_TRUE(zeroUInt256_.IsZero());
    EXPECT_FALSE(maxUInt256_.IsZero());
    EXPECT_FALSE(testUInt256_1_.IsZero());
    EXPECT_FALSE(testUInt256_2_.IsZero());
    EXPECT_FALSE(testUInt256_3_.IsZero());

    // Test default constructed UInt256
    UInt256 defaultConstructed;
    EXPECT_TRUE(defaultConstructed.IsZero());
    EXPECT_EQ(defaultConstructed, UInt256::Zero());
}

TEST_F(UT_UInt256, StringConversion)
{
    // Test: String conversion methods

    // Test ToHexString() and ToString()
    std::string hexString = testUInt256_1_.ToHexString();
    std::string toString = testUInt256_1_.ToString();

    EXPECT_EQ(hexString, toString);                    // ToString() should call ToHexString()
    EXPECT_EQ(hexString.length(), UInt256::Size * 2);  // 2 hex chars per byte

    // Verify hex string contains only valid hex characters
    for (char c : hexString)
    {
        EXPECT_TRUE(std::isxdigit(c)) << "Invalid hex character: " << c;
    }

    // Test with zero value
    std::string zeroHex = zeroUInt256_.ToHexString();
    std::string expectedZero(UInt256::Size * 2, '0');
    EXPECT_EQ(zeroHex, expectedZero);

    // Test with max value
    std::string maxHex = maxUInt256_.ToHexString();
    std::string expectedMax(UInt256::Size * 2, 'f');
    EXPECT_EQ(maxHex, expectedMax);
}

TEST_F(UT_UInt256, Parsing)
{
    // Test: Parse and TryParse methods

    // Test Parse with valid hex strings
    std::string testHex = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
    UInt256 parsed = UInt256::Parse(testHex);

    std::string parsedBack = parsed.ToHexString();
    EXPECT_EQ(parsedBack, testHex);

    // Test Parse with 0x prefix
    std::string prefixedHex = "0x" + testHex;
    UInt256 parsedPrefixed = UInt256::Parse(prefixedHex);
    EXPECT_EQ(parsed, parsedPrefixed);

    // Test TryParse with valid string
    UInt256 result;
    EXPECT_TRUE(UInt256::TryParse(testHex, result));
    EXPECT_EQ(result, parsed);

    // Test TryParse with invalid strings
    UInt256 invalidResult;
    EXPECT_FALSE(UInt256::TryParse("invalid", invalidResult));
    EXPECT_FALSE(UInt256::TryParse("123", invalidResult));           // Too short
    EXPECT_FALSE(UInt256::TryParse(testHex + "00", invalidResult));  // Too long

    // Test Parse with invalid string (should throw)
    EXPECT_THROW(UInt256::Parse("invalid"), std::invalid_argument);
    EXPECT_THROW(UInt256::Parse("123"), std::invalid_argument);
}

TEST_F(UT_UInt256, Serialization)
{
    // Test: Binary serialization and deserialization

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);

    // Serialize test value
    testUInt256_1_.Serialize(writer);

    // Reset stream position
    stream.seekg(0);

    // Deserialize
    BinaryReader reader(stream);
    UInt256 deserialized;
    deserialized.Deserialize(reader);

    // Verify
    EXPECT_EQ(deserialized, testUInt256_1_);

    // Test serialization of different values
    std::vector<UInt256> testValues = {zeroUInt256_, maxUInt256_, testUInt256_2_, testUInt256_3_};

    for (const auto& value : testValues)
    {
        std::stringstream testStream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter testWriter(testStream);

        value.Serialize(testWriter);

        testStream.seekg(0);
        BinaryReader testReader(testStream);
        UInt256 testDeserialized;
        testDeserialized.Deserialize(testReader);

        EXPECT_EQ(testDeserialized, value);
    }
}

TEST_F(UT_UInt256, FromStringMethods)
{
    // Test: FromString and FromLittleEndianString methods

    std::string testHex = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";

    // Test FromString
    UInt256 fromString = UInt256::FromString(testHex);
    EXPECT_EQ(fromString.ToHexString(), testHex);

    // Test FromString with 0x prefix
    UInt256 fromStringPrefixed = UInt256::FromString("0x" + testHex);
    EXPECT_EQ(fromString, fromStringPrefixed);

    // Test FromLittleEndianString
    UInt256 fromLittleEndian = UInt256::FromLittleEndianString(testHex);

    // Little endian should be different from big endian (unless palindrome)
    if (testHex != std::string(testHex.rbegin(), testHex.rend()))
    {
        EXPECT_NE(fromString, fromLittleEndian);
    }
}

TEST_F(UT_UInt256, LittleEndianString)
{
    // Test: ToLittleEndianString method

    // Create a non-palindrome test value
    UInt256::value_type testData;
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        testData[i] = static_cast<uint8_t>(i);
    }
    UInt256 testValue(testData);

    std::string littleEndianStr = testValue.ToLittleEndianString();
    std::string bigEndianStr = testValue.ToHexString();

    // They should be different for non-palindrome values
    EXPECT_NE(littleEndianStr, bigEndianStr);

    // Test round-trip
    UInt256 roundTrip = UInt256::FromLittleEndianString(littleEndianStr);
    EXPECT_EQ(roundTrip, testValue);
}

TEST_F(UT_UInt256, HashFunction)
{
    // Test: std::hash specialization

    std::hash<UInt256> hasher;

    // Test hash calculation
    size_t hash1 = hasher(testUInt256_1_);
    size_t hash2 = hasher(testUInt256_2_);
    size_t hash3 = hasher(testUInt256_1_);  // Same as hash1

    // Same values should have same hash
    EXPECT_EQ(hash1, hash3);

    // Different values should generally have different hashes
    EXPECT_NE(hash1, hash2);

    // Test with zero and max values
    size_t zeroHash = hasher(zeroUInt256_);
    size_t maxHash = hasher(maxUInt256_);

    EXPECT_NE(zeroHash, maxHash);

    // Test hash is usable in unordered containers
    std::unordered_set<UInt256> hashSet;
    hashSet.insert(testUInt256_1_);
    hashSet.insert(testUInt256_2_);
    hashSet.insert(testUInt256_3_);

    EXPECT_EQ(hashSet.size(), 3u);
    EXPECT_TRUE(hashSet.find(testUInt256_1_) != hashSet.end());
    EXPECT_TRUE(hashSet.find(testUInt256_2_) != hashSet.end());
    EXPECT_TRUE(hashSet.find(testUInt256_3_) != hashSet.end());
}

TEST_F(UT_UInt256, EdgeCases)
{
    // Test: Edge cases and boundary conditions

    // Test with all bytes set to different values
    UInt256::value_type uniqueData;
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        uniqueData[i] = static_cast<uint8_t>(i * 8 % 256);
    }
    UInt256 uniqueValue(uniqueData);

    // Verify each byte is set correctly
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(uniqueValue[i], static_cast<uint8_t>(i * 8 % 256));
    }

    // Test comparison with single bit differences
    UInt256::value_type almostZero;
    almostZero.fill(0x00);
    almostZero[31] = 0x01;  // Only last bit set
    UInt256 almostZeroValue(almostZero);

    EXPECT_FALSE(almostZeroValue.IsZero());
    EXPECT_TRUE(almostZeroValue > zeroUInt256_);
    EXPECT_TRUE(almostZeroValue < maxUInt256_);

    // Test with alternating patterns
    UInt256::value_type pattern1, pattern2;
    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        pattern1[i] = (i % 2 == 0) ? 0xFF : 0x00;
        pattern2[i] = (i % 2 == 0) ? 0x00 : 0xFF;
    }
    UInt256 value1(pattern1), value2(pattern2);

    EXPECT_NE(value1, value2);
    EXPECT_NE(value1.ToHexString(), value2.ToHexString());
}

TEST_F(UT_UInt256, MemoryLayout)
{
    // Test: Verify memory layout and data integrity

    UInt256 testValue = testUInt256_3_;

    // Verify data is stored contiguously
    const uint8_t* data = testValue.Data();
    const auto& dataArray = testValue.GetData();

    for (size_t i = 0; i < UInt256::Size; ++i)
    {
        EXPECT_EQ(data[i], dataArray[i]);
        EXPECT_EQ(data[i], testValue[i]);
        EXPECT_EQ(data[i], static_cast<uint8_t>(i));
    }

    // Verify ByteSpan points to same memory
    ByteSpan span = testValue.AsSpan();
    EXPECT_EQ(span.Data(), data);
    EXPECT_EQ(span.Size(), UInt256::Size);

    // Modify via pointer and verify changes are visible
    uint8_t* mutableData = testValue.Data();
    mutableData[0] = 0xFF;

    EXPECT_EQ(testValue[0], 0xFF);
    EXPECT_EQ(testValue.GetData()[0], 0xFF);
    EXPECT_EQ(testValue.AsSpan()[0], 0xFF);
}

TEST_F(UT_UInt256, Performance)
{
    // Test: Basic performance characteristics

    const size_t iterations = 1000;

    // Test construction performance
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i)
    {
        UInt256 temp;
        temp[0] = static_cast<uint8_t>(i);  // Prevent optimization
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto constructionTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should be fast (less than 10ms for 1000 constructions)
    EXPECT_LT(constructionTime.count(), 10000);

    // Test comparison performance
    start = std::chrono::high_resolution_clock::now();
    bool result = false;
    for (size_t i = 0; i < iterations; ++i)
    {
        result = (testUInt256_1_ == testUInt256_2_);
    }
    end = std::chrono::high_resolution_clock::now();
    auto comparisonTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should be fast (less than 5ms for 1000 comparisons)
    EXPECT_LT(comparisonTime.count(), 5000);
    EXPECT_FALSE(result);  // Prevent optimization

    // Test serialization performance
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    BinaryWriter writer(stream);

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i)
    {
        stream.seekp(0);
        testUInt256_1_.Serialize(writer);
    }
    end = std::chrono::high_resolution_clock::now();
    auto serializationTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should be fast (less than 10ms for 1000 serializations)
    EXPECT_LT(serializationTime.count(), 10000);
}