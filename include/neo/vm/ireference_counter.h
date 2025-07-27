#pragma once

#include <memory>

namespace neo::vm
{
// Forward declaration
class StackItem;

/**
 * @brief Interface for reference counting in the VM.
 */
class IReferenceCounter
{
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IReferenceCounter() = default;

    /**
     * @brief Adds a reference to a stack item.
     * @param item The stack item to add a reference to.
     */
    virtual void AddReference(std::shared_ptr<StackItem> item) = 0;

    /**
     * @brief Removes a reference to a stack item.
     * @param item The stack item to remove a reference from.
     */
    virtual void RemoveReference(std::shared_ptr<StackItem> item) = 0;

    /**
     * @brief Checks if an item is being tracked.
     * @param item The stack item to check.
     * @return True if the item is being tracked.
     */
    virtual bool IsTracked(std::shared_ptr<StackItem> item) const = 0;

    /**
     * @brief Gets the current reference count.
     * @return The current reference count.
     */
    virtual size_t GetReferenceCount() const = 0;
};
}  // namespace neo::vm
