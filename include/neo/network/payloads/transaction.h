/**
 * @file transaction.h
 * @brief Transaction types and processing
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

// Forward declaration header for network transaction payload
// This allows tests to include the transaction header without full implementation

#include <neo/ledger/transaction.h>

namespace neo::network::payloads
{
// Alias the ledger transaction as network payload transaction
using Transaction = neo::ledger::Transaction;
}  // namespace neo::network::payloads