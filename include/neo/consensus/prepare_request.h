#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/transaction.h>
#include <cstdint>
#include <vector>
#include <memory>

namespace neo::consensus
{
    /**
     * @brief Represents a prepare request message.
     */
    class PrepareRequest : public ConsensusMessage
    {
    public:
        /**
         * @brief Constructs a PrepareRequest.
         * @param viewNumber The view number.
         * @param timestamp The timestamp.
         * @param nonce The nonce.
         * @param nextConsensus The next consensus.
         */
        PrepareRequest(uint8_t viewNumber, uint64_t timestamp, uint64_t nonce, const io::UInt160& nextConsensus);
        
        /**
         * @brief Gets the timestamp.
         * @return The timestamp.
         */
        uint64_t GetTimestamp() const;
        
        /**
         * @brief Gets the nonce.
         * @return The nonce.
         */
        uint64_t GetNonce() const;
        
        /**
         * @brief Gets the next consensus.
         * @return The next consensus.
         */
        const io::UInt160& GetNextConsensus() const;
        
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
         * @brief Gets the transaction hashes.
         * @return The transaction hashes.
         */
        const std::vector<io::UInt256>& GetTransactionHashes() const;
        
        /**
         * @brief Sets the transaction hashes.
         * @param transactionHashes The transaction hashes.
         */
        void SetTransactionHashes(const std::vector<io::UInt256>& transactionHashes);
        
        /**
         * @brief Gets the invocation script.
         * @return The invocation script.
         */
        const io::ByteVector& GetInvocationScript() const;
        
        /**
         * @brief Sets the invocation script.
         * @param invocationScript The invocation script.
         */
        void SetInvocationScript(const io::ByteVector& invocationScript);
        
        /**
         * @brief Gets the verification script.
         * @return The verification script.
         */
        const io::ByteVector& GetVerificationScript() const;
        
        /**
         * @brief Sets the verification script.
         * @param verificationScript The verification script.
         */
        void SetVerificationScript(const io::ByteVector& verificationScript);
        
        /**
         * @brief Serializes the object.
         * @param writer The writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;
        
        /**
         * @brief Deserializes the object.
         * @param reader The reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Gets the message data.
         * @return The message data.
         */
        io::ByteVector GetData() const override;
        
    private:
        uint64_t timestamp_;
        uint64_t nonce_;
        io::UInt160 nextConsensus_;
        std::vector<std::shared_ptr<ledger::Transaction>> transactions_;
        std::vector<io::UInt256> transactionHashes_;
        io::ByteVector invocationScript_;
        io::ByteVector verificationScript_;
    };
}
