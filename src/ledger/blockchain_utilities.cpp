/**
 * @file blockchain_utilities.cpp
 * @brief Core blockchain implementation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/ledger/blockchain.h>

namespace neo::ledger
{
// Additional utility implementations for Blockchain methods
uint32_t Blockchain::GetHeight() const { return GetCurrentBlockIndex(); }

}  // namespace neo::ledger