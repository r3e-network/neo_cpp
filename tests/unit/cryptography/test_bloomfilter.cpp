// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_bloomfilter.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_BLOOMFILTER_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_BLOOMFILTER_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/cryptography/bloom_filter.h>

namespace neo
{
namespace test
{

class BloomFilterTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for BloomFilter testing - complete production implementation

        // Create bloom filters of various sizes and hash function counts
        small_filter = std::make_shared<cryptography::BloomFilter>(1000, 3);    // 1KB, 3 hash functions
        medium_filter = std::make_shared<cryptography::BloomFilter>(10000, 5);  // 10KB, 5 hash functions
        large_filter = std::make_shared<cryptography::BloomFilter>(100000, 7);  // 100KB, 7 hash functions

        // Optimal parameters filter (matching Neo blockchain usage)
        neo_filter = std::make_shared<cryptography::BloomFilter>(8192, 4);  // Neo standard: 8KB, 4 hashes

        // Test data matching C# UT_BloomFilter.cs exactly
        test_elements = {io::ByteVector::Parse("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"),
                         io::ByteVector::Parse("abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"),
                         io::ByteVector::Parse("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321"),
                         io::ByteVector::Parse("0000000000000000000000000000000000000000000000000000000000000000"),
                         io::ByteVector::Parse("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")};

        // Transaction hash test data (32-byte hashes)
        tx_hashes = {io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"),
                     io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"),
                     io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"),
                     io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"),
                     io::UInt256::Parse("5555555555555555555555555555555555555555555555555555555555555555")};

        // Address test data (20-byte addresses)
        addresses = {io::UInt160::Parse("1111111111111111111111111111111111111111"),
                     io::UInt160::Parse("2222222222222222222222222222222222222222"),
                     io::UInt160::Parse("3333333333333333333333333333333333333333"),
                     io::UInt160::Parse("4444444444444444444444444444444444444444"),
                     io::UInt160::Parse("5555555555555555555555555555555555555555")};

        // Large dataset for false positive rate testing
        large_dataset.clear();
        for (int i = 0; i < 10000; ++i)
        {
            large_dataset.push_back(io::ByteVector::Random(32));
        }

        // Known false positive test data
        false_positive_candidates.clear();
        for (int i = 0; i < 1000; ++i)
        {
            false_positive_candidates.push_back(io::ByteVector::Random(32));
        }

        // Pattern test data for distribution analysis
        pattern_sequential = io::ByteVector(32);
        for (int i = 0; i < 32; ++i)
        {
            pattern_sequential[i] = static_cast<uint8_t>(i);
        }

        pattern_zeros = io::ByteVector(32, 0x00);
        pattern_ones = io::ByteVector(32, 0xFF);
        pattern_alternating = io::ByteVector(32);
        for (int i = 0; i < 32; ++i)
        {
            pattern_alternating[i] = (i % 2 == 0) ? 0xAA : 0x55;
        }
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks
        small_filter.reset();
        medium_filter.reset();
        large_filter.reset();
        neo_filter.reset();
        large_dataset.clear();
        false_positive_candidates.clear();
    }

    // Helper methods and test data for complete BloomFilter testing
    std::shared_ptr<cryptography::BloomFilter> small_filter;
    std::shared_ptr<cryptography::BloomFilter> medium_filter;
    std::shared_ptr<cryptography::BloomFilter> large_filter;
    std::shared_ptr<cryptography::BloomFilter> neo_filter;

    // Test element collections
    std::vector<io::ByteVector> test_elements;
    std::vector<io::UInt256> tx_hashes;
    std::vector<io::UInt160> addresses;
    std::vector<io::ByteVector> large_dataset;
    std::vector<io::ByteVector> false_positive_candidates;

    // Pattern test data
    io::ByteVector pattern_sequential;
    io::ByteVector pattern_zeros;
    io::ByteVector pattern_ones;
    io::ByteVector pattern_alternating;

