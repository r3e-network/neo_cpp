#pragma once

#include <neo/ledger/transaction.h>
#include <neo/io/byte_vector.h>
#include <memory>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Factory for creating transactions from various sources.
     */
    class TransactionFactory
    {
    public:
        /**
         * @brief Creates a transaction from raw bytes.
         * @param data The raw transaction data.
         * @return A new transaction instance.
         */
        static std::shared_ptr<ledger::Transaction> FromBytes(const io::ByteVector& data);

        /**
         * @brief Creates a transaction from a byte array.
         * @param data The raw transaction data.
         * @param size The size of the data.
         * @return A new transaction instance.
         */
        static std::shared_ptr<ledger::Transaction> FromBytes(const uint8_t* data, size_t size);

        /**
         * @brief Creates a transaction from a hex string.
         * @param hex The hex string representation.
         * @return A new transaction instance.
         */
        static std::shared_ptr<ledger::Transaction> FromHexString(const std::string& hex);

        /**
         * @brief Creates an empty transaction.
         * @return A new empty transaction instance.
         */
        static std::shared_ptr<ledger::Transaction> CreateEmpty();
    };

} // namespace neo::network::p2p::payloads 