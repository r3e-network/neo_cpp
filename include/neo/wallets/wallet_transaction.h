#pragma once

#include <neo/network/p2p/payloads/transaction.h>
#include <neo/io/uint256.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <string>
#include <cstdint>
#include <chrono>
#include <memory>

namespace neo::wallets
{
    /**
     * @brief Represents a transaction in a wallet.
     */
    class WalletTransaction : public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty WalletTransaction.
         */
        WalletTransaction();

        /**
         * @brief Constructs a WalletTransaction with the specified transaction.
         * @param transaction The transaction.
         */
        explicit WalletTransaction(const network::p2p::payloads::Transaction& transaction);

        /**
         * @brief Constructs a WalletTransaction with the specified transaction and height.
         * @param transaction The transaction.
         * @param height The height of the block containing the transaction.
         */
        WalletTransaction(const network::p2p::payloads::Transaction& transaction, uint32_t height);

        /**
         * @brief Gets the hash of the transaction.
         * @return The hash of the transaction.
         */
        const io::UInt256& GetHash() const;

        /**
         * @brief Sets the hash of the transaction.
         * @param hash The hash of the transaction.
         */
        void SetHash(const io::UInt256& hash);

        /**
         * @brief Gets the transaction.
         * @return The transaction.
         */
        const std::shared_ptr<network::p2p::payloads::Transaction>& GetTransaction() const;

        /**
         * @brief Sets the transaction.
         * @param transaction The transaction.
         */
        void SetTransaction(const std::shared_ptr<network::p2p::payloads::Transaction>& transaction);

        /**
         * @brief Gets the height of the block containing the transaction.
         * @return The height of the block containing the transaction.
         */
        uint32_t GetHeight() const;

        /**
         * @brief Sets the height of the block containing the transaction.
         * @param height The height of the block containing the transaction.
         */
        void SetHeight(uint32_t height);

        /**
         * @brief Gets the time when the transaction was added to the wallet.
         * @return The time when the transaction was added to the wallet.
         */
        const std::chrono::system_clock::time_point& GetTime() const;

        /**
         * @brief Sets the time when the transaction was added to the wallet.
         * @param time The time when the transaction was added to the wallet.
         */
        void SetTime(const std::chrono::system_clock::time_point& time);

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
        io::UInt256 hash_;
        std::shared_ptr<network::p2p::payloads::Transaction> transaction_;
        uint32_t height_;
        std::chrono::system_clock::time_point time_;
    };
}
