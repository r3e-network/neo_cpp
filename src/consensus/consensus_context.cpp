#include <neo/consensus/consensus_context.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/cryptography/hash.h>
#include <neo/vm/script_builder.h>
#include <chrono>
#include <algorithm>

namespace neo::consensus
{
    ConsensusContext::ConsensusContext(std::shared_ptr<ledger::NeoSystem> neoSystem,
                                     std::shared_ptr<ProtocolSettings> settings,
                                     std::shared_ptr<sign::ISigner> signer)
        : neoSystem_(neoSystem)
        , settings_(settings)
        , signer_(signer)
    {
        Reset(0);
    }

    void ConsensusContext::Reset(uint8_t viewNumber)
    {
        if (viewNumber == 0)
        {
            // Initialize for new block
            Snapshot = neoSystem_->GetStoreView();
            uint32_t height = smartcontract::native::LedgerContract::GetCurrentIndex(Snapshot);
            
            Block = std::make_shared<ledger::Block>();
            Block->SetPrevHash(smartcontract::native::LedgerContract::GetCurrentHash(Snapshot));
            Block->SetIndex(height + 1);
            
            // Set validators for next block
            Validators = smartcontract::native::NeoToken::GetNextBlockValidators(Snapshot, settings_->GetValidatorsCount());
            
            // Calculate witness size if needed
            if (witnessSize_ == 0 || PreparationPayloads.size() != Validators.size())
            {
                vm::ScriptBuilder sb;
                auto buf = io::ByteVector(64);
                for (int x = 0; x < M(); x++)
                {
                    sb.EmitPush(buf);
                }
                
                cryptography::Witness witness;
                witness.SetInvocationScript(sb.ToArray());
                witness.SetVerificationScript(cryptography::CreateMultiSigRedeemScript(M(), Validators));
                witnessSize_ = witness.GetSize();
            }
            
            // Find my index in validators
            MyIndex = -1;
            for (size_t i = 0; i < Validators.size(); i++)
            {
                if (signer_->ContainsSignable(Validators[i]))
                {
                    MyIndex = static_cast<int>(i);
                    myPublicKey_ = Validators[MyIndex];
                    break;
                }
            }
            
            // Initialize arrays
            ChangeViewPayloads.resize(Validators.size());
            LastChangeViewPayloads.resize(Validators.size());
            CommitPayloads.resize(Validators.size());
            
            // Initialize last seen messages
            if (LastSeenMessage.empty())
            {
                for (const auto& validator : Validators)
                {
                    LastSeenMessage[validator] = height;
                }
            }
            
            cachedMessages_.clear();
        }
        else
        {
            // View change - keep some state
            for (size_t i = 0; i < LastChangeViewPayloads.size(); i++)
            {
                if (ChangeViewPayloads[i])
                {
                    auto msg = ConsensusPayloadHelper::GetMessage(*ChangeViewPayloads[i]);
                    if (msg && msg->GetViewNumber() >= viewNumber)
                    {
                        LastChangeViewPayloads[i] = ChangeViewPayloads[i];
                    }
                    else
                    {
                        LastChangeViewPayloads[i] = nullptr;
                    }
                }
            }
        }
        
        ViewNumber = viewNumber;
        Block->SetPrimaryIndex(GetPrimaryIndex(viewNumber));
        Block->SetMerkleRoot(io::UInt256::Zero());
        Block->SetTimestamp(0);
        Block->SetNonce(0);
        Block->SetTransactions(nullptr);
        TransactionHashes.clear();
        PreparationPayloads.resize(Validators.size());
        
        if (MyIndex >= 0)
        {
            LastSeenMessage[Validators[MyIndex]] = Block->GetIndex();
        }
    }

    uint8_t ConsensusContext::GetPrimaryIndex(uint8_t viewNumber) const
    {
        if (Validators.empty())
            return 0;
        
        return static_cast<uint8_t>((Block->GetIndex() - viewNumber) % Validators.size());
    }

    bool ConsensusContext::RequestSentOrReceived() const
    {
        return PreparationPayloads[Block->GetPrimaryIndex()] != nullptr;
    }

    bool ConsensusContext::ResponseSent() const
    {
        return !WatchOnly() && PreparationPayloads[MyIndex] != nullptr;
    }

    bool ConsensusContext::CommitSent() const
    {
        return !WatchOnly() && CommitPayloads[MyIndex] != nullptr;
    }

    bool ConsensusContext::BlockSent() const
    {
        return Block->GetTransactions() != nullptr;
    }

