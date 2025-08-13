/**
 * @file prepare_request.cpp
 * @brief Prepare Request
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/prepare_request.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

#include <sstream>

namespace neo::consensus
{
PrepareRequest::PrepareRequest(uint8_t viewNumber, uint64_t timestamp, uint64_t nonce, const io::UInt160& nextConsensus)
    : ConsensusMessage(MessageType::PrepareRequest, viewNumber),
      timestamp_(timestamp),
      nonce_(nonce),
      nextConsensus_(nextConsensus)
{
}

uint64_t PrepareRequest::GetTimestamp() const { return timestamp_; }

uint64_t PrepareRequest::GetNonce() const { return nonce_; }

const io::UInt160& PrepareRequest::GetNextConsensus() const { return nextConsensus_; }

const std::vector<std::shared_ptr<ledger::Transaction>>& PrepareRequest::GetTransactions() const
{
    return transactions_;
}

void PrepareRequest::SetTransactions(const std::vector<std::shared_ptr<ledger::Transaction>>& transactions)
{
    transactions_ = transactions;
}

const std::vector<io::UInt256>& PrepareRequest::GetTransactionHashes() const { return transactionHashes_; }

void PrepareRequest::SetTransactionHashes(const std::vector<io::UInt256>& transactionHashes)
{
    transactionHashes_ = transactionHashes;
}

const io::ByteVector& PrepareRequest::GetInvocationScript() const { return invocationScript_; }

void PrepareRequest::SetInvocationScript(const io::ByteVector& invocationScript)
{
    invocationScript_ = invocationScript;
}

const io::ByteVector& PrepareRequest::GetVerificationScript() const { return verificationScript_; }

void PrepareRequest::SetVerificationScript(const io::ByteVector& verificationScript)
{
    verificationScript_ = verificationScript;
}

void PrepareRequest::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);
    writer.WriteUInt64(timestamp_);
    writer.WriteUInt64(nonce_);
    writer.Write(nextConsensus_);

    writer.WriteVarInt(transactionHashes_.size());
    for (const auto& hash : transactionHashes_)
    {
        writer.Write(hash);
    }

    writer.WriteVarBytes(invocationScript_.AsSpan());
    writer.WriteVarBytes(verificationScript_.AsSpan());
}

void PrepareRequest::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);
    timestamp_ = reader.ReadUInt64();
    nonce_ = reader.ReadUInt64();
    nextConsensus_ = reader.ReadSerializable<io::UInt160>();

    uint64_t count = reader.ReadVarInt();
    transactionHashes_.clear();
    transactionHashes_.reserve(count);
    for (uint64_t i = 0; i < count; i++)
    {
        transactionHashes_.push_back(reader.ReadSerializable<io::UInt256>());
    }

    invocationScript_ = reader.ReadVarBytes();
    verificationScript_ = reader.ReadVarBytes();
}

io::ByteVector PrepareRequest::GetData() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    writer.WriteByte(static_cast<uint8_t>(GetType()));
    writer.WriteByte(GetViewNumber());
    writer.WriteUInt16(GetValidatorIndex());
    writer.WriteUInt64(timestamp_);
    writer.WriteUInt64(nonce_);
    writer.Write(nextConsensus_);

    writer.WriteVarInt(transactionHashes_.size());
    for (const auto& hash : transactionHashes_)
    {
        writer.Write(hash);
    }

    std::string data = stream.str();

    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}
}  // namespace neo::consensus
