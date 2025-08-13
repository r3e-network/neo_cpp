/**
 * @file prepare_request.h
 * @brief Prepare Request
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/consensus/consensus_message.h>

namespace neo::consensus
{
// PrepareRequest is an alias for PrepareRequestMessage to maintain backward compatibility
using PrepareRequest = PrepareRequestMessage;
}  // namespace neo::consensus
