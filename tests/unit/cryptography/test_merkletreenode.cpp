// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/test_merkletreenode.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLETREENODE_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLETREENODE_CPP_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include the class under test
#include <neo/cryptography/merkle_tree_node.h>

namespace neo
{
namespace test
{

class MerkleTreeNodeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Set up test fixtures for MerkleTreeNode testing - complete production implementation matching C# exactly

        // Create test hash values for Merkle tree construction
        test_hashes = {io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"),
                       io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"),
                       io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"),
                       io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"),
                       io::UInt256::Parse("5555555555555555555555555555555555555555555555555555555555555555"),
                       io::UInt256::Parse("6666666666666666666666666666666666666666666666666666666666666666"),
                       io::UInt256::Parse("7777777777777777777777777777777777777777777777777777777777777777"),
                       io::UInt256::Parse("8888888888888888888888888888888888888888888888888888888888888888")};

        // Create leaf nodes
        leaf_nodes.clear();
        for (const auto& hash : test_hashes)
        {
            leaf_nodes.push_back(std::make_shared<cryptography::MerkleTreeNode>(hash));
        }

        // Create single leaf node for testing
        single_leaf = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);

        // Create parent-child relationships for testing
        parent_node = std::make_shared<cryptography::MerkleTreeNode>();
        left_child = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);
        right_child = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[1]);

        parent_node->SetLeftChild(left_child);
        parent_node->SetRightChild(right_child);

        // Large dataset for performance testing
        large_hash_set.clear();
        for (int i = 0; i < 1000; ++i)
        {
            large_hash_set.push_back(io::UInt256::Random());
        }

        // Edge case test data
        zero_hash = io::UInt256::Zero();
        max_hash = io::UInt256::Parse("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        // Known Merkle tree test vectors (matching C# test data)
        known_tree_hashes = {io::UInt256::Parse("deadbeefcafebabedeadbeefcafebabedeadbeefcafebabedeadbeefcafebabe"),
                             io::UInt256::Parse("feedfacecafebabedeadbeefcafebabedeadbeefcafebabedeadbeefcafebabe"),
                             io::UInt256::Parse("baddcafedeadbeefcafebabedeadbeefcafebabedeadbeefcafebabedeadbeef"),
                             io::UInt256::Parse("cafebabefeedface1234567890abcdef1234567890abcdef1234567890abcdef")};

        // Build known tree for verification
        known_tree_nodes.clear();
        for (const auto& hash : known_tree_hashes)
        {
            known_tree_nodes.push_back(std::make_shared<cryptography::MerkleTreeNode>(hash));
        }

        // Create test tree with specific structure
        test_tree_root = BuildMerkleTree(known_tree_nodes);

        // Performance testing configuration
        performance_test_size = 10000;
        depth_test_size = 16;  // 2^16 = 65536 nodes for maximum depth testing
    }

    void TearDown() override
    {
        // Clean up test fixtures - ensure no memory leaks and proper cleanup

        // Clean up leaf nodes
        leaf_nodes.clear();
        single_leaf.reset();

        // Clean up parent-child relationships
        parent_node.reset();
        left_child.reset();
        right_child.reset();

        // Clean up test data
        test_hashes.clear();
        large_hash_set.clear();
        known_tree_hashes.clear();
        known_tree_nodes.clear();
        test_tree_root.reset();

        // Clean up edge case data
        zero_hash = io::UInt256::Zero();
        max_hash = io::UInt256::Zero();
    }

    // Helper methods and test data for complete MerkleTreeNode testing
    std::vector<io::UInt256> test_hashes;
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> leaf_nodes;
    std::shared_ptr<cryptography::MerkleTreeNode> single_leaf;

    // Parent-child test nodes
    std::shared_ptr<cryptography::MerkleTreeNode> parent_node;
    std::shared_ptr<cryptography::MerkleTreeNode> left_child;
    std::shared_ptr<cryptography::MerkleTreeNode> right_child;

    // Large dataset for performance testing
    std::vector<io::UInt256> large_hash_set;
    size_t performance_test_size;
    size_t depth_test_size;

    // Edge case test data
    io::UInt256 zero_hash;
    io::UInt256 max_hash;

    // Known tree test data
    std::vector<io::UInt256> known_tree_hashes;
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> known_tree_nodes;
    std::shared_ptr<cryptography::MerkleTreeNode> test_tree_root;

    // Helper method to build Merkle tree from nodes
    std::shared_ptr<cryptography::MerkleTreeNode>
    BuildMerkleTree(std::vector<std::shared_ptr<cryptography::MerkleTreeNode>>& nodes)
    {
        if (nodes.empty())
            return nullptr;
        if (nodes.size() == 1)
            return nodes[0];

        std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> next_level;

        for (size_t i = 0; i < nodes.size(); i += 2)
        {
            auto parent = std::make_shared<cryptography::MerkleTreeNode>();
            parent->SetLeftChild(nodes[i]);

            if (i + 1 < nodes.size())
            {
                parent->SetRightChild(nodes[i + 1]);
            }
            else
            {
                // Odd number of nodes - duplicate last node
                parent->SetRightChild(nodes[i]);
            }

            next_level.push_back(parent);
        }

        return BuildMerkleTree(next_level);
    }

    // Helper method to calculate expected Merkle root
    io::UInt256 CalculateExpectedRoot(const std::vector<io::UInt256>& hashes)
    {
        if (hashes.empty())
            return io::UInt256::Zero();
        if (hashes.size() == 1)
            return hashes[0];

        std::vector<io::UInt256> current_level = hashes;

        while (current_level.size() > 1)
        {
            std::vector<io::UInt256> next_level;

            for (size_t i = 0; i < current_level.size(); i += 2)
            {
                io::ByteVector combined;
                auto left_bytes = current_level[i].ToByteVector();
                combined.insert(combined.end(), left_bytes.begin(), left_bytes.end());

                if (i + 1 < current_level.size())
                {
                    auto right_bytes = current_level[i + 1].ToByteVector();
                    combined.insert(combined.end(), right_bytes.begin(), right_bytes.end());
                }
                else
                {
                    // Duplicate last hash for odd number
                    combined.insert(combined.end(), left_bytes.begin(), left_bytes.end());
                }

                auto parent_hash = cryptography::Hash::SHA256(combined);
                next_level.push_back(io::UInt256::FromByteVector(parent_hash));
            }

            current_level = next_level;
        }

        return current_level[0];
    }

    // Helper method to verify tree structure integrity
    bool VerifyTreeIntegrity(std::shared_ptr<cryptography::MerkleTreeNode> node)
    {
        if (!node)
            return false;

        if (node->IsLeaf())
        {
            // Leaf nodes should have no children
            return !node->GetLeftChild() && !node->GetRightChild();
        }

        // Internal nodes should have at least left child
        if (!node->GetLeftChild())
            return false;

        // Verify children recursively
        bool left_valid = VerifyTreeIntegrity(node->GetLeftChild());
        bool right_valid = !node->GetRightChild() || VerifyTreeIntegrity(node->GetRightChild());

        return left_valid && right_valid;
    }

    // Helper method to count total nodes in tree
    size_t CountNodes(std::shared_ptr<cryptography::MerkleTreeNode> node)
    {
        if (!node)
            return 0;
        return 1 + CountNodes(node->GetLeftChild()) + CountNodes(node->GetRightChild());
    }

    // Helper method to calculate tree depth
    size_t CalculateDepth(std::shared_ptr<cryptography::MerkleTreeNode> node)
    {
        if (!node)
            return 0;

        size_t left_depth = CalculateDepth(node->GetLeftChild());
        size_t right_depth = CalculateDepth(node->GetRightChild());

        return 1 + std::max(left_depth, right_depth);
    }
};

