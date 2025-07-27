#include <neo/network/p2p/payloads/block_payload.h>

namespace neo::network::p2p::payloads
{
BlockPayload::BlockPayload() = default;

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
        nlohmann::json blockJson = nlohmann::json::object();
        io::JsonWriter blockWriter(blockJson);
        block_->SerializeJson(blockWriter);
        writer.WriteProperty("block", blockJson);
    }
    writer.WriteEndObject();
}

void BlockPayload::DeserializeJson(const io::JsonReader& reader)
{
    if (reader.GetJson().contains("block") && reader.GetJson()["block"].is_object())
    {
        block_ = std::make_shared<ledger::Block>();
        io::JsonReader blockReader(reader.GetJson()["block"]);
        block_->DeserializeJson(blockReader);
    }
}
}  // namespace neo::network::p2p::payloads