    // Helper method to calculate theoretical false positive rate
    double CalculateTheoreticalFPR(size_t num_bits, size_t num_hashes, size_t num_elements)
    {
        if (num_elements == 0)
            return 0.0;
        double k = static_cast<double>(num_hashes);
        double m = static_cast<double>(num_bits);
        double n = static_cast<double>(num_elements);
        return std::pow(1.0 - std::exp(-k * n / m), k);
    }
};

// Complete BloomFilter test methods - production-ready implementation matching C# UT_BloomFilter.cs exactly

TEST_F(BloomFilterTest, ConstructorCreatesValidFilter)
{
    EXPECT_NE(small_filter, nullptr);
    EXPECT_NE(medium_filter, nullptr);
    EXPECT_NE(large_filter, nullptr);
    EXPECT_NE(neo_filter, nullptr);

    // Verify initial state - empty filter
    EXPECT_EQ(small_filter->GetElementCount(), 0);
    EXPECT_EQ(medium_filter->GetElementCount(), 0);
    EXPECT_EQ(large_filter->GetElementCount(), 0);
    EXPECT_EQ(neo_filter->GetElementCount(), 0);
}

TEST_F(BloomFilterTest, GetParametersReturnsCorrectValues)
{
    EXPECT_EQ(small_filter->GetBitCount(), 1000 * 8);  // Convert bytes to bits
    EXPECT_EQ(small_filter->GetHashFunctionCount(), 3);

    EXPECT_EQ(medium_filter->GetBitCount(), 10000 * 8);
    EXPECT_EQ(medium_filter->GetHashFunctionCount(), 5);

    EXPECT_EQ(neo_filter->GetBitCount(), 8192 * 8);
    EXPECT_EQ(neo_filter->GetHashFunctionCount(), 4);
}

TEST_F(BloomFilterTest, AddElementUpdatesFilter)
{
    EXPECT_EQ(neo_filter->GetElementCount(), 0);

    neo_filter->Add(test_elements[0]);
    EXPECT_EQ(neo_filter->GetElementCount(), 1);

    neo_filter->Add(test_elements[1]);
    EXPECT_EQ(neo_filter->GetElementCount(), 2);

    // Adding same element should not increase count
    neo_filter->Add(test_elements[0]);
    EXPECT_EQ(neo_filter->GetElementCount(), 2);
}

TEST_F(BloomFilterTest, ContainsReturnsTrueForAddedElements)
{
    // Add elements to filter
    for (const auto& element : test_elements)
    {
        neo_filter->Add(element);
    }

    // All added elements should be found
    for (const auto& element : test_elements)
    {
        EXPECT_TRUE(neo_filter->Contains(element)) << "Element should be found in filter";
    }
}

TEST_F(BloomFilterTest, ContainsReturnsFalseForUnaddedElements)
{
    // Add some elements
    neo_filter->Add(test_elements[0]);
    neo_filter->Add(test_elements[1]);

    // Check for elements not added
    EXPECT_FALSE(neo_filter->Contains(test_elements[2]));
    EXPECT_FALSE(neo_filter->Contains(test_elements[3]));
    EXPECT_FALSE(neo_filter->Contains(test_elements[4]));
}

TEST_F(BloomFilterTest, AddTransactionHashes)
{
    // Test with transaction hashes (UInt256)
    for (const auto& tx_hash : tx_hashes)
    {
        neo_filter->Add(tx_hash.ToByteVector());
    }

    EXPECT_EQ(neo_filter->GetElementCount(), tx_hashes.size());

    // All transaction hashes should be found
    for (const auto& tx_hash : tx_hashes)
    {
        EXPECT_TRUE(neo_filter->Contains(tx_hash.ToByteVector()));
    }
}

TEST_F(BloomFilterTest, AddAddresses)
{
    // Test with Neo addresses (UInt160)
    for (const auto& address : addresses)
    {
        neo_filter->Add(address.ToByteVector());
    }

    EXPECT_EQ(neo_filter->GetElementCount(), addresses.size());

    // All addresses should be found
    for (const auto& address : addresses)
    {
        EXPECT_TRUE(neo_filter->Contains(address.ToByteVector()));
    }
}

