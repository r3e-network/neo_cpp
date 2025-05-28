#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/consensus/consensus_message.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/change_view_message.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace neo::consensus
{
    /**
     * @brief Represents the consensus context.
     */
    class ConsensusContext
    {
    public:
        /**
         * @brief Constructs a ConsensusContext.
         * @param validators The validators.
         * @param myIndex The index of the current node in the validators list.
         * @param keyPair The key pair.
         * @param blockIndex The block index.
         */
        ConsensusContext(const std::vector<cryptography::ecc::ECPoint>& validators, uint16_t myIndex, const cryptography::ecc::KeyPair& keyPair, uint32_t blockIndex);

        /**
         * @brief Gets the validators.
         * @return The validators.
         */
        const std::vector<cryptography::ecc::ECPoint>& GetValidators() const;

        /**
         * @brief Gets the validator index.
         * @return The validator index.
         */
        uint16_t GetValidatorIndex() const;

        /**
         * @brief Gets the primary index.
         * @return The primary index.
         */
        uint16_t GetPrimaryIndex() const;

        /**
         * @brief Gets the primary index for the specified view.
         * @param viewNumber The view number.
         * @return The primary index.
         */
        uint16_t GetPrimaryIndex(uint8_t viewNumber) const;

        /**
         * @brief Gets the view number.
         * @return The view number.
         */
        uint8_t GetViewNumber() const;

        /**
         * @brief Sets the view number.
         * @param viewNumber The view number.
         */
        void SetViewNumber(uint8_t viewNumber);

        /**
         * @brief Gets the block index.
         * @return The block index.
         */
        uint32_t GetBlockIndex() const;

        /**
         * @brief Gets the block.
         * @return The block.
         */
        std::shared_ptr<ledger::Block> GetBlock() const;

        /**
         * @brief Sets the block.
         * @param block The block.
         */
        void SetBlock(std::shared_ptr<ledger::Block> block);

        /**
         * @brief Gets the transactions.
         * @return The transactions.
         */
        const std::vector<std::shared_ptr<ledger::Transaction>>& GetTransactions() const;

        /**
         * @brief Sets the transactions.
         * @param transactions The transactions.
         */
        void SetTransactions(const std::vector<std::shared_ptr<ledger::Transaction>>& transactions);

        /**
         * @brief Gets the prepare request message.
         * @return The prepare request message.
         */
        std::shared_ptr<PrepareRequest> GetPrepareRequestMessage() const;

        /**
         * @brief Sets the prepare request message.
         * @param message The prepare request message.
         */
        void SetPrepareRequestMessage(std::shared_ptr<PrepareRequest> message);

        /**
         * @brief Gets the prepare response messages.
         * @return The prepare response messages.
         */
        const std::unordered_map<uint16_t, std::shared_ptr<PrepareResponse>>& GetPrepareResponseMessages() const;

        /**
         * @brief Adds a prepare response message.
         * @param validatorIndex The validator index.
         * @param message The prepare response message.
         */
        void AddPrepareResponseMessage(uint16_t validatorIndex, std::shared_ptr<PrepareResponse> message);

        /**
         * @brief Gets the commit messages.
         * @return The commit messages.
         */
        const std::unordered_map<uint16_t, std::shared_ptr<CommitMessage>>& GetCommitMessages() const;

        /**
         * @brief Adds a commit message.
         * @param validatorIndex The validator index.
         * @param message The commit message.
         */
        void AddCommitMessage(uint16_t validatorIndex, std::shared_ptr<CommitMessage> message);

        /**
         * @brief Gets the change view messages.
         * @return The change view messages.
         */
        const std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>>& GetChangeViewMessages() const;

        /**
         * @brief Adds a change view message.
         * @param validatorIndex The validator index.
         * @param message The change view message.
         */
        void AddChangeViewMessage(uint16_t validatorIndex, std::shared_ptr<ChangeViewMessage> message);

        /**
         * @brief Gets the recovery messages.
         * @return The recovery messages.
         */
        const std::unordered_map<uint16_t, std::shared_ptr<RecoveryMessage>>& GetRecoveryMessages() const;

        /**
         * @brief Adds a recovery message.
         * @param validatorIndex The validator index.
         * @param message The recovery message.
         */
        void AddRecoveryMessage(uint16_t validatorIndex, std::shared_ptr<RecoveryMessage> message);

        /**
         * @brief Resets the context.
         */
        void Reset();

        /**
         * @brief Checks if the node is primary.
         * @return True if the node is primary, false otherwise.
         */
        bool IsPrimary() const;

        /**
         * @brief Checks if the node is backup.
         * @return True if the node is backup, false otherwise.
         */
        bool IsBackup() const;

        /**
         * @brief Checks if the node has received enough prepare responses.
         * @return True if the node has received enough prepare responses, false otherwise.
         */
        bool HasReceivedEnoughPrepareResponses() const;

        /**
         * @brief Checks if the node has received enough commits.
         * @return True if the node has received enough commits, false otherwise.
         */
        bool HasReceivedEnoughCommits() const;

        /**
         * @brief Checks if the node has received enough change view messages.
         * @param viewNumber The view number.
         * @return True if the node has received enough change view messages, false otherwise.
         */
        bool HasReceivedEnoughChangeViewMessages(uint8_t viewNumber) const;

    private:
        std::vector<cryptography::ecc::ECPoint> validators_;
        uint16_t validatorIndex_;
        cryptography::ecc::KeyPair keyPair_;
        uint8_t viewNumber_;
        uint32_t blockIndex_;
        std::shared_ptr<ledger::Block> block_;
        std::vector<std::shared_ptr<ledger::Transaction>> transactions_;
        std::shared_ptr<PrepareRequest> prepareRequestMessage_;
        std::unordered_map<uint16_t, std::shared_ptr<PrepareResponse>> prepareResponseMessages_;
        std::unordered_map<uint16_t, std::shared_ptr<CommitMessage>> commitMessages_;
        std::unordered_map<uint16_t, std::shared_ptr<ChangeViewMessage>> changeViewMessages_;
        std::unordered_map<uint16_t, std::shared_ptr<RecoveryMessage>> recoveryMessages_;
    };
}
