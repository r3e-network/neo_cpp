#pragma once

#include <neo/io/uint256.h>
#include <vector>

namespace neo::cryptography
{
    /**
     * @brief Provides Merkle tree functionality.
     */
    class MerkleTree
    {
    public:
        /**
         * @brief Computes the Merkle root from a list of hashes.
         * @param hashes The list of hashes.
         * @return The Merkle root.
         */
        static io::UInt256 ComputeRoot(const std::vector<io::UInt256>& hashes);

        /**
         * @brief Computes the Merkle root from a single hash.
         * @param hash The hash.
         * @return The same hash (root of single-element tree).
         */
        static io::UInt256 ComputeRoot(const io::UInt256& hash);

    private:
        /**
         * @brief Combines two hashes into one.
         * @param left The left hash.
         * @param right The right hash.
         * @return The combined hash.
         */
        static io::UInt256 CombineHash(const io::UInt256& left, const io::UInt256& right);
    };
}