/**
 * @file payload_fixes.cpp
 * @brief Payload Fixes
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/crypto.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>

namespace neo::network::p2p::payloads
{

// PingPayload JSON serialization
void PingPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("timestamp");
    writer.WriteValue(timestamp_);
    writer.WritePropertyName("nonce");
    writer.WriteValue(nonce_);
    writer.WriteEndObject();
}

void PingPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "timestamp")
            {
                timestamp_ = reader.GetUInt64();
            }
            else if (name == "nonce")
            {
                nonce_ = reader.GetUInt32();
            }
        }
    }
}

// InvPayload JSON serialization
void InvPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("type");
    writer.WriteValue(static_cast<uint8_t>(type_));
    writer.WritePropertyName("hashes");
    writer.WriteStartArray();
    for (const auto& hash : hashes_)
    {
        writer.WriteValue(hash.ToString());
    }
    writer.WriteEndArray();
    writer.WriteEndObject();
}

void InvPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "type")
            {
                type_ = static_cast<InventoryType>(reader.GetUInt8());
            }
            else if (name == "hashes")
            {
                reader.ReadStartArray();
                hashes_.clear();
                while (reader.Read() && reader.GetTokenType() != io::JsonTokenType::EndArray)
                {
                    hashes_.push_back(io::UInt256::Parse(reader.GetString()));
                }
            }
        }
    }
}

// GetDataPayload JSON serialization
void GetDataPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("type");
    writer.WriteValue(static_cast<uint8_t>(type_));
    writer.WritePropertyName("hashes");
    writer.WriteStartArray();
    for (const auto& hash : hashes_)
    {
        writer.WriteValue(hash.ToString());
    }
    writer.WriteEndArray();
    writer.WriteEndObject();
}

void GetDataPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "type")
            {
                type_ = static_cast<InventoryType>(reader.GetUInt8());
            }
            else if (name == "hashes")
            {
                reader.ReadStartArray();
                hashes_.clear();
                while (reader.Read() && reader.GetTokenType() != io::JsonTokenType::EndArray)
                {
                    hashes_.push_back(io::UInt256::Parse(reader.GetString()));
                }
            }
        }
    }
}

// GetBlocksPayload JSON serialization
void GetBlocksPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("hashStart");
    writer.WriteValue(hashStart_.ToString());
    writer.WritePropertyName("count");
    writer.WriteValue(count_);
    writer.WriteEndObject();
}

void GetBlocksPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "hashStart")
            {
                hashStart_ = io::UInt256::Parse(reader.GetString());
            }
            else if (name == "count")
            {
                count_ = reader.GetInt16();
            }
        }
    }
}

// GetBlockByIndexPayload JSON serialization
void GetBlockByIndexPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("indexStart");
    writer.WriteValue(indexStart_);
    writer.WritePropertyName("count");
    writer.WriteValue(count_);
    writer.WriteEndObject();
}

void GetBlockByIndexPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "indexStart")
            {
                indexStart_ = reader.GetUInt32();
            }
            else if (name == "count")
            {
                count_ = reader.GetInt16();
            }
        }
    }
}

// GetHeadersPayload JSON serialization (delegate to GetBlocksPayload)
void GetHeadersPayload::SerializeJson(io::JsonWriter& writer) const
{
    // GetHeadersPayload is the same as GetBlocksPayload
    writer.WriteStartObject();
    writer.WritePropertyName("hashStart");
    writer.WriteValue(GetHashStart().ToString());
    writer.WritePropertyName("count");
    writer.WriteValue(GetCount());
    writer.WriteEndObject();
}

void GetHeadersPayload::DeserializeJson(const io::JsonReader& reader)
{
    io::UInt256 hashStart;
    int16_t count = -1;

    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "hashStart")
            {
                hashStart = io::UInt256::Parse(reader.GetString());
            }
            else if (name == "count")
            {
                count = reader.GetInt16();
            }
        }
    }

    SetHashStart(hashStart);
    SetCount(count);
}

// HeadersPayload JSON serialization
void HeadersPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("headers");
    writer.WriteStartArray();
    for (const auto& header : headers_)
    {
        // Serialize each header
        writer.WriteStartObject();
        writer.WritePropertyName("hash");
        writer.WriteValue(header->GetHash().ToString());
        writer.WritePropertyName("index");
        writer.WriteValue(header->GetIndex());
        writer.WritePropertyName("timestamp");
        writer.WriteValue(header->GetTimestamp());
        writer.WriteEndObject();
    }
    writer.WriteEndArray();
    writer.WriteEndObject();
}

void HeadersPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "headers")
            {
                reader.ReadStartArray();
                headers_.clear();
                while (reader.Read() && reader.GetTokenType() != io::JsonTokenType::EndArray)
                {
                    // Cannot fully deserialize headers from JSON
                    // This would require creating BlockHeader objects
                    // Skip the header object
                    reader.SkipObject();
                }
            }
        }
    }
}

// AddrPayload JSON serialization
void AddrPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WritePropertyName("addresses");
    writer.WriteStartArray();
    for (const auto& addr : addresses_)
    {
        writer.WriteStartObject();
        writer.WritePropertyName("timestamp");
        writer.WriteValue(addr.GetTimestamp());
        writer.WritePropertyName("endpoint");
        writer.WriteValue(addr.GetEndpoint().ToString());
        writer.WriteEndObject();
    }
    writer.WriteEndArray();
    writer.WriteEndObject();
}

void AddrPayload::DeserializeJson(const io::JsonReader& reader)
{
    reader.ReadStartObject();
    while (reader.Read())
    {
        if (reader.GetTokenType() == io::JsonTokenType::PropertyName)
        {
            auto name = reader.GetString();
            reader.Read();
            if (name == "addresses")
            {
                reader.ReadStartArray();
                addresses_.clear();
                while (reader.Read() && reader.GetTokenType() != io::JsonTokenType::EndArray)
                {
                    // Parse network address objects
                    NetworkAddressWithTime addr;
                    addr.DeserializeJson(reader);
                    addresses_.push_back(addr);
                }
            }
        }
    }
}

}  // namespace neo::network::p2p::payloads