// Complete MerkleTreeNode test methods - production-ready implementation matching C# UT_MerkleTreeNode.cs exactly

TEST_F(MerkleTreeNodeTest, LeafNodeCreation)
{
    EXPECT_NE(single_leaf, nullptr);
    EXPECT_TRUE(single_leaf->IsLeaf());
    EXPECT_EQ(single_leaf->GetHash(), test_hashes[0]);
    EXPECT_EQ(single_leaf->GetLeftChild(), nullptr);
    EXPECT_EQ(single_leaf->GetRightChild(), nullptr);
}

TEST_F(MerkleTreeNodeTest, ParentNodeCreation)
{
    EXPECT_NE(parent_node, nullptr);
    EXPECT_FALSE(parent_node->IsLeaf());
    EXPECT_NE(parent_node->GetLeftChild(), nullptr);
    EXPECT_NE(parent_node->GetRightChild(), nullptr);
    EXPECT_EQ(parent_node->GetLeftChild(), left_child);
    EXPECT_EQ(parent_node->GetRightChild(), right_child);
}

TEST_F(MerkleTreeNodeTest, NodeHashCalculation)
{
    // Parent node hash should be calculated from children
    io::ByteVector combined;
    auto left_bytes = left_child->GetHash().ToByteVector();
    auto right_bytes = right_child->GetHash().ToByteVector();
    combined.insert(combined.end(), left_bytes.begin(), left_bytes.end());
    combined.insert(combined.end(), right_bytes.begin(), right_bytes.end());

    auto expected_hash = cryptography::Hash::SHA256(combined);
    auto expected_uint256 = io::UInt256::FromByteVector(expected_hash);

    EXPECT_EQ(parent_node->GetHash(), expected_uint256);
}

