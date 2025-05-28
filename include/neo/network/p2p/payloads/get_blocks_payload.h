#pragma once

#include <neo/network/p2p/ipayload.h>
#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/uint256.h>
#include <vector>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Represents a get blocks payload.
     */
    class GetBlocksPayload : public IPayload
    {
    public:
        /**
         * @brief Constructs a GetBlocksPayload.
         */
        GetBlocksPayload();

        /**
         * @brief Constructs a GetBlocksPayload.
         * @param hashStart The start hash.
         */
        explicit GetBlocksPayload(const io::UInt256& hashStart);

        /**
         * @brief Gets the hash start.
         * @return The hash start.
         */
        const io::UInt256& GetHashStart() const;

        /**
         * @brief Sets the hash start.
         * @param hashStart The hash start.
         */
        void SetHashStart(const io::UInt256& hashStart);

        /**
         * @brief Gets the count.
         * @return The count.
         */
        int16_t GetCount() const;

        /**
         * @brief Sets the count.
         * @param count The count.
         */
        void SetCount(int16_t count);

        /**
         * @brief Gets the size of the payload.
         * @return The size of the payload.
         */
        int GetSize() const;

        /**
         * @brief Creates a new GetBlocksPayload with the specified parameters.
         * @param hashStart The start hash.
         * @param count The count.
         * @return The created payload.
         */
        static GetBlocksPayload Create(const io::UInt256& hashStart, int16_t count = -1);

        /**
         * @brief Serializes the GetBlocksPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the GetBlocksPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the GetBlocksPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the GetBlocksPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        io::UInt256 hashStart_;
        int16_t count_;
    };
}
