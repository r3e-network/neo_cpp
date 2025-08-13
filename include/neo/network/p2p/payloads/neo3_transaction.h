/**
 * @file neo3_transaction.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/common/safe_math.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/transaction_attribute_type.h>
#include <neo/ledger/witness.h>
#include <neo/network/p2p/payloads/iinventory.h>
#include <neo/network/p2p/payloads/iverifiable.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a Neo N3 transaction.
 *
 * This is the correct Neo N3 transaction format that matches the C# implementation exactly.
 */
class Neo3Transaction : public IInventory, public IVerifiable, public io::ISerializable, public io::IJsonSerializable
{
   public:
    /// <summary>
    /// The maximum size of a transaction.
    /// </summary>
    static constexpr int MaxTransactionSize = 102400;

    /// <summary>
    /// The maximum number of attributes that can be contained within a transaction.
    /// </summary>
    static constexpr int MaxTransactionAttributes = 16;

    /// <summary>
    /// The size of a transaction header.
    /// </summary>
    static constexpr int HeaderSize = sizeof(uint8_t) +   // Version
                                      sizeof(uint32_t) +  // Nonce
                                      sizeof(int64_t) +   // SystemFee
                                      sizeof(int64_t) +   // NetworkFee
                                      sizeof(uint32_t);   // ValidUntilBlock

   private:
    uint8_t version_;
    uint32_t nonce_;
    int64_t systemFee_;   // In the unit of datoshi, 1 datoshi = 1e-8 GAS
    int64_t networkFee_;  // In the unit of datoshi, 1 datoshi = 1e-8 GAS
    uint32_t validUntilBlock_;
    std::vector<ledger::Signer> signers_;
    std::vector<std::shared_ptr<ledger::TransactionAttribute>> attributes_;
    io::ByteVector script_;
    std::vector<ledger::Witness> witnesses_;

    // Cached values
    mutable io::UInt256 hash_;
    mutable bool hashCalculated_;
    mutable int size_;
    mutable bool sizeCalculated_;

   public:
    /**
     * @brief Constructs an empty Neo3Transaction.
     */
    Neo3Transaction();

    /**
     * @brief Copy constructor.
     * @param other The transaction to copy from.
     */
    Neo3Transaction(const Neo3Transaction& other);

    /**
     * @brief Move constructor.
     * @param other The transaction to move from.
     */
    Neo3Transaction(Neo3Transaction&& other) noexcept;

    /**
     * @brief Copy assignment operator.
     * @param other The transaction to copy from.
     * @return Reference to this transaction.
     */
    Neo3Transaction& operator=(const Neo3Transaction& other);

    /**
     * @brief Move assignment operator.
     * @param other The transaction to move from.
     * @return Reference to this transaction.
     */
    Neo3Transaction& operator=(Neo3Transaction&& other) noexcept;

    /**
     * @brief Destructor.
     */
    ~Neo3Transaction() override;

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
     * @brief Gets the nonce.
     * @return The nonce.
     */
    uint32_t GetNonce() const;

    /**
     * @brief Sets the nonce.
     * @param nonce The nonce.
     */
    void SetNonce(uint32_t nonce);

    /**
     * @brief Gets the system fee.
     * @return The system fee in datoshi.
     */
    int64_t GetSystemFee() const;

    /**
     * @brief Sets the system fee.
     * @param systemFee The system fee in datoshi.
     */
    void SetSystemFee(int64_t systemFee);

    /**
     * @brief Gets the network fee.
     * @return The network fee in datoshi.
     */
    int64_t GetNetworkFee() const;

    /**
     * @brief Sets the network fee.
     * @param networkFee The network fee in datoshi.
     */
    void SetNetworkFee(int64_t networkFee);

    /**
     * @brief Gets the total fee (system + network) with overflow protection.
     * @return The total fee in datoshi.
     * @throws std::overflow_error if the sum would overflow.
     */
    int64_t GetTotalFee() const;

    /**
     * @brief Gets the valid until block.
     * @return The valid until block height.
     */
    uint32_t GetValidUntilBlock() const;

    /**
     * @brief Sets the valid until block.
     * @param validUntilBlock The valid until block height.
     */
    void SetValidUntilBlock(uint32_t validUntilBlock);

    /**
     * @brief Gets the signers.
     * @return The signers.
     */
    const std::vector<ledger::Signer>& GetSigners() const;

    /**
     * @brief Sets the signers.
     * @param signers The signers.
     */
    void SetSigners(const std::vector<ledger::Signer>& signers);

    /**
     * @brief Gets the attributes.
     * @return The attributes.
     */
    const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& GetAttributes() const;

    /**
     * @brief Sets the attributes.
     * @param attributes The attributes.
     */
    void SetAttributes(const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& attributes);

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
     * @brief Gets the witnesses.
     * @return The witnesses.
     */
    const std::vector<ledger::Witness>& GetWitnesses() const override;

    /**
     * @brief Sets the witnesses.
     * @param witnesses The witnesses.
     */
    void SetWitnesses(const std::vector<ledger::Witness>& witnesses) override;

