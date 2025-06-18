#include <neo/cryptography/merkletree.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <stdexcept>
#include <cmath>
#include <cstring>

namespace neo::cryptography
{
    std::optional<io::UInt256> MerkleTree::ComputeRootOptional(const std::vector<io::UInt256>& hashes)
    {
        if (hashes.empty())
            return std::nullopt;

        return ComputeRoot(std::vector<io::UInt256>(hashes));
    }

    io::UInt256 MerkleTree::ComputeRoot(std::vector<io::UInt256> hashes)
    {
        if (hashes.size() == 1)
            return hashes[0];

        while (hashes.size() > 1)
        {
            if (hashes.size() % 2 != 0)
                hashes.push_back(hashes.back());

            std::vector<io::UInt256> parents;
            for (size_t i = 0; i < hashes.size(); i += 2)
            {
                parents.push_back(ComputeParent(hashes[i], hashes[i + 1]));
            }

            hashes = std::move(parents);
        }

        return hashes[0];
    }

    io::UInt256 MerkleTree::ComputeParent(const io::UInt256& left, const io::UInt256& right)
    {
        // Create a buffer to hold both hashes
        io::ByteVector buffer(64);

        // Copy left hash to buffer
        for (size_t i = 0; i < 32; i++)
        {
            buffer[i] = left.Data()[i];
        }

        // Copy right hash to buffer
        for (size_t i = 0; i < 32; i++)
        {
            buffer[32 + i] = right.Data()[i];
        }

        // Hash the buffer
        return Hash::Hash256(buffer.AsSpan());
    }

    std::vector<io::UInt256> MerkleTree::ComputePath(const std::vector<io::UInt256>& hashes, size_t index)
    {
        if (hashes.empty())
            return {};

        if (index >= hashes.size())
            throw std::out_of_range("Index out of range");

        std::vector<io::UInt256> path;
        std::vector<io::UInt256> level = hashes;

        while (level.size() > 1)
        {
            if (level.size() % 2 != 0)
                level.push_back(level.back());

            std::vector<io::UInt256> parents;
            for (size_t i = 0; i < level.size(); i += 2)
            {
                if (i == index || i + 1 == index)
                {
                    path.push_back(level[i ^ 1]);
                    index = i / 2;
                }

                parents.push_back(ComputeParent(level[i], level[i + 1]));
            }

            level = std::move(parents);
        }

        return path;
    }

    bool MerkleTree::VerifyPath(const io::UInt256& leaf, const std::vector<io::UInt256>& path, size_t index, const io::UInt256& root)
    {
        if (path.empty())
            return leaf == root;

        io::UInt256 hash = leaf;

        for (size_t i = 0; i < path.size(); i++)
        {
            if (index % 2 == 0)
            {
                hash = ComputeParent(hash, path[i]);
            }
            else
            {
                hash = ComputeParent(path[i], hash);
            }

            index /= 2;
        }

        return hash == root;
    }
}
