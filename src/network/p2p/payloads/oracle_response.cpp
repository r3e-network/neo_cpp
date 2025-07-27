#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/oracle_response.h>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
OracleResponse::OracleResponse() : id_(0), code_(OracleResponseCode::Success) {}

OracleResponse::OracleResponse(uint64_t id, OracleResponseCode code, const io::ByteVector& result)
    : id_(id), code_(code), result_(result)
{
}

uint64_t OracleResponse::GetId() const
{
    return id_;
}

void OracleResponse::SetId(uint64_t id)
{
    id_ = id;
}

OracleResponseCode OracleResponse::GetCode() const
{
    return code_;
}

void OracleResponse::SetCode(OracleResponseCode code)
{
    code_ = code;
}

const io::ByteVector& OracleResponse::GetResult() const
{
    return result_;
}

void OracleResponse::SetResult(const io::ByteVector& result)
{
    result_ = result;
}

void OracleResponse::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(id_);
    writer.Write(static_cast<uint8_t>(code_));
    writer.WriteVarBytes(result_.AsSpan());
}

void OracleResponse::Deserialize(io::BinaryReader& reader)
{
    id_ = reader.ReadUInt64();
    code_ = static_cast<OracleResponseCode>(reader.ReadUInt8());

    // Validate the response code
    switch (code_)
    {
        case OracleResponseCode::Success:
        case OracleResponseCode::ProtocolNotSupported:
        case OracleResponseCode::ConsensusUnreachable:
        case OracleResponseCode::NotFound:
        case OracleResponseCode::Timeout:
        case OracleResponseCode::Forbidden:
        case OracleResponseCode::ResponseTooLarge:
        case OracleResponseCode::InsufficientFunds:
        case OracleResponseCode::ContentTypeNotSupported:
        case OracleResponseCode::Error:
            break;
        default:
            throw std::runtime_error("Invalid response code");
    }

    result_ = reader.ReadVarBytes(MaxResultSize);

    // Validate the result
    if (code_ != OracleResponseCode::Success && !result_.empty())
        throw std::runtime_error("Result must be empty for non-success response codes");
}

void OracleResponse::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("id", id_);
    writer.Write("code", static_cast<uint8_t>(code_));
    writer.Write("result", result_.ToBase64String());
}

void OracleResponse::DeserializeJson(const io::JsonReader& reader)
{
    id_ = reader.ReadUInt64("id");
    code_ = static_cast<OracleResponseCode>(reader.ReadUInt8("code"));
    result_ = io::ByteVector::FromBase64String(reader.ReadString("result"));

    // Validate the response code
    switch (code_)
    {
        case OracleResponseCode::Success:
        case OracleResponseCode::ProtocolNotSupported:
        case OracleResponseCode::ConsensusUnreachable:
        case OracleResponseCode::NotFound:
        case OracleResponseCode::Timeout:
        case OracleResponseCode::Forbidden:
        case OracleResponseCode::ResponseTooLarge:
        case OracleResponseCode::InsufficientFunds:
        case OracleResponseCode::ContentTypeNotSupported:
        case OracleResponseCode::Error:
            break;
        default:
            throw std::runtime_error("Invalid response code");
    }

    // Validate the result
    if (code_ != OracleResponseCode::Success && !result_.empty())
        throw std::runtime_error("Result must be empty for non-success response codes");
}
}  // namespace neo::network::p2p::payloads
