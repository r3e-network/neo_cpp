#pragma once

#include <cstdint>

namespace neo::smartcontract
{
/**
 * @brief Represents a call flags.
 */
enum class CallFlags : uint8_t
{
    None = 0,
    ReadStates = 0x01,
    WriteStates = 0x02,
    AllowCall = 0x04,
    AllowNotify = 0x08,
    States = ReadStates | WriteStates,
    ReadOnly = ReadStates | AllowCall,
    All = States | AllowCall | AllowNotify
};

/**
 * @brief Bitwise OR operator for CallFlags.
 */
inline CallFlags operator|(CallFlags lhs, CallFlags rhs)
{
    return static_cast<CallFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/**
 * @brief Bitwise AND operator for CallFlags.
 */
inline CallFlags operator&(CallFlags lhs, CallFlags rhs)
{
    return static_cast<CallFlags>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

/**
 * @brief Bitwise XOR operator for CallFlags.
 */
inline CallFlags operator^(CallFlags lhs, CallFlags rhs)
{
    return static_cast<CallFlags>(static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
}

/**
 * @brief Bitwise NOT operator for CallFlags.
 */
inline CallFlags operator~(CallFlags flags) { return static_cast<CallFlags>(~static_cast<uint8_t>(flags)); }

/**
 * @brief Checks if a CallFlags value has a specific flag set.
 * @param flags The flags to check.
 * @param flag The flag to check for.
 * @return True if the flag is set, false otherwise.
 */
inline bool HasFlag(CallFlags flags, CallFlags flag)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}
}  // namespace neo::smartcontract
