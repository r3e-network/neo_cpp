#pragma once

#include <neo/ledger/transaction.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/logging/logger.h>
#include <neo/metrics/metrics.h>
#include <neo/cache/cache.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace neo::smartcontract
{
    /**
     * @brief Result of transaction verification.
     */
    enum class VerificationResult
    {
        Succeed,
        Failed,
        Invalid,
        PolicyFail,
        InsufficientFunds,
        AlreadyExists,
        AlreadyInPool,
        OutOfMemory,
        UnableToVerify,
        Expired,
        InsufficientNetworkFee,
        InsufficientSystemFee
    };

    /**
     * @brief Converts a verification result to a string.
     * @param result The verification result.
     * @return The string representation of the verification result.
     */
    inline std::string VerificationResultToString(VerificationResult result)
    {
        switch (result)
        {
            case VerificationResult::Succeed:
                return "Succeed";
            case VerificationResult::Failed:
                return "Failed";
            case VerificationResult::Invalid:
                return "Invalid";
            case VerificationResult::PolicyFail:
                return "PolicyFail";
            case VerificationResult::InsufficientFunds:
                return "InsufficientFunds";
            case VerificationResult::AlreadyExists:
                return "AlreadyExists";
            case VerificationResult::AlreadyInPool:
                return "AlreadyInPool";
            case VerificationResult::OutOfMemory:
                return "OutOfMemory";
            case VerificationResult::UnableToVerify:
                return "UnableToVerify";
            case VerificationResult::Expired:
                return "Expired";
            case VerificationResult::InsufficientNetworkFee:
                return "InsufficientNetworkFee";
            case VerificationResult::InsufficientSystemFee:
                return "InsufficientSystemFee";
            default:
                return "Unknown";
        }
    }

    /**
     * @brief Transaction verification context.
     */
    struct VerificationContext
    {
        /**
         * @brief The snapshot.
         */
        std::shared_ptr<persistence::DataCache> snapshot;

        /**
         * @brief The persisting block.
         */
        const ledger::Block* persistingBlock = nullptr;

        /**
         * @brief The maximum gas allowed.
         */
        int64_t maxGas = ApplicationEngine::TestModeGas;

        /**
         * @brief Whether to skip signature verification.
         */
        bool skipSignatureVerification = false;

        /**
         * @brief Whether to skip witness verification.
         */
        bool skipWitnessVerification = false;

        /**
         * @brief Constructs a VerificationContext.
         * @param snapshot The snapshot.
         * @param persistingBlock The persisting block.
         * @param maxGas The maximum gas allowed.
         * @param skipSignatureVerification Whether to skip signature verification.
         * @param skipWitnessVerification Whether to skip witness verification.
         */
        VerificationContext(std::shared_ptr<persistence::DataCache> snapshot, const ledger::Block* persistingBlock = nullptr, int64_t maxGas = ApplicationEngine::TestModeGas, bool skipSignatureVerification = false, bool skipWitnessVerification = false)
            : snapshot(snapshot), persistingBlock(persistingBlock), maxGas(maxGas), skipSignatureVerification(skipSignatureVerification), skipWitnessVerification(skipWitnessVerification)
        {
        }
    };

    /**
     * @brief Transaction verification result.
     */
    struct VerificationOutput
    {
        /**
         * @brief The verification result.
         */
        VerificationResult result;

        /**
         * @brief The error message.
         */
        std::string errorMessage;

        /**
         * @brief The gas consumed.
         */
        int64_t gasConsumed = 0;

        /**
         * @brief The application engine.
         */
        std::unique_ptr<ApplicationEngine> engine;

        /**
         * @brief Constructs a VerificationOutput.
         * @param result The verification result.
         * @param errorMessage The error message.
         * @param gasConsumed The gas consumed.
         * @param engine The application engine.
         */
        VerificationOutput(VerificationResult result, const std::string& errorMessage = "", int64_t gasConsumed = 0, std::unique_ptr<ApplicationEngine> engine = nullptr)
            : result(result), errorMessage(errorMessage), gasConsumed(gasConsumed), engine(std::move(engine))
        {
        }
    };

    /**
     * @brief Transaction verifier.
     *
     * The transaction verifier is responsible for verifying transactions.
     */
    class TransactionVerifier
    {
    public:
        /**
         * @brief Gets the singleton instance of the transaction verifier.
         * @return The singleton instance of the transaction verifier.
         */
        static TransactionVerifier& Instance();

        /**
         * @brief Verifies a transaction.
         * @param transaction The transaction to verify.
         * @param context The verification context.
         * @return The verification output.
         */
        VerificationOutput VerifyTransaction(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Verifies a transaction's signature.
         * @param transaction The transaction to verify.
         * @param context The verification context.
         * @return The verification output.
         */
        VerificationOutput VerifySignature(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Verifies a transaction's witness.
         * @param transaction The transaction to verify.
         * @param context The verification context.
         * @return The verification output.
         */
        VerificationOutput VerifyWitness(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Verifies a transaction's network fee.
         * @param transaction The transaction to verify.
         * @param context The verification context.
         * @return The verification output.
         */
        VerificationOutput VerifyNetworkFee(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Verifies a transaction's system fee.
         * @param transaction The transaction to verify.
         * @param context The verification context.
         * @return The verification output.
         */
        VerificationOutput VerifySystemFee(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Calculates the network fee for a transaction.
         * @param transaction The transaction.
         * @param context The verification context.
         * @return The network fee.
         */
        int64_t CalculateNetworkFee(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Calculates the system fee for a transaction.
         * @param transaction The transaction.
         * @param context The verification context.
         * @return The system fee.
         */
        int64_t CalculateSystemFee(const ledger::Transaction& transaction, const VerificationContext& context);

    private:
        TransactionVerifier();
        ~TransactionVerifier() = default;
        TransactionVerifier(const TransactionVerifier&) = delete;
        TransactionVerifier& operator=(const TransactionVerifier&) = delete;

        /**
         * @brief Calculates the witness verification fee for a transaction.
         * @param transaction The transaction.
         * @param context The verification context.
         * @return The witness verification fee.
         */
        int64_t CalculateWitnessVerificationFee(const ledger::Transaction& transaction, const VerificationContext& context);

        /**
         * @brief Checks if a script is a signature contract.
         * @param script The script to check.
         * @return True if the script is a signature contract, false otherwise.
         */
        bool IsSignatureContract(const io::ByteVector& script);

        /**
         * @brief Checks if a script is a multi-signature contract.
         * @param script The script to check.
         * @return True if the script is a multi-signature contract, false otherwise.
         */
        bool IsMultiSignatureContract(const io::ByteVector& script);

        std::shared_ptr<cache::Cache<VerificationOutput>> verificationCache_;
        std::shared_ptr<metrics::Counter> verificationCounter_;
        std::shared_ptr<metrics::Counter> verificationSuccessCounter_;
        std::shared_ptr<metrics::Counter> verificationFailureCounter_;
        std::shared_ptr<metrics::Histogram> verificationTimeHistogram_;
        std::shared_ptr<metrics::Histogram> verificationGasHistogram_;
    };

    /**
     * @brief Gets the singleton instance of the transaction verifier.
     * @return The singleton instance of the transaction verifier.
     */
    inline TransactionVerifier& Verifier()
    {
        return TransactionVerifier::Instance();
    }
}
