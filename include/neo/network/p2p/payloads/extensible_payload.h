#pragma once

#include <neo/network/p2p/payloads/ipayload.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/ledger/witness.h>
#include <string>
#include <vector>
#include <memory>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Represents an extensible payload for network communication.
     * This matches the C# ExtensiblePayload.cs implementation exactly.
     */
    class ExtensiblePayload : public IPayload, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Default constructor.
         */
        ExtensiblePayload() = default;

        /**
         * @brief Constructs an ExtensiblePayload with specified parameters.
         * @param category The category string.
         * @param valid_block_start The starting block for validity.
         * @param valid_block_end The ending block for validity.
         * @param sender The sender's script hash.
         * @param data The payload data.
         * @param witness The witness for verification.
         */
        ExtensiblePayload(const std::string& category,
                         uint32_t valid_block_start,
                         uint32_t valid_block_end,
                         const io::UInt160& sender,
                         const io::ByteVector& data,
                         const ledger::Witness& witness);

        /**
         * @brief Gets the category string.
         * @return The category.
         */
        const std::string& GetCategory() const { return category_; }

        /**
         * @brief Sets the category string.
         * @param category The category to set.
         */
        void SetCategory(const std::string& category) { category_ = category; }

        /**
         * @brief Gets the valid block start.
         * @return The valid block start.
         */
        uint32_t GetValidBlockStart() const { return valid_block_start_; }

        /**
         * @brief Sets the valid block start.
         * @param start The valid block start.
         */
        void SetValidBlockStart(uint32_t start) { valid_block_start_ = start; }

        /**
         * @brief Gets the valid block end.
         * @return The valid block end.
         */
        uint32_t GetValidBlockEnd() const { return valid_block_end_; }

        /**
         * @brief Sets the valid block end.
         * @param end The valid block end.
         */
        void SetValidBlockEnd(uint32_t end) { valid_block_end_ = end; }

        /**
         * @brief Gets the sender's script hash.
         * @return The sender.
         */
        const io::UInt160& GetSender() const { return sender_; }

        /**
         * @brief Sets the sender's script hash.
         * @param sender The sender to set.
         */
        void SetSender(const io::UInt160& sender) { sender_ = sender; }

        /**
         * @brief Gets the payload data.
         * @return The data.
         */
        const io::ByteVector& GetData() const { return data_; }

        /**
         * @brief Sets the payload data.
         * @param data The data to set.
         */
        void SetData(const io::ByteVector& data) { data_ = data; }

        /**
         * @brief Gets the witness.
         * @return The witness.
         */
        const ledger::Witness& GetWitness() const { return witness_; }

        /**
         * @brief Sets the witness.
         * @param witness The witness to set.
         */
        void SetWitness(const ledger::Witness& witness) { witness_ = witness; }

        // IPayload implementation
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
        io::UInt256 GetHash() const override;
        size_t GetSize() const override;

        // IJsonSerializable implementation
        void SerializeJson(io::JsonWriter& writer) const override;
        void DeserializeJson(const io::JsonReader& reader) override;

        /**
         * @brief Verifies the extensible payload.
         * @param settings The protocol settings.
         * @param snapshot The data cache snapshot.
         * @return True if valid, false otherwise.
         */
        bool Verify(std::shared_ptr<neo::config::ProtocolSettings> settings,
                   std::shared_ptr<neo::persistence::DataCache> snapshot) const;

        /**
         * @brief Checks if the payload is valid for the specified block index.
         * @param block_index The block index to check.
         * @return True if valid, false otherwise.
         */
        bool IsValidFor(uint32_t block_index) const;

        /**
         * @brief Gets the unsigned data for verification.
         * @return The unsigned data.
         */
        io::ByteVector GetUnsignedData() const;

        /**
         * @brief Creates an extensible payload.
         * @param category The category.
         * @param valid_block_start The starting block.
         * @param valid_block_end The ending block.
         * @param sender The sender.
         * @param data The payload data.
         * @return The created extensible payload.
         */
        static std::shared_ptr<ExtensiblePayload> Create(const std::string& category,
                                                        uint32_t valid_block_start,
                                                        uint32_t valid_block_end,
                                                        const io::UInt160& sender,
                                                        const io::ByteVector& data);

    private:
        std::string category_;
        uint32_t valid_block_start_ = 0;
        uint32_t valid_block_end_ = 0;
        io::UInt160 sender_;
        io::ByteVector data_;
        ledger::Witness witness_;

        // Cached hash
        mutable std::optional<io::UInt256> hash_cache_;
        mutable bool hash_calculated_ = false;
    };

} // namespace neo::network::p2p::payloads
