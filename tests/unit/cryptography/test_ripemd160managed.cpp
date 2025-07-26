// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_ripemd160managed.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_RIPEMD160MANAGED_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_RIPEMD160MANAGED_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/ripemd160_managed.h>

namespace neo {
namespace test {

class RIPEMD160ManagedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for RIPEMD160Managed testing
        ripemd160 = std::make_shared<cryptography::RIPEMD160Managed>();
        
        // Test vectors from RIPEMD-160 specification
        empty_input = io::ByteVector();
        short_input = io::ByteVector::Parse("61"); // "a"
        standard_input = io::ByteVector::Parse("616263"); // "abc"
        long_input = io::ByteVector::Parse("6162636465666768696a6b6c6d6e6f707172737475767778797a"); // "abcdefghijklmnopqrstuvwxyz"
        
        // Neo-specific test vectors
        hello_world = io::ByteVector::Parse("48656c6c6f20576f726c64"); // "Hello World"
        bitcoin_genesis = io::ByteVector::Parse("546865205469656d65732030332f4a616e2f3230303920436861656e636c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73");
        
        // Expected hash values (from RIPEMD-160 test vectors)
        expected_empty = io::ByteVector::Parse("9c1185a5c5e9fc54612808977ee8f548b2258d31");
        expected_short = io::ByteVector::Parse("0bdc9d2d256b3ee9daae347be6f4dc835a467ffe");
        expected_standard = io::ByteVector::Parse("8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
        expected_long = io::ByteVector::Parse("f71c27109c692c1b56bbdceb5b9d2865b3708dbc");
        
        // Large input for performance testing
        large_input = io::ByteVector::Random(10000);
        
        // Multiple input data for batch testing
        batch_inputs = {
            io::ByteVector::Parse("31"),     // "1"
            io::ByteVector::Parse("3132"),   // "12"
            io::ByteVector::Parse("313233") // "123"
        };
    }

    void TearDown() override {
        // Clean up test fixtures
        ripemd160.reset();
        batch_inputs.clear();
    }

