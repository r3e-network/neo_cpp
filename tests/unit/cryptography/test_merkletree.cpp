#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkletree.h>
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
    UInt256 expected = Hash::Hash256(buffer.AsSpan());

    EXPECT_EQ(root, expected);
}

TEST(MerkleTreeTest, BuildAndDepthFirstSearch)
{
    // Based on C# UT_MerkleTree.TestBuildAndDepthFirstSearch()
    ByteVector array1{1, 2, 3};
    ByteVector array2{4, 5, 6};
    ByteVector array3{7, 8, 9};

    UInt256 hash1 = Hash::Sha256(array1.AsSpan());
    UInt256 hash2 = Hash::Sha256(array2.AsSpan());
    UInt256 hash3 = Hash::Sha256(array3.AsSpan());

    std::vector<UInt256> hashes = {hash1, hash2, hash3};

    auto root = MerkleTree::ComputeRoot(hashes);

    // For odd number of hashes, the last hash should be duplicated
    // First level: hash1+hash2, hash3+hash3
    ByteVector buffer1(UInt256::Size * 2);
    std::memcpy(buffer1.Data(), hash1.Data(), UInt256::Size);
    std::memcpy(buffer1.Data() + UInt256::Size, hash2.Data(), UInt256::Size);
    UInt256 parent1 = Hash::Hash256(buffer1.AsSpan());

    ByteVector buffer2(UInt256::Size * 2);
    std::memcpy(buffer2.Data(), hash3.Data(), UInt256::Size);
    std::memcpy(buffer2.Data() + UInt256::Size, hash3.Data(), UInt256::Size);
    UInt256 parent2 = Hash::Hash256(buffer2.AsSpan());

    // Second level: parent1+parent2
    ByteVector buffer3(UInt256::Size * 2);
    std::memcpy(buffer3.Data(), parent1.Data(), UInt256::Size);
    std::memcpy(buffer3.Data() + UInt256::Size, parent2.Data(), UInt256::Size);
    UInt256 expected = Hash::Hash256(buffer3.AsSpan());

    EXPECT_EQ(root, expected);
}