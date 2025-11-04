#include <neo/consensus/consensus_service.h>

#include <neo/consensus/consensus_payload_helper.h>
#include <neo/consensus/dbft_consensus.h>
#include <neo/core/configuration_manager.h>
#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/core/protocol_settings.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/ledger/block.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/event_system.h>
#include <neo/ledger/memory_pool.h>
#include <neo/ledger/transaction_validator.h>
#include <neo/ledger/verify_result.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p_server.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/vm/script_builder.h>
#include <neo/io/memory_stream.h>
#include <neo/io/binary_reader.h>

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace neo::consensus
{
namespace
{
auto& Logger() { return *core::Logger::GetInstance(); }
}  // namespace

ConsensusService::ConsensusService(std::shared_ptr<core::ProtocolSettings> protocolSettings,
                                   std::shared_ptr<ledger::Blockchain> blockchain,
                                   std::shared_ptr<ledger::MemoryPool> memoryPool,
                                   std::shared_ptr<network::p2p::P2PServer> p2pServer)
    : protocolSettings_(std::move(protocolSettings)),
      blockchain_(std::move(blockchain)),
      memoryPool_(std::move(memoryPool)),
      p2pServer_(std::move(p2pServer))
{
    if (!protocolSettings_) throw std::invalid_argument("ConsensusService requires protocol settings");
    if (!blockchain_) throw std::invalid_argument("ConsensusService requires blockchain instance");
    if (!memoryPool_) throw std::invalid_argument("ConsensusService requires memory pool");
}

void ConsensusService::SetKeyPair(std::unique_ptr<cryptography::ecc::KeyPair> keyPair)
{
    if (!keyPair) return;

    std::lock_guard<std::mutex> lock(mutex_);
    keyPair_ = std::move(keyPair);
    nodeScriptHash_ = keyPair_->GetScriptHash();

    if (consensus_)
    {
        consensus_->SetSignatureProvider([this](const io::ByteSpan& data) { return SignConsensusData(data); });
    }

    Logger().Info("Consensus key pair configured with script hash {}", nodeScriptHash_.ToString());
}

void ConsensusService::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;

    EnsureConsensusInitialised();

    if (!consensus_)
    {
        Logger().Error("ConsensusService failed to initialise DbftConsensus engine");
        return;
    }

    EnsureTransactionSubscription();

    running_ = true;
    consensus_->Start();

    // Register with LocalNode so inbound payloads are forwarded here.
    network::p2p::LocalNode::GetInstance().SetConsensusService(shared_from_this());

    Logger().Info("Consensus service started with {} validators", validators_.size());
}

void ConsensusService::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) return;

    running_ = false;
    if (consensus_) consensus_->Stop();

    Logger().Info("Consensus service stopped");
}

bool ConsensusService::IsRunning() const { return running_.load(); }

