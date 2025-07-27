#pragma once

#include <cstdint>

namespace neo::ledger
{
/**
 * @brief Neo N3 Transaction Attribute Types (matches C# TransactionAttributeType exactly)
 */
enum class TransactionAttributeType : uint8_t
{
    /// <summary>
    /// Indicates that the transaction should be processed with high priority.
    /// </summary>
    HighPriority = 0x01,

    /// <summary>
    /// Indicates that the transaction is an Oracle response.
    /// </summary>
    OracleResponse = 0x11,

    /// <summary>
    /// Indicates that the transaction is not valid before the specified block height.
    /// </summary>
    NotValidBefore = 0x20,

    /// <summary>
    /// Indicates that the transaction conflicts with the specified transaction.
    /// </summary>
    Conflicts = 0x21
};

/**
 * @brief Base class for Neo N3 transaction attributes
 */
class TransactionAttributeBase
{
  public:
    virtual ~TransactionAttributeBase() = default;

    /**
     * @brief Gets the transaction attribute type.
     * @return The type.
     */
    virtual TransactionAttributeType GetType() const = 0;

    /**
     * @brief Checks if multiple instances of this attribute are allowed.
     * @return True if multiple instances are allowed.
     */
    virtual bool AllowMultiple() const = 0;

    /**
     * @brief Gets the size of the attribute.
     * @return The size in bytes.
     */
    virtual int GetSize() const = 0;

    /**
     * @brief Verifies the attribute.
     * @return True if valid.
     */
    virtual bool Verify(/* DataCache& snapshot, const Transaction& transaction */) const = 0;

    /**
     * @brief Calculates the network fee for this attribute.
     * @return The network fee.
     */
    virtual int64_t CalculateNetworkFee(/* DataCache& snapshot, const Transaction& transaction */) const = 0;
};
}  // namespace neo::ledger