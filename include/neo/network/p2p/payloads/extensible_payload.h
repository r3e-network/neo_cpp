#pragma once

#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/iinventory.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/inventory_vector.h>
#include <neo/cryptography/ecc/witness.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/types.h>
#include <string>
#include <memory>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Represents an extensible message that can be relayed.
     */
    class ExtensiblePayload : public IPayload, public IInventory
    {
    public:
        /**
         * @brief Default constructor.
         */
        ExtensiblePayload();

        /**
         * @brief Gets the category of the extension.
         * @return The category of the extension.
         */
        const std::string& GetCategory() const;

        /**
         * @brief Sets the category of the extension.
         * @param category The category of the extension.
         */
        void SetCategory(const std::string& category);

        /**
         * @brief Gets the block height at which the payload becomes valid.
         * @return The block height at which the payload becomes valid.
         */
        uint32_t GetValidBlockStart() const;

        /**
         * @brief Sets the block height at which the payload becomes valid.
         * @param validBlockStart The block height at which the payload becomes valid.
         */
        void SetValidBlockStart(uint32_t validBlockStart);

        /**
         * @brief Gets the block height at which the payload becomes invalid.
         * @return The block height at which the payload becomes invalid.
         */
        uint32_t GetValidBlockEnd() const;

        /**
         * @brief Sets the block height at which the payload becomes invalid.
         * @param validBlockEnd The block height at which the payload becomes invalid.
         */
        void SetValidBlockEnd(uint32_t validBlockEnd);

        /**
         * @brief Gets the sender of the payload.
         * @return The sender of the payload.
         */
        const types::UInt160& GetSender() const;

        /**
         * @brief Sets the sender of the payload.
         * @param sender The sender of the payload.
         */
        void SetSender(const types::UInt160& sender);

        /**
         * @brief Gets the data of the payload.
         * @return The data of the payload.
         */
        const io::ByteVector& GetData() const;

        /**
         * @brief Sets the data of the payload.
         * @param data The data of the payload.
         */
        void SetData(const io::ByteVector& data);

        /**
         * @brief Gets the witness of the payload.
         * @return The witness of the payload.
         */
        const cryptography::ecc::Witness& GetWitness() const;

        /**
         * @brief Sets the witness of the payload.
         * @param witness The witness of the payload.
         */
        void SetWitness(const cryptography::ecc::Witness& witness);

        /**
         * @brief Gets the hash of the payload.
         * @return The hash of the payload.
         */
        const types::UInt256& GetHash() const override;

        /**
         * @brief Gets the inventory type of the payload.
         * @return The inventory type of the payload.
         */
        InventoryType GetInventoryType() const override;

        /**
         * @brief Serializes the payload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the payload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the payload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the payload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        std::string category_;
        uint32_t validBlockStart_;
        uint32_t validBlockEnd_;
        types::UInt160 sender_;
        io::ByteVector data_;
        cryptography::ecc::Witness witness_;
        mutable types::UInt256 hash_;
        mutable bool hashCalculated_;

        /**
         * @brief Calculates the hash of the payload.
         * @return The hash of the payload.
         */
        types::UInt256 CalculateHash() const;
    };
}
