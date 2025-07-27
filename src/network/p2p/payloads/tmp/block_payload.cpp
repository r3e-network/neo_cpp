#include <neo/network/payloads/block_payload.h>

namespace neo::network::payloads
{
BlockPayload::BlockPayload() = default;

BlockPayload::BlockPayload(std::shared_ptr<blockchain::Block> block) : block_(block) {}

std::shared_ptr<blockchain::Block> BlockPayload::GetBlock() const
{
    return block_;
}

void BlockPayload::SetBlock(std::shared_ptr<blockchain::Block> block)
{
    block_ = block;
}

void BlockPayload::Serialize(io::BinaryWriter& writer) const
{
    if (block_)
    {
        writer.Write(*block_);
    }
}

void BlockPayload::Deserialize(io::BinaryReader& reader)
{
    block_ = std::make_shared<blockchain::Block>();
    block_->Deserialize(reader);
}

void BlockPayload::SerializeJson(io::JsonWriter& writer) const
{
    if (block_)
    {
        writer.WriteStartObject("block");
        block_->SerializeJson(writer);
        writer.WriteEndObject();
    }
    else
    {
        writer.Write("block", nullptr);
    }
}

void BlockPayload::DeserializeJson(const io::JsonReader& reader)
{
    if (reader.HasField("block") && !reader.IsNull("block"))
    {
        block_ = std::make_shared<blockchain::Block>();
        io::JsonReader blockReader = reader.ReadObject("block");
        block_->DeserializeJson(blockReader);
    }
}
}  // namespace neo::network::payloads
