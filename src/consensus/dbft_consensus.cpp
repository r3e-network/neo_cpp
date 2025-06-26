#include <neo/consensus/dbft_consensus.h>
#include <neo/consensus/consensus_message.h>
#include <neo/ledger/block.h>
#include <neo/cryptography/crypto.h>
#include <neo/common/safe_math.h>
#include <algorithm>
#include <random>

namespace neo::consensus
{
    DbftConsensus::DbftConsensus(const ConsensusConfig& config,
                                 const io::UInt160& node_id,
                                 const std::vector<io::UInt160>& validators)
        : config_(config),
          node_id_(node_id),
          validators_(validators),
          state_(std::make_shared<ConsensusState>()),
          logger_(core::Logger::GetInstance())
    {
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
    }

    DbftConsensus::~DbftConsensus()
    {
        Stop();
    }

    void DbftConsensus::Start()
    {
        if (running_.exchange(true))
        {
            return; // Already running
        }

        LOG_INFO("Starting dBFT consensus for node {}", node_id_.ToString());

        consensus_thread_ = std::thread(&DbftConsensus::ConsensusLoop, this);
        timer_thread_ = std::thread(&DbftConsensus::TimerLoop, this);

        // Initialize state
        state_->SetBlockIndex(0); // Should be set from blockchain
        state_->SetViewNumber(0);
        state_->SetPhase(ConsensusPhase::Initial);
    }

