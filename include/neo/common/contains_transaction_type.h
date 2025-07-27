// Copyright (C) 2015-2025 The Neo Project.
//
// contains_transaction_type.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

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