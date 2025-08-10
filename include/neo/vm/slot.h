#pragma once

#include <neo/vm/stack_item.h>

#include <memory>
#include <vector>

namespace neo::vm
{
/**
 * @brief VM slot for local variables, arguments, or static fields
 */
class Slot
{
   private:
    std::vector<std::shared_ptr<StackItem>> items_;

   public:
    /**
     * @brief Constructor with capacity
     * @param capacity Number of slots
     */
    explicit Slot(size_t capacity);

    /**
     * @brief Default constructor
     */
    Slot() = default;

    /**
     * @brief Virtual destructor
     */
    virtual ~Slot() = default;

    /**
     * @brief Get item at index
     * @param index Slot index
     * @return Stack item at index
     */
    std::shared_ptr<StackItem> Get(size_t index) const;

    /**
     * @brief Set item at index
     * @param index Slot index
     * @param item Stack item to set
     */
    void Set(size_t index, std::shared_ptr<StackItem> item);

    /**
     * @brief Get slot count
     * @return Number of slots
     */
    size_t Count() const { return items_.size(); }

    /**
     * @brief Check if index is valid
     * @param index Index to check
     * @return True if index is valid
     */
    bool IsValidIndex(size_t index) const { return index < items_.size(); }

    /**
     * @brief Clear all slots
     */
    void Clear();

    /**
     * @brief Resize slot container
     * @param new_size New size
     */
    void Resize(size_t new_size);

    /**
     * @brief Get all items (for debugging)
     * @return Vector of all items
     */
    const std::vector<std::shared_ptr<StackItem>>& GetItems() const { return items_; }

    // Iterator support
    auto begin() { return items_.begin(); }
    auto end() { return items_.end(); }
    auto begin() const { return items_.begin(); }
    auto end() const { return items_.end(); }
};
}  // namespace neo::vm