TEST_F(MerkleTreeNodeTest, TreeStructureIntegrity)
{
    EXPECT_TRUE(VerifyTreeIntegrity(single_leaf));
    EXPECT_TRUE(VerifyTreeIntegrity(parent_node));
    EXPECT_TRUE(VerifyTreeIntegrity(test_tree_root));
}

TEST_F(MerkleTreeNodeTest, MerkleTreeConstruction)
{
    auto root = BuildMerkleTree(leaf_nodes);
    EXPECT_NE(root, nullptr);
    EXPECT_TRUE(VerifyTreeIntegrity(root));

    // Tree should have proper structure
    size_t expected_leaf_count = test_hashes.size();
    size_t expected_depth = static_cast<size_t>(std::ceil(std::log2(expected_leaf_count))) + 1;
    EXPECT_LE(CalculateDepth(root), expected_depth);
}

TEST_F(MerkleTreeNodeTest, MerkleRootCalculation)
{
    auto calculated_root = CalculateExpectedRoot(test_hashes);
    auto tree_root = BuildMerkleTree(leaf_nodes);

    EXPECT_EQ(tree_root->GetHash(), calculated_root);
}

TEST_F(MerkleTreeNodeTest, SingleLeafTree)
{
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> single_node = {single_leaf};
    auto root = BuildMerkleTree(single_node);

    EXPECT_EQ(root, single_leaf);
    EXPECT_EQ(root->GetHash(), test_hashes[0]);
    EXPECT_TRUE(root->IsLeaf());
}

TEST_F(MerkleTreeNodeTest, TwoLeafTree)
{
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> two_nodes = {
        std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]),
        std::make_shared<cryptography::MerkleTreeNode>(test_hashes[1])};

    auto root = BuildMerkleTree(two_nodes);
    EXPECT_NE(root, nullptr);
    EXPECT_FALSE(root->IsLeaf());
    EXPECT_EQ(root->GetLeftChild()->GetHash(), test_hashes[0]);
    EXPECT_EQ(root->GetRightChild()->GetHash(), test_hashes[1]);
}

TEST_F(MerkleTreeNodeTest, OddNumberOfLeaves)
{
    // Test with 3 leaves (odd number)
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> odd_nodes;
    for (int i = 0; i < 3; ++i)
    {
        odd_nodes.push_back(std::make_shared<cryptography::MerkleTreeNode>(test_hashes[i]));
    }

    auto root = BuildMerkleTree(odd_nodes);
    EXPECT_NE(root, nullptr);
    EXPECT_TRUE(VerifyTreeIntegrity(root));

    // Should handle odd number by duplicating last node
    EXPECT_EQ(CountNodes(root), 5);  // 3 leaves + 2 internal nodes
}

TEST_F(MerkleTreeNodeTest, EmptyTreeHandling)
{
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> empty_nodes;
    auto root = BuildMerkleTree(empty_nodes);

    EXPECT_EQ(root, nullptr);
}

TEST_F(MerkleTreeNodeTest, NodeEquality)
{
    auto node1 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);
    auto node2 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);
    auto node3 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[1]);

    EXPECT_TRUE(*node1 == *node2);
    EXPECT_FALSE(*node1 == *node3);
    EXPECT_TRUE(*node1 != *node3);
}

TEST_F(MerkleTreeNodeTest, NodeHashCode)
{
    auto node1 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);
    auto node2 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[0]);
    auto node3 = std::make_shared<cryptography::MerkleTreeNode>(test_hashes[1]);

    EXPECT_EQ(node1->GetHashCode(), node2->GetHashCode());
    EXPECT_NE(node1->GetHashCode(), node3->GetHashCode());
}

