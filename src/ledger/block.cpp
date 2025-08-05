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

io::UInt256 Block::CalculateHash() const
{
    return header_.GetHash();
}

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
}  // namespace neo::ledger