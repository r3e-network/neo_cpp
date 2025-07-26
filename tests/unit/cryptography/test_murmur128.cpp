// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_murmur128.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_MURMUR128_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_MURMUR128_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/murmur128.h>

namespace neo {
namespace test {

class Murmur128Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for Murmur128 testing
        murmur128 = std::make_shared<cryptography::Murmur128>();
        
        // Test data with various patterns
        empty_input = io::ByteVector();
        single_byte = io::ByteVector::Parse("42");
        short_input = io::ByteVector::Parse("48656c6c6f"); // "Hello"
        standard_input = io::ByteVector::Parse("48656c6c6f20576f726c64"); // "Hello World"
        long_input = io::ByteVector::Parse("546865207175696b62726f776e20666f78206a756d7073206f76657220746865206c617a7920646f67"); // "The quick brown fox jumps over the lazy dog"
        
        // Test seeds for different hash values
        seed_zero = 0;
        seed_one = 1;
        seed_max = 0xFFFFFFFF;
        seed_neo = 0x4E454F; // 'NEO' in hex
        
        // Aligned and unaligned data for testing edge cases
        aligned_data = io::ByteVector(16, 0xAA); // 16 bytes (128-bit aligned)
        unaligned_data = io::ByteVector(15, 0xBB); // 15 bytes (unaligned)
        large_aligned = io::ByteVector(1024, 0xCC); // Large aligned data
        
        // Random data for collision testing
        random_data1 = io::ByteVector::Random(100);
        random_data2 = io::ByteVector::Random(100);
        random_data3 = io::ByteVector::Random(100);
        
        // Incremental data for avalanche testing
        incremental_base = io::ByteVector::Parse("0123456789abcdef");
        incremental_modified = io::ByteVector::Parse("0123456789abcdef");
        incremental_modified[7] ^= 0x01; // Flip one bit
    }

    void TearDown() override {
        // Clean up test fixtures
        murmur128.reset();
    }

    // Helper methods and test data for Murmur128 testing
    std::shared_ptr<cryptography::Murmur128> murmur128;
    io::ByteVector empty_input;
    io::ByteVector single_byte;
    io::ByteVector short_input;
    io::ByteVector standard_input;
    io::ByteVector long_input;
    uint32_t seed_zero;
    uint32_t seed_one;
    uint32_t seed_max;
    uint32_t seed_neo;
    io::ByteVector aligned_data;
    io::ByteVector unaligned_data;
    io::ByteVector large_aligned;
    io::ByteVector random_data1;
    io::ByteVector random_data2;
    io::ByteVector random_data3;
    io::ByteVector incremental_base;
    io::ByteVector incremental_modified;
};

// Murmur128 test methods converted from C# UT_Murmur128.cs functionality

TEST_F(Murmur128Test, HashSizeIsCorrect) {
    EXPECT_EQ(murmur128->GetHashSize(), 16); // MurmurHash 128-bit produces 16-byte hashes
}

TEST_F(Murmur128Test, HashEmptyInputWithZeroSeed) {
    auto hash = murmur128->ComputeHash(empty_input, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector(16, 0)); // Should not be all zeros
}

TEST_F(Murmur128Test, HashEmptyInputWithDifferentSeeds) {
    auto hash_seed0 = murmur128->ComputeHash(empty_input, seed_zero);
    auto hash_seed1 = murmur128->ComputeHash(empty_input, seed_one);
    auto hash_seedmax = murmur128->ComputeHash(empty_input, seed_max);
    
    EXPECT_NE(hash_seed0, hash_seed1);
    EXPECT_NE(hash_seed1, hash_seedmax);
    EXPECT_NE(hash_seed0, hash_seedmax);
}

TEST_F(Murmur128Test, HashSingleByte) {
    auto hash = murmur128->ComputeHash(single_byte, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector()); // Should not be empty
}

