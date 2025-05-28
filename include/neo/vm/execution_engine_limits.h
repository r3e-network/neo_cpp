#pragma once

#include <cstdint>

namespace neo::vm
{
    /**
     * @brief Represents the restrictions on the VM.
     */
    class ExecutionEngineLimits
    {
    public:
        /**
         * @brief The default strategy.
         */
        static const ExecutionEngineLimits Default;

        /**
         * @brief The maximum number of bits that SHL and SHR can shift.
         */
        int32_t MaxShift = 256;

        /**
         * @brief The maximum number of items that can be contained in the VM's evaluation stacks and slots.
         */
        uint32_t MaxStackSize = 2 * 1024;

        /**
         * @brief The maximum size of an item in the VM.
         */
        uint32_t MaxItemSize = 65535 * 2;

        /**
         * @brief The maximum number of frames in the invocation stack.
         */
        uint32_t MaxInvocationStackSize = 1024;

        /**
         * @brief The largest comparable size. If a ByteString or Struct exceeds this size, comparison operations on it cannot be performed in the VM.
         */
        uint32_t MaxComparableSize = 65536;

        /**
         * @brief The maximum nesting depth of try-catch-finally blocks.
         */
        uint32_t MaxTryNestingDepth = 16;

        /**
         * @brief The maximum number of initial elements in an array or structure.
         */
        uint32_t MaxInitialElementCount = 1024 * 1024;

        /**
         * @brief The maximum number of elements in an array or structure.
         */
        uint32_t MaxElementCount = 1024 * 1024;

        /**
         * @brief The maximum number of bytes that can be returned by a script.
         */
        uint32_t MaxReturnSize = 1024 * 1024;

        /**
         * @brief The maximum number of bytes that can be stored in a slot.
         */
        uint32_t MaxSlotCount = 1024;

        /**
         * @brief Whether to catch engine exceptions.
         */
        bool CatchEngineExceptions = true;

        /**
         * @brief Constructs a new ExecutionEngineLimits with default values.
         */
        ExecutionEngineLimits() = default;
    };
}