    void DbftConsensus::Stop()
    {
        if (!running_.exchange(false))
        {
            return; // Already stopped
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
            return false;
        }

        // Validate message basics
        if (message.GetBlockIndex() != state_->GetBlockIndex())
        {
            LOG_DEBUG("Ignoring message for different block height");
            return false;
        }

        if (message.GetValidatorIndex() >= validators_.size())
        {
            LOG_WARNING("Invalid validator index in message");
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
        if (!tx_verifier_ || !tx_verifier_(tx))
        {
            LOG_DEBUG("Transaction verification failed");
            return false;
        }

        return state_->AddTransaction(tx);
    }

    const ConsensusState& DbftConsensus::GetState() const
    {
        return *state_;
    }

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

            if (elapsed > config_.view_timeout)
            {
                OnTimeout();
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        LOG_INFO("Timer loop stopped");
    }

    void DbftConsensus::StartNewRound()
    {
        LOG_DEBUG("Starting new consensus round");

        view_started_ = std::chrono::steady_clock::now();
        
        if (IsPrimary())
        {
            state_->SetPhase(ConsensusPhase::Primary);
        }
        else
        {
            state_->SetPhase(ConsensusPhase::Backup);
        }
    }

    bool DbftConsensus::IsPrimary() const
    {
        return validator_index_ == GetPrimaryIndex(state_->GetViewNumber());
    }

    uint32_t DbftConsensus::GetPrimaryIndex(uint32_t view_number) const
    {
        return (state_->GetBlockIndex() - view_number) % validators_.size();
    }

    void DbftConsensus::SendPrepareRequest()
    {
        LOG_INFO("Sending prepare request as primary");

        // Select transactions for block
        auto transactions = state_->GetTransactionsForBlock(config_.max_transactions_per_block);
        
        // Create prepare request
        PrepareRequestMessage request;
        request.SetBlockIndex(state_->GetBlockIndex());
        request.SetViewNumber(state_->GetViewNumber());
        request.SetValidatorIndex(validator_index_);
        request.SetTimestamp(std::chrono::system_clock::now());
        request.SetNonce(GenerateNonce());

        // Set transaction hashes
        std::vector<io::UInt256> tx_hashes;
        for (const auto& tx : transactions)
        {
            tx_hashes.push_back(tx.GetHash());
        }
        request.SetTransactionHashes(tx_hashes);

        // Update state
        state_->SetPrepareRequest(request.GetHash(), transactions, request.GetTimestamp(), request.GetNonce());
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
        if (message.GetValidatorIndex() != GetPrimaryIndex(message.GetViewNumber()))
        {
            LOG_WARNING("Prepare request from non-primary");
            return;
        }

        // TODO: Verify transactions exist and are valid

        // Update state
        std::vector<network::p2p::payloads::Neo3Transaction> txs; // Would need to fetch from pool
        state_->SetPrepareRequest(message.GetHash(), txs, message.GetTimestamp(), message.GetNonce());
        state_->SetPhase(ConsensusPhase::RequestReceived);
    }

    void DbftConsensus::SendPrepareResponse()
    {
        LOG_DEBUG("Sending prepare response");

        PrepareResponseMessage response;
        response.SetBlockIndex(state_->GetBlockIndex());
        response.SetViewNumber(state_->GetViewNumber());
        response.SetValidatorIndex(validator_index_);
        response.SetPrepareRequestHash(state_->GetPrepareRequestHash());

        // Add our response to state
        state_->AddPrepareResponse(validator_index_, state_->GetPrepareRequestHash());
        state_->SetPhase(ConsensusPhase::SignatureSent);

        // Broadcast
        if (message_broadcaster_)
        {
            message_broadcaster_(response);
        }
    }

    void DbftConsensus::ProcessPrepareResponse(const PrepareResponseMessage& message)
    {
        LOG_DEBUG("Processing prepare response from validator {}", message.GetValidatorIndex());

        // Verify response matches our prepare request
        if (message.GetPrepareRequestHash() != state_->GetPrepareRequestHash())
        {
            LOG_WARNING("Prepare response for different request");
            return;
        }

        state_->AddPrepareResponse(message.GetValidatorIndex(), message.GetPrepareRequestHash());

        // Check if we have enough responses
        if (HasEnoughPrepareResponses() && state_->GetPhase() == ConsensusPhase::SignatureSent)
        {
            SendCommit();
        }
    }

    void DbftConsensus::SendCommit()
    {
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
        std::vector<uint8_t> signature; // Would need actual signing

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

        // TODO: Verify signature

        state_->AddCommit(message.GetValidatorIndex(), message.GetSignature());

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

    void DbftConsensus::RequestViewChange()
    {
        LOG_INFO("Requesting view change");

        ViewChangeMessage view_change;
        view_change.SetBlockIndex(state_->GetBlockIndex());
        view_change.SetViewNumber(state_->GetViewNumber());
        view_change.SetValidatorIndex(validator_index_);
        view_change.SetNewViewNumber(state_->GetViewNumber() + 1);
        view_change.SetTimestamp(std::chrono::system_clock::now());

        state_->AddViewChange(validator_index_);
        state_->SetPhase(ConsensusPhase::ViewChanging);

        // Broadcast
        if (message_broadcaster_)
        {
            message_broadcaster_(view_change);
        }
    }

    void DbftConsensus::ProcessViewChange(const ViewChangeMessage& message)
    {
        LOG_DEBUG("Processing view change from validator {}", message.GetValidatorIndex());

        if (message.GetNewViewNumber() <= state_->GetViewNumber())
        {
            return;
        }

        state_->AddViewChange(message.GetValidatorIndex());

        if (HasEnoughViewChanges())
        {
            LOG_INFO("View change to view {}", message.GetNewViewNumber());
            state_->SetViewNumber(message.GetNewViewNumber());
            state_->ResetForViewChange();
            view_started_ = std::chrono::steady_clock::now();
            StartNewRound();
        }
    }

    bool DbftConsensus::HasEnoughPrepareResponses() const
    {
        return state_->GetPrepareResponseCount() >= 2 * GetF() + 1;
    }

    bool DbftConsensus::HasEnoughCommits() const
    {
        return state_->GetCommitCount() >= 2 * GetF() + 1;
    }

    bool DbftConsensus::HasEnoughViewChanges() const
    {
        return state_->GetViewChangeCount() >= GetF() + 1;
    }

    uint32_t DbftConsensus::GetF() const
    {
        return (validators_.size() - 1) / 3;
    }

    std::shared_ptr<ledger::Block> DbftConsensus::CreateBlock()
    {
        // TODO: Implement actual block creation
        auto block = std::make_shared<ledger::Block>();
        
        // Set block properties
        block->SetIndex(state_->GetBlockIndex());
        block->SetTimestamp(std::chrono::system_clock::now());
        
        // Add transactions
        // Add consensus data
        
        return block;
    }

    bool DbftConsensus::VerifyBlock(const std::shared_ptr<ledger::Block>& block)
    {
        // TODO: Implement block verification
        return true;
    }

    void DbftConsensus::Reset()
    {
        LOG_DEBUG("Resetting consensus state");

        state_->Reset();
        state_->SetBlockIndex(state_->GetBlockIndex() + 1);
        state_->SetViewNumber(0);
        view_started_ = std::chrono::steady_clock::now();
        StartNewRound();
    }

    void DbftConsensus::OnTimeout()
    {
        LOG_WARNING("Consensus timeout in phase {}", static_cast<int>(state_->GetPhase()));

        if (state_->GetPhase() != ConsensusPhase::ViewChanging)
        {
            RequestViewChange();
        }
    }

    uint64_t DbftConsensus::GenerateNonce()
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        return dis(gen);
    }
}