/**
 * @file high_priority.h
 * @brief High Priority
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/transaction_attribute_type.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Indicates that the transaction has high priority in the mempool.
 */
class HighPriority : public ledger::TransactionAttribute
{
    struct TypeProxy
    {
        HighPriority* owner_;
        explicit TypeProxy(HighPriority* owner) : owner_(owner) {}
        TypeProxy& operator=(ledger::TransactionAttributeType value)
        {
            (void)value;
            return *this;
        }
        operator ledger::TransactionAttributeType() const
        {
            return ledger::TransactionAttributeType::HighPriority;
        }
    };

   public:
    /**
     * @brief Constructs a HighPriority attribute.
     */
    HighPriority();
    HighPriority(const HighPriority& other);
    HighPriority& operator=(const HighPriority& other);
    HighPriority(HighPriority&& other) noexcept;
    HighPriority& operator=(HighPriority&& other) noexcept;

    TypeProxy Type;

    /**
     * @brief Gets the transaction attribute type.
     * @return The type.
     */
    ledger::TransactionAttribute::Usage GetType() const;

    /**
     * @brief Checks if multiple instances of this attribute are allowed.
     * @return False, as only one HighPriority attribute is allowed per transaction.
     */
    bool AllowMultiple() const;

    /**
     * @brief Gets the size of the attribute.
     * @return The size in bytes.
     */
    int GetSize() const;

    /**
     * @brief Serializes the attribute to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the attribute from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the attribute to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the attribute from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Verifies the attribute.
     * @param snapshot The data cache snapshot.
     * @param transaction The transaction containing this attribute.
     * @return True if valid.
     */
    bool Verify(/* DataCache& snapshot, const Transaction& transaction */) const;

    /**
     * @brief Calculates the network fee for this attribute.
     * @param snapshot The data cache snapshot.
     * @param transaction The transaction containing this attribute.
     * @return The network fee.
     */
    int64_t CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const;

    /**
     * @brief Checks if this attribute equals another.
     * @param other The other attribute.
     * @return True if equal.
     */
    bool operator==(const HighPriority& other) const;

    /**
     * @brief Checks if this attribute does not equal another.
     * @param other The other attribute.
     * @return True if not equal.
     */
    bool operator!=(const HighPriority& other) const;
};
}  // namespace neo::network::p2p::payloads
