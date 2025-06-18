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
        Invalid = 3,
        HasConflicts = 4,
        UnableToVerify = 5,
        OutOfMemory = 6
    };

} // namespace neo::ledger 