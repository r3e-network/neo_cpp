#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/oracle_response.h>

namespace neo::ledger
{
OracleResponse::OracleResponse() : id_(0), code_(OracleResponseCode::Success) {}

OracleResponse::OracleResponse(uint64_t id, OracleResponseCode code, const io::ByteVector& result)
    : id_(id), code_(code), result_(result)
{
}

uint64_t OracleResponse::GetId() const { return id_; }

void OracleResponse::SetId(uint64_t id) { id_ = id; }

OracleResponseCode OracleResponse::GetCode() const { return code_; }

void OracleResponse::SetCode(OracleResponseCode code) { code_ = code; }

const io::ByteVector& OracleResponse::GetResult() const { return result_; }

void OracleResponse::SetResult(const io::ByteVector& result) { result_ = result; }

void OracleResponse::Serialize(io::BinaryWriter& writer) const
{
    // Serialize base TransactionAttribute first
    TransactionAttribute::Serialize(writer);

    // Serialize OracleResponse specific data
    writer.Write(id_);
    writer.Write(static_cast<uint8_t>(code_));
    writer.WriteVarBytes(result_.AsSpan());
}

void OracleResponse::Deserialize(io::BinaryReader& reader)
{
    // Deserialize base TransactionAttribute first
    TransactionAttribute::Deserialize(reader);

    // Deserialize OracleResponse specific data
    id_ = reader.ReadUInt64();
    code_ = static_cast<OracleResponseCode>(reader.ReadByte());
    result_ = reader.ReadVarBytes();
}

size_t OracleResponse::GetSize() const
{
    return TransactionAttribute::GetSize() + sizeof(uint64_t) +  // id_
           sizeof(uint8_t) +                                     // code_
           result_.GetVarSize();                                 // result_
}

bool OracleResponse::operator==(const OracleResponse& other) const
{
    return TransactionAttribute::operator==(other) && id_ == other.id_ && code_ == other.code_ &&
           result_ == other.result_;
}

bool OracleResponse::operator!=(const OracleResponse& other) const { return !(*this == other); }
}  // namespace neo::ledger
