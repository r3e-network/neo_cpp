#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint256.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace neo::wallets
{
// Use Neo3Transaction from network namespace
using Neo3Transaction = network::p2p::payloads::Neo3Transaction;

/**
 * @brief Represents a wallet transaction.
 */
class WalletTransaction : public io::IJsonSerializable
{
   public:
    /**
     * @brief Constructs a wallet transaction.
     * @param transaction The transaction.
     */
    explicit WalletTransaction(std::shared_ptr<Neo3Transaction> transaction);

    /**
     * @brief Gets the transaction.
     * @return The transaction.
     */
    std::shared_ptr<Neo3Transaction> GetTransaction() const { return transaction_; }

    /**
     * @brief Gets the transaction hash.
     * @return The transaction hash.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Gets the confirmation status.
     * @return True if confirmed, false otherwise.
     */
    bool IsConfirmed() const { return confirmed_; }

    /**
     * @brief Sets the confirmation status.
     * @param confirmed The confirmation status.
     */
    void SetConfirmed(bool confirmed) { confirmed_ = confirmed; }

    /**
     * @brief Gets the block height.
     * @return The block height.
     */
    uint32_t GetBlockHeight() const { return block_height_; }

    /**
     * @brief Sets the block height.
     * @param height The block height.
     */
    void SetBlockHeight(uint32_t height) { block_height_ = height; }

    /**
     * @brief Serializes the WalletTransaction to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the WalletTransaction from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    std::shared_ptr<Neo3Transaction> transaction_;
    bool confirmed_;
    uint32_t block_height_;
};
}  // namespace neo::wallets
