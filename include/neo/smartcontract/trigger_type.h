/**
 * @file trigger_type.h
 * @brief Trigger Type
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>

namespace neo::smartcontract
{
/**
 * @brief Represents a trigger type.
 */
enum class TriggerType : uint8_t
{
    OnPersist = 0x01,
    PostPersist = 0x02,
    Verification = 0x20,
    Application = 0x40,
    System = 0x80,
    All = OnPersist | PostPersist | Verification | Application | System
};
}  // namespace neo::smartcontract
