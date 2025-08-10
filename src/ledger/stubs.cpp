#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>

namespace neo::ledger
{
// Additional implementations for Blockchain methods
uint32_t Blockchain::GetHeight() const { return GetCurrentBlockIndex(); }

}  // namespace neo::ledger