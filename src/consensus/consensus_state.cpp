/**
 * @file consensus_state.cpp
 * @brief Consensus State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_state.h>

#include <algorithm>

namespace neo::consensus
{
uint32_t ConsensusState::GetViewNumber() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_number_;
}

void ConsensusState::SetViewNumber(uint32_t view)
{
    std::lock_guard<std::mutex> lock(mutex_);
    view_number_ = view;
}

uint32_t ConsensusState::GetBlockIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return block_index_;
}

void ConsensusState::SetBlockIndex(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    block_index_ = index;
}

ConsensusPhase ConsensusState::GetPhase() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return phase_;
}

void ConsensusState::SetPhase(ConsensusPhase phase)
{
    std::lock_guard<std::mutex> lock(mutex_);
    phase_ = phase;
}

io::UInt256 ConsensusState::GetPrepareRequestHash() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return prepare_request_hash_;
}

void ConsensusState::SetPrepareRequest(const io::UInt256& hash,
                                       const std::vector<network::p2p::payloads::Neo3Transaction>& txs,
                                       std::chrono::system_clock::time_point timestamp, uint64_t nonce)
{
    std::lock_guard<std::mutex> lock(mutex_);
    prepare_request_hash_ = hash;
    proposed_transactions_ = txs;
    timestamp_ = timestamp;
    nonce_ = nonce;
}

bool ConsensusState::AddPrepareResponse(uint32_t validator_index, const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (prepare_responses_.count(validator_index) > 0)
    {
        return false;  // Already have response from this validator
    }

    prepare_responses_[validator_index] = hash;
    return true;
}

size_t ConsensusState::GetPrepareResponseCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return prepare_responses_.size();
}

bool ConsensusState::HasPrepareResponse(uint32_t validator_index) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return prepare_responses_.count(validator_index) > 0;
}

bool ConsensusState::AddCommit(uint32_t validator_index, const std::vector<uint8_t>& signature)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (commits_.count(validator_index) > 0)
    {
        return false;  // Already have commit from this validator
    }

    commits_[validator_index] = signature;
    return true;
}

size_t ConsensusState::GetCommitCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return commits_.size();
}

bool ConsensusState::HasCommit(uint32_t validator_index) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return commits_.count(validator_index) > 0;
}

bool ConsensusState::AddViewChange(uint32_t validator_index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_changes_.insert(validator_index).second;
}

size_t ConsensusState::GetViewChangeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_changes_.size();
}

bool ConsensusState::AddTransaction(const network::p2p::payloads::Neo3Transaction& tx)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto hash = tx.GetHash();
    if (transaction_pool_.count(hash) > 0)
    {
        return false;  // Already have this transaction
    }

    transaction_pool_[hash] = tx;
    return true;
}

void ConsensusState::RemoveTransaction(const io::UInt256& hash)
{
    std::lock_guard<std::mutex> lock(mutex_);
    transaction_pool_.erase(hash);
}

std::vector<network::p2p::payloads::Neo3Transaction> ConsensusState::GetTransactionsForBlock(size_t max_count) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<network::p2p::payloads::Neo3Transaction> result;
    result.reserve(std::min(max_count, transaction_pool_.size()));

    // Sort by fee per byte (highest first)
    std::vector<std::pair<io::UInt256, const network::p2p::payloads::Neo3Transaction*>> sorted_txs;
    for (const auto& [hash, tx] : transaction_pool_)
    {
        sorted_txs.push_back({hash, &tx});
    }

    std::sort(sorted_txs.begin(), sorted_txs.end(),
              [](const auto& a, const auto& b)
              {
                  // Calculate fee per byte
                  double fee_per_byte_a = static_cast<double>(a.second->GetNetworkFee()) / a.second->GetSize();
                  double fee_per_byte_b = static_cast<double>(b.second->GetNetworkFee()) / b.second->GetSize();
                  return fee_per_byte_a > fee_per_byte_b;
              });

    // Select top transactions
    for (size_t i = 0; i < std::min(max_count, sorted_txs.size()); ++i)
    {
        result.push_back(*sorted_txs[i].second);
    }

    return result;
}

std::unordered_map<uint32_t, std::vector<uint8_t>> ConsensusState::GetCommits() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return commits_;
}

void ConsensusState::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);

    phase_ = ConsensusPhase::Initial;
    prepare_request_hash_ = io::UInt256();
    proposed_transactions_.clear();
    timestamp_ = std::chrono::system_clock::time_point();
    nonce_ = 0;
    prepare_responses_.clear();
    commits_.clear();
    view_changes_.clear();

    // Keep transaction pool for next round
}

void ConsensusState::ResetForViewChange()
{
    std::lock_guard<std::mutex> lock(mutex_);

    phase_ = ConsensusPhase::Initial;
    prepare_request_hash_ = io::UInt256();
    proposed_transactions_.clear();
    timestamp_ = std::chrono::system_clock::time_point();
    nonce_ = 0;
    prepare_responses_.clear();
    commits_.clear();
    view_changes_.clear();

    // Keep transaction pool and view number
}

std::chrono::system_clock::time_point ConsensusState::GetTimestamp() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return timestamp_;
}

uint64_t ConsensusState::GetNonce() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return nonce_;
}

std::vector<network::p2p::payloads::Neo3Transaction> ConsensusState::GetTransactions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return proposed_transactions_;
}
}  // namespace neo::consensus