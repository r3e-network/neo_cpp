#include <neo/network/p2p/payloads/block_payload.h>

namespace neo::network::p2p::payloads
{
BlockPayload::BlockPayload() = default;

BlockPayload::~BlockPayload() = default;

BlockPayload::BlockPayload(std::shared_ptr<ledger::Block> block) : block_(block) {}

std::shared_ptr<ledger::Block> BlockPayload::GetBlock() const
{
    return block_;
}

void BlockPayload::SetBlock(std::shared_ptr<ledger::Block> block)
{
    block_ = block;
}

int BlockPayload::GetSize() const
{
    return block_ ? static_cast<int>(block_->GetSize()) : 0;
}

BlockPayload BlockPayload::Create(std::shared_ptr<ledger::Block> block)
{
    return BlockPayload(block);
}

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
        writer.WriteProperty("timestamp", static_cast<uint64_t>(block_->GetTimestamp().time_since_epoch().count()));
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
    // Block doesn't implement IJsonSerializable, so we can't deserialize it from JSON
    // This would require implementing JSON deserialization for Block
    throw std::runtime_error("BlockPayload JSON deserialization not implemented");
}
}  // namespace neo::network::p2p::payloads