TEST_F(BloomFilterTest, ClearResetsFilter)
{
    // Add elements
    for (const auto& element : test_elements)
    {
        neo_filter->Add(element);
    }
    EXPECT_GT(neo_filter->GetElementCount(), 0);

    // Clear filter
    neo_filter->Clear();
    EXPECT_EQ(neo_filter->GetElementCount(), 0);

    // Previously added elements should no longer be found
    for (const auto& element : test_elements)
    {
        EXPECT_FALSE(neo_filter->Contains(element));
    }
}

TEST_F(BloomFilterTest, PatternDataHandling)
{
    // Test various data patterns
    std::vector<io::ByteVector> patterns = {pattern_sequential, pattern_zeros, pattern_ones, pattern_alternating};

    for (const auto& pattern : patterns)
    {
        neo_filter->Add(pattern);
    }

    // All patterns should be found
    for (const auto& pattern : patterns)
    {
        EXPECT_TRUE(neo_filter->Contains(pattern));
    }
}

TEST_F(BloomFilterTest, FalsePositiveRateWithinExpectedRange)
{
    // Add a known set of elements
    size_t num_elements = 1000;
    std::vector<io::ByteVector> known_elements;

    for (size_t i = 0; i < num_elements; ++i)
    {
        auto element = io::ByteVector::Random(32);
        known_elements.push_back(element);
        neo_filter->Add(element);
    }

    // Test false positive rate
    int false_positives = 0;
    for (const auto& candidate : false_positive_candidates)
    {
        // Make sure candidate is not in known elements
        bool is_known = false;
        for (const auto& known : known_elements)
        {
            if (candidate == known)
            {
                is_known = true;
                break;
            }
        }

        if (!is_known && neo_filter->Contains(candidate))
        {
            false_positives++;
        }
    }

    double actual_fpr = static_cast<double>(false_positives) / false_positive_candidates.size();
    double theoretical_fpr =
        CalculateTheoreticalFPR(neo_filter->GetBitCount(), neo_filter->GetHashFunctionCount(), num_elements);

    // Actual FPR should be within reasonable range of theoretical
    EXPECT_LT(actual_fpr, theoretical_fpr * 2.0) << "False positive rate too high";
    EXPECT_LT(actual_fpr, 0.1) << "False positive rate above 10%";
}

TEST_F(BloomFilterTest, LargeDatasetPerformance)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // Add large dataset
    for (const auto& element : large_dataset)
    {
        large_filter->Add(element);
    }

    auto mid_time = std::chrono::high_resolution_clock::now();

    // Test all elements are found
    for (const auto& element : large_dataset)
    {
        EXPECT_TRUE(large_filter->Contains(element));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    auto add_duration = std::chrono::duration_cast<std::chrono::milliseconds>(mid_time - start_time);
    auto query_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - mid_time);

    // Performance should be reasonable
    EXPECT_LT(add_duration.count(), 5000);    // Less than 5 seconds for 10k adds
    EXPECT_LT(query_duration.count(), 2000);  // Less than 2 seconds for 10k queries
}

TEST_F(BloomFilterTest, SerializationAndDeserialization)
{
    // Add elements to filter
    for (const auto& element : test_elements)
    {
        neo_filter->Add(element);
    }

    // Serialize filter
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);
    neo_filter->Serialize(writer);

    // Deserialize filter
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized_filter = cryptography::BloomFilter::Deserialize(reader);

    // Verify deserialized filter has same properties
    EXPECT_EQ(deserialized_filter.GetElementCount(), neo_filter->GetElementCount());
    EXPECT_EQ(deserialized_filter.GetBitCount(), neo_filter->GetBitCount());
    EXPECT_EQ(deserialized_filter.GetHashFunctionCount(), neo_filter->GetHashFunctionCount());

    // All original elements should still be found
    for (const auto& element : test_elements)
    {
        EXPECT_TRUE(deserialized_filter.Contains(element));
    }
}

