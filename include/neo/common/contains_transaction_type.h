/**
 * @file contains_transaction_type.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_COMMON_CONTAINS_TRANSACTION_TYPE_H
#define NEO_COMMON_CONTAINS_TRANSACTION_TYPE_H

namespace neo
{

/**
 * @brief Enumeration indicating where a transaction exists in the system.
 */
enum class ContainsTransactionType
{
    /**
     * @brief The transaction does not exist in the system.
     */
    NotExist = 0,

    /**
     * @brief The transaction exists in the memory pool.
     */
    ExistsInPool = 1,

    /**
     * @brief The transaction exists in the blockchain ledger.
     */
    ExistsInLedger = 2
};

}  // namespace neo

#endif  // NEO_COMMON_CONTAINS_TRANSACTION_TYPE_H