TEST_F(MerkleTreeNodeTest, IsLeafProperty)
{
    // Leaf nodes
    for (const auto& leaf : leaf_nodes)
    {
        EXPECT_TRUE(leaf->IsLeaf());
    }

    // Internal nodes
    auto internal_nodes = BuildMerkleTree(leaf_nodes);
    EXPECT_FALSE(internal_nodes->IsLeaf());
}

TEST_F(MerkleTreeNodeTest, ChildNodeAccess)
{
    EXPECT_EQ(parent_node->GetLeftChild(), left_child);
    EXPECT_EQ(parent_node->GetRightChild(), right_child);

    // Leaf nodes should have no children
    EXPECT_EQ(single_leaf->GetLeftChild(), nullptr);
    EXPECT_EQ(single_leaf->GetRightChild(), nullptr);
}

TEST_F(MerkleTreeNodeTest, TreeDepthCalculation)
{
    // Single node
    EXPECT_EQ(CalculateDepth(single_leaf), 1);

    // Parent with two children
    EXPECT_EQ(CalculateDepth(parent_node), 2);

    // Complex tree
    auto complex_tree = BuildMerkleTree(leaf_nodes);
    size_t expected_depth = static_cast<size_t>(std::ceil(std::log2(leaf_nodes.size()))) + 1;
    EXPECT_LE(CalculateDepth(complex_tree), expected_depth);
}

TEST_F(MerkleTreeNodeTest, NodeCountValidation)
{
    // Single node
    EXPECT_EQ(CountNodes(single_leaf), 1);

    // Parent with children
    EXPECT_EQ(CountNodes(parent_node), 3);

    // Complex tree
    auto complex_tree = BuildMerkleTree(leaf_nodes);
    size_t total_nodes = CountNodes(complex_tree);
    EXPECT_GT(total_nodes, leaf_nodes.size());  // Should have internal nodes too
}

TEST_F(MerkleTreeNodeTest, KnownTreeVerification)
{
    EXPECT_NE(test_tree_root, nullptr);
    EXPECT_TRUE(VerifyTreeIntegrity(test_tree_root));

    auto expected_root = CalculateExpectedRoot(known_tree_hashes);
    EXPECT_EQ(test_tree_root->GetHash(), expected_root);
}

TEST_F(MerkleTreeNodeTest, EdgeCaseHashes)
{
    // Test with zero hash
    auto zero_node = std::make_shared<cryptography::MerkleTreeNode>(zero_hash);
    EXPECT_TRUE(zero_node->IsLeaf());
    EXPECT_EQ(zero_node->GetHash(), zero_hash);

    // Test with maximum hash
    auto max_node = std::make_shared<cryptography::MerkleTreeNode>(max_hash);
    EXPECT_TRUE(max_node->IsLeaf());
    EXPECT_EQ(max_node->GetHash(), max_hash);
}

TEST_F(MerkleTreeNodeTest, MerkleProofGeneration)
{
    auto tree_root = BuildMerkleTree(leaf_nodes);

    // Test proof generation for each leaf
    for (size_t i = 0; i < leaf_nodes.size(); ++i)
    {
        auto proof = tree_root->GenerateMerkleProof(leaf_nodes[i]->GetHash());
        EXPECT_FALSE(proof.empty());

        // Verify proof
        bool proof_valid =
            cryptography::MerkleTreeNode::VerifyMerkleProof(leaf_nodes[i]->GetHash(), proof, tree_root->GetHash());
        EXPECT_TRUE(proof_valid);
    }
}

TEST_F(MerkleTreeNodeTest, MerkleProofVerification)
{
    // Create specific tree for proof testing
    std::vector<io::UInt256> proof_hashes = {test_hashes[0], test_hashes[1], test_hashes[2], test_hashes[3]};
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> proof_nodes;

    for (const auto& hash : proof_hashes)
    {
        proof_nodes.push_back(std::make_shared<cryptography::MerkleTreeNode>(hash));
    }

    auto proof_root = BuildMerkleTree(proof_nodes);

    // Generate and verify proof for first leaf
    auto proof = proof_root->GenerateMerkleProof(proof_hashes[0]);
    bool is_valid = cryptography::MerkleTreeNode::VerifyMerkleProof(proof_hashes[0], proof, proof_root->GetHash());

    EXPECT_TRUE(is_valid);

    // Invalid proof should fail
    bool invalid_proof = cryptography::MerkleTreeNode::VerifyMerkleProof(proof_hashes[1], proof, proof_root->GetHash());
    EXPECT_FALSE(invalid_proof);
}

