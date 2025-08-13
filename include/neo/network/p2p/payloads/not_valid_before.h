/**
 * @file not_valid_before.h
 * @brief Not Valid Before
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

#include <cstdint>

namespace neo::network::p2p::payloads
{
/**
 * @brief Indicates that the transaction is not valid before the specified block height.
 */
class NotValidBefore : public ledger::TransactionAttribute
{
   private:
    uint32_t height_;

   public:
    /**
     * @brief Constructs an empty NotValidBefore attribute.
     */
    NotValidBefore();

    /**
     * @brief Constructs a NotValidBefore attribute with the specified height.
     * @param height The block height.
     */
    explicit NotValidBefore(uint32_t height);

    /**
     * @brief Gets the block height.
     * @return The block height.
     */
    uint32_t GetHeight() const;

    /**
     * @brief Sets the block height.
     * @param height The block height.
     */
    void SetHeight(uint32_t height);

    /**
     * @brief Gets the transaction attribute type.
     * @return The type.
     */
    ledger::TransactionAttribute::Usage GetType() const;

    /**
     * @brief Checks if multiple instances of this attribute are allowed.
     * @return False, as multiple NotValidBefore attributes are not allowed.
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
    bool operator==(const NotValidBefore& other) const;

    /**
     * @brief Checks if this attribute does not equal another.
     * @param other The other attribute.
     * @return True if not equal.
     */
    bool operator!=(const NotValidBefore& other) const;
};
}  // namespace neo::network::p2p::payloads