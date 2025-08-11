#include <neo/network/p2p/payloads/block_payload.h>

namespace neo::network::p2p::payloads
{
BlockPayload::BlockPayload() = default;

BlockPayload::~BlockPayload() = default;

BlockPayload::BlockPayload(std::shared_ptr<ledger::Block> block) : block_(block) {}

std::shared_ptr<ledger::Block> BlockPayload::GetBlock() const { return block_; }

void BlockPayload::SetBlock(std::shared_ptr<ledger::Block> block) { block_ = block; }

int BlockPayload::GetSize() const { return block_ ? static_cast<int>(block_->GetSize()) : 0; }

BlockPayload BlockPayload::Create(std::shared_ptr<ledger::Block> block) { return BlockPayload(block); }

void BlockPayload::Serialize(io::BinaryWriter& writer) const
{
    if (block_)
    {
        block_->Serialize(writer);
    }
}

void BlockPayload::Deserialize(io::BinaryReader& reader)
{
    block_ = std::make_shared<ledger::Block>();
    block_->Deserialize(reader);
}

void BlockPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    if (block_)
    {
        // Block doesn't implement IJsonSerializable, so serialize basic fields
        writer.WritePropertyName("block");
        writer.WriteStartObject();
        writer.WriteProperty("version", block_->GetVersion());
        writer.WriteProperty("previousHash", block_->GetPreviousHash().ToString());
        writer.WriteProperty("merkleRoot", block_->GetMerkleRoot().ToString());
        writer.WriteProperty("timestamp", static_cast<uint64_t>(block_->GetTimestamp()));
        writer.WriteProperty("nonce", block_->GetNonce());
        writer.WriteProperty("index", block_->GetIndex());
        writer.WriteProperty("primaryIndex", block_->GetPrimaryIndex());
        writer.WriteProperty("nextConsensus", block_->GetNextConsensus().ToString());
        writer.WriteEndObject();
    }
    writer.WriteEndObject();
}

void BlockPayload::DeserializeJson(const io::JsonReader& reader)
{
    // Create new block and populate from JSON
    block_ = std::make_shared<ledger::Block>();

    // Read block properties from JSON using the correct API
    auto version = reader.ReadUInt32("version");
    auto previousHashStr = reader.ReadString("previousHash");
    auto merkleRootStr = reader.ReadString("merkleRoot");
    auto timestamp = reader.ReadUInt64("timestamp");
    auto nonce = reader.ReadUInt64("nonce");
    auto index = reader.ReadUInt32("index");
    auto primaryIndex = reader.ReadUInt8("primaryIndex");
    auto nextConsensusStr = reader.ReadString("nextConsensus");

    // Convert strings to appropriate types
    io::UInt256 previousHash = io::UInt256::Parse(previousHashStr);
    io::UInt256 merkleRoot = io::UInt256::Parse(merkleRootStr);
    io::UInt160 nextConsensus = io::UInt160::Parse(nextConsensusStr);

    // Set block properties (assuming Block has appropriate setters or constructor)
    // Note: This assumes Block class has methods to set these properties
    // Using the Block API to set deserialized properties
    block_->SetVersion(version);
    block_->SetPreviousHash(previousHash);
    block_->SetMerkleRoot(merkleRoot);
    block_->SetTimestamp(timestamp);
    block_->SetNonce(nonce);
    block_->SetIndex(index);
    block_->SetPrimaryIndex(primaryIndex);
    block_->SetNextConsensus(nextConsensus);
}
}  // namespace neo::network::p2p::payloads
