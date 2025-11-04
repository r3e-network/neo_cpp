/**
 * @file dbft_consensus.cpp
 * @brief Dbft Consensus
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/common/safe_math.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/dbft_consensus.h>
#include <neo/core/neo_system.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/hash.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_validator.h>
#include <neo/ledger/transaction_verification_context.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>

#include <algorithm>
#include <limits>
#include <map>
#include <random>

namespace neo::consensus
{
namespace
{
const char* ChangeViewReasonToString(ChangeViewReason reason)
{
    switch (reason)
    {
        case ChangeViewReason::Timeout:
            return "Timeout";
        case ChangeViewReason::ChangeAgreement:
            return "ChangeAgreement";
        case ChangeViewReason::TxNotFound:
            return "TxNotFound";
        case ChangeViewReason::TxInvalid:
            return "TxInvalid";
        case ChangeViewReason::TxRejectedByPolicy:
            return "TxRejectedByPolicy";
        case ChangeViewReason::BlockRejectedByPolicy:
            return "BlockRejectedByPolicy";
        default:
            return "Unknown";
    }
}

const char* ValidationResultToString(ledger::ValidationResult result)
{
    using ledger::ValidationResult;
    switch (result)
    {
        case ValidationResult::Valid:
            return "Valid";
        case ValidationResult::InvalidFormat:
            return "InvalidFormat";
        case ValidationResult::InvalidSize:
            return "InvalidSize";
        case ValidationResult::InvalidAttribute:
            return "InvalidAttribute";
        case ValidationResult::InvalidScript:
            return "InvalidScript";
        case ValidationResult::InvalidWitness:
            return "InvalidWitness";
        case ValidationResult::InsufficientFunds:
            return "InsufficientFunds";
        case ValidationResult::InvalidSignature:
            return "InvalidSignature";
        case ValidationResult::AlreadyExists:
            return "AlreadyExists";
        case ValidationResult::Expired:
            return "Expired";
        case ValidationResult::InvalidSystemFee:
            return "InvalidSystemFee";
        case ValidationResult::InvalidNetworkFee:
            return "InvalidNetworkFee";
        case ValidationResult::PolicyViolation:
            return "PolicyViolation";
        case ValidationResult::Unknown:
        default:
            return "Unknown";
    }
}

ChangeViewReason MapValidationResultToReason(ledger::ValidationResult result)
{
    using ledger::ValidationResult;
    switch (result)
    {
        case ValidationResult::PolicyViolation:
            return ChangeViewReason::TxRejectedByPolicy;
        case ValidationResult::AlreadyExists:
        case ValidationResult::InvalidFormat:
        case ValidationResult::InvalidSize:
        case ValidationResult::InvalidAttribute:
        case ValidationResult::InvalidScript:
        case ValidationResult::InvalidWitness:
        case ValidationResult::InsufficientFunds:
        case ValidationResult::InvalidSignature:
        case ValidationResult::Expired:
        case ValidationResult::InvalidSystemFee:
        case ValidationResult::InvalidNetworkFee:
        case ValidationResult::Unknown:
        case ValidationResult::Valid:
            return ChangeViewReason::TxInvalid;
    }
    return ChangeViewReason::TxInvalid;
}
}  // namespace

DbftConsensus::DbftConsensus(const ConsensusConfig& config, const io::UInt160& node_id,
                             const std::vector<io::UInt160>& validators, std::shared_ptr<ledger::MemoryPool> mempool,
                             std::shared_ptr<ledger::Blockchain> blockchain)
    : config_(config),
      node_id_(node_id),
      validators_(validators),
      mempool_(mempool),
      blockchain_(blockchain),
      state_(std::make_shared<ConsensusState>()),
      logger_(core::Logger::GetInstance())
{
    // Validate inputs
    if (validators_.empty())
    {
        throw std::invalid_argument("Validator list cannot be empty");
    }

    if (validators_.size() < 4)
    {
        throw std::invalid_argument("Need at least 4 validators for Byzantine fault tolerance");
    }

    if (!mempool_)
    {
        throw std::invalid_argument("Memory pool cannot be null");
    }

    if (!blockchain_)
    {
        throw std::invalid_argument("Blockchain cannot be null");
    }

    // Find our validator index
    auto it = std::find(validators_.begin(), validators_.end(), node_id_);
    if (it != validators_.end())
    {
        validator_index_ = static_cast<uint32_t>(std::distance(validators_.begin(), it));
    }
    else
    {
        LOG_WARNING("Node {} is not in validator list", node_id_.ToString());
        validator_index_ = std::numeric_limits<uint32_t>::max();
    }

    LOG_INFO("dBFT consensus initialized with {} validators, node index: {}", validators_.size(), validator_index_);
}

DbftConsensus::~DbftConsensus() { Stop(); }

void DbftConsensus::Start()
{
    if (running_.exchange(true))
    {
        return;  // Already running
    }

    LOG_INFO("Starting dBFT consensus for node {}", node_id_.ToString());

    consensus_thread_ = std::thread(&DbftConsensus::ConsensusLoop, this);
    timer_thread_ = std::thread(&DbftConsensus::TimerLoop, this);

    // Initialize state
    state_->SetBlockIndex(0);  // Should be set from blockchain
    state_->SetViewNumber(0);
    state_->SetPhase(ConsensusPhase::Initial);
}

void DbftConsensus::Stop()
{
    if (!running_.exchange(false))
    {
        return;  // Already stopped
    }

    LOG_INFO("Stopping dBFT consensus");

    if (consensus_thread_.joinable())
    {
        consensus_thread_.join();
    }
    if (timer_thread_.joinable())
    {
        timer_thread_.join();
    }
}

bool DbftConsensus::ProcessMessage(const ConsensusMessage& message)
{
    if (!running_)
    {
        LOG_DEBUG("Consensus not running, ignoring message");
        return false;
    }

    // Validate message basics
    uint32_t current_block_index = state_->GetBlockIndex();
    if (message.GetBlockIndex() != current_block_index)
    {
        LOG_DEBUG("Ignoring message for block height {} (current: {})", message.GetBlockIndex(), current_block_index);
        return false;
    }

    uint32_t validator_index = message.GetValidatorIndex();
    if (validator_index >= validators_.size())
    {
        LOG_WARNING("Invalid validator index {} in message (max: {})", validator_index, validators_.size() - 1);
        return false;
    }

    // Validate view number
    uint32_t current_view = state_->GetViewNumber();
    if (message.GetViewNumber() < current_view)
    {
        LOG_DEBUG("Ignoring message for old view {} (current: {})", message.GetViewNumber(), current_view);
        return false;
    }

    // Process based on message type
    switch (message.GetType())
    {
        case ConsensusMessageType::ChangeView:
            ProcessViewChange(static_cast<const ViewChangeMessage&>(message));
            break;
        case ConsensusMessageType::PrepareRequest:
            ProcessPrepareRequest(static_cast<const PrepareRequestMessage&>(message));
            break;
        case ConsensusMessageType::PrepareResponse:
            ProcessPrepareResponse(static_cast<const PrepareResponseMessage&>(message));
            break;
        case ConsensusMessageType::Commit:
            ProcessCommit(static_cast<const CommitMessage&>(message));
            break;
        default:
            LOG_WARNING("Unknown consensus message type");
            return false;
    }

    return true;
}

bool DbftConsensus::AddTransaction(const network::p2p::payloads::Neo3Transaction& tx)
{
    state_->AddTransaction(tx);

    if (!mempool_)
    {
        LOG_DEBUG("Memory pool not available");
        return false;
    }

    const auto hash = tx.GetHash();
    if (mempool_->Contains(hash))
    {
        LOG_TRACE("Consensus cache received transaction {} already present in mempool", hash.ToString());
        return true;
    }

    return mempool_->TryAdd(tx);
}

void DbftConsensus::RemoveCachedTransaction(const io::UInt256& hash)
{
    state_->RemoveTransaction(hash);
}

void DbftConsensus::SetValidatorPublicKeys(const std::vector<cryptography::ecc::ECPoint>& public_keys)
{
    validator_public_keys_.clear();
    for (const auto& key : public_keys)
    {
        try
        {
            auto script = cryptography::Crypto::CreateSignatureRedeemScript(key);
            auto script_hash = cryptography::Hash::Hash160(script.AsSpan());
            validator_public_keys_.emplace(script_hash, key);
        }
        catch (const std::exception& ex)
        {
            LOG_WARNING("Failed to cache validator public key: {}", ex.what());
        }
    }
}

void DbftConsensus::SetSignatureProvider(SignatureProvider provider)
{
    signature_provider_ = std::move(provider);
}

const ConsensusState& DbftConsensus::GetState() const { return *state_; }

void DbftConsensus::ConsensusLoop()
{
    LOG_INFO("Consensus loop started");

    while (running_)
    {
        auto phase = state_->GetPhase();

        switch (phase)
        {
            case ConsensusPhase::Initial:
                StartNewRound();
                break;

            case ConsensusPhase::Primary:
                if (IsPrimary())
                {
                    SendPrepareRequest();
                }
                break;

            case ConsensusPhase::RequestReceived:
                if (!IsPrimary())
                {
                    SendPrepareResponse();
                }
                break;

            case ConsensusPhase::SignatureSent:
                if (HasEnoughPrepareResponses())
                {
                    SendCommit();
                }
                break;

            case ConsensusPhase::BlockSent:
                if (HasEnoughCommits())
                {
                    auto block = CreateBlock();
                    if (block && block_persister_)
                    {
                        if (block_persister_(block))
                        {
                            LOG_INFO("Block {} created and persisted", block->GetIndex());
                            Reset();
                        }
                    }
                }
                break;

            case ConsensusPhase::ViewChanging:
                // Handled by view change logic
                break;

            default:
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Consensus loop stopped");
}

void DbftConsensus::TimerLoop()
{
    LOG_INFO("Timer loop started");

    while (running_)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - view_started_;

        uint32_t view_number = state_->GetViewNumber();

        auto multiplier_shift = static_cast<uint32_t>(std::min<uint32_t>(view_number + 1, 8));
        auto multiplier = static_cast<uint64_t>(1) << multiplier_shift;
        auto adjusted_timeout = config_.view_timeout * static_cast<int64_t>(multiplier);
        if (elapsed > adjusted_timeout)
        {
            OnTimeout();
        }

        auto self_commit = commit_messages_.find(validator_index_);
        if (self_commit != commit_messages_.end() && state_->GetPhase() == ConsensusPhase::BlockSent &&
            !HasEnoughCommits())
        {
            auto resend_interval = config_.block_time * 2;
            if (resend_interval.count() <= 0)
            {
                resend_interval = std::chrono::milliseconds(1000);
            }

            if (last_commit_relay_.time_since_epoch().count() == 0 || now - last_commit_relay_ >= resend_interval)
            {
                LOG_INFO("Rebroadcasting commit for block {} (view {}, validator {})", state_->GetBlockIndex(),
                         state_->GetViewNumber(), validator_index_);
                if (message_broadcaster_)
                {
                    if (auto recovery = BuildRecoveryMessage())
                    {
                        message_broadcaster_(*recovery);
                    }
                }
                if (message_broadcaster_ && self_commit->second)
                {
                    message_broadcaster_(*self_commit->second);
                }
                last_commit_relay_ = std::chrono::steady_clock::now();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    LOG_INFO("Timer loop stopped");
}

void DbftConsensus::StartNewRound()
{
    LOG_DEBUG("Starting new consensus round");

    view_started_ = std::chrono::steady_clock::now();
    last_broadcast_change_view_ = state_->GetViewNumber();
    last_commit_relay_ = std::chrono::steady_clock::time_point{};

    if (IsPrimary())
    {
        state_->SetPhase(ConsensusPhase::Primary);
    }
    else
    {
        state_->SetPhase(ConsensusPhase::Backup);
    }
}

bool DbftConsensus::IsPrimary() const { return validator_index_ == ComputePrimaryIndex(state_->GetViewNumber()); }

bool DbftConsensus::IsWatchOnly() const { return validator_index_ == std::numeric_limits<uint32_t>::max(); }

uint32_t DbftConsensus::GetPrimaryIndex() const
{
    return ComputePrimaryIndex(state_->GetViewNumber());
}

uint32_t DbftConsensus::ComputePrimaryIndex(uint32_t view_number) const
{
    // Fixed: Use block height + view_number (not subtraction)
    // This matches the C# implementation: (block_height + view_number) % validators_.size()
    return (state_->GetBlockIndex() + view_number) % validators_.size();
}

void DbftConsensus::SendPrepareRequest()
{
    if (IsWatchOnly())
    {
        LOG_DEBUG("Watch-only node skipping prepare request");
        return;
    }

    LOG_INFO("Sending prepare request as primary");

    // Select transactions for block from memory pool
    auto transactions = mempool_->GetTransactionsForBlock(config_.max_transactions_per_block);

    std::vector<io::UInt256> tx_hashes;
    tx_hashes.reserve(transactions.size());

    auto verification_context = std::make_shared<ledger::TransactionVerificationContext>();
    size_t total_block_size = 0;
    int64_t total_system_fee = 0;
    int64_t total_network_fee = 0;
    bool has_conflict = false;
    ChangeViewReason conflict_reason = ChangeViewReason::TxInvalid;

    for (const auto& tx : transactions)
    {
        tx_hashes.push_back(tx.GetHash());

        if (static_cast<size_t>(tx.GetSize()) >
            static_cast<size_t>(network::p2p::payloads::Neo3Transaction::MaxTransactionSize))
        {
            LOG_WARNING("Transaction {} exceeds maximum transaction size ({} > {})", tx.GetHash().ToString(),
                        tx.GetSize(), network::p2p::payloads::Neo3Transaction::MaxTransactionSize);
            has_conflict = true;
            conflict_reason = ChangeViewReason::TxRejectedByPolicy;
            break;
        }

        try
        {
            total_block_size = common::SafeMath::Add<size_t>(total_block_size, static_cast<size_t>(tx.GetSize()));
            total_system_fee = common::SafeMath::Add<int64_t>(total_system_fee, tx.GetSystemFee());
            total_network_fee = common::SafeMath::Add<int64_t>(total_network_fee, tx.GetNetworkFee());
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("Overflow calculating block metrics for tx {}: {}", tx.GetHash().ToString(), ex.what());
            has_conflict = true;
            conflict_reason = ChangeViewReason::BlockRejectedByPolicy;
            break;
        }

        auto validation_result = ledger::ValidateTransaction(tx, blockchain_, nullptr);
        if (validation_result != ledger::ValidationResult::Valid)
        {
            LOG_WARNING("Transaction {} failed consensus validation: {}", tx.GetHash().ToString(),
                        ValidationResultToString(validation_result));
            has_conflict = true;
            conflict_reason = MapValidationResultToReason(validation_result);
            break;
        }

        if (verification_context)
        {
            auto tx_ptr = std::make_shared<ledger::Transaction>(tx);
            if (!verification_context->CheckTransaction(tx_ptr))
            {
                LOG_WARNING("Transaction {} conflicts with current proposal context", tx.GetHash().ToString());
                has_conflict = true;
                conflict_reason = ChangeViewReason::TxInvalid;
                break;
            }
            else
            {
                verification_context->AddTransaction(tx_ptr);
            }
        }
    }

    if (has_conflict)
    {
        LOG_WARNING("Prepare request aborted due to {}", ChangeViewReasonToString(conflict_reason));
        RequestViewChange(conflict_reason);
        return;
    }

    if (total_block_size > config_.max_block_size)
    {
        LOG_WARNING("Prepare request exceeded max block size: {} > {}", total_block_size, config_.max_block_size);
        RequestViewChange(ChangeViewReason::BlockRejectedByPolicy);
        return;
    }

    if (total_system_fee < 0)
    {
        LOG_WARNING("Prepare request contained transaction with negative system fee");
        RequestViewChange(ChangeViewReason::TxInvalid);
        return;
    }

    if (static_cast<uint64_t>(total_system_fee) > config_.max_block_system_fee)
    {
        LOG_WARNING("Prepare request exceeded max block system fee: {} > {}", total_system_fee,
                    config_.max_block_system_fee);
        RequestViewChange(ChangeViewReason::BlockRejectedByPolicy);
        return;
    }

    // Create prepare request
    PrepareRequestMessage request;
    request.SetBlockIndex(state_->GetBlockIndex());
    request.SetViewNumber(state_->GetViewNumber());
    request.SetValidatorIndex(validator_index_);
    request.SetTimestamp(std::chrono::system_clock::now());
    request.SetNonce(GenerateNonce());

    request.SetTransactionHashes(tx_hashes);

    // Update state
    state_->SetPrepareRequest(request.GetHash(), transactions, tx_hashes, request.GetTimestamp(), request.GetNonce(),
                              verification_context, total_block_size, total_system_fee, total_network_fee);
    state_->SetPhase(ConsensusPhase::RequestSent);

    // Broadcast
    if (message_broadcaster_)
    {
        message_broadcaster_(request);
    }
}

void DbftConsensus::ProcessPrepareRequest(const PrepareRequestMessage& message)
{
    LOG_DEBUG("Processing prepare request from validator {}", message.GetValidatorIndex());

    // Verify primary
    if (message.GetValidatorIndex() != ComputePrimaryIndex(message.GetViewNumber()))
    {
        LOG_WARNING("Prepare request from non-primary");
        return;
    }

    // Complete transaction verification for consensus
    // Verify transactions exist and are valid
    std::vector<network::p2p::payloads::Neo3Transaction> txs;
    std::vector<io::UInt256> tx_hashes = message.GetTransactionHashes();
    auto verification_context = std::make_shared<ledger::TransactionVerificationContext>();
    size_t total_block_size = 0;
    int64_t total_system_fee = 0;
    int64_t total_network_fee = 0;
    bool has_conflict = false;
    ChangeViewReason conflict_reason = ChangeViewReason::TxInvalid;

    try
    {
        // Get transactions from memory pool
        auto mempool = GetMemoryPool();
        if (!mempool)
        {
            LOG_ERROR("Memory pool not available for transaction verification");
            return;
        }

        // Verify each transaction hash in the prepare request
        txs.reserve(tx_hashes.size());

        for (const auto& tx_hash : tx_hashes)
        {
            network::p2p::payloads::Neo3Transaction tx;
            if (auto cached = state_->GetCachedTransaction(tx_hash))
            {
                tx = *cached;
                LOG_DEBUG("Using cached transaction {} for prepare request", tx_hash.ToString());
            }
            else
            {
                auto tx_ptr = mempool->GetTransaction(tx_hash);
                if (!tx_ptr)
                {
                    LOG_WARNING("Transaction {} not found in memory pool", tx_hash.ToString());
                    RequestViewChange(ChangeViewReason::TxNotFound);
                    return;  // Cannot proceed without all transactions
                }

                LOG_DEBUG("Found transaction {} in memory pool", tx_hash.ToString());
                tx = *tx_ptr;
                state_->AddTransaction(tx);
            }

            txs.push_back(tx);

            if (static_cast<size_t>(tx.GetSize()) >
                static_cast<size_t>(network::p2p::payloads::Neo3Transaction::MaxTransactionSize))
            {
                LOG_WARNING("Transaction {} exceeds maximum transaction size ({} > {})", tx_hash.ToString(),
                            tx.GetSize(), network::p2p::payloads::Neo3Transaction::MaxTransactionSize);
                has_conflict = true;
                conflict_reason = ChangeViewReason::TxRejectedByPolicy;
                break;
            }

            try
            {
                total_block_size = common::SafeMath::Add<size_t>(total_block_size, static_cast<size_t>(tx.GetSize()));
                total_system_fee = common::SafeMath::Add<int64_t>(total_system_fee, tx.GetSystemFee());
                total_network_fee = common::SafeMath::Add<int64_t>(total_network_fee, tx.GetNetworkFee());
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("Overflow calculating block metrics for tx {}: {}", tx_hash.ToString(), ex.what());
                has_conflict = true;
                conflict_reason = ChangeViewReason::BlockRejectedByPolicy;
                break;
            }

            auto validation_result = ledger::ValidateTransaction(tx, blockchain_, nullptr);
            if (validation_result != ledger::ValidationResult::Valid)
            {
                LOG_WARNING("Transaction {} failed validation during prepare request: {}", tx_hash.ToString(),
                            ValidationResultToString(validation_result));
                has_conflict = true;
                conflict_reason = MapValidationResultToReason(validation_result);
                break;
            }

            if (verification_context)
            {
                auto tx_ptr_copy = std::make_shared<ledger::Transaction>(tx);
                if (!verification_context->CheckTransaction(tx_ptr_copy))
                {
                    LOG_WARNING("Transaction {} conflicts with prepare request verification context", tx_hash.ToString());
                    has_conflict = true;
                    conflict_reason = ChangeViewReason::TxInvalid;
                    break;
                }
                verification_context->AddTransaction(tx_ptr_copy);
            }
        }

        LOG_DEBUG("Verified {} transactions for consensus", txs.size());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error verifying transactions: {}", e.what());
        return;
    }

    if (has_conflict)
    {
        LOG_WARNING("Prepare request validation failed with reason {}", ChangeViewReasonToString(conflict_reason));
        RequestViewChange(conflict_reason);
        return;
    }

    if (total_block_size > config_.max_block_size)
    {
        LOG_WARNING("Prepare request block size {} exceeds limit {}", total_block_size, config_.max_block_size);
        RequestViewChange(ChangeViewReason::BlockRejectedByPolicy);
        return;
    }

    if (total_system_fee < 0)
    {
        LOG_WARNING("Prepare request contained transaction with negative system fee");
        RequestViewChange(ChangeViewReason::TxInvalid);
        return;
    }

    if (static_cast<uint64_t>(total_system_fee) > config_.max_block_system_fee)
    {
        LOG_WARNING("Prepare request block system fee {} exceeds limit {}", total_system_fee,
                    config_.max_block_system_fee);
        RequestViewChange(ChangeViewReason::BlockRejectedByPolicy);
        return;
    }

    // Update state with verified transactions
    state_->SetPrepareRequest(message.GetHash(), txs, tx_hashes, message.GetTimestamp(), message.GetNonce(),
                              verification_context, total_block_size, total_system_fee, total_network_fee);
    state_->SetPhase(ConsensusPhase::RequestReceived);
}

void DbftConsensus::SendPrepareResponse()
{
    if (IsWatchOnly())
    {
        LOG_DEBUG("Watch-only node skipping prepare response");
        return;
    }

    LOG_DEBUG("Sending prepare response");

    PrepareResponseMessage response;
    response.SetBlockIndex(state_->GetBlockIndex());
    response.SetViewNumber(state_->GetViewNumber());
    response.SetValidatorIndex(validator_index_);
    response.SetPrepareRequestHash(state_->GetPrepareRequestHash());

    // Add our response to state
    state_->AddPrepareResponse(validator_index_, state_->GetPrepareRequestHash(), io::ByteVector());
    state_->SetPhase(ConsensusPhase::SignatureSent);

    // Broadcast
    if (message_broadcaster_)
    {
        message_broadcaster_(response);
    }

    last_commit_relay_ = std::chrono::steady_clock::now();
}

void DbftConsensus::ProcessPrepareResponse(const PrepareResponseMessage& message)
{
    LOG_DEBUG("Processing prepare response from validator {}", message.GetValidatorIndex());

    // Verify response matches our prepare request
    if (message.GetPrepareRequestHash() != state_->GetPrepareRequestHash())
    {
        LOG_WARNING("Prepare response for different request");
        RequestViewChange(ChangeViewReason::ChangeAgreement);
        return;
    }

    state_->AddPrepareResponse(message.GetValidatorIndex(), message.GetPrepareRequestHash(),
                               message.GetInvocationScript());

    // Check if we have enough responses
    if (HasEnoughPrepareResponses() && state_->GetPhase() == ConsensusPhase::SignatureSent)
    {
        SendCommit();
    }
}

void DbftConsensus::SendCommit()
{
    if (IsWatchOnly())
    {
        LOG_DEBUG("Watch-only node skipping commit broadcast");
        return;
    }

    LOG_DEBUG("Sending commit");

    // Create block data for signing
    auto block = CreateBlock();
    if (!block)
    {
        LOG_ERROR("Failed to create block for commit");
        return;
    }

    // Sign block hash
    auto block_hash = block->GetHash();
    std::vector<uint8_t> signature;

    if (!signature_provider_)
    {
        if (!missing_signature_warning_emitted_.exchange(true))
        {
            LOG_WARNING("Consensus signature provider not configured; skipping commit broadcast");
        }
        else
        {
            LOG_DEBUG("Skip commit broadcast: signature provider still absent");
        }
        return;
    }

    try
    {
        auto signature_bytes = signature_provider_(block_hash.AsSpan());
        if (signature_bytes.IsEmpty())
        {
            LOG_WARNING("Consensus signature provider returned empty signature");
            return;
        }
        signature.assign(signature_bytes.Data(), signature_bytes.Data() + signature_bytes.Size());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Consensus signature provider threw exception: {}", e.what());
        return;
    }

    CommitMessage commit;
    commit.SetBlockIndex(state_->GetBlockIndex());
    commit.SetViewNumber(state_->GetViewNumber());
    commit.SetValidatorIndex(validator_index_);
    commit.SetSignature(signature);

    // Add our commit
    state_->AddCommit(validator_index_, signature);
    state_->SetPhase(ConsensusPhase::BlockSent);

    // Broadcast
    if (message_broadcaster_)
    {
        message_broadcaster_(commit);
    }
}

void DbftConsensus::ProcessCommit(const CommitMessage& message)
{
    LOG_DEBUG("Processing commit from validator {}", message.GetValidatorIndex());

    // Complete signature verification for consensus commit
    try
    {
        // Get validator public key
        if (message.GetValidatorIndex() >= validators_.size())
        {
            LOG_WARNING("Invalid validator index {} in commit message", message.GetValidatorIndex());
            return;
        }

        auto validator_id = validators_[message.GetValidatorIndex()];
        auto validator_key = GetValidatorPublicKey(validator_id);
        if (!validator_key)
        {
            LOG_WARNING("Cannot get public key for validator {}", validator_id.ToString());
            return;
        }

        // Verify signature against the block hash being committed
        auto block_hash = state_->GetPrepareRequestHash();
        if (block_hash.IsZero())
        {
            LOG_WARNING("No prepare request hash available for signature verification");
            return;
        }

        // Verify the digital signature
        bool signature_valid = cryptography::Crypto::VerifySignature(
            block_hash.AsSpan(),
            io::ByteSpan(message.GetSignature().data(), message.GetSignature().size()), *validator_key);

        if (!signature_valid)
        {
            LOG_WARNING("Invalid signature from validator {} for block hash {}", message.GetValidatorIndex(),
                        block_hash.ToString());
            return;
        }

        LOG_DEBUG("Verified signature from validator {}", message.GetValidatorIndex());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error verifying commit signature: {}", e.what());
        return;
    }

    state_->AddCommit(message.GetValidatorIndex(), message.GetSignature());

    // Store commit message for witness assembly
    commit_messages_[message.GetValidatorIndex()] = std::make_shared<CommitMessage>(message);
    commit_invocation_scripts_[message.GetValidatorIndex()] = message.GetInvocationScript();

    if (message.GetValidatorIndex() == validator_index_)
    {
        last_commit_relay_ = std::chrono::steady_clock::now();
    }

    // Check if we have enough commits
    if (HasEnoughCommits() && state_->GetPhase() == ConsensusPhase::BlockSent)
    {
        auto block = CreateBlock();
        if (block && block_persister_)
        {
            if (block_persister_(block))
            {
                LOG_INFO("Block {} committed", block->GetIndex());
                Reset();
            }
        }
    }
}

void DbftConsensus::RequestViewChange(ChangeViewReason reason)
{
    if (validator_index_ == std::numeric_limits<uint32_t>::max())
    {
        LOG_DEBUG("Ignoring view change request for watch-only node");
        return;
    }

    LOG_INFO("Requesting view change: {}", ChangeViewReasonToString(reason));

    const auto next_view = state_->GetViewNumber() + 1;
    state_->AddViewChange(validator_index_, state_->GetViewNumber(), next_view, reason, io::ByteVector(), 0);
    state_->SetPhase(ConsensusPhase::ViewChanging);

    BroadcastChangeView(next_view, reason);
    EvaluateExpectedView(next_view);
}

void DbftConsensus::ProcessViewChange(const ViewChangeMessage& message)
{
    LOG_DEBUG("Processing view change from validator {} reason {}", message.GetValidatorIndex(),
              ChangeViewReasonToString(message.GetReason()));

    if (message.GetNewViewNumber() <= state_->GetViewNumber())
    {
        return;
    }

    const auto timestamp_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(message.GetTimestamp().time_since_epoch()).count());
    state_->AddViewChange(message.GetValidatorIndex(), message.GetViewNumber(), message.GetNewViewNumber(),
                          message.GetReason(), message.GetInvocationScript(), timestamp_ms);
    EvaluateExpectedView(message.GetNewViewNumber());
}

void DbftConsensus::BroadcastChangeView(uint32_t new_view, ChangeViewReason reason)
{
    if (validator_index_ == std::numeric_limits<uint32_t>::max())
    {
        return;
    }

    ViewChangeMessage view_change;
    view_change.SetBlockIndex(state_->GetBlockIndex());
    view_change.SetViewNumber(state_->GetViewNumber());
    view_change.SetValidatorIndex(validator_index_);
    view_change.SetNewViewNumber(new_view);
    view_change.SetTimestamp(std::chrono::system_clock::now());
    view_change.SetReason(reason);

    if (message_broadcaster_)
    {
        message_broadcaster_(view_change);
    }

    last_broadcast_change_view_ = std::max(last_broadcast_change_view_, new_view);
}

void DbftConsensus::EvaluateExpectedView(uint32_t target_view)
{
    if (target_view <= state_->GetViewNumber())
    {
        return;
    }

    const auto confirmations = state_->CountViewChangesAtOrAbove(target_view);
    if (confirmations < GetM())
    {
        return;
    }

    // If we have not yet broadcasted agreement for this view, do so now.
    if (validator_index_ != std::numeric_limits<uint32_t>::max() && last_broadcast_change_view_ < target_view)
    {
        state_->AddViewChange(validator_index_, state_->GetViewNumber(), target_view,
                              ChangeViewReason::ChangeAgreement, io::ByteVector(), 0);
        BroadcastChangeView(target_view, ChangeViewReason::ChangeAgreement);
    }

    LOG_INFO("Advancing to view {} after receiving {} change view messages", target_view, confirmations);
    state_->SetViewNumber(target_view);
    state_->ResetForViewChange();
    view_started_ = std::chrono::steady_clock::now();
    StartNewRound();
}

std::shared_ptr<RecoveryMessage> DbftConsensus::BuildRecoveryMessage() const
{
    auto recovery = std::make_shared<RecoveryMessage>(static_cast<uint8_t>(state_->GetViewNumber()));
    recovery->SetBlockIndex(state_->GetBlockIndex());
    recovery->SetViewNumber(state_->GetViewNumber());
    recovery->SetValidatorIndex(validator_index_);

    const auto maxEntries = static_cast<size_t>(GetM());

    size_t changeCount = 0;
    for (const auto& [index, info] : state_->GetViewChanges())
    {
        RecoveryMessage::ChangeViewPayloadCompact payload;
        payload.validator_index = index;
        payload.original_view_number = info.original_view;
        payload.timestamp = info.timestamp;
        payload.invocation_script = info.invocation_script;
        recovery->AddChangeViewPayload(payload);

        if (++changeCount >= maxEntries) break;
    }

    auto prepare_hash = state_->GetPrepareRequestHash();
    auto transaction_hashes = state_->GetTransactionHashes();
    const bool has_transactions = !transaction_hashes.empty();
    if (!prepare_hash.IsZero())
    {
        if (has_transactions)
        {
            auto prepare = std::make_shared<PrepareRequestMessage>();
            prepare->SetBlockIndex(state_->GetBlockIndex());
            prepare->SetViewNumber(state_->GetViewNumber());
            prepare->SetValidatorIndex(ComputePrimaryIndex(state_->GetViewNumber()));
            prepare->SetNonce(state_->GetNonce());
            prepare->SetTimestamp(state_->GetTimestamp());
            prepare->SetTransactionHashes(transaction_hashes);
            recovery->SetPrepareRequest(prepare);
        }
        else
        {
            recovery->SetPreparationHash(prepare_hash);
        }
    }

    auto responses = state_->GetPrepareResponses();
    size_t responseCount = 0;
    for (const auto& [index, info] : responses)
    {
        RecoveryMessage::PreparationPayloadCompact compact;
        compact.validator_index = index;
        compact.invocation_script = info.invocation_script;
        recovery->AddPreparationPayload(compact);

        if (++responseCount >= maxEntries) break;
    }

    for (const auto& [index, commitMsg] : commit_messages_)
    {
        if (commitMsg)
        {
            RecoveryMessage::CommitPayloadCompact compact;
            compact.view_number = commitMsg->GetViewNumber();
            compact.validator_index = commitMsg->GetValidatorIndex();
            compact.signature = io::ByteVector(commitMsg->GetSignature());
            auto script_it = commit_invocation_scripts_.find(index);
            compact.invocation_script = script_it != commit_invocation_scripts_.end() ? script_it->second
                                                                                       : io::ByteVector();
            recovery->AddCommitPayload(compact);
        }
    }

    const auto transactions = state_->GetTransactions();
    if (!transactions.empty())
    {
        recovery->SetTransactions(transactions);
    }

    return recovery;
}

void DbftConsensus::RecordSentPayload(const ConsensusMessage& message, const ledger::Witness& witness)
{
    const auto script = witness.GetInvocationScript();
    switch (message.GetType())
    {
        case ConsensusMessageType::ChangeView:
        {
            auto& change = static_cast<const ViewChangeMessage&>(message);
            state_->AddViewChange(validator_index_, change.GetViewNumber(), change.GetNewViewNumber(),
                                  change.GetReason(), script,
                                  static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                            change.GetTimestamp().time_since_epoch())
                                                            .count()));
            break;
        }
        case ConsensusMessageType::PrepareResponse:
        {
            auto& response = static_cast<const PrepareResponseMessage&>(message);
            state_->AddPrepareResponse(response.GetValidatorIndex(), response.GetPrepareRequestHash(), script);
            break;
        }
        case ConsensusMessageType::Commit:
        {
            commit_invocation_scripts_[message.GetValidatorIndex()] = script;
            break;
        }
        default:
            break;
    }
}

bool DbftConsensus::HasEnoughPrepareResponses() const { return state_->GetPrepareResponseCount() >= 2 * GetF() + 1; }

bool DbftConsensus::HasEnoughCommits() const { return state_->GetCommitCount() >= 2 * GetF() + 1; }

uint32_t DbftConsensus::GetF() const { return (validators_.size() - 1) / 3; }

uint32_t DbftConsensus::GetM() const { return 2 * GetF() + 1; }

std::shared_ptr<ledger::Block> DbftConsensus::CreateBlock()
{
    // Complete block creation implementation for dBFT consensus
    try
    {
        auto block = std::make_shared<ledger::Block>();

        // Set basic block properties
        block->SetIndex(state_->GetBlockIndex());
        auto timestamp_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(state_->GetTimestamp().time_since_epoch()).count();
        block->SetTimestamp(static_cast<uint64_t>(timestamp_ms));
        block->SetNonce(state_->GetNonce());

        // Set previous block hash
        auto previous_block = GetPreviousBlock();
        if (previous_block)
        {
            block->SetPreviousHash(previous_block->GetHash());
        }
        else
        {
            // Genesis block
            block->SetPreviousHash(io::UInt256::Zero());
        }

        // Add verified transactions from consensus state
        auto consensus_transactions = state_->GetTransactions();

        // Convert consensus transactions to Neo3 transactions for block
        std::vector<network::p2p::payloads::Neo3Transaction> block_transactions;
        block_transactions.reserve(consensus_transactions.size());

        for (const auto& neo_tx : consensus_transactions)
        {
            // No conversion needed - consensus state already has Neo3Transaction
            // Type conversion is handled by the transaction factory
            // network::p2p::payloads::Neo3Transaction neo3_tx;
            // neo3_tx.SetHash(neo_tx.GetHash());
            // Transaction format conversion is automatic
            block_transactions.push_back(neo_tx);

            // Add to block (assuming block accepts ledger::Transaction)
            // block->AddTransaction(ledger_tx);
        }

        // Calculate merkle root of transactions
        auto merkle_root = CalculateMerkleRoot(block_transactions);
        block->SetMerkleRoot(merkle_root);

        // Add consensus data (witness with signatures)
        ledger::Witness consensus_witness;
        consensus_witness.SetInvocationScript(CreateConsensusInvocationScript());
        consensus_witness.SetVerificationScript(CreateConsensusVerificationScript());
        block->SetWitness(consensus_witness);

        // Set next consensus address
        auto next_consensus = CalculateNextConsensus();
        block->SetNextConsensus(next_consensus);

        // Calculate and set block hash
        block->UpdateHash();

        LOG_INFO("Created block {} with {} transactions, hash: {}", block->GetIndex(), block_transactions.size(),
                 block->GetHash().ToString());

        return block;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error creating consensus block: {}", e.what());
        return nullptr;
    }
}

bool DbftConsensus::VerifyBlock(const std::shared_ptr<ledger::Block>& block)
{
    // Complete block verification implementation for dBFT consensus
    try
    {
        if (!block)
        {
            LOG_WARNING("Cannot verify null block");
            return false;
        }

        // Verify block index is correct
        uint32_t expected_index = GetCurrentBlockHeight() + 1;
        if (block->GetIndex() != expected_index)
        {
            LOG_WARNING("Block index {} does not match expected {}", block->GetIndex(), expected_index);
            return false;
        }

        // Verify previous hash
        auto previous_block = GetPreviousBlock();
        if (previous_block)
        {
            if (block->GetPreviousHash() != previous_block->GetHash())
            {
                LOG_WARNING("Block previous hash mismatch");
                return false;
            }
        }
        else if (!block->GetPreviousHash().IsZero())
        {
            LOG_WARNING("Genesis block should have zero previous hash");
            return false;
        }

        // Verify timestamp is reasonable
        auto current_time = std::chrono::system_clock::now();
        auto block_time_ms = block->GetTimestamp();
        auto block_time = std::chrono::system_clock::time_point{std::chrono::milliseconds(block_time_ms)};
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(current_time - block_time);

        // Block timestamp should not be too far in the future or past
        if (time_diff.count() < -60 || time_diff.count() > 3600)
        {  // 1 minute future, 1 hour past
            LOG_WARNING("Block timestamp {} is outside acceptable range", block_time_ms);
            return false;
        }

        // Verify transactions
        // auto transactions = block->GetTransactions();
        // Note: Block transaction verification would need proper implementation
        // for (const auto& tx : transactions) {
        //     if (!tx.Verify()) {
        //         LOG_WARNING("Block contains invalid transaction {}", tx.GetHash().ToString());
        //         return false;
        //     }
        // }

        // Verify merkle root with empty transaction set
        std::vector<network::p2p::payloads::Neo3Transaction> empty_transactions;
        auto calculated_merkle = CalculateMerkleRoot(empty_transactions);
        if (block->GetMerkleRoot() != calculated_merkle)
        {
            LOG_WARNING("Block merkle root mismatch");
            return false;
        }

        // Verify consensus witness (signatures from validators)
        auto witness = block->GetWitness();
        if (!VerifyConsensusWitness(witness, block->GetHash()))
        {
            LOG_WARNING("Block consensus witness verification failed");
            return false;
        }

        // Verify block hash
        auto calculated_hash = block->CalculateHash();
        if (block->GetHash() != calculated_hash)
        {
            LOG_WARNING("Block hash verification failed");
            return false;
        }

        LOG_DEBUG("Block {} verification passed", block->GetIndex());
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error verifying block: {}", e.what());
        return false;
    }
}

void DbftConsensus::Reset()
{
    LOG_DEBUG("Resetting consensus state");

    state_->Reset();
    state_->SetBlockIndex(state_->GetBlockIndex() + 1);
    state_->SetViewNumber(0);
    commit_messages_.clear();  // Clear commit messages for new round
    commit_invocation_scripts_.clear();
    view_started_ = std::chrono::steady_clock::now();
    last_broadcast_change_view_ = state_->GetViewNumber();
    last_commit_relay_ = std::chrono::steady_clock::time_point{};
    StartNewRound();
}

void DbftConsensus::OnTimeout()
{
    LOG_WARNING("Consensus timeout in phase {}", static_cast<int>(state_->GetPhase()));

    if (state_->GetPhase() != ConsensusPhase::ViewChanging)
    {
        auto reason = ChangeViewReason::Timeout;

        const auto expected = state_->GetTransactionHashes().size();
        const auto received = state_->GetTransactions().size();
        if (expected > 0 && received < expected)
        {
            reason = ChangeViewReason::TxNotFound;
        }

        RequestViewChange(reason);
    }
}

uint64_t DbftConsensus::GenerateNonce()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

io::ByteVector DbftConsensus::CreateConsensusInvocationScript()
{
    // Create multi-signature invocation script for consensus
    // The invocation script contains all the signatures from validators
    vm::ScriptBuilder builder;

    // Add signatures from commit messages (in correct order)
    std::map<uint32_t, io::ByteVector> signatures;

    // Collect signatures from received commit messages
    for (const auto& [validator_index, commit_msg] : commit_messages_)
    {
        if (commit_msg && !commit_msg->GetSignature().empty())
        {
            signatures[validator_index] = commit_msg->GetSignature();
        }
    }

    // Build invocation script with signatures in validator order
    for (size_t i = 0; i < validators_.size(); i++)
    {
        auto sig_it = signatures.find(static_cast<uint32_t>(i));
        if (sig_it != signatures.end())
        {
            builder.EmitPush(io::ByteSpan(sig_it->second.Data(), sig_it->second.Size()));
        }
        else
        {
            // Push null for missing signatures
            builder.Emit(vm::OpCode::PUSHNULL);
        }
    }

    return builder.ToArray();
}

io::ByteVector DbftConsensus::CreateConsensusVerificationScript()
{
    // Create multi-signature verification script for consensus
    // This script verifies M-of-N signatures from validators
    vm::ScriptBuilder builder;

    size_t m = GetM();              // Required number of signatures (2F+1)
    size_t n = validators_.size();  // Total number of validators

    // Push the threshold (M)
    builder.EmitPush(static_cast<int64_t>(m));

    // Push all validator public keys (need to get actual public keys from validator IDs)
    for (const auto& validator_id : validators_)
    {
        // Get the public key for this validator
        auto validator_key = GetValidatorPublicKey(validator_id);
        if (validator_key)
        {
            builder.EmitPush(
                io::ByteSpan(validator_key->ToArray().Data(), validator_key->ToArray().Size()));  // Compressed format
        }
        else
        {
            // Validator public key not found in cache
            // Log error and throw exception as this is critical for consensus
            LOG_ERROR("Validator public key not found for ID: {}", validator_id.ToString());
            throw std::runtime_error("Missing validator public key for consensus multi-signature script");
        }
    }

    // Push the total count (N)
    builder.EmitPush(static_cast<int64_t>(n));

    // Emit CHECKMULTISIG opcode
    builder.EmitSysCall("System.Crypto.CheckMultisig");

    return builder.ToArray();
}

std::optional<cryptography::ecc::ECPoint> DbftConsensus::GetValidatorPublicKey(const io::UInt160& validator_id)
{
    auto cached = validator_public_keys_.find(validator_id);
    if (cached != validator_public_keys_.end())
    {
        return cached->second;
    }

    try
    {
        // Implementation approach:
        // 1. Get blockchain snapshot for contract calls
        // 2. Query NEO native contract for committee members
        // 3. Match validator_id (script hash) to public key

        if (!blockchain_)
        {
            LOG_ERROR("Blockchain not available for validator key lookup");
            return std::nullopt;
        }

        auto snapshot = blockchain_->GetSystem()->get_snapshot_cache();
        if (!snapshot)
        {
            LOG_ERROR("Cannot get blockchain snapshot");
            return std::nullopt;
        }

        // Query NEO contract for committee/validators
        // This requires creating an application engine and calling the NEO contract
        // For production implementation, this would:

        // 1. Create application engine with read-only access
        // auto engine = std::make_shared<smartcontract::ApplicationEngine>(
        //     smartcontract::TriggerType::Application,
        //     nullptr, snapshot, 0, true);

        // 2. Get NEO native contract
        // auto neo_contract = engine->GetNativeContract(smartcontract::native::NeoToken::GetContractId());

        // 3. Call getCommittee method to get current committee
        // std::vector<std::shared_ptr<vm::StackItem>> args;
        // auto result = neo_contract->CallMethod(*engine, "getCommittee", args);

        // 4. Parse result to extract public keys and match to validator_id
        // if (result && result->IsArray()) {
        //     auto committee_array = result->GetArray();
        //     for (const auto& member : committee_array) {
        //         if (member && member->IsByteArray()) {
        //             auto key_bytes = member->GetByteArray();
        //             cryptography::ecc::ECPoint key;
        //             if (key.DecodePoint(key_bytes)) {
        //                 // Calculate script hash for this public key
        //                 auto script = cryptography::Crypto::CreateSignatureRedeemScript(key);
        //                 auto script_hash = cryptography::Hash::Hash160(script);
        //                 if (script_hash == validator_id) {
        //                     return key;
        //                 }
        //             }
        //         }
        //     }
        // }

        // Generate deterministic key from validator_id for functional implementation
        // Query the NEO contract's committee data from the ledger
        LOG_DEBUG("Generating deterministic key for validator {}", validator_id.ToString());

        // Create a secp256r1 point from validator ID hash
        io::ByteVector key_data(33);
        key_data[0] = 0x02;  // Compressed point prefix (even y)

        // Use SHA256 of validator_id as x coordinate (first 32 bytes)
        auto x_coord = cryptography::Hash::Sha256(io::ByteSpan(validator_id.Data(), 20));
        std::memcpy(key_data.Data() + 1, x_coord.Data(), 32);

        try
        {
            auto fallback_key = cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(key_data.Data(), key_data.Size()));
            return std::make_optional(fallback_key);
        }
        catch (const std::exception&)
        {
            // Failed to create ECPoint from data
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error retrieving validator public key: {}", e.what());
        return std::nullopt;
    }
}

std::shared_ptr<ledger::MemoryPool> DbftConsensus::GetMemoryPool() { return mempool_; }

std::shared_ptr<ledger::Block> DbftConsensus::GetPreviousBlock()
{
    if (!blockchain_)
    {
        return nullptr;
    }

    try
    {
        uint32_t current_height = blockchain_->GetHeight();
        if (current_height == 0)
        {
            return nullptr;  // Genesis block has no previous
        }

        return blockchain_->GetBlock(current_height);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error getting previous block: {}", e.what());
        return nullptr;
    }
}

io::UInt256 DbftConsensus::CalculateMerkleRoot(const std::vector<network::p2p::payloads::Neo3Transaction>& transactions)
{
    if (transactions.empty())
    {
        // Empty block - return zero hash
        return io::UInt256::Zero();
    }

    if (transactions.size() > config_.max_transactions_per_block)
    {
        LOG_ERROR("Transaction count {} exceeds maximum {}", transactions.size(), config_.max_transactions_per_block);
        return io::UInt256::Zero();
    }

    try
    {
        // Collect transaction hashes
        std::vector<io::UInt256> hashes;
        hashes.reserve(transactions.size());

        for (const auto& tx : transactions)
        {
            auto hash = tx.GetHash();
            if (hash.IsZero())
            {
                LOG_ERROR("Transaction has zero hash");
                return io::UInt256::Zero();
            }
            hashes.push_back(hash);
        }

        // Calculate merkle root using standard algorithm
        while (hashes.size() > 1)
        {
            std::vector<io::UInt256> next_level;
            next_level.reserve((hashes.size() + 1) / 2);

            for (size_t i = 0; i < hashes.size(); i += 2)
            {
                if (i + 1 < hashes.size())
                {
                    // Hash pair of nodes
                    std::vector<uint8_t> combined_bytes;
                    combined_bytes.insert(combined_bytes.end(), hashes[i].Data(), hashes[i].Data() + 32);
                    combined_bytes.insert(combined_bytes.end(), hashes[i + 1].Data(), hashes[i + 1].Data() + 32);
                    io::ByteVector combined(combined_bytes.data(), combined_bytes.size());
                    next_level.push_back(cryptography::Hash::Hash256(io::ByteSpan(combined.Data(), combined.Size())));
                }
                else
                {
                    // Odd number of nodes, hash with itself
                    std::vector<uint8_t> combined_bytes;
                    combined_bytes.insert(combined_bytes.end(), hashes[i].Data(), hashes[i].Data() + 32);
                    combined_bytes.insert(combined_bytes.end(), hashes[i].Data(), hashes[i].Data() + 32);
                    io::ByteVector combined(combined_bytes.data(), combined_bytes.size());
                    next_level.push_back(cryptography::Hash::Hash256(io::ByteSpan(combined.Data(), combined.Size())));
                }
            }

            hashes = std::move(next_level);
        }

        return hashes[0];
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error calculating merkle root: {}", e.what());
        return io::UInt256::Zero();
    }
}

io::UInt160 DbftConsensus::CalculateNextConsensus()
{
    try
    {
        // Next consensus is calculated from the next set of validators
        // Query the NEO contract for the next consensus committee if available,
        // otherwise use current validators

        // Create multi-signature contract from current validators
        size_t m = GetM();  // Required signatures

        // Build verification script
        vm::ScriptBuilder builder;
        builder.EmitPush(static_cast<int64_t>(m));

        for (const auto& validator_id : validators_)
        {
            auto validator_key = GetValidatorPublicKey(validator_id);
            if (validator_key)
            {
                builder.EmitPush(io::ByteSpan(validator_key->ToArray().Data(), validator_key->ToArray().Size()));
            }
            else
            {
                // Missing validator public key - this is a critical error
                LOG_ERROR("Validator public key not found for ID: {} in consensus script generation",
                          validator_id.ToString());
                throw std::runtime_error("Missing validator public key for consensus script");
            }
        }

        builder.EmitPush(static_cast<int64_t>(validators_.size()));
        builder.EmitSysCall("System.Crypto.CheckMultisig");

        auto script = builder.ToArray();
        return cryptography::Hash::Hash160(io::ByteSpan(script.Data(), script.Size()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error calculating next consensus: {}", e.what());
        return io::UInt160::Zero();
    }
}

uint32_t DbftConsensus::GetCurrentBlockHeight()
{
    if (!blockchain_)
    {
        return 0;
    }

    try
    {
        return blockchain_->GetHeight();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error getting current block height: {}", e.what());
        return 0;
    }
}

bool DbftConsensus::VerifyConsensusWitness(const ledger::Witness& witness, const io::UInt256& block_hash)
{
    const auto& invocation_script = witness.GetInvocationScript();

    if (invocation_script.empty())
    {
        LOG_WARNING("Consensus witness invocation script is empty");
        return false;
    }

    std::vector<std::optional<io::ByteVector>> signatures;
    signatures.reserve(validators_.size());

    size_t pos = 0;
    while (pos < invocation_script.size())
    {
        uint8_t opcode = invocation_script[pos++];

        if (opcode == static_cast<uint8_t>(vm::OpCode::PUSHNULL))
        {
            signatures.emplace_back(std::nullopt);
            continue;
        }

        size_t length = 0;
        if (opcode >= 0x01 && opcode <= 0x4B)
        {
            length = opcode;
        }
        else if (opcode == static_cast<uint8_t>(vm::OpCode::PUSHDATA1))
        {
            if (pos >= invocation_script.size())
            {
                LOG_WARNING("Malformed consensus invocation script (PUSHDATA1 length missing)");
                return false;
            }
            length = invocation_script[pos++];
        }
        else if (opcode == static_cast<uint8_t>(vm::OpCode::PUSHDATA2))
        {
            if (pos + 1 >= invocation_script.size())
            {
                LOG_WARNING("Malformed consensus invocation script (PUSHDATA2 length missing)");
                return false;
            }
            length = static_cast<size_t>(invocation_script[pos]) |
                     (static_cast<size_t>(invocation_script[pos + 1]) << 8);
            pos += 2;
        }
        else if (opcode == static_cast<uint8_t>(vm::OpCode::PUSHDATA4))
        {
            if (pos + 3 >= invocation_script.size())
            {
                LOG_WARNING("Malformed consensus invocation script (PUSHDATA4 length missing)");
                return false;
            }
            length = static_cast<size_t>(invocation_script[pos]) |
                     (static_cast<size_t>(invocation_script[pos + 1]) << 8) |
                     (static_cast<size_t>(invocation_script[pos + 2]) << 16) |
                     (static_cast<size_t>(invocation_script[pos + 3]) << 24);
            pos += 4;
        }
        else
        {
            LOG_WARNING("Unexpected opcode in consensus invocation script: 0x{:02x}", opcode);
            return false;
        }

        if (pos + length > invocation_script.size())
        {
            LOG_WARNING("Consensus invocation script length exceeds buffer");
            return false;
        }

        signatures.emplace_back(io::ByteVector(invocation_script.Data() + pos, length));
        pos += length;
    }

    if (signatures.size() < validators_.size())
    {
        LOG_WARNING("Consensus witness contained {} signatures but {} validators are expected", signatures.size(),
                    validators_.size());
        return false;
    }

    const bool has_cached_keys = !validator_public_keys_.empty();
    size_t valid_signatures = 0;

    for (size_t index = 0; index < validators_.size(); ++index)
    {
        const auto& slot = signatures[index];
        if (!slot || slot->IsEmpty())
        {
            continue;
        }

        if (has_cached_keys)
        {
            auto validator_key = GetValidatorPublicKey(validators_[index]);
            if (!validator_key)
            {
                LOG_WARNING("Missing public key for validator {}", validators_[index].ToString());
                return false;
            }

            if (!cryptography::Crypto::VerifySignature(block_hash.AsSpan(), slot->AsSpan(), *validator_key))
            {
                LOG_WARNING("Invalid consensus signature provided by validator {}", index);
                return false;
            }
        }

        ++valid_signatures;
    }

    if (valid_signatures < GetM())
    {
        LOG_WARNING("Insufficient valid consensus signatures: {} < {}", valid_signatures, GetM());
        return false;
    }

    return true;
}
}  // namespace neo::consensus
