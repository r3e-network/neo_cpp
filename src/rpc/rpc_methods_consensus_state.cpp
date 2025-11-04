#include <neo/rpc/rpc_methods.h>

#include <neo/consensus/consensus_message.h>
#include <neo/consensus/consensus_service.h>
#include <neo/network/p2p/local_node.h>
#include <neo/node/neo_system.h>
#include <neo/rpc/error_codes.h>

#include <limits>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace neo::rpc
{
namespace
{
std::mutex g_consensus_override_mutex;
std::optional<consensus::ConsensusService::Status> g_consensus_status_override;
std::vector<io::UInt160> g_consensus_validator_hash_override;

std::string ConsensusPhaseToString(consensus::ConsensusPhase phase)
{
    switch (phase)
    {
        case consensus::ConsensusPhase::Initial:
            return "Initial";
        case consensus::ConsensusPhase::Primary:
            return "Primary";
        case consensus::ConsensusPhase::Backup:
            return "Backup";
        case consensus::ConsensusPhase::RequestSent:
            return "RequestSent";
        case consensus::ConsensusPhase::RequestReceived:
            return "RequestReceived";
        case consensus::ConsensusPhase::SignatureSent:
            return "SignatureSent";
        case consensus::ConsensusPhase::BlockSent:
            return "BlockSent";
        case consensus::ConsensusPhase::ViewChanging:
            return "ViewChanging";
        default:
            return "Unknown";
    }
}

std::string ChangeViewReasonToString(consensus::ChangeViewReason reason)
{
    switch (reason)
    {
        case consensus::ChangeViewReason::Timeout:
            return "Timeout";
        case consensus::ChangeViewReason::ChangeAgreement:
            return "ChangeAgreement";
        case consensus::ChangeViewReason::TxNotFound:
            return "TxNotFound";
        case consensus::ChangeViewReason::TxRejectedByPolicy:
            return "TxRejectedByPolicy";
        case consensus::ChangeViewReason::TxInvalid:
            return "TxInvalid";
        case consensus::ChangeViewReason::BlockRejectedByPolicy:
            return "BlockRejectedByPolicy";
        default:
            return "Unknown";
    }
}
}  // namespace

nlohmann::json RPCMethods::GetConsensusState(std::shared_ptr<node::NeoSystem> neoSystem,
                                             const nlohmann::json& params)
{
    (void)params;

    nlohmann::json result;
    std::optional<consensus::ConsensusService::Status> overrideStatus;
    std::vector<io::UInt160> overrideValidatorHashes;
    {
        std::lock_guard<std::mutex> lock(g_consensus_override_mutex);
        overrideStatus = g_consensus_status_override;
        if (g_consensus_status_override)
        {
            overrideValidatorHashes = g_consensus_validator_hash_override;
        }
    }

    consensus::ConsensusService::Status status;
    std::vector<io::UInt160> validatorHashes;

    if (overrideStatus)
    {
        status = *overrideStatus;
        validatorHashes = std::move(overrideValidatorHashes);
    }
    else
    {
        auto& localNode = network::p2p::LocalNode::GetInstance();
        auto consensus = localNode.GetConsensusService();
        if (!consensus)
        {
            result["running"] = false;
            result["error"] = "Consensus service unavailable";
            return result;
        }

        status = consensus->GetStatus();
        const auto& hashesRef = consensus->GetValidatorHashes();
        validatorHashes.assign(hashesRef.begin(), hashesRef.end());
    }

    result["running"] = status.running;
    result["blockindex"] = status.blockIndex;
    result["viewnumber"] = status.viewNumber;
    result["phase"] = ConsensusPhaseToString(status.phase);
    result["prepareresponses"] = status.prepareResponseCount;
    result["commits"] = status.commitCount;
    result["viewchanges"] = status.viewChangeCount;
    result["expectedtransactions"] = status.expectedTransactionCount;
    result["transactioncount"] = status.transactionCount;

    if (status.prepareRequestHash)
    {
        result["proposalhash"] = status.prepareRequestHash->ToString();
    }
    else
    {
        result["proposalhash"] = nullptr;
    }

    if (status.timestampMilliseconds)
    {
        result["timestamp"] = *status.timestampMilliseconds;
    }
    else
    {
        result["timestamp"] = nullptr;
    }

    if (status.nonce)
    {
        result["nonce"] = *status.nonce;
    }
    else
    {
        result["nonce"] = nullptr;
    }

    if (!status.validators.empty())
    {
        result["primaryindex"] = status.primaryIndex;
    }
    else
    {
        result["primaryindex"] = nullptr;
    }

    if (status.validatorIndex == std::numeric_limits<uint16_t>::max())
    {
        result["validatorindex"] = nullptr;
    }
    else
    {
        result["validatorindex"] = status.validatorIndex;
    }

    nlohmann::json validators = nlohmann::json::array();
    for (size_t i = 0; i < status.validators.size(); ++i)
    {
        const bool hasState = i < status.validatorStates.size();
        nlohmann::json entry;
        entry["index"] = i;
        entry["publickey"] = status.validators[i].ToString();
        if (i < validatorHashes.size())
        {
            entry["scripthash"] = validatorHashes[i].ToString();
        }
        else
        {
            entry["scripthash"] = nullptr;
        }

        entry["isprimary"] = status.validators.size() > i && status.running && status.primaryIndex == i;
        entry["isme"] = status.validatorIndex != std::numeric_limits<uint16_t>::max() && status.validatorIndex == i;
        entry["hasproposal"] = hasState && status.validatorStates[i].hasProposal;
        entry["hasprepareresponse"] = hasState && status.validatorStates[i].hasPrepareResponse;
        entry["hascommit"] = hasState && status.validatorStates[i].hasCommit;
        if (hasState && status.validatorStates[i].viewChangeReason.has_value())
        {
            entry["viewchangereason"] = ChangeViewReasonToString(*status.validatorStates[i].viewChangeReason);
        }
        else
        {
            entry["viewchangereason"] = nullptr;
        }
        if (hasState && status.validatorStates[i].requestedView.has_value())
        {
            entry["requestedview"] = *status.validatorStates[i].requestedView;
        }
        else
        {
            entry["requestedview"] = nullptr;
        }
        validators.push_back(std::move(entry));
    }
    result["validatorcount"] = status.validators.size();
    result["validators"] = std::move(validators);

    return result;
}

nlohmann::json RPCMethods::StartConsensus(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.is_array() && !params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "startconsensus does not accept parameters");
    }

    auto& localNode = network::p2p::LocalNode::GetInstance();
    auto consensus = localNode.GetConsensusService();
    if (!consensus)
    {
        throw RpcException(ErrorCode::ConsensusError, "Consensus service unavailable");
    }

    if (!localNode.IsRunning())
    {
        throw RpcException(ErrorCode::ConsensusError, "Local node is not running");
    }

    const bool started = consensus->StartManually();
    return started && consensus->IsRunning();
}

