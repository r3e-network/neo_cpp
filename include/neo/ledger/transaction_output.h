#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/io/fixed8.h>

namespace neo::ledger
{
    /**
     * @brief Represents a transaction output.
     */
    class TransactionOutput : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty TransactionOutput.
         */
        TransactionOutput();

        /**
         * @brief Constructs a TransactionOutput with the specified asset ID, value, and script hash.
         * @param assetId The asset ID.
         * @param value The value.
         * @param scriptHash The script hash.
         */
        TransactionOutput(const io::UInt256& assetId, const io::Fixed8& value, const io::UInt160& scriptHash);

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
        const io::Fixed8& GetValue() const;

        /**
         * @brief Sets the value.
         * @param value The value.
         */
        void SetValue(const io::Fixed8& value);

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

        /**
         * @brief Serializes the TransactionOutput to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the TransactionOutput from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the TransactionOutput to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the TransactionOutput from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Checks if this TransactionOutput is equal to another TransactionOutput.
         * @param other The other TransactionOutput.
         * @return True if the TransactionOutputs are equal, false otherwise.
         */
        bool operator==(const TransactionOutput& other) const;

        /**
         * @brief Checks if this TransactionOutput is not equal to another TransactionOutput.
         * @param other The other TransactionOutput.
         * @return True if the TransactionOutputs are not equal, false otherwise.
         */
        bool operator!=(const TransactionOutput& other) const;

    private:
        io::UInt256 assetId_;
        io::Fixed8 value_;
        io::UInt160 scriptHash_;
    };
}
