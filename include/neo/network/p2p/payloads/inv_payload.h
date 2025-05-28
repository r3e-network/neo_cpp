#pragma once

#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/inventory_type.h>
#include <neo/network/p2p/inventory_vector.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/uint256.h>
#include <vector>
#include <cstdint>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Represents an inv message payload.
     */
    class InvPayload : public IPayload
    {
    public:
        /**
         * @brief Indicates the maximum number of inventories sent each time.
         */
        static constexpr int MaxHashesCount = 500;
        
        /**
         * @brief Constructs an empty InvPayload.
         */
        InvPayload();
        
        /**
         * @brief Constructs an InvPayload with the specified type and hashes.
         * @param type The type.
         * @param hashes The hashes.
         */
        InvPayload(InventoryType type, const std::vector<io::UInt256>& hashes);
        
        /**
         * @brief Constructs an InvPayload with the specified inventory vectors.
         * @param inventories The inventory vectors.
         */
        explicit InvPayload(const std::vector<InventoryVector>& inventories);
        
        /**
         * @brief Gets the type of the inventories.
         * @return The type.
         */
        InventoryType GetType() const;
        
        /**
         * @brief Sets the type of the inventories.
         * @param type The type.
         */
        void SetType(InventoryType type);
        
        /**
         * @brief Gets the hashes of the inventories.
         * @return The hashes.
         */
        std::vector<io::UInt256> GetHashes() const;
        
        /**
         * @brief Sets the hashes of the inventories.
         * @param hashes The hashes.
         */
        void SetHashes(const std::vector<io::UInt256>& hashes);
        
        /**
         * @brief Gets the inventory vectors.
         * @return The inventory vectors.
         */
        const std::vector<InventoryVector>& GetInventories() const;
        
        /**
         * @brief Sets the inventory vectors.
         * @param inventories The inventory vectors.
         */
        void SetInventories(const std::vector<InventoryVector>& inventories);
        
        /**
         * @brief Gets the size of the payload in bytes.
         * @return The size of the payload.
         */
        uint32_t GetSize() const;
        
        /**
         * @brief Creates a new instance of the InvPayload class.
         * @param type The type of the inventories.
         * @param hashes The hashes of the inventories.
         * @return The created payload.
         */
        static InvPayload Create(InventoryType type, const std::vector<io::UInt256>& hashes);
        
        /**
         * @brief Creates a group of the InvPayload instances.
         * @param type The type of the inventories.
         * @param hashes The hashes of the inventories.
         * @return The created payloads.
         */
        static std::vector<InvPayload> CreateGroup(InventoryType type, const std::vector<io::UInt256>& hashes);
        
        /**
         * @brief Serializes the InvPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;
        
        /**
         * @brief Deserializes the InvPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Serializes the InvPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;
        
        /**
         * @brief Deserializes the InvPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;
        
    private:
        InventoryType type_;
        std::vector<io::UInt256> hashes_;
    };
}
