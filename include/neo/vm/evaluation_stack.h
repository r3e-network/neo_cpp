/**
 * @file evaluation_stack.h
 * @brief Evaluation Stack
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/stack_item.h>

#include <memory>
#include <stack>
#include <vector>

namespace neo::vm
{
/**
 * @brief VM evaluation stack for Neo virtual machine
 */
class EvaluationStack
{
   private:
    std::stack<std::shared_ptr<StackItem>> stack_;
    size_t max_size_;

   public:
    /**
     * @brief Constructor with maximum size
     * @param max_size Maximum stack size (default 2048)
     */
    explicit EvaluationStack(size_t max_size = 2048);

    /**
     * @brief Push item onto stack
     * @param item Item to push
     * @throws std::runtime_error if stack is full
     */
    void Push(std::shared_ptr<StackItem> item);

    /**
     * @brief Pop item from stack
     * @return Popped item
     * @throws std::runtime_error if stack is empty
     */
    std::shared_ptr<StackItem> Pop();

    /**
     * @brief Peek at top item without removing
     * @param index Index from top (0 = top, 1 = second from top, etc.)
     * @return Item at index
     * @throws std::runtime_error if index is out of bounds
     */
    std::shared_ptr<StackItem> Peek(size_t index = 0) const;

    /**
     * @brief Get stack size
     * @return Number of items on stack
     */
    size_t Size() const { return stack_.size(); }

    /**
     * @brief Check if stack is empty
     * @return True if empty
     */
    bool IsEmpty() const { return stack_.empty(); }

    /**
     * @brief Check if stack is full
     * @return True if at maximum capacity
     */
    bool IsFull() const { return stack_.size() >= max_size_; }

    /**
     * @brief Clear all items from stack
     */
    void Clear();

    /**
     * @brief Duplicate top item
     * @throws std::runtime_error if stack is empty or full
     */
    void Dup();

    /**
     * @brief Swap top two items
     * @throws std::runtime_error if stack has fewer than 2 items
     */
    void Swap();

    /**
     * @brief Rotate top 3 items (third item moves to top)
     * @throws std::runtime_error if stack has fewer than 3 items
     */
    void Rot();

    /**
     * @brief Remove item at specified depth
     * @param depth Depth from top (0 = top)
     * @throws std::runtime_error if depth is out of bounds
     */
    void Remove(size_t depth);

    /**
     * @brief Insert item at specified depth
     * @param depth Depth from top
     * @param item Item to insert
     * @throws std::runtime_error if depth is out of bounds or stack is full
     */
    void Insert(size_t depth, std::shared_ptr<StackItem> item);

    /**
     * @brief Get maximum stack size
     * @return Maximum stack size
     */
    size_t GetMaxSize() const { return max_size_; }

    /**
     * @brief Set maximum stack size
     * @param max_size New maximum size
     */
    void SetMaxSize(size_t max_size) { max_size_ = max_size; }

    /**
     * @brief Convert stack to vector (for debugging)
     * @return Vector with items from bottom to top
     */
    std::vector<std::shared_ptr<StackItem>> ToVector() const;
};
}  // namespace neo::vm