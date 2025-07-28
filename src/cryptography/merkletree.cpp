#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/byte_vector.h>

namespace neo::cryptography
{
io::UInt256 MerkleTree::ComputeRoot(std::vector<io::UInt256> hashes)
{
    if (hashes.empty())
    {
        return io::UInt256();
    }

    if (hashes.size() == 1)
    {
        return hashes[0];
    }

    std::vector<io::UInt256> current = std::move(hashes);

    while (current.size() > 1)
    {
        std::vector<io::UInt256> next;

        for (size_t i = 0; i < current.size(); i += 2)
        {
            if (i + 1 < current.size())
            {
                // Combine two hashes
                next.push_back(ComputeParent(current[i], current[i + 1]));
            }
            else
            {
                // Odd number of hashes, duplicate the last one
                next.push_back(ComputeParent(current[i], current[i]));
            }
        }

        current = std::move(next);
    }

    return current[0];
}

io::UInt256 MerkleTree::ComputeParent(const io::UInt256& left, const io::UInt256& right)
{
    // Combine two hashes by concatenating them and hashing the result
    io::ByteVector combined;
    const auto& leftData = left.GetData();
    const auto& rightData = right.GetData();
    combined.insert(combined.end(), leftData.begin(), leftData.end());
    combined.insert(combined.end(), rightData.begin(), rightData.end());

    return Hash::Hash256(io::ByteSpan(combined.Data(), combined.Size()));
}

std::vector<io::UInt256> MerkleTree::GetProof(const std::vector<io::UInt256>& hashes, size_t index)
{
    std::vector<io::UInt256> proof;
    
    if (hashes.empty() || index >= hashes.size())
    {
        return proof;
    }
    
    if (hashes.size() == 1)
    {
        return proof;
    }
    
    std::vector<io::UInt256> current = hashes;
    size_t currentIndex = index;
    
    while (current.size() > 1)
    {
        std::vector<io::UInt256> next;
        
        for (size_t i = 0; i < current.size(); i += 2)
        {
            if (i == currentIndex || i + 1 == currentIndex)
            {
                // Add sibling to proof
                if (i == currentIndex && i + 1 < current.size())
                {
                    proof.push_back(current[i + 1]);
                }
                else if (i + 1 == currentIndex)
                {
                    proof.push_back(current[i]);
                }
                else
                {
                    // Odd number, duplicate last hash
                    proof.push_back(current[i]);
                }
                
                // Update index for next level
                currentIndex = i / 2;
            }
            
            if (i + 1 < current.size())
            {
                next.push_back(ComputeParent(current[i], current[i + 1]));
            }
            else
            {
                next.push_back(ComputeParent(current[i], current[i]));
            }
        }
        
        current = std::move(next);
    }
    
    return proof;
}
}  // namespace neo::cryptography