ConsensusService::Status ConsensusService::GetStatus() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    Status status;
    if (test_status_override_)
    {
        status = *test_status_override_;
    }
    else
    {
        status.running = running_ && consensus_ && consensus_->IsRunning();
        status.validators = validators_;

        if (consensus_)
        {
            const auto& state = consensus_->GetState();
            status.blockIndex = state.GetBlockIndex();
            status.viewNumber = state.GetViewNumber();
            status.phase = state.GetPhase();
            status.prepareResponseCount = state.GetPrepareResponseCount();
            status.commitCount = state.GetCommitCount();
            status.viewChangeCount = state.GetViewChangeCount();
            status.primaryIndex = consensus_->GetPrimaryIndex();

            status.validatorStates.assign(status.validators.size(), {});

            const auto requestHash = state.GetPrepareRequestHash();
            if (!requestHash.IsZero())
            {
                status.prepareRequestHash = requestHash;
            }

            status.expectedTransactionCount = state.GetTransactionHashes().size();
            status.transactionCount = state.GetTransactions().size();

            const auto timestamp = state.GetTimestamp();
            if (timestamp.time_since_epoch().count() > 0)
            {
                status.timestampMilliseconds = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
            }

            const auto nonce = state.GetNonce();
            if (nonce != 0)
            {
                status.nonce = nonce;
            }

            for (size_t i = 0; i < status.validatorStates.size(); ++i)
            {
                const uint32_t index = static_cast<uint32_t>(i);
                auto& validatorStatus = status.validatorStates[i];
                validatorStatus.hasPrepareResponse = state.HasPrepareResponse(index);
                validatorStatus.hasCommit = state.HasCommit(index);
                if (const auto reason = state.GetViewChangeReason(index))
                {
                    validatorStatus.viewChangeReason = reason;
                }
                if (const auto requested = state.GetViewChangeView(index))
                {
                    validatorStatus.requestedView = requested;
                }
            }

            if (status.prepareRequestHash && status.primaryIndex < status.validatorStates.size())
            {
                status.validatorStates[status.primaryIndex].hasProposal = true;
            }
        }
    }

    if (status.validators.empty())
    {
        status.validators = validators_;
    }

    if (status.validatorStates.size() < status.validators.size())
    {
        status.validatorStates.resize(status.validators.size());
    }

    if (test_primary_index_override_)
    {
        status.primaryIndex = *test_primary_index_override_;
    }

    if (test_validator_index_override_)
    {
        status.validatorIndex = *test_validator_index_override_;
    }
    else
    {
        auto it = std::find(validatorHashes_.begin(), validatorHashes_.end(), nodeScriptHash_);
        status.validatorIndex = it == validatorHashes_.end()
                                   ? std::numeric_limits<uint16_t>::max()
                                   : static_cast<uint16_t>(std::distance(validatorHashes_.begin(), it));
    }

    return status;
}

uint32_t ConsensusService::GetPrimaryIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (test_primary_index_override_) return *test_primary_index_override_;
    if (!consensus_) return 0;
    return consensus_->GetPrimaryIndex();
}

uint16_t ConsensusService::GetValidatorIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (test_validator_index_override_) return *test_validator_index_override_;
    auto it = std::find(validatorHashes_.begin(), validatorHashes_.end(), nodeScriptHash_);
    if (it == validatorHashes_.end()) return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(std::distance(validatorHashes_.begin(), it));
}

uint32_t ConsensusService::GetBlockIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!consensus_) return 0;
    return consensus_->GetState().GetBlockIndex();
}

uint32_t ConsensusService::GetViewNumber() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!consensus_) return 0;
    return consensus_->GetState().GetViewNumber();
}

const std::vector<cryptography::ecc::ECPoint>& ConsensusService::GetValidators() const { return validators_; }

const std::vector<io::UInt160>& ConsensusService::GetValidatorHashes() const { return validatorHashes_; }

void ConsensusService::SetStatusForTesting(const Status& status, const std::vector<io::UInt160>& validatorHashes,
                                           std::optional<uint32_t> primaryIndex,
                                           std::optional<uint16_t> validatorIndex)
{
    std::lock_guard<std::mutex> lock(mutex_);
    test_status_override_ = status;
    if (test_status_override_->validatorStates.size() < test_status_override_->validators.size())
    {
        test_status_override_->validatorStates.resize(test_status_override_->validators.size());
    }
    validators_ = status.validators;
    validatorHashes_ = validatorHashes;
    test_primary_index_override_ = primaryIndex;
    test_validator_index_override_ = validatorIndex;
}

void ConsensusService::ClearStatusOverrideForTesting()
{
    std::lock_guard<std::mutex> lock(mutex_);
    test_status_override_.reset();
    test_primary_index_override_.reset();
    test_validator_index_override_.reset();
}

