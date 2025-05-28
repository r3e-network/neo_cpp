#include <neo/consensus/consensus_service.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/message.h>
#include <neo/network/p2p/message_command.h>
#include <chrono>
#include <algorithm>
#include <sstream>

namespace neo::consensus
{
    std::shared_ptr<ledger::Block> ConsensusService::CreateBlock()
    {
        // Check if prepare request is received
        if (!prepareRequest_)
            throw std::runtime_error("Prepare request not received");

        // Validate transactions
        std::vector<std::shared_ptr<ledger::Transaction>> validTransactions;
        for (const auto& tx : transactions_)
        {
            // Skip invalid transactions
            if (!node_->GetBlockchain()->VerifyTransaction(tx))
                continue;

            // Add valid transaction
            validTransactions.push_back(tx);
        }

        // Sort transactions by fee and hash
        std::sort(validTransactions.begin(), validTransactions.end(), [](const auto& a, const auto& b) {
            // Sort by fee (descending)
            int64_t feeA = a->GetSystemFee() + a->GetNetworkFee();
            int64_t feeB = b->GetSystemFee() + b->GetNetworkFee();

            if (feeA != feeB)
                return feeA > feeB;

            // Sort by hash (ascending)
            return a->GetHash() < b->GetHash();
        });

        // Create block
        auto block = std::make_shared<ledger::Block>();
        block->SetVersion(0);
        block->SetPrevHash(node_->GetBlockchain()->GetCurrentBlockHash());
        block->SetTimestamp(prepareRequest_->GetTimestamp());
        block->SetIndex(blockIndex_);
        block->SetNextConsensus(prepareRequest_->GetNextConsensus());
        block->SetTransactions(validTransactions);

        // Calculate merkle root
        std::vector<io::UInt256> hashes;
        for (const auto& tx : validTransactions)
        {
            hashes.push_back(tx->GetHash());
        }
        block->SetMerkleRoot(cryptography::Hash::ComputeMerkleRoot(hashes));

        // Set primary index
        block->SetPrimaryIndex(GetPrimaryIndex());

        return block;
    }

    void ConsensusService::ProcessBlock(std::shared_ptr<ledger::Block> block)
    {
        try
        {
            // Verify block
            if (!node_->GetBlockchain()->VerifyBlock(block))
                throw std::runtime_error("Block verification failed");

            // Add block to blockchain
            if (!node_->GetBlockchain()->AddBlock(block))
                throw std::runtime_error("Failed to add block to blockchain");

            // Remove transactions from memory pool
            for (const auto& tx : block->GetTransactions())
            {
                node_->GetMemoryPool()->RemoveTransaction(tx->GetHash());
            }

            // Update block index
            blockIndex_ = block->GetIndex() + 1;

            // Update last block time
            lastBlockTime_ = GetCurrentTimestamp();

            // Log block
            std::cout << "Block " << block->GetIndex() << " added to blockchain" << std::endl;

            // Notify block added
            node_->OnBlockAdded(block);
        }
        catch (const std::exception& ex)
        {
            // Log error
            std::cerr << "Failed to process block: " << ex.what() << std::endl;

            // Reset consensus
            Reset();

            // Initialize consensus
            Initialize();
        }
    }

    void ConsensusService::ChangeView(uint8_t viewNumber)
    {
        // Update view number
        viewNumber_ = viewNumber;

        // Reset consensus state
        prepareRequest_ = nullptr;
        prepareResponses_.clear();

        // Move pending messages to current view
        if (pendingPrepareRequests_.find(viewNumber_) != pendingPrepareRequests_.end())
        {
            prepareRequest_ = pendingPrepareRequests_[viewNumber_];
            pendingPrepareRequests_.erase(viewNumber_);
        }

        if (pendingPrepareResponses_.find(viewNumber_) != pendingPrepareResponses_.end())
        {
            prepareResponses_ = pendingPrepareResponses_[viewNumber_];
            pendingPrepareResponses_.erase(viewNumber_);
        }

        // Update last change view time
        lastChangeViewTime_ = GetCurrentTimestamp();

        // Check if node is primary
        if (IsPrimary())
        {
            // Send prepare request
            SendPrepareRequest();
        }
    }

