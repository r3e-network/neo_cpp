#include <neo/consensus/consensus_context.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace neo::consensus
{
    ConsensusContext::ConsensusContext(const std::vector<cryptography::ecc::ECPoint>& validators, uint16_t myIndex, const cryptography::ecc::KeyPair& keyPair, uint32_t blockIndex)
        : validators_(validators), validatorIndex_(myIndex), keyPair_(keyPair), viewNumber_(0), blockIndex_(blockIndex)
    {
    }

    const std::vector<cryptography::ecc::ECPoint>& ConsensusContext::GetValidators() const
    {
        return validators_;
    }

    uint16_t ConsensusContext::GetValidatorIndex() const
    {
        return validatorIndex_;
    }

    uint16_t ConsensusContext::GetPrimaryIndex() const
    {
        return GetPrimaryIndex(viewNumber_);
    }

    uint16_t ConsensusContext::GetPrimaryIndex(uint8_t viewNumber) const
    {
        if (validators_.empty())
            return 0;
        
        return (blockIndex_ - viewNumber) % validators_.size();
    }

    uint8_t ConsensusContext::GetViewNumber() const
    {
        return viewNumber_;
    }

    void ConsensusContext::SetViewNumber(uint8_t viewNumber)
    {
        viewNumber_ = viewNumber;
    }

    uint32_t ConsensusContext::GetBlockIndex() const
    {
        return blockIndex_;
    }

    std::shared_ptr<ledger::Block> ConsensusContext::GetBlock() const
    {
        return block_;
    }

    void ConsensusContext::SetBlock(std::shared_ptr<ledger::Block> block)
    {
        block_ = block;
    }

    const std::vector<std::shared_ptr<ledger::Transaction>>& ConsensusContext::GetTransactions() const
    {
        return transactions_;
    }

    void ConsensusContext::SetTransactions(const std::vector<std::shared_ptr<ledger::Transaction>>& transactions)
    {
        transactions_ = transactions;
    }

    std::shared_ptr<PrepareRequest> ConsensusContext::GetPrepareRequestMessage() const
    {
        return prepareRequestMessage_;
    }

    void ConsensusContext::SetPrepareRequestMessage(std::shared_ptr<PrepareRequest> message)
    {
        prepareRequestMessage_ = message;
    }

    const std::unordered_map<uint16_t, std::shared_ptr<PrepareResponse>>& ConsensusContext::GetPrepareResponseMessages() const
    {
        return prepareResponseMessages_;
    }

    void ConsensusContext::AddPrepareResponseMessage(uint16_t validatorIndex, std::shared_ptr<PrepareResponse> message)
    {
        prepareResponseMessages_[validatorIndex] = message;
    }

    const std::unordered_map<uint16_t, std::shared_ptr<CommitMessage>>& ConsensusContext::GetCommitMessages() const
    {
        return commitMessages_;
    }

    void ConsensusContext::AddCommitMessage(uint16_t validatorIndex, std::shared_ptr<CommitMessage> message)
    {
        commitMessages_[validatorIndex] = message;
    }

    const std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>>& ConsensusContext::GetChangeViewMessages() const
    {
        return changeViewMessages_;
    }

    void ConsensusContext::AddChangeViewMessage(uint16_t validatorIndex, std::shared_ptr<ChangeViewMessage> message)
    {
        changeViewMessages_[validatorIndex] = message;
    }

    const std::unordered_map<uint16_t, std::shared_ptr<RecoveryMessage>>& ConsensusContext::GetRecoveryMessages() const
    {
        return recoveryMessages_;
    }

    void ConsensusContext::AddRecoveryMessage(uint16_t validatorIndex, std::shared_ptr<RecoveryMessage> message)
    {
        recoveryMessages_[validatorIndex] = message;
    }

    void ConsensusContext::Reset()
    {
        viewNumber_ = 0;
        block_ = nullptr;
        transactions_.clear();
        prepareRequestMessage_ = nullptr;
        prepareResponseMessages_.clear();
        commitMessages_.clear();
        changeViewMessages_.clear();
        recoveryMessages_.clear();
    }

    bool ConsensusContext::IsPrimary() const
    {
        return validatorIndex_ == GetPrimaryIndex();
    }

    bool ConsensusContext::IsBackup() const
    {
        return validatorIndex_ != GetPrimaryIndex() && validatorIndex_ < validators_.size();
    }

    bool ConsensusContext::HasReceivedEnoughPrepareResponses() const
    {
        if (!prepareRequestMessage_)
            return false;
        
        size_t m = validators_.size() - (validators_.size() - 1) / 3;
        return prepareResponseMessages_.size() >= m - 1;
    }

    bool ConsensusContext::HasReceivedEnoughCommits() const
    {
        if (!prepareRequestMessage_)
            return false;
        
        size_t m = validators_.size() - (validators_.size() - 1) / 3;
        return commitMessages_.size() >= m;
    }

    bool ConsensusContext::HasReceivedEnoughChangeViewMessages(uint8_t viewNumber) const
    {
        size_t m = validators_.size() - (validators_.size() - 1) / 3;
        size_t count = 0;
        
        for (const auto& pair : changeViewMessages_)
        {
            if (pair.second->GetNewViewNumber() >= viewNumber)
                count++;
        }
        
        return count >= m;
    }
}
