/**
 * @file change_view_message.h
 * @brief Network message handling
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/consensus/consensus_message.h>

namespace neo::consensus
{
// ChangeViewMessage is an alias for ViewChangeMessage to maintain backward compatibility
using ChangeViewMessage = ViewChangeMessage;
}  // namespace neo::consensus
