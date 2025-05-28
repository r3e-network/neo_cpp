#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint256.h>
#include <cstdint>

namespace neo::ledger
{
    /**
     * @brief Represents a transaction input.
     */
    class CoinReference : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty CoinReference.
         */
        CoinReference();

        /**
         * @brief Constructs a CoinReference with the specified previous hash and index.
         * @param prevHash The previous hash.
         * @param prevIndex The previous index.
         */
        CoinReference(const io::UInt256& prevHash, uint16_t prevIndex);

        /**
         * @brief Gets the previous hash.
         * @return The previous hash.
         */
        const io::UInt256& GetPrevHash() const;

        /**
         * @brief Sets the previous hash.
         * @param prevHash The previous hash.
         */
        void SetPrevHash(const io::UInt256& prevHash);

        /**
         * @brief Gets the previous index.
         * @return The previous index.
         */
        uint16_t GetPrevIndex() const;

        /**
         * @brief Sets the previous index.
         * @param prevIndex The previous index.
         */
        void SetPrevIndex(uint16_t prevIndex);

        /**
         * @brief Serializes the CoinReference to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the CoinReference from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the CoinReference to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the CoinReference from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Checks if this CoinReference is equal to another CoinReference.
         * @param other The other CoinReference.
         * @return True if the CoinReferences are equal, false otherwise.
         */
        bool operator==(const CoinReference& other) const;

        /**
         * @brief Checks if this CoinReference is not equal to another CoinReference.
         * @param other The other CoinReference.
         * @return True if the CoinReferences are not equal, false otherwise.
         */
        bool operator!=(const CoinReference& other) const;

    private:
        io::UInt256 prevHash_;
        uint16_t prevIndex_;
    };
}
