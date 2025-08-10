#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>

#include <chrono>
#include <memory>

namespace neo::ledger
{
/**
 * @brief Represents an item in the memory pool.
 */
class PoolItem
{
   public:
    /**
     * @brief Default constructor.
     */
    PoolItem() = default;

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
     * @brief Gets the transaction hash.
     * @return The transaction hash.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Gets the network fee per byte.
     * @return The network fee per byte.
     */
    uint64_t GetFeePerByte() const;

    /**
     * @brief Gets the network fee.
     * @return The network fee.
     */
    int64_t GetNetworkFee() const;

    /**
     * @brief Gets the system fee.
     * @return The system fee.
     */
    int64_t GetSystemFee() const;

    /**
     * @brief Gets the transaction size.
     * @return The size in bytes.
     */
    int GetSize() const;

    /**
     * @brief Check if this item conflicts with another transaction.
     * @param other The other pool item.
     * @return true if there's a conflict.
     */
    bool ConflictsWith(const PoolItem& other) const;

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
    io::UInt256 hash_;  // Cached hash

    /**
     * @brief Calculates the fee per byte for the transaction.
     * @return The fee per byte.
     */
    uint64_t CalculateFeePerByte() const;
};

/**
 * @brief Event arguments for transaction removed from pool
 * This matches the C# Neo TransactionRemovedEventArgs
 */
struct TransactionRemovedEventArgs
{
    std::shared_ptr<Transaction> transaction;

    enum class Reason
    {
        Expired,
        LowPriority,
        Replaced,
        InvalidTransaction,
        InsufficientFunds,
        PolicyViolation,
        Included
    } reason;

    TransactionRemovedEventArgs(std::shared_ptr<Transaction> tx, Reason r) : transaction(tx), reason(r) {}
};

}  // namespace neo::ledger
