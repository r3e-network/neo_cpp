#pragma once

#include <neo/network/ipayload.h>
#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/blockchain/transaction.h>
#include <memory>

namespace neo::network::payloads
{
    /**
     * @brief Represents a transaction payload.
     */
    class TransactionPayload : public IPayload, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty TransactionPayload.
         */
        TransactionPayload();

        /**
         * @brief Constructs a TransactionPayload with the specified transaction.
         * @param transaction The transaction.
         */
        explicit TransactionPayload(std::shared_ptr<blockchain::Transaction> transaction);

        /**
         * @brief Gets the transaction.
         * @return The transaction.
         */
        std::shared_ptr<blockchain::Transaction> GetTransaction() const;

        /**
         * @brief Sets the transaction.
         * @param transaction The transaction.
         */
        void SetTransaction(std::shared_ptr<blockchain::Transaction> transaction);

        /**
         * @brief Serializes the TransactionPayload to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the TransactionPayload from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the TransactionPayload to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the TransactionPayload from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        std::shared_ptr<blockchain::Transaction> transaction_;
    };
}