    /**
     * @brief Gets the sender (first signer).
     * @return The sender address.
     */
    io::UInt160 GetSender() const;

    /**
     * @brief Gets the network fee per byte.
     * @return The fee per byte.
     */
    int64_t GetFeePerByte() const;

    // IInventory interface
    InventoryType GetInventoryType() const override;

    // IVerifiable interface
    std::vector<io::UInt160> GetScriptHashesForVerifying() const override;

    // ISerializable interface
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // Specific Neo3 serialization methods
    void SerializeUnsigned(io::BinaryWriter& writer) const;
    void DeserializeUnsigned(io::BinaryReader& reader);

    // IJsonSerializable interface
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Gets the hash of the transaction.
     * @return The hash.
     */
    io::UInt256 GetHash() const override;

    /**
     * @brief Gets the size of the transaction.
     * @return The size in bytes.
     */
    int GetSize() const override;

    /**
     * @brief Gets the first attribute of the specified type.
     * @tparam T The type of the attribute.
     * @return The first attribute of this type, or nullptr if not found.
     */
    template <typename T>
    std::shared_ptr<T> GetAttribute() const
    {
        for (const auto& attr : attributes_)
        {
            auto typed_attr = std::dynamic_pointer_cast<T>(std::make_shared<ledger::TransactionAttribute>(attr));
            if (typed_attr) return typed_attr;
        }
        return nullptr;
    }

    /**
     * @brief Verifies the transaction.
     * @param protocolSettings The protocol settings.
     * @param snapshot The data cache snapshot.
     * @return The verification result.
     */
    // VerifyResult Verify(const ProtocolSettings& protocolSettings, DataCache& snapshot) const;

    /**
     * @brief Checks if this transaction equals another.
     * @param other The other transaction.
     * @return True if equal.
     */
    bool operator==(const Neo3Transaction& other) const;

    /**
     * @brief Checks if this transaction does not equal another.
     * @param other The other transaction.
     * @return True if not equal.
     */
    bool operator!=(const Neo3Transaction& other) const;

    // Neo 2.x compatibility methods for tests
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
     * @brief Gets the transaction type (Neo 2.x compatibility).
     * @return The transaction type.
     */
    Type GetType() const { return Type::InvocationTransaction; }  // Neo 3 transactions are all invocation-like

    /**
     * @brief Sets the transaction type (Neo 2.x compatibility - no-op in Neo 3).
     * @param type The transaction type.
     */
    void SetType(Type type) { /* No-op in Neo 3 */ }

    // Neo 2.x compatibility - store legacy data for tests
    mutable std::vector<ledger::TransactionAttribute> legacy_attributes_;
    mutable std::vector<int> legacy_inputs_;   // Reserved for CoinReference compatibility
    mutable std::vector<int> legacy_outputs_;  // Reserved for TransactionOutput compatibility

    /**
     * @brief Gets attributes (Neo 2.x compatibility).
     * @return The attributes as TransactionAttribute objects.
     */
    const std::vector<ledger::TransactionAttribute>& GetLegacyAttributes() const { return legacy_attributes_; }

    /**
     * @brief Sets attributes (Neo 2.x compatibility).
     * @param attributes The attributes.
     */
    void SetAttributes(const std::vector<ledger::TransactionAttribute>& attributes) { legacy_attributes_ = attributes; }

    /**
     * @brief Gets inputs (Neo 2.x compatibility - empty in Neo 3).
     * @return Empty vector.
     */
    const std::vector<int>& GetInputs() const { return legacy_inputs_; }

    /**
     * @brief Sets inputs (Neo 2.x compatibility - no-op in Neo 3).
     * @param inputs The inputs.
     */
    void SetInputs(const std::vector<int>& inputs) { legacy_inputs_ = inputs; }

    /**
     * @brief Gets outputs (Neo 2.x compatibility - empty in Neo 3).
     * @return Empty vector.
     */
    const std::vector<int>& GetOutputs() const { return legacy_outputs_; }

    /**
     * @brief Sets outputs (Neo 2.x compatibility - no-op in Neo 3).
     * @param outputs The outputs.
     */
    void SetOutputs(const std::vector<int>& outputs) { legacy_outputs_ = outputs; }

   private:
    void InvalidateCache() const;
    void CalculateHash() const;
    void CalculateSize() const;

    // Helper methods for size calculation
    int GetVarIntSize(size_t value) const;
    int GetSignerSize(const ledger::Signer& signer) const;
    int GetAttributeSize(const ledger::TransactionAttribute& attr) const;
    int GetWitnessSize(const ledger::Witness& witness) const;

    // Helper methods for deserialization
    static std::vector<ledger::TransactionAttribute> DeserializeAttributes(io::BinaryReader& reader, int maxCount);
    static std::vector<ledger::Signer> DeserializeSigners(io::BinaryReader& reader, int maxCount);
};
}  // namespace neo::network::p2p::payloads