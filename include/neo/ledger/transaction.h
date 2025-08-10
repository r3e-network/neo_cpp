#pragma once

// Neo N3 transaction interface
// This is the proper Neo N3 transaction type for this project

#include <neo/ledger/neo3_transaction_alias.h>

namespace neo::ledger
{
// Use the proper Neo N3 transaction type
using Transaction = network::p2p::payloads::Neo3Transaction;

// Bring in Neo N3 interfaces
using IInventory = network::p2p::payloads::IInventory;
using IVerifiable = network::p2p::payloads::IVerifiable;
using InventoryType = network::p2p::InventoryType;
}  // namespace neo::ledger