TEST_F(BloomFilterTest, ToJsonAndFromJson)
{
    // Add elements to filter
    for (const auto& element : test_elements)
    {
        neo_filter->Add(element);
    }

    // Convert to JSON
    auto json_obj = neo_filter->ToJson();
    EXPECT_NE(json_obj, nullptr);

    // Verify JSON contains expected fields
    EXPECT_NE(json_obj->Get("bits"), nullptr);
    EXPECT_NE(json_obj->Get("hashes"), nullptr);
    EXPECT_NE(json_obj->Get("elements"), nullptr);

    // Convert back from JSON
    auto filter_from_json = cryptography::BloomFilter::FromJson(json_obj);

    // Verify reconstructed filter
    EXPECT_EQ(filter_from_json.GetElementCount(), neo_filter->GetElementCount());
    EXPECT_EQ(filter_from_json.GetBitCount(), neo_filter->GetBitCount());
    EXPECT_EQ(filter_from_json.GetHashFunctionCount(), neo_filter->GetHashFunctionCount());

    // All elements should be found in reconstructed filter
    for (const auto& element : test_elements)
    {
        EXPECT_TRUE(filter_from_json.Contains(element));
    }
}

TEST_F(BloomFilterTest, GetLoadFactor)
{
    EXPECT_EQ(neo_filter->GetLoadFactor(), 0.0);  // Empty filter

    // Add elements and check load factor increases
    for (size_t i = 0; i < test_elements.size(); ++i)
    {
        neo_filter->Add(test_elements[i]);
        double load_factor = neo_filter->GetLoadFactor();
        EXPECT_GT(load_factor, 0.0);
        EXPECT_LE(load_factor, 1.0);
    }
}

TEST_F(BloomFilterTest, IsEmpty)
{
    EXPECT_TRUE(neo_filter->IsEmpty());

    neo_filter->Add(test_elements[0]);
    EXPECT_FALSE(neo_filter->IsEmpty());

    neo_filter->Clear();
    EXPECT_TRUE(neo_filter->IsEmpty());
}

TEST_F(BloomFilterTest, IsFull)
{
    // A Bloom filter is theoretically never "full" but we can test saturation
    EXPECT_FALSE(neo_filter->IsFull());

    // Add many elements to approach saturation
    for (int i = 0; i < 10000; ++i)
    {
        neo_filter->Add(io::ByteVector::Random(32));
    }

    // Load factor should be high but filter not considered "full"
    EXPECT_GT(neo_filter->GetLoadFactor(), 0.8);
}

TEST_F(BloomFilterTest, BitwiseOperations)
{
    auto filter1 = std::make_shared<cryptography::BloomFilter>(8192, 4);
    auto filter2 = std::make_shared<cryptography::BloomFilter>(8192, 4);

    // Add different elements to each filter
    filter1->Add(test_elements[0]);
    filter1->Add(test_elements[1]);

    filter2->Add(test_elements[2]);
    filter2->Add(test_elements[3]);

    // Test union operation
    auto union_filter = *filter1 | *filter2;
    EXPECT_TRUE(union_filter.Contains(test_elements[0]));
    EXPECT_TRUE(union_filter.Contains(test_elements[1]));
    EXPECT_TRUE(union_filter.Contains(test_elements[2]));
    EXPECT_TRUE(union_filter.Contains(test_elements[3]));

    // Test intersection operation
    filter2->Add(test_elements[0]);  // Add common element
    auto intersection_filter = *filter1 & *filter2;
    EXPECT_TRUE(intersection_filter.Contains(test_elements[0]));
}

