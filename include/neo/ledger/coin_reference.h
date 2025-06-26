#pragma once

#include <neo/io/uint256.h>
#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <cstdint>

namespace neo::ledger
{
    /**
     * @brief Represents a coin reference (Neo 2.x compatibility).
     * This class is provided for test compatibility only.
     */
    class CoinReference : public io::ISerializable, public io::IJsonSerializable
    {
    private:
        io::UInt256 prevHash_;
        uint16_t prevIndex_;

    public:
        /**
         * @brief Constructs an empty CoinReference.
         */
        CoinReference();

        /**
         * @brief Constructs a CoinReference with the specified values.
         * @param prevHash The previous transaction hash.
         * @param prevIndex The previous transaction output index.
         */
        CoinReference(const io::UInt256& prevHash, uint16_t prevIndex);

        /**
         * @brief Gets the previous transaction hash.
         * @return The previous transaction hash.
         */
        const io::UInt256& GetPrevHash() const;

        /**
         * @brief Sets the previous transaction hash.
         * @param prevHash The previous transaction hash.
         */
        void SetPrevHash(const io::UInt256& prevHash);

        /**
         * @brief Gets the previous transaction output index.
         * @return The previous transaction output index.
         */
        uint16_t GetPrevIndex() const;

        /**
         * @brief Sets the previous transaction output index.
         * @param prevIndex The previous transaction output index.
         */
        void SetPrevIndex(uint16_t prevIndex);

        // ISerializable interface
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
        int GetSize() const;

        // IJsonSerializable interface
        void SerializeJson(io::JsonWriter& writer) const override;
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Checks if this coin reference equals another.
         * @param other The other coin reference.
         * @return True if equal.
         */
        bool operator==(const CoinReference& other) const;

        /**
         * @brief Checks if this coin reference does not equal another.
         * @param other The other coin reference.
         * @return True if not equal.
         */
        bool operator!=(const CoinReference& other) const;
    };
}