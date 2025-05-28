#pragma once

#include <neo/network/payloads/transaction_attribute.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/uint160.h>
#include <neo/persistence/store_view.h>

namespace neo::network::payloads
{
    /**
     * @brief Represents a notary assisted transaction attribute.
     */
    class NotaryAssisted : public TransactionAttribute
    {
    private:
        /**
         * @brief The number of keys.
         */
        uint8_t nKeys;

    public:
        /**
         * @brief Constructs a NotaryAssisted.
         */
        NotaryAssisted();

        /**
         * @brief Gets the type.
         * @return The type.
         */
        TransactionAttributeType GetType() const override;

        /**
         * @brief Gets the size.
         * @return The size.
         */
        int GetSize() const override;

        /**
         * @brief Gets the number of keys.
         * @return The number of keys.
         */
        uint8_t GetNKeys() const;

        /**
         * @brief Sets the number of keys.
         * @param value The number of keys.
         */
        void SetNKeys(uint8_t value);

        /**
         * @brief Deserializes the object from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Serializes the object to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Verifies the attribute.
         * @param snapshot The snapshot.
         * @param tx The transaction.
         * @return True if successful, false otherwise.
         */
        bool Verify(std::shared_ptr<persistence::StoreView> snapshot, const Transaction& tx) const override;
    };
}