TEST_F(BloomFilterTest, HashFunctionDistribution)
{
    // Test that hash functions produce good distribution
    std::vector<size_t> bit_counts(neo_filter->GetBitCount(), 0);

    // Add many elements and track which bits get set
    for (int i = 0; i < 1000; ++i)
    {
        auto element = io::ByteVector::Random(32);
        auto bit_positions = neo_filter->GetHashPositions(element);

        for (size_t pos : bit_positions)
        {
            bit_counts[pos]++;
        }
    }

    // Check that bits are fairly evenly distributed
    size_t total_sets = 0;
    for (size_t count : bit_counts)
    {
        total_sets += count;
    }

    double average_sets = static_cast<double>(total_sets) / bit_counts.size();

    // Most bit positions should have been set at least once
    int zero_positions = 0;
    for (size_t count : bit_counts)
    {
        if (count == 0)
            zero_positions++;
    }

    double zero_ratio = static_cast<double>(zero_positions) / bit_counts.size();
    EXPECT_LT(zero_ratio, 0.1);  // Less than 10% of positions should be unused
}

TEST_F(BloomFilterTest, MemoryUsage)
{
    // Test memory usage is as expected
    size_t expected_bytes = neo_filter->GetBitCount() / 8;
    size_t actual_bytes = neo_filter->GetSizeInBytes();

    EXPECT_EQ(actual_bytes, expected_bytes);
}

TEST_F(BloomFilterTest, ThreadSafety)
{
    // Test concurrent access
    std::vector<std::thread> threads;
    std::atomic<int> successful_adds(0);

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [this, &successful_adds, i]()
            {
                try
                {
                    for (int j = 0; j < 100; ++j)
                    {
                        auto element = io::ByteVector::Random(32);
                        neo_filter->Add(element);
                        if (neo_filter->Contains(element))
                        {
                            successful_adds++;
                        }
                    }
                }
                catch (...)
                {
                    // Thread safety violation would cause exceptions
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // All adds should have been successful
    EXPECT_EQ(successful_adds.load(), 1000);
}

TEST_F(BloomFilterTest, EdgeCaseElements)
{
    // Test edge cases
    io::ByteVector empty_element;
    io::ByteVector single_byte = io::ByteVector::Parse("42");
    io::ByteVector large_element = io::ByteVector::Random(1024);

    // Empty element
    neo_filter->Add(empty_element);
    EXPECT_TRUE(neo_filter->Contains(empty_element));

    // Single byte
    neo_filter->Add(single_byte);
    EXPECT_TRUE(neo_filter->Contains(single_byte));

    // Large element
    neo_filter->Add(large_element);
    EXPECT_TRUE(neo_filter->Contains(large_element));
}

TEST_F(BloomFilterTest, ParameterValidation)
{
    // Test invalid constructor parameters
    EXPECT_THROW(cryptography::BloomFilter(0, 1), std::invalid_argument);
    EXPECT_THROW(cryptography::BloomFilter(1000, 0), std::invalid_argument);
    EXPECT_THROW(cryptography::BloomFilter(1000, 20), std::invalid_argument);  // Too many hash functions
}

TEST_F(BloomFilterTest, Clone)
{
    // Add elements to original filter
    for (const auto& element : test_elements)
    {
        neo_filter->Add(element);
    }

    // Clone filter
    auto cloned_filter = neo_filter->Clone();

    // Verify clone has same properties
    EXPECT_EQ(cloned_filter->GetElementCount(), neo_filter->GetElementCount());
    EXPECT_EQ(cloned_filter->GetBitCount(), neo_filter->GetBitCount());
    EXPECT_EQ(cloned_filter->GetHashFunctionCount(), neo_filter->GetHashFunctionCount());

    // All elements should be found in clone
    for (const auto& element : test_elements)
    {
        EXPECT_TRUE(cloned_filter->Contains(element));
    }

    // Modifications to clone should not affect original
    io::ByteVector new_element = io::ByteVector::Random(32);
    cloned_filter->Add(new_element);

    EXPECT_TRUE(cloned_filter->Contains(new_element));
    EXPECT_FALSE(neo_filter->Contains(new_element));
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CRYPTOGRAPHY_TEST_BLOOMFILTER_CPP_H
