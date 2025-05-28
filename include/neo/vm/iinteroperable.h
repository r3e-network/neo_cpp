#pragma once

#include <memory>

namespace neo::vm
{
    // Forward declarations
    class StackItem;
    class IReferenceCounter;

    /**
     * @brief Interface for objects that can be converted to/from VM stack items.
     */
    class IInteroperable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IInteroperable() = default;

        /**
         * @brief Creates a stack item from this object.
         * @param referenceCounter The reference counter for managing object references.
         * @return The stack item representation.
         */
        virtual std::shared_ptr<StackItem> ToStackItem(IReferenceCounter* referenceCounter) = 0;

        /**
         * @brief Initializes this object from a stack item.
         * @param stackItem The stack item to convert from.
         */
        virtual void FromStackItem(std::shared_ptr<StackItem> stackItem) = 0;
    };
}