void ConsensusService::HandlePayload(const network::p2p::payloads::ExtensiblePayload& payload)
{
    std::shared_ptr<DbftConsensus> consensus;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consensus = consensus_;
    }

    if (!consensus)
    {
        Logger().Debug("ConsensusService received payload before engine initialised");
        return;
    }

    auto message = ConsensusPayloadHelper::GetMessage(payload);
    if (!message)
    {
        Logger().Debug("Received non-consensus extensible payload; ignoring");
        return;
    }

    if (!consensus->ProcessMessage(*message))
    {
        Logger().Debug("Consensus message dropped by engine (type {})", static_cast<int>(message->GetType()));
    }
}

void ConsensusService::EnsureConsensusInitialised()
{
    if (consensus_) return;

    auto activeValidators = LoadActiveValidators();
    if (activeValidators.empty())
    {
        activeValidators = protocolSettings_->GetStandbyValidators();
    }
    validators_ = std::move(activeValidators);
    validatorHashes_ = BuildValidatorHashes(validators_);

    if (keyPair_)
    {
        nodeScriptHash_ = keyPair_->GetScriptHash();
    }
    else
    {
        if (!missing_key_warning_emitted_.exchange(true))
        {
            Logger().Warning("Consensus key pair not configured; operating in watcher mode");
        }
        nodeScriptHash_ = io::UInt160();
    }

    consensus::ConsensusConfig config;
    const auto& consensusConfig = core::ConfigurationManager::GetInstance().GetConsensusConfig();

    const consensus::ConsensusConfig defaults;

    auto block_time_ms = consensusConfig.block_time_ms
                               ? consensusConfig.block_time_ms
                               : static_cast<uint32_t>(protocolSettings_->GetMillisecondsPerBlock().count());
    auto view_timeout_ms = consensusConfig.view_timeout_ms
                               ? consensusConfig.view_timeout_ms
                               : static_cast<uint32_t>(defaults.view_timeout.count());

    config.block_time = std::chrono::milliseconds(block_time_ms);
    config.view_timeout = std::chrono::milliseconds(view_timeout_ms);
    config.max_transactions_per_block =
        consensusConfig.max_transactions_per_block ? consensusConfig.max_transactions_per_block
                                                   : protocolSettings_->GetMaxTransactionsPerBlock();
    config.max_block_size = consensusConfig.max_block_size ? consensusConfig.max_block_size
                                                          : protocolSettings_->GetMaxBlockSize();
    config.max_block_system_fee = consensusConfig.max_block_system_fee ? consensusConfig.max_block_system_fee
                                                                       : protocolSettings_->GetMaxBlockSystemFee();

    consensus_ = std::make_shared<DbftConsensus>(config, nodeScriptHash_, validatorHashes_, memoryPool_, blockchain_);

    consensus_->SetValidatorPublicKeys(validators_);
    consensus_->SetSignatureProvider([this](const io::ByteSpan& data) { return SignConsensusData(data); });

    consensus_->SetTransactionVerifier(
        [this](const network::p2p::payloads::Neo3Transaction& tx) { return VerifyTransaction(tx); });

    consensus_->SetBlockPersister(
        [this](const std::shared_ptr<ledger::Block>& block) { return PersistBlock(block); });

    consensus_->SetMessageBroadcaster([this](const ConsensusMessage& message) { BroadcastMessage(message); });
}

void ConsensusService::EnsureTransactionSubscription()
{
    auto weak_self = std::weak_ptr<ConsensusService>(shared_from_this());

    if (!transaction_subscription_registered_)
    {
        transaction_added_handler_ = [weak_self](std::shared_ptr<ledger::Transaction> transaction)
        {
            if (!transaction)
            {
                return;
            }

            if (auto self = weak_self.lock())
            {
                self->OnTransactionAdded(std::move(transaction));
            }
        };

        ledger::MemoryPoolEvents::SubscribeTransactionAdded(transaction_added_handler_);
        transaction_subscription_registered_ = true;
    }

    if (!transaction_removed_subscription_registered_)
    {
        transaction_removed_handler_ = [weak_self](const ledger::TransactionRemovedEventArgs& args)
        {
            if (auto self = weak_self.lock())
            {
                self->OnTransactionRemoved(args);
            }
        };

        ledger::MemoryPoolEvents::SubscribeTransactionRemoved(transaction_removed_handler_);
        transaction_removed_subscription_registered_ = true;
    }
}