TEST_F(Murmur128Test, HashShortInput) {
    auto hash = murmur128->ComputeHash(short_input, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, HashStandardInput) {
    auto hash = murmur128->ComputeHash(standard_input, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, HashLongInput) {
    auto hash = murmur128->ComputeHash(long_input, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, HashIsConsistent) {
    auto hash1 = murmur128->ComputeHash(standard_input, seed_zero);
    auto hash2 = murmur128->ComputeHash(standard_input, seed_zero);
    
    EXPECT_EQ(hash1, hash2); // Same input and seed should produce same hash
}

TEST_F(Murmur128Test, DifferentInputsProduceDifferentHashes) {
    auto hash_short = murmur128->ComputeHash(short_input, seed_zero);
    auto hash_standard = murmur128->ComputeHash(standard_input, seed_zero);
    auto hash_long = murmur128->ComputeHash(long_input, seed_zero);
    
    EXPECT_NE(hash_short, hash_standard);
    EXPECT_NE(hash_standard, hash_long);
    EXPECT_NE(hash_short, hash_long);
}

TEST_F(Murmur128Test, SameSeedProducesSameHash) {
    auto hash1 = murmur128->ComputeHash(standard_input, seed_neo);
    auto hash2 = murmur128->ComputeHash(standard_input, seed_neo);
    
    EXPECT_EQ(hash1, hash2);
}

TEST_F(Murmur128Test, DifferentSeedsProduceDifferentHashes) {
    auto hash_seed0 = murmur128->ComputeHash(standard_input, seed_zero);
    auto hash_seed1 = murmur128->ComputeHash(standard_input, seed_one);
    auto hash_seedmax = murmur128->ComputeHash(standard_input, seed_max);
    auto hash_seedneo = murmur128->ComputeHash(standard_input, seed_neo);
    
    EXPECT_NE(hash_seed0, hash_seed1);
    EXPECT_NE(hash_seed1, hash_seedmax);
    EXPECT_NE(hash_seedmax, hash_seedneo);
    EXPECT_NE(hash_seed0, hash_seedneo);
}

TEST_F(Murmur128Test, AlignedDataHashing) {
    auto hash = murmur128->ComputeHash(aligned_data, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, UnalignedDataHashing) {
    auto hash = murmur128->ComputeHash(unaligned_data, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, AlignedVsUnalignedDataProduceDifferentHashes) {
    auto hash_aligned = murmur128->ComputeHash(aligned_data, seed_zero);
    auto hash_unaligned = murmur128->ComputeHash(unaligned_data, seed_zero);
    
    EXPECT_NE(hash_aligned, hash_unaligned);
}

TEST_F(Murmur128Test, LargeAlignedDataHashing) {
    auto hash = murmur128->ComputeHash(large_aligned, seed_zero);
    EXPECT_EQ(hash.Size(), 16);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(Murmur128Test, RandomDataCollisionResistance) {
    auto hash1 = murmur128->ComputeHash(random_data1, seed_zero);
    auto hash2 = murmur128->ComputeHash(random_data2, seed_zero);
    auto hash3 = murmur128->ComputeHash(random_data3, seed_zero);
    
    // Very unlikely to have collisions with random data
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash1, hash3);
}

TEST_F(Murmur128Test, AvalancheEffect) {
    auto hash_base = murmur128->ComputeHash(incremental_base, seed_zero);
    auto hash_modified = murmur128->ComputeHash(incremental_modified, seed_zero);
    
    EXPECT_NE(hash_base, hash_modified);
    
    // Count different bits (avalanche effect should cause ~50% bit difference)
    int different_bits = 0;
    for (size_t i = 0; i < 16; ++i) {
        uint8_t xor_byte = hash_base[i] ^ hash_modified[i];
        for (int bit = 0; bit < 8; ++bit) {
            if (xor_byte & (1 << bit)) {
                different_bits++;
            }
        }
    }
    
    // Should have significant bit difference (at least 25% of 128 bits)
    EXPECT_GT(different_bits, 32);
}

TEST_F(Murmur128Test, StaticHashFunction) {
    // Test static hash function if available
    auto hash_instance = murmur128->ComputeHash(standard_input, seed_zero);
    auto hash_static = cryptography::Murmur128::Hash(standard_input, seed_zero);
    
    EXPECT_EQ(hash_instance, hash_static);
}

TEST_F(Murmur128Test, PerformanceWithLargeData) {
    // Test with different sizes to ensure performance scales linearly
    std::vector<size_t> sizes = {1000, 10000, 100000};
    std::vector<double> times;
    
    for (size_t size : sizes) {
        io::ByteVector large_data = io::ByteVector::Random(size);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto hash = murmur128->ComputeHash(large_data, seed_zero);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        EXPECT_EQ(hash.Size(), 16);
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        times.push_back(duration.count());
        
        // Should complete within reasonable time
        EXPECT_LT(duration.count(), 100000); // Less than 100ms
    }
    
    // Performance should scale roughly linearly
    // 10x data should take less than 15x time (allowing for overhead)
    EXPECT_LT(times[1], times[0] * 15);
    EXPECT_LT(times[2], times[1] * 15);
}

TEST_F(Murmur128Test, BorderCaseSizes) {
    // Test various border cases for block processing
    std::vector<size_t> test_sizes = {0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 65};
    
    for (size_t size : test_sizes) {
        io::ByteVector test_data(size, 0x55);
        auto hash = murmur128->ComputeHash(test_data, seed_zero);
        
        EXPECT_EQ(hash.Size(), 16);
        EXPECT_NE(hash, io::ByteVector()); // Should not be empty
    }
}

TEST_F(Murmur128Test, SeedSensitivity) {
    // Test that small changes in seed produce large changes in output
    std::vector<uint32_t> close_seeds = {0x12345678, 0x12345679, 0x12345680, 0x12345677};
    std::vector<io::ByteVector> hashes;
    
    for (uint32_t seed : close_seeds) {
        auto hash = murmur128->ComputeHash(standard_input, seed);
        hashes.push_back(hash);
    }
    
    // All hashes should be different despite close seeds
    for (size_t i = 0; i < hashes.size(); ++i) {
        for (size_t j = i + 1; j < hashes.size(); ++j) {
            EXPECT_NE(hashes[i], hashes[j]);
        }
    }
}

TEST_F(Murmur128Test, ZeroDataWithDifferentLengths) {
    // Test zero-filled data of different lengths
    std::vector<size_t> lengths = {1, 16, 32, 64, 128};
    std::vector<io::ByteVector> hashes;
    
    for (size_t len : lengths) {
        io::ByteVector zero_data(len, 0);
        auto hash = murmur128->ComputeHash(zero_data, seed_zero);
        hashes.push_back(hash);
    }
    
    // Different length zero data should produce different hashes
    for (size_t i = 0; i < hashes.size(); ++i) {
        for (size_t j = i + 1; j < hashes.size(); ++j) {
            EXPECT_NE(hashes[i], hashes[j]);
        }
    }
}

TEST_F(Murmur128Test, RepeatingPatterns) {
    // Test with repeating byte patterns
    io::ByteVector pattern_aa(64, 0xAA);
    io::ByteVector pattern_55(64, 0x55);
    io::ByteVector pattern_ff(64, 0xFF);
    
    auto hash_aa = murmur128->ComputeHash(pattern_aa, seed_zero);
    auto hash_55 = murmur128->ComputeHash(pattern_55, seed_zero);
    auto hash_ff = murmur128->ComputeHash(pattern_ff, seed_zero);
    
    EXPECT_NE(hash_aa, hash_55);
    EXPECT_NE(hash_55, hash_ff);
    EXPECT_NE(hash_aa, hash_ff);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_TEST_MURMUR128_CPP_H
