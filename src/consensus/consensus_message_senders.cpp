#include <neo/consensus/consensus_service.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/role_management.h>

#include <algorithm>
#include <chrono>
#include <sstream>

namespace neo::consensus
{
void ConsensusService::SendChangeView()
{
    // Create change view message
    auto message = std::make_shared<ChangeViewMessage>(viewNumber_, viewNumber_ + 1, GetCurrentTimestamp());
    message->SetValidatorIndex(validatorIndex_);
    message->Sign(keyPair_);

    // Store message
    changeViewMessages_[validatorIndex_] = message;

    // Broadcast message
    BroadcastMessage(message);
}

void ConsensusService::SendPrepareRequest()
{
    // Check if node is primary
    if (!IsPrimary()) return;

    // Create prepare request
    auto message = std::make_shared<PrepareRequest>(viewNumber_, GetCurrentTimestamp(), 0, io::UInt160());
    message->SetValidatorIndex(validatorIndex_);

    // Get transactions
    transactions_ = node_->GetMemoryPool()->GetTransactions();

    // Set transaction hashes
    std::vector<io::UInt256> transactionHashes;
    for (const auto& tx : transactions_)
    {
        transactionHashes.push_back(tx->GetHash());
    }
    message->SetTransactionHashes(transactionHashes);

    // Sign message
    message->Sign(keyPair_);

    // Store message
    prepareRequest_ = message;
    lastPrepareRequestTime_ = GetCurrentTimestamp();

    // Broadcast message
    BroadcastMessage(message);
}

void ConsensusService::SendPrepareResponse()
{
    // Check if node is backup
    if (!IsBackup()) return;

    // Check if prepare request is received
    if (!prepareRequest_) return;

    // Create prepare response
    auto message = std::make_shared<PrepareResponse>(viewNumber_, prepareRequest_->GetSignature());
    message->SetValidatorIndex(validatorIndex_);
    message->Sign(keyPair_);

    // Store message
    prepareResponses_[validatorIndex_] = message;

    // Broadcast message
    BroadcastMessage(message);
}

void ConsensusService::SendCommit()
{
    // Check if prepare request is received
    if (!prepareRequest_) return;

    // Create block
    auto block = CreateBlock();

    // Create commit message
    auto message = std::make_shared<CommitMessage>(viewNumber_, block->GetHash(), io::ByteVector());
    message->SetValidatorIndex(validatorIndex_);
    message->Sign(keyPair_);

    // Store message
    commitMessages_[validatorIndex_] = message;

    // Broadcast message
    BroadcastMessage(message);
}

void ConsensusService::SendRecoveryRequest()
{
    // Create recovery request
    auto message = std::make_shared<RecoveryRequest>(viewNumber_, GetCurrentTimestamp());
    message->SetValidatorIndex(validatorIndex_);
    message->Sign(keyPair_);

    // Broadcast message
    BroadcastMessage(message);
}

void ConsensusService::SendRecoveryMessage(uint16_t validatorIndex)
{
    // Create recovery message
    auto message = std::make_shared<RecoveryMessage>(viewNumber_);
    message->SetValidatorIndex(validatorIndex_);

    // Add change view messages
    for (const auto& [index, changeViewMessage] : changeViewMessages_)
    {
        message->AddChangeViewMessage(changeViewMessage);
    }

    // Add prepare request
    if (prepareRequest_)
    {
        message->SetPrepareRequest(prepareRequest_);
    }

    // Add prepare responses
    for (const auto& [index, prepareResponse] : prepareResponses_)
    {
        message->AddPrepareResponse(prepareResponse);
    }

    // Add commit messages
    for (const auto& [index, commitMessage] : commitMessages_)
    {
        message->AddCommitMessage(commitMessage);
    }

    // Sign message
    message->Sign(keyPair_);

    // Send message
    SendMessage(message, validatorIndex);
}

void ConsensusService::BroadcastMessage(std::shared_ptr<ConsensusMessage> message)
{
    // Serialize message
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    message->Serialize(writer);
    std::string data = stream.str();

    // Create network message
    network::Message networkMessage(
        network::MessageCommand::Consensus,
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));

    // Broadcast message
    node_->BroadcastMessage(networkMessage);
}

void ConsensusService::SendMessage(std::shared_ptr<ConsensusMessage> message, uint16_t validatorIndex)
{
    // Check if validator index is valid
    if (validatorIndex >= validators_.size()) return;

    // Serialize message
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    message->Serialize(writer);
    std::string data = stream.str();

    // Create network message
    network::Message networkMessage(
        network::MessageCommand::Consensus,
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));

    // Send message
    node_->SendMessage(networkMessage, validators_[validatorIndex]);
}
}  // namespace neo::consensus
