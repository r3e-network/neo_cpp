#pragma once

#include <neo/network/p2p/payloads/neo3_transaction.h>

namespace neo::ledger
{
    // Temporary alias to facilitate migration from old Transaction to Neo3Transaction
    // This allows us to update the codebase incrementally
    using Transaction = network::p2p::payloads::Neo3Transaction;
    
    // Also bring in the inventory and verifiable interfaces
    using network::p2p::payloads::IInventory;
    using network::p2p::payloads::IVerifiable;
    using network::p2p::payloads::InventoryType;
}