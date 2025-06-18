#pragma once

// This file provides backward compatibility during the migration to Neo3Transaction
// All new code should use neo::network::p2p::payloads::Neo3Transaction directly

#include <neo/network/p2p/payloads/neo3_transaction.h>

namespace neo::ledger
{
    // Alias for Neo3Transaction to maintain compatibility during migration
    using Transaction = network::p2p::payloads::Neo3Transaction;
}