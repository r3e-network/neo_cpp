#include <gtest/gtest.h>
#include <neo/cryptography/bloom_filter.h>
#include <neo/io/byte_vector.h>
#include <string>
#include <vector>
#include <stdexcept>

using namespace neo::cryptography;
using namespace neo::io;

TEST(BloomFilterTest, Constructor)
{
    // Test case 1: Default constructor
    BloomFilter filter1;
    EXPECT_EQ(filter1.GetK(), 0);
    EXPECT_EQ(filter1.GetM(), 0);
    EXPECT_EQ(filter1.GetN(), 0);
    EXPECT_EQ(filter1.GetFilter().Size(), 0);
    
    // Test case 2: Constructor with parameters
    BloomFilter filter2(10, 0.01);
    EXPECT_GT(filter2.GetK(), 0);
    EXPECT_GT(filter2.GetM(), 0);
    EXPECT_EQ(filter2.GetN(), 0);
    EXPECT_GT(filter2.GetFilter().Size(), 0);
    
    // Test case 3: Constructor with filter
    ByteVector filter = { 0x01, 0x02, 0x03, 0x04 };
    BloomFilter filter3(filter.AsSpan(), 3);
    EXPECT_EQ(filter3.GetK(), 3);
    EXPECT_EQ(filter3.GetM(), 32); // 4 bytes * 8 bits
    EXPECT_EQ(filter3.GetN(), 0);
    EXPECT_EQ(filter3.GetFilter(), filter);
}

TEST(BloomFilterTest, Add)
{
    // Create a bloom filter
    BloomFilter filter(10, 0.01);
    
    // Test case 1: Add a single item
    ByteVector item1 = { 0x01, 0x02, 0x03, 0x04 };
    filter.Add(item1.AsSpan());
    EXPECT_EQ(filter.GetN(), 1);
    
    // Test case 2: Add another item
    ByteVector item2 = { 0x05, 0x06, 0x07, 0x08 };
    filter.Add(item2.AsSpan());
    EXPECT_EQ(filter.GetN(), 2);
    
    // Test case 3: Add the same item again
    filter.Add(item1.AsSpan());
    EXPECT_EQ(filter.GetN(), 3); // Bloom filter doesn't detect duplicates
}

TEST(BloomFilterTest, Contains)
{
    // Create a bloom filter
    BloomFilter filter(10, 0.01);
    
    // Test case 1: Empty filter
    ByteVector item1 = { 0x01, 0x02, 0x03, 0x04 };
    EXPECT_FALSE(filter.Contains(item1.AsSpan()));
    
    // Test case 2: Add an item and check if it's contained
    filter.Add(item1.AsSpan());
    EXPECT_TRUE(filter.Contains(item1.AsSpan()));
    
    // Test case 3: Check if a different item is contained
    ByteVector item2 = { 0x05, 0x06, 0x07, 0x08 };
    EXPECT_FALSE(filter.Contains(item2.AsSpan()));
    
    // Test case 4: Add the different item and check again
    filter.Add(item2.AsSpan());
    EXPECT_TRUE(filter.Contains(item2.AsSpan()));
}

TEST(BloomFilterTest, Serialization)
{
    // Create a bloom filter
    BloomFilter filter1(10, 0.01);
    
    // Add some items
    ByteVector item1 = { 0x01, 0x02, 0x03, 0x04 };
    ByteVector item2 = { 0x05, 0x06, 0x07, 0x08 };
    filter1.Add(item1.AsSpan());
    filter1.Add(item2.AsSpan());
    
    // Serialize the filter
    ByteVector serialized = filter1.ToArray();
    
    // Deserialize the filter
    BloomFilter filter2;
    BinaryReader reader(serialized.AsSpan());
    filter2.Deserialize(reader);
    
    // Check if the deserialized filter is the same as the original
    EXPECT_EQ(filter2.GetK(), filter1.GetK());
    EXPECT_EQ(filter2.GetM(), filter1.GetM());
    EXPECT_EQ(filter2.GetN(), filter1.GetN());
    EXPECT_EQ(filter2.GetFilter(), filter1.GetFilter());
    
    // Check if the deserialized filter contains the same items
    EXPECT_TRUE(filter2.Contains(item1.AsSpan()));
    EXPECT_TRUE(filter2.Contains(item2.AsSpan()));
}

TEST(BloomFilterTest, FalsePositives)
{
    // Create a bloom filter with a known false positive rate
    double falsePositiveRate = 0.01;
    int itemCount = 100;
    BloomFilter filter(itemCount, falsePositiveRate);
    
    // Add items to the filter
    std::vector<ByteVector> items;
    for (int i = 0; i < itemCount; i++)
    {
        ByteVector item(4);
        item[0] = static_cast<uint8_t>((i >> 24) & 0xFF);
        item[1] = static_cast<uint8_t>((i >> 16) & 0xFF);
        item[2] = static_cast<uint8_t>((i >> 8) & 0xFF);
        item[3] = static_cast<uint8_t>(i & 0xFF);
        items.push_back(item);
        filter.Add(item.AsSpan());
    }
    
    // Check if all added items are contained
    for (const auto& item : items)
    {
        EXPECT_TRUE(filter.Contains(item.AsSpan()));
    }
    
    // Check false positive rate
    int falsePositives = 0;
    int testCount = 10000;
    for (int i = itemCount; i < itemCount + testCount; i++)
    {
        ByteVector item(4);
        item[0] = static_cast<uint8_t>((i >> 24) & 0xFF);
        item[1] = static_cast<uint8_t>((i >> 16) & 0xFF);
        item[2] = static_cast<uint8_t>((i >> 8) & 0xFF);
        item[3] = static_cast<uint8_t>(i & 0xFF);
        if (filter.Contains(item.AsSpan()))
        {
            falsePositives++;
        }
    }
    
    // The actual false positive rate should be close to the expected rate
    double actualFalsePositiveRate = static_cast<double>(falsePositives) / testCount;
    EXPECT_LT(actualFalsePositiveRate, falsePositiveRate * 2); // Allow some margin
}

TEST(BloomFilterTest, OptimalParameters)
{
    // Test case 1: n = 10, p = 0.01
    auto params1 = BloomFilter::OptimalParameters(10, 0.01);
    EXPECT_GT(params1.first, 0); // m
    EXPECT_GT(params1.second, 0); // k
    
    // Test case 2: n = 100, p = 0.001
    auto params2 = BloomFilter::OptimalParameters(100, 0.001);
    EXPECT_GT(params2.first, params1.first); // m should be larger
    EXPECT_GT(params2.second, params1.second); // k should be larger
    
    // Test case 3: n = 1000, p = 0.0001
    auto params3 = BloomFilter::OptimalParameters(1000, 0.0001);
    EXPECT_GT(params3.first, params2.first); // m should be larger
    EXPECT_GT(params3.second, params2.second); // k should be larger
}

TEST(BloomFilterTest, InvalidParameters)
{
    // Test case 1: n = 0
    EXPECT_THROW(BloomFilter::OptimalParameters(0, 0.01), std::invalid_argument);
    
    // Test case 2: p = 0
    EXPECT_THROW(BloomFilter::OptimalParameters(10, 0), std::invalid_argument);
    
    // Test case 3: p = 1
    EXPECT_THROW(BloomFilter::OptimalParameters(10, 1), std::invalid_argument);
    
    // Test case 4: p > 1
    EXPECT_THROW(BloomFilter::OptimalParameters(10, 1.1), std::invalid_argument);
    
    // Test case 5: p < 0
    EXPECT_THROW(BloomFilter::OptimalParameters(10, -0.1), std::invalid_argument);
}
