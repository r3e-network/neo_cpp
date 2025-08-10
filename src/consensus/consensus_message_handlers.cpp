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
void ConsensusService::OnMessage(std::shared_ptr<ConsensusMessage> message, const cryptography::ecc::ECPoint& sender)
{
    // Check if sender is validator
    auto it = std::find(validators_.begin(), validators_.end(), sender);
    if (it == validators_.end()) return;

    // Get validator index
    uint16_t validatorIndex = static_cast<uint16_t>(std::distance(validators_.begin(), it));

    // Set validator index
    message->SetValidatorIndex(validatorIndex);

    // Verify signature
    if (!message->VerifySignature(sender)) return;

    // Handle message based on type
    switch (message->GetType())
    {
        case MessageType::ChangeView:
            OnChangeViewMessage(std::static_pointer_cast<ChangeViewMessage>(message), sender);
            break;
        case MessageType::PrepareRequest:
            OnPrepareRequestMessage(std::static_pointer_cast<PrepareRequest>(message), sender);
            break;
        case MessageType::PrepareResponse:
            OnPrepareResponseMessage(std::static_pointer_cast<PrepareResponse>(message), sender);
            break;
        case MessageType::Commit:
            OnCommitMessage(std::static_pointer_cast<CommitMessage>(message), sender);
            break;
        case MessageType::RecoveryMessage:
            OnRecoveryMessage(std::static_pointer_cast<RecoveryMessage>(message), sender);
            break;
        case MessageType::RecoveryRequest:
            OnRecoveryRequestMessage(std::static_pointer_cast<RecoveryRequest>(message), sender);
            break;
    }
}

void ConsensusService::OnChangeViewMessage(std::shared_ptr<ChangeViewMessage> message,
                                           const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if message is for current block
    if (message->GetViewNumber() < viewNumber_) return;

    // Check if message is already received
    if (message->GetViewNumber() == viewNumber_ &&
        changeViewMessages_.find(message->GetValidatorIndex()) != changeViewMessages_.end())
        return;

    // Store message
    if (message->GetViewNumber() == viewNumber_)
    {
        changeViewMessages_[message->GetValidatorIndex()] = message;
    }
    else
    {
        pendingChangeViewMessages_[message->GetViewNumber()][message->GetValidatorIndex()] = message;
    }

    // Check if node has received enough change view messages
    if (HasReceivedEnoughChangeViewMessages(message->GetNewViewNumber()))
    {
        // Change view
        ChangeView(message->GetNewViewNumber());
    }
}

void ConsensusService::OnPrepareRequestMessage(std::shared_ptr<PrepareRequest> message,
                                               const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if message is for current block
    if (message->GetViewNumber() < viewNumber_) return;

    // Check if message is from primary
    if (message->GetValidatorIndex() != GetPrimaryIndex(message->GetViewNumber())) return;

    // Check if message is already received
    if (message->GetViewNumber() == viewNumber_ && prepareRequest_) return;

    // Store message
    if (message->GetViewNumber() == viewNumber_)
    {
        prepareRequest_ = message;
        lastPrepareRequestTime_ = GetCurrentTimestamp();

        // Send prepare response
        SendPrepareResponse();
    }
    else
    {
        pendingPrepareRequests_[message->GetViewNumber()] = message;
    }
}

void ConsensusService::OnPrepareResponseMessage(std::shared_ptr<PrepareResponse> message,
                                                const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if message is for current block
    if (message->GetViewNumber() < viewNumber_) return;

    // Check if message is already received
    if (message->GetViewNumber() == viewNumber_ &&
        prepareResponses_.find(message->GetValidatorIndex()) != prepareResponses_.end())
        return;

    // Store message
    if (message->GetViewNumber() == viewNumber_)
    {
        prepareResponses_[message->GetValidatorIndex()] = message;

        // Check if node has received enough prepare responses
        if (HasReceivedEnoughPrepareResponses())
        {
            // Send commit
            SendCommit();
        }
    }
    else
    {
        pendingPrepareResponses_[message->GetViewNumber()][message->GetValidatorIndex()] = message;
    }
}

