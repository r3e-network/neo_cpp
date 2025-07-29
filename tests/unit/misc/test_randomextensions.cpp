#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <neo/extensions/random_extensions.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <set>

using namespace neo::extensions;
using namespace neo::io;

/**
 * @brief Test fixture for RandomExtensions
 */
class RandomExtensionsTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // No specific setup needed
    }
};

TEST_F(RandomExtensionsTest, GenerateRandomBytes)
{
    // Test various lengths
    for (size_t length : {0, 1, 16, 32, 64, 256})
    {
        auto bytes = RandomExtensions::GenerateRandomBytes(length);
        EXPECT_EQ(length, bytes.Size());
    }

    // Test that different calls produce different results
    auto bytes1 = RandomExtensions::GenerateRandomBytes(32);
    auto bytes2 = RandomExtensions::GenerateRandomBytes(32);
    EXPECT_NE(bytes1, bytes2);

    // Test that bytes are not all zeros (extremely unlikely)
    auto bytes3 = RandomExtensions::GenerateRandomBytes(32);
    bool hasNonZero = false;
    for (size_t i = 0; i < bytes3.Size(); ++i)
    {
        if (bytes3[i] != 0)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST_F(RandomExtensionsTest, NextInt)
{
    // Test range [min, max]
    for (int i = 0; i < 100; ++i)
    {
        int value = RandomExtensions::NextInt(10, 20);
        EXPECT_GE(value, 10);
        EXPECT_LE(value, 20);
    }

    // Test range [0, max]
    for (int i = 0; i < 100; ++i)
    {
        int value = RandomExtensions::NextInt(50);
        EXPECT_GE(value, 0);
        EXPECT_LE(value, 50);
    }

    // Test full range
    int fullRange = RandomExtensions::NextInt();
    EXPECT_GE(fullRange, std::numeric_limits<int32_t>::min());
    EXPECT_LE(fullRange, std::numeric_limits<int32_t>::max());

    // Test edge cases
    EXPECT_EQ(5, RandomExtensions::NextInt(5, 5));
    EXPECT_EQ(0, RandomExtensions::NextInt(0, 0));
}

TEST_F(RandomExtensionsTest, NextUInt)
{
    // Test range [min, max]
    for (int i = 0; i < 100; ++i)
    {
        uint32_t value = RandomExtensions::NextUInt(100, 200);
        EXPECT_GE(value, 100u);
        EXPECT_LE(value, 200u);
    }

    // Test range [0, max]
    for (int i = 0; i < 100; ++i)
    {
        uint32_t value = RandomExtensions::NextUInt(1000);
        EXPECT_LE(value, 1000u);
    }

    // Test full range
    uint32_t fullRange = RandomExtensions::NextUInt();
    EXPECT_LE(fullRange, std::numeric_limits<uint32_t>::max());
}

TEST_F(RandomExtensionsTest, NextLong)
{
    // Test range [min, max]
    for (int i = 0; i < 100; ++i)
    {
        int64_t value = RandomExtensions::NextLong(-1000, 1000);
        EXPECT_GE(value, -1000);
        EXPECT_LE(value, 1000);
    }

    // Test full range
    int64_t fullRange = RandomExtensions::NextLong();
    EXPECT_GE(fullRange, std::numeric_limits<int64_t>::min());
    EXPECT_LE(fullRange, std::numeric_limits<int64_t>::max());
}

TEST_F(RandomExtensionsTest, NextULong)
{
    // Test range [min, max]
    for (int i = 0; i < 100; ++i)
    {
        uint64_t value = RandomExtensions::NextULong(10000, 20000);
        EXPECT_GE(value, 10000u);
        EXPECT_LE(value, 20000u);
    }

    // Test full range
    uint64_t fullRange = RandomExtensions::NextULong();
    EXPECT_LE(fullRange, std::numeric_limits<uint64_t>::max());
}

TEST_F(RandomExtensionsTest, NextFloat)
{
    // Test range [0, 1)
    for (int i = 0; i < 100; ++i)
    {
        float value = RandomExtensions::NextFloat();
        EXPECT_GE(value, 0.0f);
        EXPECT_LT(value, 1.0f);
    }

    // Test custom range
    for (int i = 0; i < 100; ++i)
    {
        float value = RandomExtensions::NextFloat(10.0f, 20.0f);
        EXPECT_GE(value, 10.0f);
        EXPECT_LT(value, 20.0f);
    }
}

TEST_F(RandomExtensionsTest, NextDouble)
{
    // Test range [0, 1)
    for (int i = 0; i < 100; ++i)
    {
        double value = RandomExtensions::NextDouble();
        EXPECT_GE(value, 0.0);
        EXPECT_LT(value, 1.0);
    }

    // Test custom range
    for (int i = 0; i < 100; ++i)
    {
        double value = RandomExtensions::NextDouble(-5.0, 5.0);
        EXPECT_GE(value, -5.0);
        EXPECT_LT(value, 5.0);
    }
}

TEST_F(RandomExtensionsTest, NextBool)
{
    // Test basic NextBool
    int trueCount = 0;
    int falseCount = 0;
    for (int i = 0; i < 1000; ++i)
    {
        if (RandomExtensions::NextBool())
        {
            trueCount++;
        }
        else
        {
            falseCount++;
        }
    }

    // Both should occur (extremely unlikely to get all true or all false)
    EXPECT_GT(trueCount, 0);
    EXPECT_GT(falseCount, 0);

    // Test with probability
    int probabilityTrueCount = 0;
    for (int i = 0; i < 1000; ++i)
    {
        if (RandomExtensions::NextBool(0.8))
        {
            probabilityTrueCount++;
        }
    }

    // Should be roughly 80% true (allow for variance)
    EXPECT_GT(probabilityTrueCount, 700);
    EXPECT_LT(probabilityTrueCount, 900);
}

TEST_F(RandomExtensionsTest, GenerateRandomUInt160)
{
    // Generate multiple and check they're different
    auto hash1 = RandomExtensions::GenerateRandomUInt160();
    auto hash2 = RandomExtensions::GenerateRandomUInt160();
    EXPECT_NE(hash1, hash2);

    // Check size
    EXPECT_EQ(20u, hash1.Size());
}

TEST_F(RandomExtensionsTest, GenerateRandomUInt256)
{
    // Generate multiple and check they're different
    auto hash1 = RandomExtensions::GenerateRandomUInt256();
    auto hash2 = RandomExtensions::GenerateRandomUInt256();
    EXPECT_NE(hash1, hash2);

    // Check size
    EXPECT_EQ(32u, hash1.Size());
}

TEST_F(RandomExtensionsTest, GenerateRandomString)
{
    // Test various lengths
    for (size_t length : {0, 1, 10, 50, 100})
    {
        auto str = RandomExtensions::GenerateRandomString(length);
        EXPECT_EQ(length, str.length());

        // Check all characters are alphanumeric
        for (char c : str)
        {
            EXPECT_TRUE(std::isalnum(c));
        }
    }

    // Test uniqueness
    auto str1 = RandomExtensions::GenerateRandomString(20);
    auto str2 = RandomExtensions::GenerateRandomString(20);
    EXPECT_NE(str1, str2);
}

TEST_F(RandomExtensionsTest, GenerateRandomHexString)
{
    // Test various lengths
    for (size_t length : {0, 2, 16, 32, 64})
    {
        auto hex = RandomExtensions::GenerateRandomHexString(length);
        EXPECT_EQ(length, hex.length());

        // Check all characters are hexadecimal
        for (char c : hex)
        {
            EXPECT_TRUE(std::isxdigit(c));
        }
    }

    // Test uniqueness
    auto hex1 = RandomExtensions::GenerateRandomHexString(32);
    auto hex2 = RandomExtensions::GenerateRandomHexString(32);
    EXPECT_NE(hex1, hex2);
}

TEST_F(RandomExtensionsTest, Shuffle)
{
    std::vector<int> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> shuffled = original;

    RandomExtensions::Shuffle(shuffled);

    // Check same elements exist
    EXPECT_EQ(original.size(), shuffled.size());
    std::sort(shuffled.begin(), shuffled.end());
    EXPECT_EQ(original, shuffled);

    // Test multiple shuffles produce different results
    std::vector<int> vec1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> vec2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    RandomExtensions::Shuffle(vec1);
    RandomExtensions::Shuffle(vec2);
    EXPECT_NE(vec1, vec2);  // Extremely unlikely to be equal
}

TEST_F(RandomExtensionsTest, SelectRandom)
{
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::set<int> selected;

    // Select multiple times and check all values can be selected
    for (int i = 0; i < 100; ++i)
    {
        int value = RandomExtensions::SelectRandom(vec);
        EXPECT_TRUE(std::find(vec.begin(), vec.end(), value) != vec.end());
        selected.insert(value);
    }

    // All values should have been selected at least once
    EXPECT_EQ(vec.size(), selected.size());

    // Test empty vector throws
    std::vector<int> empty;
    EXPECT_THROW(RandomExtensions::SelectRandom(empty), std::invalid_argument);
}

TEST_F(RandomExtensionsTest, SelectRandomMultiple)
{
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Test selecting subset
    auto selected = RandomExtensions::SelectRandomMultiple(vec, 5);
    EXPECT_EQ(5u, selected.size());

    // Check all selected values are from original
    for (int value : selected)
    {
        EXPECT_TRUE(std::find(vec.begin(), vec.end(), value) != vec.end());
    }

    // Check no duplicates
    std::set<int> uniqueSet(selected.begin(), selected.end());
    EXPECT_EQ(selected.size(), uniqueSet.size());

    // Test selecting all
    auto all = RandomExtensions::SelectRandomMultiple(vec, vec.size());
    EXPECT_EQ(vec.size(), all.size());

    // Test error on selecting more than available
    EXPECT_THROW(RandomExtensions::SelectRandomMultiple(vec, 20), std::invalid_argument);
}

TEST_F(RandomExtensionsTest, GenerateNonce)
{
    // Generate multiple nonces and check they're different
    std::set<uint32_t> nonces;
    for (int i = 0; i < 100; ++i)
    {
        nonces.insert(RandomExtensions::GenerateNonce());
    }

    // Should have many different values (extremely unlikely to have duplicates)
    EXPECT_GT(nonces.size(), 90u);
}

TEST_F(RandomExtensionsTest, GenerateRandomTimestamp)
{
    // Test with default parameters
    uint64_t currentTime = 1700000000;  // Some arbitrary timestamp

    for (int i = 0; i < 100; ++i)
    {
        uint64_t timestamp = RandomExtensions::GenerateRandomTimestamp(currentTime, 3600);
        EXPECT_GE(timestamp, currentTime - 3600);
        EXPECT_LE(timestamp, currentTime + 3600);
    }

    // Test with custom variation
    for (int i = 0; i < 100; ++i)
    {
        uint64_t timestamp = RandomExtensions::GenerateRandomTimestamp(currentTime, 60);
        EXPECT_GE(timestamp, currentTime - 60);
        EXPECT_LE(timestamp, currentTime + 60);
    }
}
