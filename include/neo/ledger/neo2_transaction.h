// Copyright (C) 2015-2025 The Neo Project.
//
// neo2_transaction.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_LEDGER_NEO2_TRANSACTION_H
#define NEO_LEDGER_NEO2_TRANSACTION_H

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/ledger/witness.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/transaction_output.h>
#include <neo/io/fixed8.h>
#include <vector>
#include <memory>
#include <cstdint>

namespace neo::ledger {

/**
 * @brief Neo 2.x compatible transaction class for tests.
 */
class Neo2Transaction : public io::ISerializable, public io::IJsonSerializable {
public:
    enum class Type : uint8_t {
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

private:
    Type type_;
    uint8_t version_;
    std::vector<TransactionAttribute> attributes_;
    std::vector<CoinReference> inputs_;
    std::vector<TransactionOutput> outputs_;
    std::vector<Witness> witnesses_;
    
    // InvocationTransaction specific fields
    io::ByteVector script_;
    io::Fixed8 gas_;

public:
    /**
     * @brief Default constructor.
     */
    Neo2Transaction();

    /**
     * @brief Gets the transaction type.
     * @return The transaction type.
     */
    Type GetType() const { return type_; }

    /**
     * @brief Sets the transaction type.
     * @param type The transaction type.
     */
    void SetType(Type type) { type_ = type; }

    /**
     * @brief Gets the version.
     * @return The version.
     */
    uint8_t GetVersion() const { return version_; }

    /**
     * @brief Sets the version.
     * @param version The version.
     */
    void SetVersion(uint8_t version) { version_ = version; }

    /**
     * @brief Gets the attributes.
     * @return The attributes.
     */
    const std::vector<TransactionAttribute>& GetAttributes() const { return attributes_; }

    /**
     * @brief Sets the attributes.
     * @param attributes The attributes.
     */
    void SetAttributes(const std::vector<TransactionAttribute>& attributes) { attributes_ = attributes; }

    /**
     * @brief Gets the inputs.
     * @return The inputs.
     */
    const std::vector<CoinReference>& GetInputs() const { return inputs_; }

    /**
     * @brief Sets the inputs.
     * @param inputs The inputs.
     */
    void SetInputs(const std::vector<CoinReference>& inputs) { inputs_ = inputs; }

    /**
     * @brief Gets the outputs.
     * @return The outputs.
     */
    const std::vector<TransactionOutput>& GetOutputs() const { return outputs_; }

    /**
     * @brief Sets the outputs.
     * @param outputs The outputs.
     */
    void SetOutputs(const std::vector<TransactionOutput>& outputs) { outputs_ = outputs; }

    /**
     * @brief Gets the witnesses.
     * @return The witnesses.
     */
    const std::vector<Witness>& GetWitnesses() const { return witnesses_; }

    /**
     * @brief Sets the witnesses.
     * @param witnesses The witnesses.
     */
    void SetWitnesses(const std::vector<Witness>& witnesses) { witnesses_ = witnesses; }
    
    /**
     * @brief Gets the script (for InvocationTransaction).
     * @return The script.
     */
    const io::ByteVector& GetInvocationScript() const { return script_; }
    
    /**
     * @brief Sets the script (for InvocationTransaction).
     * @param script The script.
     */
    void SetInvocationScript(const io::ByteVector& script) { script_ = script; }
    
    /**
     * @brief Gets the gas (for InvocationTransaction).
     * @return The gas amount.
     */
    io::Fixed8 GetGas() const { return gas_; }
    
    /**
     * @brief Sets the gas (for InvocationTransaction).
     * @param gas The gas amount.
     */
    void SetGas(io::Fixed8 gas) { gas_ = gas; }

    // Neo 3.x compatibility methods for native contracts
    /**
     * @brief Gets the nonce (Neo 3.x compatibility - returns 0 for Neo 2.x).
     * @return The nonce.
     */
    uint32_t GetNonce() const { return 0; }

    /**
     * @brief Gets the sender (Neo 3.x compatibility - returns zero hash for Neo 2.x).
     * @return The sender address.
     */
    io::UInt160 GetSender() const { return io::UInt160(); }

    /**
     * @brief Gets the system fee (Neo 3.x compatibility - returns 0 for Neo 2.x).
     * @return The system fee.
     */
    int64_t GetSystemFee() const { return 0; }

    /**
     * @brief Gets the network fee (Neo 3.x compatibility - returns 0 for Neo 2.x).
     * @return The network fee.
     */
    int64_t GetNetworkFee() const { return 0; }

    /**
     * @brief Gets the valid until block (Neo 3.x compatibility - returns max for Neo 2.x).
     * @return The valid until block.
     */
    uint32_t GetValidUntilBlock() const { return UINT32_MAX; }

    /**
     * @brief Gets the script (Neo 3.x compatibility - returns empty for Neo 2.x).
     * @return The script.
     */
    io::ByteVector GetScript() const { return io::ByteVector(); }

    /**
     * @brief Gets the signers for Neo 3.x compatibility.
     * @return Empty signers list for Neo 2.x transactions
     */
    std::vector<io::UInt160> GetSigners() const { return std::vector<io::UInt160>(); }

    /**
     * @brief Gets the hash of the transaction.
     * @return The hash.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Gets the size of the transaction.
     * @return The size in bytes.
     */
    int GetSize() const;

    // ISerializable interface
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // IJsonSerializable interface
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Equality operator.
     * @param other The other transaction.
     * @return True if equal.
     */
    bool operator==(const Neo2Transaction& other) const;

    /**
     * @brief Inequality operator.
     * @param other The other transaction.
     * @return True if not equal.
     */
    bool operator!=(const Neo2Transaction& other) const;
};

} // namespace neo::ledger

#endif // NEO_LEDGER_NEO2_TRANSACTION_H