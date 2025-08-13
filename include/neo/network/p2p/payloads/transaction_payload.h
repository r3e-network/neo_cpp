/**
 * @file transaction_payload.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <memory>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a transaction payload.
 *
 * This message is sent to respond to GetData messages.
 */
class TransactionPayload : public IPayload
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
    explicit TransactionPayload(std::shared_ptr<Neo3Transaction> transaction);

    /**
     * @brief Gets the transaction.
     * @return The transaction.
     */
    std::shared_ptr<Neo3Transaction> GetTransaction() const;

    /**
     * @brief Sets the transaction.
     * @param transaction The transaction.
     */
    void SetTransaction(std::shared_ptr<Neo3Transaction> transaction);

    /**
     * @brief Gets the size of the payload.
     * @return The size of the payload.
     */
    int GetSize() const;

    /**
     * @brief Creates a new TransactionPayload with the specified transaction.
     * @param transaction The transaction.
     * @return The created payload.
     */
    static TransactionPayload Create(std::shared_ptr<Neo3Transaction> transaction);

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
    std::shared_ptr<Neo3Transaction> transaction_;
};
}  // namespace neo::network::p2p::payloads