    bool ConsensusContext::ViewChanging() const
    {
        if (WatchOnly() || !ChangeViewPayloads[MyIndex])
            return false;
        
        auto msg = ConsensusPayloadHelper::GetMessage(*ChangeViewPayloads[MyIndex]);
        if (!msg)
            return false;
        
        auto changeView = std::dynamic_pointer_cast<ChangeViewMessage>(msg);
        return changeView && changeView->GetNewViewNumber() > ViewNumber;
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeSignedPayload(
        std::shared_ptr<ConsensusMessage> message)
    {
        message->SetBlockIndex(Block->GetIndex());
        message->SetValidatorIndex(static_cast<uint8_t>(MyIndex));
        message->SetViewNumber(ViewNumber);
        
        auto payload = ConsensusPayloadHelper::CreatePayload(
            message,
            myPublicKey_.GetScriptHash(),
            Block->GetIndex(),
            Block->GetIndex() + 1000 // Valid for 1000 blocks
        );
        
        SignPayload(payload);
        return payload;
    }

    void ConsensusContext::SignPayload(std::shared_ptr<network::p2p::payloads::ExtensiblePayload> payload)
    {
        try
        {
            auto witness = signer_->SignExtensiblePayload(payload, Snapshot, settings_->GetNetwork());
            payload->SetWitness(witness);
        }
        catch (const std::exception& e)
        {
            // Log error
        }
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeChangeView(uint8_t reason)
    {
        auto changeView = std::make_shared<ChangeViewMessage>();
        changeView->SetReason(reason);
        changeView->SetTimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        ChangeViewPayloads[MyIndex] = MakeSignedPayload(changeView);
        return ChangeViewPayloads[MyIndex];
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakePrepareRequest()
    {
        // Gather transactions from mempool
        auto maxTxPerBlock = settings_->GetMaxTransactionsPerBlock();
        auto txs = neoSystem_->GetMemoryPool()->GetSortedVerifiedTransactions(maxTxPerBlock);
        EnsureMaxBlockLimitation(txs);
        
        // Set block timestamp
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        // TODO: Get previous header timestamp
        Block->SetTimestamp(now);
        Block->SetNonce(0); // TODO: Generate proper nonce
        
        auto request = std::make_shared<PrepareRequest>();
        request->SetVersion(Block->GetVersion());
        request->SetPrevHash(Block->GetPrevHash());
        request->SetTimestamp(Block->GetTimestamp());
        request->SetNonce(Block->GetNonce());
        request->SetTransactionHashes(TransactionHashes);
        
        PreparationPayloads[MyIndex] = MakeSignedPayload(request);
        return PreparationPayloads[MyIndex];
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakePrepareResponse()
    {
        auto response = std::make_shared<PrepareResponse>();
        response->SetPreparationHash(PreparationPayloads[Block->GetPrimaryIndex()]->GetHash());
        
        PreparationPayloads[MyIndex] = MakeSignedPayload(response);
        return PreparationPayloads[MyIndex];
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeCommit()
    {
        if (CommitPayloads[MyIndex])
            return CommitPayloads[MyIndex];
        
        auto block = EnsureHeader();
        auto commit = std::make_shared<CommitMessage>();
        commit->SetSignature(signer_->SignBlock(block, myPublicKey_, settings_->GetNetwork()));
        
        CommitPayloads[MyIndex] = MakeSignedPayload(commit);
        return CommitPayloads[MyIndex];
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeRecoveryRequest()
    {
        auto request = std::make_shared<RecoveryRequest>();
        request->SetTimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        return MakeSignedPayload(request);
    }

    std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeRecoveryMessage()
    {
        auto recovery = std::make_shared<RecoveryMessage>();
        
        // TODO: Implement recovery message preparation
        // This requires collecting change view messages, prepare messages, and commit messages
        
        return MakeSignedPayload(recovery);
    }

    void ConsensusContext::EnsureMaxBlockLimitation(const std::vector<std::shared_ptr<ledger::Transaction>>& txs)
    {
        TransactionHashes.clear();
        Transactions.clear();
        VerificationContext = ledger::TransactionVerificationContext();
        
        // Expected block size
        auto blockSize = GetExpectedBlockSizeWithoutTransactions(txs.size());
        int64_t blockSystemFee = 0;
        
        for (const auto& tx : txs)
        {
            // Check size limit
            blockSize += tx->GetSize();
            if (blockSize > settings_->GetMaxBlockSize())
                break;
            
            // Check system fee limit
            blockSystemFee += tx->GetSystemFee();
            if (blockSystemFee > settings_->GetMaxBlockSystemFee())
                break;
            
            TransactionHashes.push_back(tx->GetHash());
            Transactions[tx->GetHash()] = tx;
            VerificationContext.AddTransaction(tx);
        }
    }

    size_t ConsensusContext::GetExpectedBlockSizeWithoutTransactions(size_t txCount) const
    {
        // Base block size + witness size + transaction count encoding
        return ledger::Block::HeaderSize + witnessSize_ + io::GetVarSize(txCount);
    }

    std::shared_ptr<ledger::Block> ConsensusContext::EnsureHeader()
    {
        if (TransactionHashes.empty())
            return Block;
        
        // Calculate merkle root
        // TODO: Implement merkle root calculation
        // Block->SetMerkleRoot(cryptography::MerkleTree::ComputeRoot(TransactionHashes));
        
        return Block;
    }

    void ConsensusContext::Save()
    {
        // TODO: Implement consensus state persistence
    }

    void ConsensusContext::Serialize(io::BinaryWriter& writer) const
    {
        // TODO: Implement serialization
    }

    void ConsensusContext::Deserialize(io::BinaryReader& reader)
    {
        // TODO: Implement deserialization
    }
}