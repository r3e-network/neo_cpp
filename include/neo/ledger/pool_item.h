#pragma once

#include <chrono>
#include <memory>
#include <neo/ledger/transaction.h>

namespace neo::ledger
{
/**
 * @brief Represents an item in the memory pool.
 */
class PoolItem
{
  public:
    /**
     * @brief Constructor.
     * @param transaction The transaction.
     */
    explicit PoolItem(std::shared_ptr<Transaction> transaction);

    /**
     * @brief Destructor.
     */
    ~PoolItem() = default;

    /**
     * @brief Gets the transaction.
     * @return The transaction.
     */
    std::shared_ptr<Transaction> GetTransaction() const;

    /**
     * @brief Gets the timestamp when the item was added to the pool.
     * @return The timestamp.
     */
    std::chrono::system_clock::time_point GetTimestamp() const;

    /**
     * @brief Gets the network fee per byte.
     * @return The network fee per byte.
     */
    uint64_t GetFeePerByte() const;

    /**
     * @brief Checks if this item has higher priority than another.
     * @param other The other item.
     * @return True if this item has higher priority.
     */
    bool HasHigherPriorityThan(const PoolItem& other) const;

    /**
     * @brief Comparison operator for priority ordering.
     * @param other The other item.
     * @return True if this item has lower priority (for max heap).
     */
    bool operator<(const PoolItem& other) const;

    /**
     * @brief Equality operator.
     * @param other The other item.
     * @return True if equal.
     */
    bool operator==(const PoolItem& other) const;

    /**
     * @brief Inequality operator.
     * @param other The other item.
     * @return True if not equal.
     */
    bool operator!=(const PoolItem& other) const;

  private:
    std::shared_ptr<Transaction> transaction_;
    std::chrono::system_clock::time_point timestamp_;
    uint64_t fee_per_byte_;

    /**
     * @brief Calculates the fee per byte for the transaction.
     * @return The fee per byte.
     */
    uint64_t CalculateFeePerByte() const;
};
}  // namespace neo::ledger