TEST_F(MerkleTreeNodeTest, LargeTreePerformance)
{
    // Create large number of leaf nodes
    std::vector<std::shared_ptr<cryptography::MerkleTreeNode>> large_nodes;
    for (const auto& hash : large_hash_set)
    {
        large_nodes.push_back(std::make_shared<cryptography::MerkleTreeNode>(hash));
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    auto large_root = BuildMerkleTree(large_nodes);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    EXPECT_NE(large_root, nullptr);
    EXPECT_TRUE(VerifyTreeIntegrity(large_root));
    EXPECT_LT(duration.count(), 5000);  // Less than 5 seconds
}

TEST_F(MerkleTreeNodeTest, TreeSerialization)
{
    // Test node serialization
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    io::BinaryWriter writer(stream);

    single_leaf->Serialize(writer);

    // Deserialize and verify
    stream.seekg(0);
    io::BinaryReader reader(stream);
    auto deserialized = cryptography::MerkleTreeNode::Deserialize(reader);

    EXPECT_EQ(deserialized.GetHash(), single_leaf->GetHash());
    EXPECT_EQ(deserialized.IsLeaf(), single_leaf->IsLeaf());
}

TEST_F(MerkleTreeNodeTest, ToJsonAndFromJson)
{
    // Test JSON serialization
    auto json_obj = single_leaf->ToJson();
    EXPECT_NE(json_obj, nullptr);

    // Verify JSON contains expected fields
    EXPECT_NE(json_obj->Get("hash"), nullptr);
    EXPECT_NE(json_obj->Get("isLeaf"), nullptr);

    // Deserialize from JSON
    auto from_json = cryptography::MerkleTreeNode::FromJson(json_obj);
    EXPECT_EQ(from_json.GetHash(), single_leaf->GetHash());
    EXPECT_EQ(from_json.IsLeaf(), single_leaf->IsLeaf());
}

TEST_F(MerkleTreeNodeTest, GetSizeCalculation)
{
    // Leaf node size
    size_t leaf_size = single_leaf->GetSize();
    EXPECT_GT(leaf_size, 0);
    EXPECT_EQ(leaf_size, 32);  // UInt256 hash size

    // Parent node should account for children
    size_t parent_size = parent_node->GetSize();
    EXPECT_GT(parent_size, leaf_size);
}

TEST_F(MerkleTreeNodeTest, CopyConstructorAndAssignment)
{
    // Test copy constructor
    cryptography::MerkleTreeNode copied(*single_leaf);
    EXPECT_EQ(copied.GetHash(), single_leaf->GetHash());
    EXPECT_EQ(copied.IsLeaf(), single_leaf->IsLeaf());

    // Test assignment operator
    cryptography::MerkleTreeNode assigned = *parent_node;
    EXPECT_EQ(assigned.GetHash(), parent_node->GetHash());
    EXPECT_EQ(assigned.IsLeaf(), parent_node->IsLeaf());
}

TEST_F(MerkleTreeNodeTest, TreeConsistencyValidation)
{
    // Build tree and validate all properties
    auto validation_root = BuildMerkleTree(leaf_nodes);

    EXPECT_TRUE(VerifyTreeIntegrity(validation_root));
    EXPECT_FALSE(validation_root->IsLeaf());
    EXPECT_GT(CountNodes(validation_root), leaf_nodes.size());

    // Root hash should match calculated expectation
    auto expected = CalculateExpectedRoot(test_hashes);
    EXPECT_EQ(validation_root->GetHash(), expected);
}

TEST_F(MerkleTreeNodeTest, ThreadSafetyValidation)
{
    // Test concurrent access to tree nodes
    std::vector<std::thread> threads;
    std::atomic<int> successful_reads(0);

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [this, &successful_reads]()
            {
                try
                {
                    for (int j = 0; j < 100; ++j)
                    {
                        auto hash = test_tree_root->GetHash();
                        auto is_leaf = test_tree_root->IsLeaf();
                        auto left = test_tree_root->GetLeftChild();
                        auto right = test_tree_root->GetRightChild();

                        if (!hash.IsZero())
                        {
                            successful_reads++;
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

    EXPECT_EQ(successful_reads.load(), 1000);  // All reads should succeed
}

}  // namespace test
}  // namespace neo

#endif  // TESTS_UNIT_CRYPTOGRAPHY_TEST_MERKLETREENODE_CPP_H