    void ConsensusService::Reset()
    {
        // Reset consensus state
        if (context_)
        {
            context_->Reset();
        }

        pendingChangeViewMessages_.clear();
        pendingPrepareRequests_.clear();
        pendingPrepareResponses_.clear();
        knownHashes_.clear();
        lastChangeViewTime_ = 0;
        lastPrepareRequestTime_ = 0;
    }

    void ConsensusService::InitializeValidators()
    {
        // Get validators
        auto roleManagement = std::static_pointer_cast<smartcontract::native::RoleManagement>(
            smartcontract::native::NativeContractManager::GetInstance().GetContract(smartcontract::native::RoleManagement::NAME));

        auto validators = roleManagement->GetDesignatedByRole(neoSystem_->GetSnapshot(), smartcontract::native::RoleManagement::ROLE_STATE_VALIDATOR);

        // Convert to ECPoint
        std::vector<cryptography::ecc::ECPoint> validatorList;
        for (const auto& validator : validators)
        {
            validatorList.push_back(cryptography::ecc::ECPoint::Parse(validator.AsSpan()));
        }

        // Find validator index
        uint16_t validatorIndex = 0xFFFF;
        for (size_t i = 0; i < validatorList.size(); i++)
        {
            if (validatorList[i] == keyPair_.GetPublicKey())
            {
                validatorIndex = static_cast<uint16_t>(i);
                break;
            }
        }

        // Store validators and validator index
        if (context_)
        {
            context_ = std::make_shared<ConsensusContext>(validatorList, validatorIndex, keyPair_, context_->GetBlockIndex());
        }
    }

    bool ConsensusService::ShouldChangeView() const
    {
        // Check if node is primary
        if (IsPrimary())
        {
            // Check if prepare request is sent
            if (!prepareRequest_)
            {
                // Check if timeout has elapsed
                uint64_t timeout = GetTimeout(viewNumber_);
                uint64_t elapsed = GetCurrentTimestamp() - lastBlockTime_;

                // Add jitter to prevent network congestion
                uint64_t jitter = (validatorIndex_ * 1000) % 5000;

                return elapsed > timeout + jitter;
            }
        }
        else
        {
            // Check if prepare request is received
            if (!prepareRequest_)
            {
                // Check if timeout has elapsed
                uint64_t timeout = GetTimeout(viewNumber_);
                uint64_t elapsed = GetCurrentTimestamp() - lastBlockTime_;

                // Add jitter to prevent network congestion
                uint64_t jitter = (validatorIndex_ * 1000) % 5000;

                return elapsed > timeout + jitter;
            }
            else
            {
                // Check if timeout has elapsed since prepare request
                uint64_t timeout = GetTimeout(viewNumber_) / 2; // Shorter timeout for prepare response
                uint64_t elapsed = GetCurrentTimestamp() - lastPrepareRequestTime_;

                // Add jitter to prevent network congestion
                uint64_t jitter = (validatorIndex_ * 500) % 2000;

                return elapsed > timeout + jitter;
            }
        }

        return false;
    }

    uint64_t ConsensusService::GetCurrentTimestamp() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    uint64_t ConsensusService::GetTimeout(uint8_t viewNumber) const
    {
        // Base timeout (15 seconds)
        uint64_t baseTimeout = 15000;

        // Use exponential backoff for higher view numbers
        if (viewNumber == 0)
            return baseTimeout;

        // Calculate timeout with exponential backoff (capped at 5 minutes)
        uint64_t timeout = baseTimeout * (1 << std::min(viewNumber, static_cast<uint8_t>(8)));
        return std::min(timeout, static_cast<uint64_t>(300000)); // Cap at 5 minutes
    }
}
