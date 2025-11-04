/**
 * @file recovery_message.cpp
 * @brief Consensus recovery message implementation
 */

#include <neo/consensus/recovery_message.h>

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

namespace neo::consensus
{

RecoveryMessage::RecoveryMessage(uint8_t viewNumber)
    : ConsensusMessage(ConsensusMessageType::RecoveryMessage)
{
    SetViewNumber(viewNumber);
}

void RecoveryMessage::AddChangeViewPayload(const ChangeViewPayloadCompact& payload)
{
    change_view_payloads_.push_back(payload);
}

std::shared_ptr<PrepareRequest> RecoveryMessage::GetPrepareRequest() const { return prepareRequest_; }

void RecoveryMessage::SetPrepareRequest(std::shared_ptr<PrepareRequest> prepareRequest)
{
    prepareRequest_ = std::move(prepareRequest);
    if (prepareRequest_) preparation_hash_.reset();
}

void RecoveryMessage::SetPreparationHash(const io::UInt256& hash)
{
    preparation_hash_ = hash;
    prepareRequest_.reset();
}

void RecoveryMessage::SetPrepareRequests(const std::vector<std::shared_ptr<PrepareRequest>>& requests)
{
    prepareRequest_.reset();
    preparation_hash_.reset();
    for (const auto& request : requests)
    {
        if (request)
        {
            prepareRequest_ = request;
            break;
        }
    }
}

void RecoveryMessage::AddPreparationPayload(const PreparationPayloadCompact& payload)
{
    preparation_payloads_.push_back(payload);
}

void RecoveryMessage::SetPreparationPayloads(const std::vector<PreparationPayloadCompact>& payloads)
{
    preparation_payloads_ = payloads;
}

void RecoveryMessage::AddCommitPayload(const CommitPayloadCompact& payload)
{
    commit_payloads_.push_back(payload);
}

void RecoveryMessage::SetCommitPayloads(const std::vector<CommitPayloadCompact>& payloads)
{
    commit_payloads_ = payloads;
}

void RecoveryMessage::AddTransaction(const network::p2p::payloads::Neo3Transaction& transaction)
{
    transactions_.push_back(transaction);
}

void RecoveryMessage::SetTransactions(const std::vector<network::p2p::payloads::Neo3Transaction>& transactions)
{
    transactions_ = transactions;
}

void RecoveryMessage::Serialize(io::BinaryWriter& writer) const
{
    ConsensusMessage::Serialize(writer);

    writer.Write(static_cast<uint32_t>(change_view_payloads_.size()));
    for (const auto& payload : change_view_payloads_)
    {
        writer.Write(payload.validator_index);
        writer.Write(payload.original_view_number);
        writer.Write(payload.timestamp);
        writer.WriteVarBytes(payload.invocation_script.AsSpan());
    }

    const bool has_prepare_request = static_cast<bool>(prepareRequest_);
    writer.Write(static_cast<uint8_t>(has_prepare_request ? 1 : 0));
    if (has_prepare_request)
    {
        prepareRequest_->Serialize(writer);
    }
    else
    {
        const bool has_hash = preparation_hash_.has_value();
        writer.Write(static_cast<uint8_t>(has_hash ? 1 : 0));
        if (has_hash) writer.Write(preparation_hash_.value());
    }

    writer.Write(static_cast<uint32_t>(preparation_payloads_.size()));
    for (const auto& payload : preparation_payloads_)
    {
        writer.Write(payload.validator_index);
        writer.WriteVarBytes(payload.invocation_script.AsSpan());
    }

    writer.Write(static_cast<uint32_t>(commit_payloads_.size()));
    for (const auto& payload : commit_payloads_)
    {
        writer.Write(payload.view_number);
        writer.Write(payload.validator_index);
        writer.WriteVarBytes(payload.signature.AsSpan());
        writer.WriteVarBytes(payload.invocation_script.AsSpan());
    }

    writer.Write(static_cast<uint32_t>(transactions_.size()));
    for (const auto& tx : transactions_)
    {
        tx.Serialize(writer);
    }
}

void RecoveryMessage::Deserialize(io::BinaryReader& reader)
{
    ConsensusMessage::Deserialize(reader);

    const auto change_count = reader.ReadUInt32();
    change_view_payloads_.clear();
    change_view_payloads_.reserve(change_count);
    for (uint32_t i = 0; i < change_count; ++i)
    {
        ChangeViewPayloadCompact payload;
        payload.validator_index = reader.ReadUInt32();
        payload.original_view_number = reader.ReadUInt32();
        payload.timestamp = reader.ReadUInt64();
        payload.invocation_script = io::ByteVector(reader.ReadVarBytes());
        change_view_payloads_.push_back(std::move(payload));
    }

    if (reader.ReadUInt8() != 0)
    {
        prepareRequest_ = std::make_shared<PrepareRequest>();
        prepareRequest_->Deserialize(reader);
        preparation_hash_.reset();
    }
    else
    {
        prepareRequest_.reset();
        if (reader.ReadUInt8() != 0)
        {
            preparation_hash_ = reader.ReadSerializable<io::UInt256>();
        }
        else
        {
            preparation_hash_.reset();
        }
    }

    const auto prep_count = reader.ReadUInt32();
    preparation_payloads_.clear();
    preparation_payloads_.reserve(prep_count);
    for (uint32_t i = 0; i < prep_count; ++i)
    {
        PreparationPayloadCompact payload;
        payload.validator_index = reader.ReadUInt32();
        payload.invocation_script = io::ByteVector(reader.ReadVarBytes());
        preparation_payloads_.push_back(std::move(payload));
    }

    const auto commit_count = reader.ReadUInt32();
    commit_payloads_.clear();
    commit_payloads_.reserve(commit_count);
    for (uint32_t i = 0; i < commit_count; ++i)
    {
        CommitPayloadCompact payload;
        payload.view_number = reader.ReadUInt32();
        payload.validator_index = reader.ReadUInt32();
        payload.signature = io::ByteVector(reader.ReadVarBytes());
        payload.invocation_script = io::ByteVector(reader.ReadVarBytes());
        commit_payloads_.push_back(std::move(payload));
    }

    const auto transaction_count = reader.ReadUInt32();
    transactions_.clear();
    transactions_.reserve(transaction_count);
    for (uint32_t i = 0; i < transaction_count; ++i)
    {
        network::p2p::payloads::Neo3Transaction tx;
        tx.Deserialize(reader);
        transactions_.push_back(std::move(tx));
    }
}

io::ByteVector RecoveryMessage::GetData() const
{
    io::ByteVector buffer;
    io::BinaryWriter writer(buffer);
    Serialize(writer);
    return buffer;
}

}  // namespace neo::consensus