std::vector<io::UInt160> ConsensusService::BuildValidatorHashes(
    const std::vector<cryptography::ecc::ECPoint>& validators) const
{
    std::vector<io::UInt160> hashes;
    hashes.reserve(validators.size());

    for (const auto& validator : validators)
    {
        auto contract = smartcontract::Contract::CreateSignatureContract(validator);
        hashes.emplace_back(contract.GetScriptHash());
    }

    return hashes;
}

std::vector<cryptography::ecc::ECPoint> ConsensusService::LoadActiveValidators() const
{
    if (!blockchain_) return {};

    try
    {
        auto system = blockchain_->GetSystem();
        if (!system) return {};

        auto neoToken = smartcontract::native::NeoToken::GetInstance();
        if (!neoToken) return {};

        auto snapshot = system->GetSnapshot();
        if (!snapshot) return {};

        auto validators = neoToken->GetValidators(snapshot);
        if (validators.empty()) return {};

        auto required = protocolSettings_->GetValidatorCount();
        if (required > 0 && validators.size() > static_cast<size_t>(required))
        {
            validators.resize(static_cast<size_t>(required));
        }

        return validators;
    }
    catch (const std::exception& ex)
    {
        Logger().Warning("Failed to load active validators from blockchain snapshot: {}", ex.what());
        return {};
    }
}

bool ConsensusService::VerifyTransaction(const network::p2p::payloads::Neo3Transaction& tx)
{
    if (!blockchain_)
    {
        Logger().Warning("ConsensusService::VerifyTransaction called without blockchain instance");
        return false;
    }

    auto result = ledger::ValidateTransaction(tx, blockchain_, memoryPool_);
    if (result != ledger::ValidationResult::Valid)
    {
        auto ToString = [](ledger::ValidationResult value) -> const char* {
            switch (value)
            {
                case ledger::ValidationResult::Valid:
                    return "Valid";
                case ledger::ValidationResult::InvalidFormat:
                    return "InvalidFormat";
                case ledger::ValidationResult::InvalidSize:
                    return "InvalidSize";
                case ledger::ValidationResult::InvalidAttribute:
                    return "InvalidAttribute";
                case ledger::ValidationResult::InvalidScript:
                    return "InvalidScript";
                case ledger::ValidationResult::InvalidWitness:
                    return "InvalidWitness";
                case ledger::ValidationResult::InsufficientFunds:
                    return "InsufficientFunds";
                case ledger::ValidationResult::InvalidSignature:
                    return "InvalidSignature";
                case ledger::ValidationResult::AlreadyExists:
                    return "AlreadyExists";
                case ledger::ValidationResult::Expired:
                    return "Expired";
                case ledger::ValidationResult::InvalidSystemFee:
                    return "InvalidSystemFee";
                case ledger::ValidationResult::InvalidNetworkFee:
                    return "InvalidNetworkFee";
                case ledger::ValidationResult::PolicyViolation:
                    return "PolicyViolation";
                case ledger::ValidationResult::Unknown:
                default:
                    return "Unknown";
            }
        };

        Logger().Debug("Consensus rejected transaction {} (reason: {})", tx.GetHash().ToString(), ToString(result));
        return false;
    }

    return true;
}

bool ConsensusService::PersistBlock(const std::shared_ptr<ledger::Block>& block)
{
    if (!block || !blockchain_) return false;

    try
    {
        auto result = blockchain_->OnNewBlock(block);
        if (result != ledger::VerifyResult::Succeed)
        {
            Logger().Warning("Consensus block rejected by blockchain (result {})", static_cast<int>(result));
            return false;
        }

        if (memoryPool_)
        {
            for (const auto& tx : block->GetTransactions())
            {
                memoryPool_->Remove(tx.GetHash());
            }
        }

        Logger().Info("Block {} persisted via consensus", block->GetIndex());
        return true;
    }
    catch (const std::exception& ex)
    {
        Logger().Error("Exception while persisting consensus block: {}", ex.what());
        return false;
    }
}

