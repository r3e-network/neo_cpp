/**
 * @file consensus_state.cpp
 * @brief Consensus State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_state.h>
#include <neo/common/safe_math.h>
#include <neo/ledger/transaction.h>

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

void ConsensusState::SetPrepareRequest(
    const io::UInt256& hash, const std::vector<network::p2p::payloads::Neo3Transaction>& txs,
    const std::vector<io::UInt256>& transaction_hashes, std::chrono::system_clock::time_point timestamp, uint64_t nonce,
    std::shared_ptr<ledger::TransactionVerificationContext> verification_context, size_t block_size_bytes,
    int64_t total_system_fee, int64_t total_network_fee)
{
    std::lock_guard<std::mutex> lock(mutex_);
    prepare_request_hash_ = hash;
    proposed_transactions_ = txs;
    transaction_hashes_ = transaction_hashes;
    timestamp_ = timestamp;
    nonce_ = nonce;
    verification_context_ = std::move(verification_context);
    block_size_bytes_ = block_size_bytes;
    total_system_fee_ = total_system_fee;
    total_network_fee_ = total_network_fee;

    if (!verification_context_)
    {
        verification_context_ = std::make_shared<ledger::TransactionVerificationContext>();

        block_size_bytes_ = 0;
        total_system_fee_ = 0;
        total_network_fee_ = 0;

        for (const auto& tx : proposed_transactions_)
        {
            try
            {
                block_size_bytes_ =
                    common::SafeMath::Add<size_t>(block_size_bytes_, static_cast<size_t>(tx.GetSize()));
                total_system_fee_ = common::SafeMath::Add<int64_t>(total_system_fee_, tx.GetSystemFee());
                total_network_fee_ = common::SafeMath::Add<int64_t>(total_network_fee_, tx.GetNetworkFee());
            }
            catch (const std::exception&)
            {
                // Ignore overflow here; policy evaluation will catch inconsistent totals.
            }

            auto tx_ptr = std::make_shared<ledger::Transaction>(tx);
            if (verification_context_->CheckTransaction(tx_ptr))
            {
                verification_context_->AddTransaction(tx_ptr);
            }
        }
    }
}

bool ConsensusState::AddPrepareResponse(uint32_t validator_index, const io::UInt256& hash,
                                        const io::ByteVector& invocation_script)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto [it, inserted] =
        prepare_responses_.emplace(validator_index, PrepareResponseInfo{hash, invocation_script});
    if (!inserted)
    {
        if (!invocation_script.IsEmpty()) it->second.invocation_script = invocation_script;
        return false;
    }
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

bool ConsensusState::AddViewChange(uint32_t validator_index, uint32_t original_view, uint32_t new_view,
                                   ChangeViewReason reason, const io::ByteVector& invocation_script,
                                   uint64_t timestamp_ms)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto [it, inserted] = view_changes_.emplace(validator_index, ViewChangeInfo{});
    auto& info = it->second;
    info.original_view = original_view;
    info.new_view = std::max(info.new_view, new_view);
    info.reason = reason;
    if (timestamp_ms != 0) info.timestamp = timestamp_ms;
    if (!invocation_script.IsEmpty()) info.invocation_script = invocation_script;
    return inserted;
}

size_t ConsensusState::GetViewChangeCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_changes_.size();
}

std::optional<ChangeViewReason> ConsensusState::GetViewChangeReason(uint32_t validator_index) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = view_changes_.find(validator_index);
    if (it == view_changes_.end())
    {
        return std::nullopt;
    }
    return it->second.reason;
}

size_t ConsensusState::CountViewChangesAtOrAbove(uint32_t view_number) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, info] : view_changes_)
    {
        if (info.new_view >= view_number)
        {
            ++count;
        }
    }
    return count;
}

std::optional<uint32_t> ConsensusState::GetViewChangeView(uint32_t validator_index) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = view_changes_.find(validator_index);
    if (it == view_changes_.end())
    {
        return std::nullopt;
    }
    return it->second.new_view;
}

std::unordered_map<uint32_t, ConsensusState::ViewChangeInfo> ConsensusState::GetViewChanges() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_changes_;
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

std::optional<network::p2p::payloads::Neo3Transaction> ConsensusState::GetCachedTransaction(
    const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transaction_pool_.find(hash);
    if (it == transaction_pool_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

std::unordered_map<uint32_t, ConsensusState::PrepareResponseInfo> ConsensusState::GetPrepareResponses() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return prepare_responses_;
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
    transaction_hashes_.clear();
    timestamp_ = std::chrono::system_clock::time_point();
    nonce_ = 0;
    verification_context_.reset();
    block_size_bytes_ = 0;
    total_system_fee_ = 0;
    total_network_fee_ = 0;
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
    transaction_hashes_.clear();
    timestamp_ = std::chrono::system_clock::time_point();
    nonce_ = 0;
    verification_context_.reset();
    block_size_bytes_ = 0;
    total_system_fee_ = 0;
    total_network_fee_ = 0;
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

std::vector<io::UInt256> ConsensusState::GetTransactionHashes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return transaction_hashes_;
}

std::shared_ptr<ledger::TransactionVerificationContext> ConsensusState::GetVerificationContext() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return verification_context_;
}

size_t ConsensusState::GetBlockSizeBytes() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return block_size_bytes_;
}

int64_t ConsensusState::GetTotalSystemFee() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return total_system_fee_;
}

int64_t ConsensusState::GetTotalNetworkFee() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return total_network_fee_;
}

std::vector<network::p2p::payloads::Neo3Transaction> ConsensusState::GetTransactions() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return proposed_transactions_;
}
}  // namespace neo::consensus
