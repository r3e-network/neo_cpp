#include <neo/network/payloads/headers_payload.h>
#include <neo/blockchain/header.h>
#include <limits>
#include <stdexcept>

namespace neo::network::payloads
{
    HeadersPayload::HeadersPayload() = default;

    HeadersPayload::HeadersPayload(const std::vector<std::shared_ptr<blockchain::Header>>& headers)
        : headers_(headers)
    {
    }

    const std::vector<std::shared_ptr<blockchain::Header>>& HeadersPayload::GetHeaders() const
    {
        return headers_;
    }

    void HeadersPayload::SetHeaders(const std::vector<std::shared_ptr<blockchain::Header>>& headers)
    {
        headers_ = headers;
    }

    void HeadersPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteVarInt(headers_.size());
        for (const auto& header : headers_)
        {
            writer.Write(*header);
        }
    }

    void HeadersPayload::Deserialize(io::BinaryReader& reader)
    {
        int64_t count = reader.ReadVarInt();
        if (count < 0 || count > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid header count");
        
        headers_.clear();
        headers_.reserve(static_cast<size_t>(count));
        
        for (int64_t i = 0; i < count; i++)
        {
            auto header = std::make_shared<blockchain::Header>();
            header->Deserialize(reader);
            headers_.push_back(header);
        }
    }

    void HeadersPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartArray("headers");
        for (const auto& header : headers_)
        {
            writer.WriteStartObject();
            header->SerializeJson(writer);
            writer.WriteEndObject();
        }
        writer.WriteEndArray();
    }

    void HeadersPayload::DeserializeJson(const io::JsonReader& reader)
    {
        auto headersArray = reader.ReadArray("headers");
        headers_.clear();
        headers_.reserve(headersArray.size());
        
        for (const auto& headerJson : headersArray)
        {
            auto header = std::make_shared<blockchain::Header>();
            io::JsonReader headerReader(headerJson);
            header->DeserializeJson(headerReader);
            headers_.push_back(header);
        }
    }
}
