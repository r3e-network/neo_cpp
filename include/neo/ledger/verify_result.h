/**
 * @file verify_result.h
 * @brief Verify Result
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

namespace neo::ledger
{
/**
 * @brief The result of verifying a block, transaction, or inventory.
 * This matches the C# VerifyResult enum exactly.
 */
enum class VerifyResult : uint8_t
{
    Succeed = 0,
    AlreadyExists = 1,
    AlreadyInPool = 2,
    OutOfMemory = 3,
    UnableToVerify = 4,
    Invalid = 5,
    InvalidScript = 6,
    InvalidAttribute = 7,
    InvalidSignature = 8,
    OverSize = 9,
    Expired = 10,
    InsufficientFunds = 11,
    PolicyFail = 12,
    HasConflicts = 13,
    Unknown = 14
};

}  // namespace neo::ledger
