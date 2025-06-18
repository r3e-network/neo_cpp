#pragma once

#include <neo/ledger/block_header.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/ledger/witness.h>
#include <neo/config/protocol_settings.h>
#include <neo/persistence/data_cache.h>
#include <memory>
#include <vector>

namespace neo::ledger
{
    // Forward declarations
    class HeaderCache;

    /**
     * @brief Represents a blockchain header (alias for BlockHeader for compatibility).
     */
    using Header = BlockHeader;

} // namespace neo::ledger 