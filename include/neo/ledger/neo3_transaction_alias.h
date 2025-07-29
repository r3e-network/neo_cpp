#pragma once

#include <neo/network/p2p/payloads/neo3_transaction.h>

namespace neo::ledger
{
// Type alias for Neo3 transaction compatibility
// This maintains backward compatibility while using the Neo3 transaction structure
using Transaction = network::p2p::payloads::Neo3Transaction;

// Also bring in the inventory and verifiable interfaces
using network::p2p::payloads::IInventory;
using network::p2p::payloads::InventoryType;
using network::p2p::payloads::IVerifiable;
}  // namespace neo::ledger