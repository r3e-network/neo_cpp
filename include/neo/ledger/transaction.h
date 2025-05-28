#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/fixed8.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/transaction_output.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/oracle_response.h>
#include <neo/ledger/signer.h>
#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace neo::ledger
{
    /**
     * @brief Represents a transaction.
     */
    class Transaction : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Enum for transaction type.
         */
        enum class Type : uint8_t
        {
            MinerTransaction = 0x00,
            IssueTransaction = 0x01,
            ClaimTransaction = 0x02,
            EnrollmentTransaction = 0x20,
            RegisterTransaction = 0x40,
            ContractTransaction = 0x80,
            StateTransaction = 0x90,
            PublishTransaction = 0xd0,
            InvocationTransaction = 0xd1
        };

        /**
         * @brief Constructs an empty Transaction.
         */
        Transaction();

        /**
         * @brief Gets the type.
         * @return The type.
         */
        Type GetType() const;

        /**
         * @brief Sets the type.
         * @param type The type.
         */
        void SetType(Type type);

        /**
         * @brief Gets the version.
         * @return The version.
         */
        uint8_t GetVersion() const;

        /**
         * @brief Sets the version.
         * @param version The version.
         */
        void SetVersion(uint8_t version);

        /**
         * @brief Gets the attributes.
         * @return The attributes.
         */
        const std::vector<TransactionAttribute>& GetAttributes() const;

        /**
         * @brief Sets the attributes.
         * @param attributes The attributes.
         */
        void SetAttributes(const std::vector<TransactionAttribute>& attributes);

        /**
         * @brief Gets the inputs.
         * @return The inputs.
         */
        const std::vector<CoinReference>& GetInputs() const;

        /**
         * @brief Sets the inputs.
         * @param inputs The inputs.
         */
        void SetInputs(const std::vector<CoinReference>& inputs);

        /**
         * @brief Gets the outputs.
         * @return The outputs.
         */
        const std::vector<TransactionOutput>& GetOutputs() const;

        /**
         * @brief Sets the outputs.
         * @param outputs The outputs.
         */
        void SetOutputs(const std::vector<TransactionOutput>& outputs);

        /**
         * @brief Gets the witnesses.
         * @return The witnesses.
         */
        const std::vector<Witness>& GetWitnesses() const;

        /**
         * @brief Sets the witnesses.
         * @param witnesses The witnesses.
         */
        void SetWitnesses(const std::vector<Witness>& witnesses);

        /**
         * @brief Gets the hash of the transaction.
         * @return The hash of the transaction.
         */
        io::UInt256 GetHash() const;

        /**
         * @brief Gets the size of the transaction in bytes.
         * @return The size of the transaction in bytes.
         */
        size_t GetSize() const;

        /**
         * @brief Gets the nonce of the transaction.
         * @return The nonce.
         */
        uint32_t GetNonce() const;

        /**
         * @brief Sets the nonce of the transaction.
         * @param nonce The nonce.
         */
        void SetNonce(uint32_t nonce);

        /**
         * @brief Gets the network fee of the transaction.
         * @return The network fee.
         */
        int64_t GetNetworkFee() const;

        /**
         * @brief Sets the network fee of the transaction.
         * @param networkFee The network fee.
         */
        void SetNetworkFee(int64_t networkFee);

        /**
         * @brief Gets the system fee of the transaction.
         * @return The system fee.
         */
        int64_t GetSystemFee() const;

        /**
         * @brief Sets the system fee of the transaction.
         * @param systemFee The system fee.
         */
        void SetSystemFee(int64_t systemFee);

        /**
         * @brief Gets the valid until block of the transaction.
         * @return The valid until block.
         */
        uint32_t GetValidUntilBlock() const;

        /**
         * @brief Sets the valid until block of the transaction.
         * @param validUntilBlock The valid until block.
         */
        void SetValidUntilBlock(uint32_t validUntilBlock);

        /**
         * @brief Gets the first attribute of the specified type.
         * @tparam T The type of the attribute.
         * @return The first attribute of this type, or nullptr if not found.
         */
        template<typename T>
        std::shared_ptr<T> GetAttribute() const
        {
            for (const auto& attr : attributes_)
            {
                auto typed_attr = std::dynamic_pointer_cast<T>(std::make_shared<TransactionAttribute>(attr));
                if (typed_attr)
                    return typed_attr;
            }
            return nullptr;
        }

        /**
         * @brief Gets the oracle response attribute.
         * @return The oracle response attribute, or nullptr if not found.
         */
        std::shared_ptr<class OracleResponse> GetOracleResponse() const;

        /**
         * @brief Gets the sender (temporary implementation).
         * @return The sender address.
         */
        io::UInt160 GetSender() const;

        /**
         * @brief Gets the script.
         * @return The script.
         */
        const io::ByteVector& GetScript() const;

        /**
         * @brief Sets the script.
         * @param script The script.
         */
        void SetScript(const io::ByteVector& script);

        /**
         * @brief Gets the signers.
         * @return The signers.
         */
        const std::vector<Signer>& GetSigners() const;

        /**
         * @brief Sets the signers.
         * @param signers The signers.
         */
        void SetSigners(const std::vector<Signer>& signers);

        /**
         * @brief Serializes the Transaction to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the Transaction from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the Transaction to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the Transaction from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Verifies the transaction.
         * @return True if the transaction is valid, false otherwise.
         */
        bool Verify() const;

        /**
         * @brief Verifies the witnesses.
         * @return True if the witnesses are valid, false otherwise.
         */
        bool VerifyWitnesses() const;

        /**
         * @brief Serializes exclusive data for the transaction type.
         * @param writer The binary writer.
         */
        virtual void SerializeExclusiveData(io::BinaryWriter& writer) const;

        /**
         * @brief Deserializes exclusive data for the transaction type.
         * @param reader The binary reader.
         */
        virtual void DeserializeExclusiveData(io::BinaryReader& reader);

        /**
         * @brief Checks if this Transaction is equal to another Transaction.
         * @param other The other Transaction.
         * @return True if the Transactions are equal, false otherwise.
         */
        bool operator==(const Transaction& other) const;

        /**
         * @brief Checks if this Transaction is not equal to another Transaction.
         * @param other The other Transaction.
         * @return True if the Transactions are not equal, false otherwise.
         */
        bool operator!=(const Transaction& other) const;

    private:
        Type type_;
        uint8_t version_;
        uint32_t nonce_;
        int64_t systemFee_;
        int64_t networkFee_;
        uint32_t validUntilBlock_;
        std::vector<TransactionAttribute> attributes_;
        std::vector<CoinReference> inputs_;
        std::vector<TransactionOutput> outputs_;
        std::vector<Witness> witnesses_;
        io::ByteVector script_;
        std::vector<Signer> signers_;

        // Helper methods for witness verification
        bool IsSignatureContract(const io::ByteVector& script) const;
        bool IsMultiSignatureContract(const io::ByteVector& script) const;
        bool VerifySignatureContract(const Witness& witness, const io::UInt160& hash) const;
        bool VerifyMultiSignatureContract(const Witness& witness, const io::UInt160& hash) const;
        bool VerifyScriptContract(const Witness& witness, const io::UInt160& hash) const;
        io::ByteVector ExtractSignatureFromInvocationScript(const io::ByteVector& invocationScript) const;
        io::ByteVector ExtractPublicKeyFromVerificationScript(const io::ByteVector& verificationScript) const;
        io::ByteVector GetSignData() const;
    };
}