void ConsensusService::BroadcastMessage(const ConsensusMessage& message)
{
    std::shared_ptr<DbftConsensus> consensus;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consensus = consensus_;
    }

    auto copy_unique = ConsensusMessage::CreateFromType(message.GetType());
    if (!copy_unique)
    {
        Logger().Warning("Unable to create consensus message copy for broadcast");
        return;
    }

    auto copy = std::shared_ptr<ConsensusMessage>(std::move(copy_unique));

    io::MemoryStream stream;
    io::BinaryWriter writer(stream);
    message.Serialize(writer);

    auto data = stream.ToByteVector();
    io::BinaryReader reader(data);
    copy->Deserialize(reader);

    auto payload = ConsensusPayloadHelper::CreatePayload(copy, nodeScriptHash_, message.GetBlockIndex(),
                                                         message.GetBlockIndex() + 1);
    if (!payload)
    {
        Logger().Warning("Failed to wrap consensus message for broadcast");
        return;
    }

    if (keyPair_)
    {
        auto unsigned_data = payload->GetUnsignedData();
        auto signature = SignConsensusData(unsigned_data.AsSpan());

        if (!signature.IsEmpty())
        {
            vm::ScriptBuilder builder;
            builder.EmitPush(signature.AsSpan());

            ledger::Witness witness;
            witness.SetInvocationScript(builder.ToArray());
            auto verification = cryptography::Crypto::CreateSignatureRedeemScript(keyPair_->GetPublicKey());
            witness.SetVerificationScript(verification);
            ConsensusPayloadHelper::SignPayload(*payload, witness);
            copy->SetInvocationScript(witness.GetInvocationScript());
            if (consensus)
            {
                consensus->RecordSentPayload(*copy, witness);
            }
        }
        else
        {
            Logger().Warning("Consensus payload signing returned an empty signature; broadcast aborted");
            return;
        }
    }
    else
    {
        Logger().Warning("Consensus key pair is not configured; broadcasting unsigned payload");
        if (consensus)
        {
            ledger::Witness dummy;
            consensus->RecordSentPayload(*copy, dummy);
        }
    }

    network::p2p::LocalNode::GetInstance().RelayExtensiblePayload(std::move(payload));
}

uint16_t ConsensusService::ResolveValidatorIndex() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (test_validator_index_override_) return *test_validator_index_override_;
    auto it = std::find(validatorHashes_.begin(), validatorHashes_.end(), nodeScriptHash_);
    if (it == validatorHashes_.end()) return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(std::distance(validatorHashes_.begin(), it));
}

void ConsensusService::OnTransactionAdded(std::shared_ptr<ledger::Transaction> transaction)
{
    if (!transaction)
    {
        return;
    }

    if (!running_.load())
    {
        return;
    }

    std::shared_ptr<DbftConsensus> consensus;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consensus = consensus_;
    }

    if (!consensus)
    {
        return;
    }

    consensus->AddTransaction(*transaction);
}

void ConsensusService::OnTransactionRemoved(const ledger::TransactionRemovedEventArgs& args)
{
    if (!running_.load())
    {
        return;
    }

    auto tx = args.transaction;
    if (!tx)
    {
        return;
    }

    std::shared_ptr<DbftConsensus> consensus;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consensus = consensus_;
    }

    if (!consensus)
    {
        return;
    }

    consensus->RemoveCachedTransaction(tx->GetHash());
}

io::ByteVector ConsensusService::SignConsensusData(const io::ByteSpan& data) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!keyPair_)
    {
        return {};
    }

    io::ByteVector buffer(data);
    return keyPair_->Sign(buffer);
}
}  // namespace neo::consensus
