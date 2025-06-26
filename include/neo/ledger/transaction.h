#pragma once

// This file provides backward compatibility for Neo 2.x style transactions
// Tests and legacy code can use this interface

#include <neo/ledger/neo2_transaction.h>

namespace neo::ledger
{
    // Alias for Neo2Transaction to maintain compatibility with tests
    using Transaction = Neo2Transaction;
}