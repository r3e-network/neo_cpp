#include <neo/cryptography/hash.h>
#include <neo/cryptography/merkletree.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block.h>
#include <sstream>

namespace neo::ledger
{
// BlockHeader JSON serialization/deserialization
void BlockHeader::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("version", version_);
    writer.Write("previousblockhash", prevHash_);
    writer.Write("merkleroot", merkleRoot_);
    writer.Write("time", timestamp_);
    writer.Write("index", index_);
    writer.Write("primary", GetPrimaryIndex());
    writer.Write("nextconsensus", nextConsensus_);

    // Serialize the witness
    nlohmann::json witnessJson = nlohmann::json::object();
    io::JsonWriter witnessWriter(witnessJson);
    witness_.SerializeJson(witnessWriter);
    writer.Write("witness", witnessJson);

    // Add the hash
    writer.Write("hash", GetHash());
}

void BlockHeader::DeserializeJson(const io::JsonReader& reader)
{
    version_ = reader.ReadUInt32("version");
    prevHash_ = reader.ReadUInt256("previousblockhash");
    merkleRoot_ = reader.ReadUInt256("merkleroot");
    timestamp_ = reader.ReadUInt64("time");
    index_ = reader.ReadUInt32("index");
    nextConsensus_ = reader.ReadUInt160("nextconsensus");

    // Deserialize the witness
    auto witnessJson = reader.ReadObject("witness");
    io::JsonReader witnessReader(witnessJson);
    witness_.DeserializeJson(witnessReader);
}

// Block JSON serialization/deserialization
void Block::SerializeJson(io::JsonWriter& writer) const
{
    // Serialize the block header as a BlockHeader
    BlockHeader header(*this);
    header.SerializeJson(writer);

    // Serialize the transactions
    nlohmann::json txArray = nlohmann::json::array();
    for (const auto& tx : transactions_)
    {
        nlohmann::json txJson = nlohmann::json::object();
        io::JsonWriter txWriter(txJson);
        tx->SerializeJson(txWriter);
        txArray.push_back(txJson);
    }
    writer.Write("tx", txArray);

    // Calculate actual block size
    try
    {
        // Get the actual serialized size of the block
        size_t actual_size = GetSize();
        writer.Write("size", static_cast<int64_t>(actual_size));
    }
    catch (const std::exception& e)
    {
        // Fallback to calculating size from components if GetSize() is not available
        size_t estimated_size = 0;

        // Block header size (version + prevHash + merkleRoot + timestamp + index + primaryIndex + nextConsensus +
        // witness)
        estimated_size += sizeof(uint32_t);  // version
        estimated_size += 32;                // prevHash (UInt256)
        estimated_size += 32;                // merkleRoot (UInt256)
        estimated_size += sizeof(uint64_t);  // timestamp
        estimated_size += sizeof(uint32_t);  // index
        estimated_size += sizeof(uint32_t);  // primaryIndex
        estimated_size += 20;                // nextConsensus (UInt160)
        estimated_size += 1;                 // witness count (varint)

        // Add witness size estimation
        if (GetWitness())
        {
            estimated_size += GetWitness()->GetSize();
        }

        // Add transactions size
        const auto& transactions = GetTransactions();
        estimated_size += GetVarintSize(transactions.size());  // transaction count
        for (const auto& tx : transactions)
        {
            if (tx)
            {
                estimated_size += tx->GetSize();
            }
        }

        writer.Write("size", static_cast<int64_t>(estimated_size));
    }
}

void Block::DeserializeJson(const io::JsonReader& reader)
{
    // Deserialize the block header
    BlockHeader header;
    header.DeserializeJson(reader);

    // Copy the header fields to this block
    version_ = header.GetVersion();
    prevHash_ = header.GetPrevHash();
    merkleRoot_ = header.GetMerkleRoot();
    timestamp_ = header.GetTimestamp();
    index_ = header.GetIndex();
    nextConsensus_ = header.GetNextConsensus();
    witness_ = header.GetWitness();

    // Deserialize the transactions
    auto txArray = reader.ReadArray("tx");
    transactions_.clear();
    transactions_.reserve(txArray.size());

    for (const auto& txJson : txArray)
    {
        auto tx = std::make_shared<Neo3Transaction>();
        io::JsonReader txReader(txJson);
        tx->DeserializeJson(txReader);
        transactions_.push_back(tx);
    }

    // Rebuild the merkle root
    RebuildMerkleRoot();
}
}  // namespace neo::ledger