void ConsensusService::OnCommitMessage(std::shared_ptr<CommitMessage> message, const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if message is for current block
    if (message->GetViewNumber() < viewNumber_) return;

    // Check if message is already received
    if (commitMessages_.find(message->GetValidatorIndex()) != commitMessages_.end()) return;

    // Store message
    commitMessages_[message->GetValidatorIndex()] = message;

    // Check if node has received enough commits
    if (HasReceivedEnoughCommits())
    {
        // Create block
        auto block = CreateBlock();

        // Process block
        ProcessBlock(block);

        // Reset consensus
        Reset();

        // Initialize consensus
        Initialize();
    }
}

void ConsensusService::OnRecoveryMessage(std::shared_ptr<RecoveryMessage> message,
                                         const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if message is for current block
    if (message->GetViewNumber() < viewNumber_) return;

    // Store message
    recoveryMessages_[message->GetValidatorIndex()] = message;

    // Process change view messages
    for (const auto& changeViewMessage : message->GetChangeViewMessages())
    {
        OnChangeViewMessage(changeViewMessage, validators_[changeViewMessage->GetValidatorIndex()]);
    }

    // Process prepare request
    if (message->GetPrepareRequest())
    {
        OnPrepareRequestMessage(message->GetPrepareRequest(),
                                validators_[message->GetPrepareRequest()->GetValidatorIndex()]);
    }

    // Process prepare responses
    for (const auto& prepareResponse : message->GetPrepareResponses())
    {
        OnPrepareResponseMessage(prepareResponse, validators_[prepareResponse->GetValidatorIndex()]);
    }

    // Process commit messages
    for (const auto& commitMessage : message->GetCommitMessages())
    {
        OnCommitMessage(commitMessage, validators_[commitMessage->GetValidatorIndex()]);
    }
}

void ConsensusService::OnRecoveryRequestMessage(std::shared_ptr<RecoveryRequest> message,
                                                const cryptography::ecc::ECPoint& sender)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Send recovery message
    SendRecoveryMessage(message->GetValidatorIndex());
}

bool ConsensusService::HasReceivedEnoughChangeViewMessages(uint8_t viewNumber) const
{
    // Get change view messages for the specified view
    const auto& messages = viewNumber == this->viewNumber_
                               ? changeViewMessages_
                               : (pendingChangeViewMessages_.find(viewNumber) != pendingChangeViewMessages_.end()
                                      ? pendingChangeViewMessages_.at(viewNumber)
                                      : std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>>());

    // Count messages
    uint16_t count = 0;
    for (const auto& [index, message] : messages)
    {
        if (message->GetNewViewNumber() >= viewNumber) count++;
    }

    // Calculate fault tolerance (f)
    uint16_t f = static_cast<uint16_t>((validators_.size() - 1) / 3);

    // Check if node has received enough messages (2f + 1)
    return count >= validators_.size() - f;
}

bool ConsensusService::HasReceivedEnoughPrepareResponses() const
{
    // Check if prepare request is received
    if (!prepareRequest_) return false;

    // Count prepare responses
    uint16_t count = 0;
    for (const auto& [index, message] : prepareResponses_)
    {
        if (message->GetPreparationHash() == prepareRequest_->GetSignature()) count++;
    }

    // Calculate fault tolerance (f)
    uint16_t f = static_cast<uint16_t>((validators_.size() - 1) / 3);

    // Check if node has received enough prepare responses (2f + 1)
    return count >= validators_.size() - f;
}

bool ConsensusService::HasReceivedEnoughCommits() const
{
    // Check if prepare request is received
    if (!prepareRequest_) return false;

    // Count commit messages with valid signatures
    uint16_t count = 0;
    for (const auto& [index, message] : commitMessages_)
    {
        // Verify signature using the validator's public key (exactly like C# implementation)
        if (index < validators_.size())
        {
            const auto& validatorPublicKey = validators_[index];
            if (message->VerifySignature(validatorPublicKey))
            {
                count++;
            }
        }
    }

    // Calculate fault tolerance (f)
    uint16_t f = static_cast<uint16_t>((validators_.size() - 1) / 3);

    // Check if node has received enough commit messages (2f + 1)
    return count >= validators_.size() - f;
}
}  // namespace neo::consensus
