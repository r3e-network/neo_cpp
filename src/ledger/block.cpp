#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block.h>

namespace neo::ledger
{
io::UInt256 Block::GetHash() const
{
    if (!hash_calculated_)
    {
        hash_ = header_.GetHash();
        hash_calculated_ = true;
    }
    return hash_;
}

io::UInt256 Block::CalculateHash() const { return header_.GetHash(); }

uint32_t Block::GetSize() const
{
    uint32_t size = header_.GetSize();

    // Transactions size
    size += sizeof(uint32_t);  // transaction count
    for (const auto& tx : transactions_)
    {
        size += static_cast<uint32_t>(tx.GetSize());
    }

    return size;
}

void Block::Serialize(io::BinaryWriter& writer) const
{
    // Serialize header
    header_.Serialize(writer);

    // Serialize transactions
    writer.Write(static_cast<uint32_t>(transactions_.size()));
    for (const auto& tx : transactions_)
    {
        tx.Serialize(writer);
    }
}

void Block::Deserialize(io::BinaryReader& reader)
{
    // Deserialize header
    header_.Deserialize(reader);

    // Deserialize transactions
    auto tx_count = reader.ReadUInt32();
    transactions_.clear();
    transactions_.reserve(tx_count);

    for (uint32_t i = 0; i < tx_count; ++i)
    {
        Transaction tx;
        tx.Deserialize(reader);
        transactions_.push_back(std::move(tx));
    }

    // Clear cached hash
    hash_calculated_ = false;
}

io::UInt256 Block::ComputeMerkleRoot() const
{
    if (transactions_.empty())
    {
        return io::UInt256::Zero();
    }
    
    std::vector<io::UInt256> hashes;
    hashes.reserve(transactions_.size());
    
    // Get transaction hashes
    for (const auto& tx : transactions_)
    {
        hashes.push_back(tx.GetHash());
    }
    
    // Build merkle tree
    while (hashes.size() > 1)
    {
        std::vector<io::UInt256> new_hashes;
        new_hashes.reserve((hashes.size() + 1) / 2);
        
        for (size_t i = 0; i < hashes.size(); i += 2)
        {
            if (i + 1 < hashes.size())
            {
                // Combine two hashes
                io::ByteVector combined;
                auto bytes_i = hashes[i].ToArray();
                auto bytes_i1 = hashes[i + 1].ToArray();
                combined.insert(combined.end(), bytes_i.begin(), bytes_i.end());
                combined.insert(combined.end(), bytes_i1.begin(), bytes_i1.end());
                new_hashes.push_back(cryptography::Hash::Sha256(combined.AsSpan()));
            }
            else
            {
                // Odd number - duplicate last hash
                io::ByteVector combined;
                auto bytes_i = hashes[i].ToArray();
                combined.insert(combined.end(), bytes_i.begin(), bytes_i.end());
                combined.insert(combined.end(), bytes_i.begin(), bytes_i.end());
                new_hashes.push_back(cryptography::Hash::Sha256(combined.AsSpan()));
            }
        }
        
        hashes = std::move(new_hashes);
    }
    
    return hashes.empty() ? io::UInt256::Zero() : hashes[0];
}

bool Block::VerifyWitnesses() const
{
    // Basic witness verification
    // In a production implementation, this would verify signatures against the consensus nodes
    
    const auto& witness = header_.GetWitness();
    
    // Check if witness exists
    if (witness.GetInvocationScript().empty() || witness.GetVerificationScript().empty())
    {
        return false;
    }
    
    // In production, this would:
    // 1. Execute the verification script
    // 2. Check signatures against consensus node public keys
    // 3. Verify the required number of signatures (M of N multisig)
    
    // For now, do basic validation
    return witness.GetInvocationScript().Size() > 0 && 
           witness.GetVerificationScript().Size() > 0;
}
}  // namespace neo::ledger