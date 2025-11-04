#include <neo/ledger/transaction_validator.h>

#include <neo/core/logging.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/protocol_settings.h>
#include <neo/smartcontract/transaction_verifier.h>

#include <limits>

namespace neo::ledger
{
namespace
{
ValidationResult ValidateBasicFormat(const Transaction& transaction, const ProtocolSettings* settings)
{
    const auto maxTransactionSize =
        settings != nullptr ? settings->MAX_TRANSACTION_SIZE : ProtocolSettings::MAX_TRANSACTION_SIZE;
    const auto maxScriptLength =
        settings != nullptr ? settings->MAX_SCRIPT_LENGTH : ProtocolSettings::MAX_SCRIPT_LENGTH;
    const auto maxWitnesses =
        settings != nullptr ? settings->MAX_WITNESSES_PER_TX : ProtocolSettings::MAX_WITNESSES_PER_TX;
    const auto maxAttributes =
        settings != nullptr ? settings->MAX_TRANSACTION_ATTRIBUTES : ProtocolSettings::MAX_TRANSACTION_ATTRIBUTES;

    const auto size = static_cast<size_t>(transaction.GetSize());
    if (size > maxTransactionSize)
    {
        LOG_WARNING("Transaction {} exceeds maximum size ({} > {})", transaction.GetHash().ToString(), size,
                    maxTransactionSize);
        return ValidationResult::InvalidSize;
    }

    const auto& script = transaction.GetScript();
    if (script.IsEmpty())
    {
        LOG_WARNING("Transaction {} has an empty script", transaction.GetHash().ToString());
        return ValidationResult::InvalidScript;
    }

    if (script.Size() > maxScriptLength)
    {
        LOG_WARNING("Transaction {} script exceeds maximum length ({} > {})", transaction.GetHash().ToString(),
                    script.Size(), maxScriptLength);
        return ValidationResult::InvalidScript;
    }

    if (transaction.GetSystemFee() < 0)
    {
        LOG_WARNING("Transaction {} has negative system fee {}", transaction.GetHash().ToString(),
                    transaction.GetSystemFee());
        return ValidationResult::InvalidSystemFee;
    }

    if (transaction.GetNetworkFee() < 0)
    {
        LOG_WARNING("Transaction {} has negative network fee {}", transaction.GetHash().ToString(),
                    transaction.GetNetworkFee());
        return ValidationResult::InvalidNetworkFee;
    }

    const auto& signers = transaction.GetSigners();
    if (signers.empty())
    {
        LOG_WARNING("Transaction {} is missing signers", transaction.GetHash().ToString());
        return ValidationResult::InvalidWitness;
    }

    if (signers.size() > maxAttributes)
    {
        LOG_WARNING("Transaction {} has too many signers ({})", transaction.GetHash().ToString(), signers.size());
        return ValidationResult::InvalidAttribute;
    }

    const auto& witnesses = transaction.GetWitnesses();
    if (witnesses.empty())
    {
        LOG_WARNING("Transaction {} has no witnesses", transaction.GetHash().ToString());
        return ValidationResult::InvalidWitness;
    }

    if (witnesses.size() > maxWitnesses)
    {
        LOG_WARNING("Transaction {} has too many witnesses ({})", transaction.GetHash().ToString(), witnesses.size());
        return ValidationResult::InvalidWitness;
    }

    if (signers.size() != witnesses.size())
    {
        LOG_WARNING("Transaction {} signer ({}) / witness ({}) count mismatch", transaction.GetHash().ToString(),
                    signers.size(), witnesses.size());
        return ValidationResult::InvalidWitness;
    }

    return ValidationResult::Valid;
}

ValidationResult MapVerificationResult(smartcontract::VerificationResult result)
{
    using smartcontract::VerificationResult;
    switch (result)
    {
        case VerificationResult::Succeed:
            return ValidationResult::Valid;
        case VerificationResult::AlreadyExists:
        case VerificationResult::AlreadyInPool:
            return ValidationResult::AlreadyExists;
        case VerificationResult::Expired:
            return ValidationResult::Expired;
        case VerificationResult::InsufficientFunds:
            return ValidationResult::InsufficientFunds;
        case VerificationResult::InsufficientNetworkFee:
            return ValidationResult::InvalidNetworkFee;
        case VerificationResult::InsufficientSystemFee:
            return ValidationResult::InvalidSystemFee;
        case VerificationResult::InvalidSignature:
            return ValidationResult::InvalidSignature;
        case VerificationResult::PolicyFail:
            return ValidationResult::PolicyViolation;
        case VerificationResult::Invalid:
        case VerificationResult::Failed:
            return ValidationResult::InvalidScript;
        case VerificationResult::UnableToVerify:
            return ValidationResult::Unknown;
        case VerificationResult::OutOfMemory:
            return ValidationResult::PolicyViolation;
        default:
            return ValidationResult::Unknown;
    }
}

bool ExceedsValidUntilBlockLimit(const Transaction& transaction, const ProtocolSettings& settings, uint32_t height)
{
    const auto validUntil = transaction.GetValidUntilBlock();
    const auto maxIncrement = settings.GetMaxValidUntilBlockIncrement();
    if (validUntil < height) return true;

    const auto maxAllowed = height > std::numeric_limits<uint32_t>::max() - maxIncrement
                                ? std::numeric_limits<uint32_t>::max()
                                : height + maxIncrement;
    return validUntil > maxAllowed;
}
}  // namespace

ValidationResult ValidateTransaction(const Transaction& transaction, std::shared_ptr<Blockchain> blockchain,
                                     std::shared_ptr<MemoryPool> mempool)
{
    if (!blockchain)
    {
        LOG_WARNING("ValidateTransaction invoked without blockchain context");
        return ValidationResult::Unknown;
    }

    auto system = blockchain->GetSystem();
    if (!system)
    {
        LOG_WARNING("ValidateTransaction: blockchain has no associated NeoSystem");
        return ValidationResult::Unknown;
    }

    const auto protocolSettings = system->GetSettings();

    const auto formatResult = ValidateBasicFormat(transaction, protocolSettings.get());
    if (formatResult != ValidationResult::Valid)
    {
        return formatResult;
    }

    if (blockchain->ContainsTransaction(transaction.GetHash()))
    {
        return ValidationResult::AlreadyExists;
    }

    if (mempool && mempool->Contains(transaction.GetHash()))
    {
        return ValidationResult::AlreadyExists;
    }

    const auto currentHeight = blockchain->GetHeight();
    if (transaction.GetValidUntilBlock() <= currentHeight)
    {
        LOG_WARNING("Transaction {} expired at height {} (current height {})", transaction.GetHash().ToString(),
                    transaction.GetValidUntilBlock(), currentHeight);
        return ValidationResult::Expired;
    }

    if (protocolSettings &&
        ExceedsValidUntilBlockLimit(transaction, *protocolSettings, currentHeight))
    {
        LOG_WARNING("Transaction {} validUntilBlock {} exceeds policy window (height {}, max increment {})",
                    transaction.GetHash().ToString(), transaction.GetValidUntilBlock(), currentHeight,
                    protocolSettings->GetMaxValidUntilBlockIncrement());
        return ValidationResult::PolicyViolation;
    }

    auto snapshot = system->GetSnapshot();
    if (!snapshot)
    {
        LOG_WARNING("ValidateTransaction: unable to acquire blockchain snapshot");
        return ValidationResult::Unknown;
    }

    smartcontract::VerificationContext context(snapshot, nullptr, smartcontract::ApplicationEngine::TestModeGas, false,
                                               false);
    auto verification = smartcontract::TransactionVerifier::Instance().VerifyTransaction(transaction, context);
    if (verification.result != smartcontract::VerificationResult::Succeed)
    {
        const auto message = verification.errorMessage.empty()
                                 ? smartcontract::VerificationResultToString(verification.result)
                                 : verification.errorMessage;
        LOG_WARNING("Transaction {} failed verification: {}", transaction.GetHash().ToString(), message);
        return MapVerificationResult(verification.result);
    }

    return ValidationResult::Valid;
}

}  // namespace neo::ledger