    // Helper methods and test data for RIPEMD160Managed testing
    std::shared_ptr<cryptography::RIPEMD160Managed> ripemd160;
    io::ByteVector empty_input;
    io::ByteVector short_input;
    io::ByteVector standard_input;
    io::ByteVector long_input;
    io::ByteVector hello_world;
    io::ByteVector bitcoin_genesis;
    io::ByteVector large_input;
    io::ByteVector expected_empty;
    io::ByteVector expected_short;
    io::ByteVector expected_standard;
    io::ByteVector expected_long;
    std::vector<io::ByteVector> batch_inputs;
};

// RIPEMD160Managed test methods converted from C# UT_RIPEMD160Managed.cs functionality

TEST_F(RIPEMD160ManagedTest, HashSizeIsCorrect) {
    EXPECT_EQ(ripemd160->GetHashSize(), 20); // RIPEMD-160 produces 160-bit (20-byte) hashes
}

TEST_F(RIPEMD160ManagedTest, HashEmptyInput) {
    auto hash = ripemd160->ComputeHash(empty_input);
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_EQ(hash, expected_empty);
}

TEST_F(RIPEMD160ManagedTest, HashSingleCharacter) {
    auto hash = ripemd160->ComputeHash(short_input);
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_EQ(hash, expected_short);
}

TEST_F(RIPEMD160ManagedTest, HashStandardTestVector) {
    auto hash = ripemd160->ComputeHash(standard_input); // "abc"
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_EQ(hash, expected_standard);
}

TEST_F(RIPEMD160ManagedTest, HashLongInput) {
    auto hash = ripemd160->ComputeHash(long_input); // alphabet
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_EQ(hash, expected_long);
}

TEST_F(RIPEMD160ManagedTest, HashNeoSpecificInput) {
    auto hash = ripemd160->ComputeHash(hello_world);
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_NE(hash, io::ByteVector()); // Should not be empty
}

TEST_F(RIPEMD160ManagedTest, HashBitcoinGenesisBlock) {
    auto hash = ripemd160->ComputeHash(bitcoin_genesis);
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(RIPEMD160ManagedTest, HashLargeInput) {
    auto hash = ripemd160->ComputeHash(large_input);
    EXPECT_EQ(hash.Size(), 20);
    EXPECT_NE(hash, io::ByteVector());
}

TEST_F(RIPEMD160ManagedTest, HashIsConsistent) {
    auto hash1 = ripemd160->ComputeHash(standard_input);
    auto hash2 = ripemd160->ComputeHash(standard_input);
    
    EXPECT_EQ(hash1, hash2); // Same input should produce same hash
}

TEST_F(RIPEMD160ManagedTest, DifferentInputsProduceDifferentHashes) {
    auto hash1 = ripemd160->ComputeHash(short_input);
    auto hash2 = ripemd160->ComputeHash(standard_input);
    auto hash3 = ripemd160->ComputeHash(long_input);
    
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash1, hash3);
}

TEST_F(RIPEMD160ManagedTest, InitializeAndReset) {
    ripemd160->Initialize();
    
    // Should be able to compute hash after initialize
    auto hash1 = ripemd160->ComputeHash(standard_input);
    EXPECT_EQ(hash1, expected_standard);
    
    // Reset should not affect subsequent computations
    ripemd160->Reset();
    auto hash2 = ripemd160->ComputeHash(standard_input);
    EXPECT_EQ(hash2, expected_standard);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(RIPEMD160ManagedTest, IncrementalHashing) {
    // Hash in one go
    auto hash_single = ripemd160->ComputeHash(standard_input);
    
    // Hash incrementally
    ripemd160->Initialize();
    ripemd160->TransformBlock(standard_input.Data(), 0, standard_input.Size());
    auto hash_incremental = ripemd160->TransformFinalBlock();
    
    EXPECT_EQ(hash_single, hash_incremental);
}

TEST_F(RIPEMD160ManagedTest, MultipleIncrementalUpdates) {
    // Create test input by concatenating parts
    io::ByteVector part1 = io::ByteVector::Parse("61");   // "a"
    io::ByteVector part2 = io::ByteVector::Parse("62");   // "b"
    io::ByteVector part3 = io::ByteVector::Parse("63");   // "c"
    
    // Hash all at once
    auto hash_single = ripemd160->ComputeHash(standard_input); // "abc"
    
    // Hash incrementally
    ripemd160->Initialize();
    ripemd160->TransformBlock(part1.Data(), 0, part1.Size());
    ripemd160->TransformBlock(part2.Data(), 0, part2.Size());
    ripemd160->TransformBlock(part3.Data(), 0, part3.Size());
    auto hash_incremental = ripemd160->TransformFinalBlock();
    
    EXPECT_EQ(hash_single, hash_incremental);
}

TEST_F(RIPEMD160ManagedTest, BatchHashing) {
    std::vector<io::ByteVector> hashes;
    
    for (const auto& input : batch_inputs) {
        auto hash = ripemd160->ComputeHash(input);
        EXPECT_EQ(hash.Size(), 20);
        hashes.push_back(hash);
    }
    
    // All hashes should be different
    for (size_t i = 0; i < hashes.size(); ++i) {
        for (size_t j = i + 1; j < hashes.size(); ++j) {
            EXPECT_NE(hashes[i], hashes[j]);
        }
    }
}

TEST_F(RIPEMD160ManagedTest, CanReuseAfterDispose) {
    auto hash1 = ripemd160->ComputeHash(standard_input);
    
    ripemd160->Dispose();
    ripemd160->Initialize(); // Should reinitialize after dispose
    
    auto hash2 = ripemd160->ComputeHash(standard_input);
    EXPECT_EQ(hash1, hash2);
}

TEST_F(RIPEMD160ManagedTest, StaticHashFunction) {
    // Test static hash function if available
    auto hash_instance = ripemd160->ComputeHash(standard_input);
    auto hash_static = cryptography::RIPEMD160Managed::Hash(standard_input);
    
    EXPECT_EQ(hash_instance, hash_static);
}

TEST_F(RIPEMD160ManagedTest, CloneProducesSameResults) {
    auto cloned = ripemd160->Clone();
    
    auto hash_original = ripemd160->ComputeHash(standard_input);
    auto hash_cloned = cloned->ComputeHash(standard_input);
    
    EXPECT_EQ(hash_original, hash_cloned);
}

TEST_F(RIPEMD160ManagedTest, HashBoundaryConditions) {
    // Test 55-byte input (one less than block boundary)
    io::ByteVector boundary_input(55, 0x42);
    auto hash_55 = ripemd160->ComputeHash(boundary_input);
    EXPECT_EQ(hash_55.Size(), 20);
    
    // Test 56-byte input (block boundary for padding)
    io::ByteVector block_input(56, 0x42);
    auto hash_56 = ripemd160->ComputeHash(block_input);
    EXPECT_EQ(hash_56.Size(), 20);
    
    // Test 64-byte input (exact block size)
    io::ByteVector full_block(64, 0x42);
    auto hash_64 = ripemd160->ComputeHash(full_block);
    EXPECT_EQ(hash_64.Size(), 20);
    
    // All should be different
    EXPECT_NE(hash_55, hash_56);
    EXPECT_NE(hash_56, hash_64);
    EXPECT_NE(hash_55, hash_64);
}

TEST_F(RIPEMD160ManagedTest, PerformanceWithLargeData) {
    // Test with different sizes to ensure performance scales reasonably
    std::vector<size_t> sizes = {1000, 10000, 100000};
    
    for (size_t size : sizes) {
        io::ByteVector large_data = io::ByteVector::Random(size);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto hash = ripemd160->ComputeHash(large_data);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        EXPECT_EQ(hash.Size(), 20);
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        // Should complete within reasonable time (adjust threshold as needed)
        EXPECT_LT(duration.count(), 1000); // Less than 1 second
    }
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_TEST_RIPEMD160MANAGED_CPP_H
