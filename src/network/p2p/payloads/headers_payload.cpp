#include <neo/network/p2p/payloads/headers_payload.h>

namespace neo::network::p2p::payloads
{
HeadersPayload::HeadersPayload() = default;

HeadersPayload::HeadersPayload(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers) : headers_(headers) {}

const std::vector<std::shared_ptr<ledger::BlockHeader>>& HeadersPayload::GetHeaders() const
{
    return headers_;
}

void HeadersPayload::SetHeaders(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers)
{
    headers_ = headers;
}

void HeadersPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(headers_.size());

    for (const auto& header : headers_)
    {
        header->Serialize(writer);
    }
}

int HeadersPayload::GetSize() const
{
    // Calculate the size based on headers
    // This matches the C# implementation: Headers.GetVarSize()
    int size = 0;

    // VarInt for count
    if (headers_.size() < 0xFD)
        size += sizeof(uint8_t);
    else if (headers_.size() <= 0xFFFF)
        size += sizeof(uint8_t) + sizeof(uint16_t);
    else
        size += sizeof(uint8_t) + sizeof(uint32_t);

    // Each header has its own size
    for (const auto& header : headers_)
    {
        size += header->GetSize();
    }

    return size;
}

HeadersPayload HeadersPayload::Create(const std::vector<std::shared_ptr<ledger::BlockHeader>>& headers)
{
    return HeadersPayload(headers);
}

void HeadersPayload::Deserialize(io::BinaryReader& reader)
{
    uint64_t count = reader.ReadVarInt(MaxHeadersCount);
    headers_.clear();
    headers_.reserve(count);

    for (uint64_t i = 0; i < count; i++)
    {
        auto header = std::make_shared<ledger::BlockHeader>();
        header->Deserialize(reader);
        headers_.push_back(header);
    }
}

void HeadersPayload::SerializeJson(io::JsonWriter& writer) const
{
    nlohmann::json headersArray = nlohmann::json::array();

    for (const auto& header : headers_)
    {
        nlohmann::json headerJson = nlohmann::json::object();
        io::JsonWriter headerWriter(headerJson);
        header->SerializeJson(headerWriter);
        headersArray.push_back(headerJson);
    }

    writer.Write("headers", headersArray);
}

void HeadersPayload::DeserializeJson(const io::JsonReader& reader)
{
    auto headersArray = reader.ReadArray("headers");
    headers_.clear();
    headers_.reserve(headersArray.size());

    for (const auto& headerJson : headersArray)
    {
        auto header = std::make_shared<ledger::BlockHeader>();
        io::JsonReader headerReader(headerJson);
        header->DeserializeJson(headerReader);
        headers_.push_back(header);
    }
}
}  // namespace neo::network::p2p::payloads
