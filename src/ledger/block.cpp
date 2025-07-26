#include <neo/ledger/block.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>

namespace neo::ledger
{
    io::UInt256 Block::GetHash() const
    {
        if (!hash_calculated_)
        {
            hash_ = CalculateHash();
            hash_calculated_ = true;
        }
        return hash_;
    }
    
    io::UInt256 Block::CalculateHash() const
    {
        io::ByteVector buffer;
        io::BinaryWriter writer(buffer);
        
        // Serialize block header for hashing
        writer.Write(version_);
        writer.Write(previous_hash_);
        writer.Write(merkle_root_);
        writer.Write(static_cast<uint64_t>(timestamp_.time_since_epoch().count()));
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primary_index_);
        writer.Write(next_consensus_);
        
        return cryptography::Hash::Hash256(io::ByteSpan(buffer.Data(), buffer.Size()));
    }

    uint32_t Block::GetSize() const
    {
        uint32_t size = 0;
        
        // Header size
        size += sizeof(version_);
        size += io::UInt256::Size;
        size += io::UInt256::Size;
        size += sizeof(uint64_t); // timestamp
        size += sizeof(uint64_t); // nonce
        size += sizeof(index_);
        size += sizeof(primary_index_);
        size += io::UInt160::Size;
        
        // Witness size
        size += witness_.GetSize();
        
        // Transactions size
        size += sizeof(uint32_t); // transaction count
        for (const auto& tx : transactions_)
        {
            size += static_cast<uint32_t>(tx.GetSize());
        }
        
        return size;
    }

    void Block::Serialize(io::BinaryWriter& writer) const
    {
        // Serialize header
        writer.Write(version_);
        writer.Write(previous_hash_);
        writer.Write(merkle_root_);
        writer.Write(static_cast<uint64_t>(timestamp_.time_since_epoch().count()));
        writer.Write(nonce_);
        writer.Write(index_);
        writer.Write(primary_index_);
        writer.Write(next_consensus_);
        
        // Serialize witness
        witness_.Serialize(writer);
        
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
        version_ = reader.ReadUInt32();
        previous_hash_ = reader.Read<io::UInt256>();
        merkle_root_ = reader.Read<io::UInt256>();
        
        auto timestamp_ticks = reader.ReadUInt64();
        timestamp_ = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(timestamp_ticks));
        
        nonce_ = reader.ReadUInt64();
        index_ = reader.ReadUInt32();
        primary_index_ = reader.ReadUInt32();
        next_consensus_ = reader.Read<io::UInt160>();
        
        // Deserialize witness
        witness_.Deserialize(reader);
        
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
}