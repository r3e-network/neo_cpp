/**
 * @file blockchain_utilities.cpp
 * @brief Core blockchain implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>
#include <neo/core/neo_system.h>
#include <neo/smartcontract/native/ledger_contract.h>

namespace neo::ledger
{
// Additional utility implementations for Blockchain methods
// Provide a non-recursive, authoritative height accessor wired to the
// native LedgerContract (matches C# semantics).
uint32_t Blockchain::GetHeight() const
{
    try
    {
        if (!system_) return 0;
        // Use current data cache snapshot for consistency if available
        // data_cache_ is maintained by Blockchain and backed by the store
        auto ledger = system_->GetLedgerContract();
        if (!ledger)
        {
            return 0;
        }
        return ledger->GetCurrentIndex(data_cache_);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("GetHeight failed: {}", e.what());
        return 0;
    }
}

}  // namespace neo::ledger
