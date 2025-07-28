#pragma once

#include <neo/io/uint256.h>
#include <optional>
#include <vector>

namespace neo::cryptography
{
/**
 * @brief Represents a Merkle tree.
 */
class MerkleTree
{
  public:
    /**
     * @brief Computes the Merkle root of the specified hashes.
     * @param hashes The hashes.
     * @return The Merkle root, or std::nullopt if the hashes vector is empty.
     */
    static std::optional<io::UInt256> ComputeRootOptional(const std::vector<io::UInt256>& hashes);

    /**
     * @brief Computes the Merkle path of the specified leaf hash.
     * @param hashes The hashes.
     * @param index The index of the leaf hash.
     * @return The Merkle path.
     * @throws std::out_of_range if the index is out of range.
     */
    static std::vector<io::UInt256> ComputePath(const std::vector<io::UInt256>& hashes, size_t index);

    /**
     * @brief Verifies the Merkle path of the specified leaf hash.
     * @param leaf The leaf hash.
     * @param path The Merkle path.
     * @param index The index of the leaf hash.
     * @param root The Merkle root.
     * @return True if the Merkle path is valid, false otherwise.
     */
    static bool VerifyPath(const io::UInt256& leaf, const std::vector<io::UInt256>& path, size_t index,
                           const io::UInt256& root);

  public:
    /**
     * @brief Computes the Merkle root of the specified hashes.
     * @param hashes The hashes.
     * @return The Merkle root.
     */
    static io::UInt256 ComputeRoot(std::vector<io::UInt256> hashes);

    /**
     * @brief Computes the parent hash of the specified left and right hashes.
     * @param left The left hash.
     * @param right The right hash.
     * @return The parent hash.
     */
    static io::UInt256 ComputeParent(const io::UInt256& left, const io::UInt256& right);

    /**
     * @brief Gets the Merkle proof for a specific element.
     * @param hashes The hashes.
     * @param index The index of the element to get proof for.
     * @return The Merkle proof path.
     */
    static std::vector<io::UInt256> GetProof(const std::vector<io::UInt256>& hashes, size_t index);
};
}  // namespace neo::cryptography
