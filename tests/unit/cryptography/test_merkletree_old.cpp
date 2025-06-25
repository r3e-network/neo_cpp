#include <gtest/gtest.h>
#include <neo/cryptography/merkletree.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

TEST(MerkleTreeTest, ComputeRootEmpty)
{
    std::vector<UInt256> hashes;
    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_EQ(root, UInt256::Zero());
}

TEST(MerkleTreeTest, ComputeRootSingle)
{
    UInt256 hash = Hash::Sha256(ByteVector{1, 2, 3}.AsSpan());
    std::vector<UInt256> hashes = {hash};
    
    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_EQ(root, hash);
}

TEST(MerkleTreeTest, ComputeRootPair)
{
    UInt256 hash1 = Hash::Sha256(ByteVector{1, 2, 3}.AsSpan());
    UInt256 hash2 = Hash::Sha256(ByteVector{4, 5, 6}.AsSpan());
    std::vector<UInt256> hashes = {hash1, hash2};
    
    auto root = MerkleTree::ComputeRoot(hashes);
    
    // Compute the expected root
    ByteVector buffer(UInt256::Size + UInt256::Size);
    std::memcpy(buffer.Data(), hash1.Data(), UInt256::Size);
    std::memcpy(buffer.Data() + UInt256::Size, hash2.Data(), UInt256::Size);
    UInt256 expected = Hash::Sha256(buffer.AsSpan());
    
    EXPECT_EQ(root, expected);
}

TEST(MerkleTreeTest, ComputeRootOdd)
{
    UInt256 hash1 = Hash::Sha256(ByteVector{1, 2, 3}.AsSpan());
    UInt256 hash2 = Hash::Sha256(ByteVector{4, 5, 6}.AsSpan());
    UInt256 hash3 = Hash::Sha256(ByteVector{7, 8, 9}.AsSpan());
    std::vector<UInt256> hashes = {hash1, hash2, hash3};
    
    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_TRUE(root.has_value());
    
    // Compute the expected root
    ByteVector buffer1(hash1.Size() + hash2.Size());
    std::memcpy(buffer1.Data(), hash1.Data(), hash1.Size());
    std::memcpy(buffer1.Data() + hash1.Size(), hash2.Data(), hash2.Size());
    UInt256 parent1 = Hash::Sha256(buffer1.AsSpan());
    
    ByteVector buffer2(hash3.Size() + hash3.Size());
    std::memcpy(buffer2.Data(), hash3.Data(), hash3.Size());
    std::memcpy(buffer2.Data() + hash3.Size(), hash3.Data(), hash3.Size());
    UInt256 parent2 = Hash::Sha256(buffer2.AsSpan());
    
    ByteVector buffer3(parent1.Size() + parent2.Size());
    std::memcpy(buffer3.Data(), parent1.Data(), parent1.Size());
    std::memcpy(buffer3.Data() + parent1.Size(), parent2.Data(), parent2.Size());
    UInt256 expected = Hash::Sha256(buffer3.AsSpan());
    
    EXPECT_EQ(*root, expected);
}

TEST(MerkleTreeTest, ComputePath)
{
    UInt256 hash1 = Hash::Sha256(ByteVector{1, 2, 3}.AsSpan());
    UInt256 hash2 = Hash::Sha256(ByteVector{4, 5, 6}.AsSpan());
    UInt256 hash3 = Hash::Sha256(ByteVector{7, 8, 9}.AsSpan());
    UInt256 hash4 = Hash::Sha256(ByteVector{10, 11, 12}.AsSpan());
    std::vector<UInt256> hashes = {hash1, hash2, hash3, hash4};
    
    // Compute the root
    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_TRUE(root.has_value());
    
    // Compute the path for hash1
    auto path1 = MerkleTree::ComputePath(hashes, 0);
    EXPECT_EQ(path1.size(), 2);
    EXPECT_EQ(path1[0], hash2);
    
    // Verify the path for hash1
    bool valid1 = MerkleTree::VerifyPath(hash1, path1, 0, *root);
    EXPECT_TRUE(valid1);
    
    // Compute the path for hash2
    auto path2 = MerkleTree::ComputePath(hashes, 1);
    EXPECT_EQ(path2.size(), 2);
    EXPECT_EQ(path2[0], hash1);
    
    // Verify the path for hash2
    bool valid2 = MerkleTree::VerifyPath(hash2, path2, 1, *root);
    EXPECT_TRUE(valid2);
    
    // Compute the path for hash3
    auto path3 = MerkleTree::ComputePath(hashes, 2);
    EXPECT_EQ(path3.size(), 2);
    EXPECT_EQ(path3[0], hash4);
    
    // Verify the path for hash3
    bool valid3 = MerkleTree::VerifyPath(hash3, path3, 2, *root);
    EXPECT_TRUE(valid3);
    
    // Compute the path for hash4
    auto path4 = MerkleTree::ComputePath(hashes, 3);
    EXPECT_EQ(path4.size(), 2);
    EXPECT_EQ(path4[0], hash3);
    
    // Verify the path for hash4
    bool valid4 = MerkleTree::VerifyPath(hash4, path4, 3, *root);
    EXPECT_TRUE(valid4);
    
    // Invalid index
    EXPECT_THROW(MerkleTree::ComputePath(hashes, 4), std::out_of_range);
    
    // Empty hashes
    auto emptyPath = MerkleTree::ComputePath({}, 0);
    EXPECT_TRUE(emptyPath.empty());
}

TEST(MerkleTreeTest, VerifyPath)
{
    UInt256 hash1 = Hash::Sha256(ByteVector{1, 2, 3}.AsSpan());
    UInt256 hash2 = Hash::Sha256(ByteVector{4, 5, 6}.AsSpan());
    std::vector<UInt256> hashes = {hash1, hash2};
    
    // Compute the root
    auto root = MerkleTree::ComputeRoot(hashes);
    EXPECT_TRUE(root.has_value());
    
    // Compute the path for hash1
    auto path1 = MerkleTree::ComputePath(hashes, 0);
    EXPECT_EQ(path1.size(), 1);
    EXPECT_EQ(path1[0], hash2);
    
    // Verify the path for hash1
    bool valid1 = MerkleTree::VerifyPath(hash1, path1, 0, *root);
    EXPECT_TRUE(valid1);
    
    // Invalid leaf
    UInt256 invalidLeaf = Hash::Sha256(ByteVector{7, 8, 9}.AsSpan());
    bool invalidLeafValid = MerkleTree::VerifyPath(invalidLeaf, path1, 0, *root);
    EXPECT_FALSE(invalidLeafValid);
    
    // Invalid path
    std::vector<UInt256> invalidPath = {invalidLeaf};
    bool invalidPathValid = MerkleTree::VerifyPath(hash1, invalidPath, 0, *root);
    EXPECT_FALSE(invalidPathValid);
    
    // Invalid index
    bool invalidIndexValid = MerkleTree::VerifyPath(hash1, path1, 1, *root);
    EXPECT_FALSE(invalidIndexValid);
    
    // Invalid root
    UInt256 invalidRoot = Hash::Sha256(ByteVector{10, 11, 12}.AsSpan());
    bool invalidRootValid = MerkleTree::VerifyPath(hash1, path1, 0, invalidRoot);
    EXPECT_FALSE(invalidRootValid);
    
    // Empty path
    bool emptyPathValid = MerkleTree::VerifyPath(hash1, {}, 0, hash1);
    EXPECT_TRUE(emptyPathValid);
}
