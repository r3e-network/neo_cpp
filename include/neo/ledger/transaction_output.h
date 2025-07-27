#pragma once

#include <cstdint>
#include <neo/core/fixed8.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

namespace neo::ledger
{
/**
 * @brief Represents a transaction output (Neo 2.x compatibility).
 * This class is provided for test compatibility only.
 */
class TransactionOutput : public io::ISerializable, public io::IJsonSerializable
{
  private:
    io::UInt256 assetId_;
    core::Fixed8 value_;
    io::UInt160 scriptHash_;

  public:
    /**
     * @brief Constructs an empty TransactionOutput.
     */
    TransactionOutput();

    /**
     * @brief Constructs a TransactionOutput with the specified values.
     * @param assetId The asset ID.
     * @param value The value.
     * @param scriptHash The script hash.
     */
    TransactionOutput(const io::UInt256& assetId, const core::Fixed8& value, const io::UInt160& scriptHash);

    /**
     * @brief Gets the asset ID.
     * @return The asset ID.
     */
    const io::UInt256& GetAssetId() const;

    /**
     * @brief Sets the asset ID.
     * @param assetId The asset ID.
     */
    void SetAssetId(const io::UInt256& assetId);

    /**
     * @brief Gets the value.
     * @return The value.
     */
    const core::Fixed8& GetValue() const;

    /**
     * @brief Sets the value.
     * @param value The value.
     */
    void SetValue(const core::Fixed8& value);

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Sets the script hash.
     * @param scriptHash The script hash.
     */
    void SetScriptHash(const io::UInt160& scriptHash);

    // ISerializable interface
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;
    int GetSize() const;

    // IJsonSerializable interface
    void SerializeJson(io::JsonWriter& writer) const override;
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Checks if this transaction output equals another.
     * @param other The other transaction output.
     * @return True if equal.
     */
    bool operator==(const TransactionOutput& other) const;

    /**
     * @brief Checks if this transaction output does not equal another.
     * @param other The other transaction output.
     * @return True if not equal.
     */
    bool operator!=(const TransactionOutput& other) const;
};
}  // namespace neo::ledger