nlohmann::json RPCMethods::StopConsensus(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.is_array() && !params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "stopconsensus does not accept parameters");
    }

    auto& localNode = network::p2p::LocalNode::GetInstance();
    auto consensus = localNode.GetConsensusService();
    if (!consensus)
    {
        throw RpcException(ErrorCode::ConsensusError, "Consensus service unavailable");
    }

    const bool wasRunning = consensus->IsRunning();
    if (wasRunning)
    {
        consensus->Stop();
    }
    return wasRunning;
}

nlohmann::json RPCMethods::RestartConsensus(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    (void)neoSystem;
    if (params.is_array() && !params.empty())
    {
        throw RpcException(ErrorCode::InvalidParams, "restartconsensus does not accept parameters");
    }

    auto& localNode = network::p2p::LocalNode::GetInstance();
    auto consensus = localNode.GetConsensusService();
    if (!consensus)
    {
        throw RpcException(ErrorCode::ConsensusError, "Consensus service unavailable");
    }

    if (!localNode.IsRunning())
    {
        throw RpcException(ErrorCode::ConsensusError, "Local node is not running");
    }

    const bool wasRunning = consensus->IsRunning();
    if (wasRunning)
    {
        consensus->Stop();
    }

    const bool started = consensus->StartManually();
    return started && consensus->IsRunning();
}

void RPCMethods::SetConsensusServiceOverrideForTesting(std::optional<consensus::ConsensusService::Status> status,
                                                       std::vector<io::UInt160> validatorHashes)
{
    std::lock_guard<std::mutex> lock(g_consensus_override_mutex);
    if (status.has_value())
    {
        g_consensus_status_override = std::move(status);
        g_consensus_validator_hash_override = std::move(validatorHashes);
    }
    else
    {
        g_consensus_status_override.reset();
        g_consensus_validator_hash_override.clear();
    }
}
}  // namespace neo::rpc
