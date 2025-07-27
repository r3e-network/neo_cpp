#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>

namespace neo::ledger
{
// Stub implementations for Blockchain methods not in blockchain_complete.cpp
uint32_t Blockchain::GetHeight() const
{
    return GetCurrentBlockIndex();
}

}  // namespace neo::ledger