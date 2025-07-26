// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_merkle_tree.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLE_TREE_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLE_TREE_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/merkle_tree.h>

namespace neo {
namespace test {

class MerkleTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for MerkleTree testing
        single_hash = io::UInt256::Parse("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
        hash_list.push_back(io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"));
        hash_list.push_back(io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"));
        hash_list.push_back(io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"));
        hash_list.push_back(io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"));
    }

    void TearDown() override {
        // Clean up test fixtures
        hash_list.clear();
    }

    // Helper methods and test data for MerkleTree testing
    io::UInt256 single_hash;
    std::vector<io::UInt256> hash_list;
};

// MerkleTree test methods converted from C# UT_MerkleTree.cs functionality

TEST_F(MerkleTreeTest, ConstructWithSingleHash) {
    std::vector<io::UInt256> single_item = {single_hash};
    auto tree = MerkleTree::ComputeRoot(single_item);
    EXPECT_EQ(tree, single_hash); // Root of single item should be the item itself
}

TEST_F(MerkleTreeTest, ConstructWithMultipleHashes) {
    auto root = MerkleTree::ComputeRoot(hash_list);
    EXPECT_NE(root, io::UInt256()); // Root should not be empty
    EXPECT_NE(root, hash_list[0]); // Root should be different from any single hash
}

TEST_F(MerkleTreeTest, ConstructWithEmptyList) {
    std::vector<io::UInt256> empty_list;
    auto root = MerkleTree::ComputeRoot(empty_list);
    EXPECT_EQ(root, io::UInt256()); // Empty list should produce empty root
}

TEST_F(MerkleTreeTest, ConstructWithTwoHashes) {
    std::vector<io::UInt256> two_hashes = {hash_list[0], hash_list[1]};
    auto root = MerkleTree::ComputeRoot(two_hashes);
    EXPECT_NE(root, io::UInt256()); // Root should not be empty
    EXPECT_NE(root, hash_list[0]); // Root should be different from individual hashes
    EXPECT_NE(root, hash_list[1]);
}

TEST_F(MerkleTreeTest, DeterministicRoot) {
    auto root1 = MerkleTree::ComputeRoot(hash_list);
    auto root2 = MerkleTree::ComputeRoot(hash_list);
    EXPECT_EQ(root1, root2); // Same input should produce same root
}

TEST_F(MerkleTreeTest, DifferentOrderDifferentRoot) {
    std::vector<io::UInt256> reversed_list = hash_list;
    std::reverse(reversed_list.begin(), reversed_list.end());
    
    auto root1 = MerkleTree::ComputeRoot(hash_list);
    auto root2 = MerkleTree::ComputeRoot(reversed_list);
    EXPECT_NE(root1, root2); // Different order should produce different root
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLE_TREE_CPP_H
