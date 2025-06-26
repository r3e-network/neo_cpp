#pragma once

// Forward declaration header for smartcontract storage item
// This allows tests to include storage item in smartcontract context

#include <neo/persistence/storage_item.h>

namespace neo::smartcontract
{
    // Alias the persistence storage item for smart contract use
    using StorageItem = neo::persistence::StorageItem;
}